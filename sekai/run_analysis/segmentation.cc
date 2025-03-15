#include "sekai/run_analysis/segmentation.h"

#include <cstddef>
#include <cstdint>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/log/log.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/time/time.h"
#include "base/util.h"
#include "sekai/ranges_util.h"
#include "sekai/run_analysis/segmentation.h"
#include "sekai/run_analysis/snapshot.h"

namespace sekai::run_analysis {

namespace {

constexpr int kDetectionDelay = 6;

struct Window {
  Snapshot start;
  Snapshot end;
  int diff = 0;
  bool invalid = false;
};

std::vector<Window> GetSmoothedDiffs(const Sequence& seq, int window) {
  std::vector<Window> smooth_seq;
  smooth_seq.reserve(seq.size());
  for (int i = 0; i < window; ++i) {
    smooth_seq.push_back(Window{.start = seq.points[0], .end = seq.points[i], .invalid = true});
  }
  if (seq.size() >= static_cast<std::size_t>(window)) {
    auto windows = std::views::zip(std::span(seq.begin(), seq.end() - window),
                                   std::span(seq.begin() + window, seq.end())) |
                   std::views::transform([&](const auto& pts) {
                     return Window{
                         std::get<0>(pts),
                         std::get<1>(pts),
                         (std::get<1>(pts).points - std::get<0>(pts).points) / window,
                     };
                   });
    smooth_seq.insert(smooth_seq.end(), windows.begin(), windows.end());
  }
  return smooth_seq;
}

Sequence SmoothedDiffsToSequence(std::span<const Window> windows, absl::Time ref) {
  Sequence seq{ref};
  seq.points =
      RangesTo<std::vector<Snapshot>>(windows | std::views::transform([](const auto& window) {
                                        return Snapshot{window.start.time, window.diff};
                                      }));
  return seq;
}

float DiffChangeFactor(const Snapshot& lhs, const Snapshot& rhs) {
  if (lhs.diff * rhs.diff == 0) {
    return lhs.diff == rhs.diff ? 1 : 0;
  }
  return std::min(static_cast<float>(lhs.diff) / rhs.diff, static_cast<float>(rhs.diff) / lhs.diff);
}

class DetectorState {
 public:
  DetectorState(absl::Time ref, const SegmentationOptions& opts, std::size_t disallow_breaks = 0)
      : window_(opts.window),
        min_segment_length_(std::max(opts.window + opts.min_segment_length, disallow_breaks)),
        max_segment_gap_(opts.max_segment_gap),
        major_shift_threshold_(opts.major_shift_threshold),
        mean_shift_threshold_(opts.mean_shift_threshold),
        decay_(opts.shift_detection_decay),
        shift_(opts.shift_detection_factor) {
    constexpr std::size_t kAverageSegmentLength = 64;
    current_run_.time_offset = ref;
    current_run_.reserve(kAverageSegmentLength);
  }

  struct Verdict {
    bool major_change = false;
    bool major_gap = false;
    float score = 0;

    bool ShouldBreak(float thresh) const { return major_change || major_gap || (score >= thresh); }
  };
  absl::StatusOr<Verdict> AddPoint(const Snapshot& pt, const Window& window) {
    if (absl::AbsDuration(pt.time - window.end.time) > absl::Seconds(1)) {
      return absl::InvalidArgumentError(
          absl::StrFormat("Expected time of pt (%v) and end of window (%v) to be equal.", pt.time,
                          window.end.time));
    }

    // 1. If current_run has no point, always add the point.
    if (current_run_.empty()) {
      current_run_.push_back(pt);
      return Verdict{};
    }

    // 2. If gap to previous point too large, break the sequence.
    if (pt.time - current_run_.back().time > max_segment_gap_) {
      return Verdict{
          .major_gap = true,
          .score = mean_shift_threshold_ + 2,
      };
    }

    // 3. Check for major changes if there is at least one segment.
    if (current_run_.size() >= 2 &&
        DiffChangeFactor(current_run_.back(), pt) < major_shift_threshold_) {
      return Verdict{
          .major_change = true,
          .score = mean_shift_threshold_ + 1,
      };
    }

    // 4. Skip the size of the smoothing window, but add to the segment.
    if (current_run_.size() < window_) {
      current_run_.push_back(pt);
      return Verdict{};
    }

    // 5. If the segment is long enough, compute the score.
    if (window.invalid) {
      return absl::InvalidArgumentError(absl::StrFormat(
          "Attempting to use an invalid window (%v-%v).", window.start.time, window.end.time));
    }
    Verdict verdict;
    if (current_run_.size() >= min_segment_length_) {
      if (total_ == 0) {
        last_res_ = {0.0f, 0.0f};
      } else {
        float mean = total_ / counts_;
        last_res_ = {
            std::max(0.0f, static_cast<float>(window.diff - mean) * shift_ / mean +
                               last_res_.first * decay_),
            std::max(0.0f, static_cast<float>(mean - window.diff) * shift_ / mean +
                               last_res_.second * decay_),
        };
      }
      verdict.score = std::max(last_res_.first, last_res_.second);
    }

    current_run_.push_back(pt);
    total_ += window.diff;
    ++counts_;
    return verdict;
  }

  int size() const { return current_run_.size(); }
  const Sequence& current_run() const { return current_run_; }

 private:
  std::size_t window_ = 0;
  std::size_t min_segment_length_ = 0;
  absl::Duration max_segment_gap_ = absl::InfiniteDuration();
  float major_shift_threshold_ = 0;
  float mean_shift_threshold_ = 0;
  float decay_ = 0;
  float shift_ = 0;
  uint64_t total_ = 0;
  int counts_ = 0;
  std::pair<float, float> last_res_ = {0, 0};
  Sequence current_run_;
};

struct DetectionResult {
  std::vector<Sequence> segments;
  Sequence breakpoint_scores;
};

absl::StatusOr<DetectionResult> BreakpointDetection(const Sequence& seq,
                                                    std::span<const Window> smoothed_diffs,
                                                    const SegmentationOptions& opts) {
  constexpr int kBreakpointScoreFactor = 1000;
  constexpr int kAverageSegmentCount = 32;
  if (seq.size() != smoothed_diffs.size()) {
    return absl::InvalidArgumentError(
        absl::StrFormat("seq (size=%d) and smoothed_diffs (size=%d) should have the same size",
                        seq.size(), smoothed_diffs.size()));
  }
  DetectionResult res;
  if (opts.debug) {
    res.breakpoint_scores = seq.CopyEmptyAndReserve(seq.size());
  }
  res.segments.reserve(kAverageSegmentCount);
  DetectorState state{seq.time_offset, opts};
  for (std::size_t i = 0; i < seq.size(); ++i) {
    const Snapshot& pt = seq.points[i];
    const Window& window = smoothed_diffs[i];
    ASSIGN_OR_RETURN(const DetectorState::Verdict verdict, state.AddPoint(pt, window));
    if (opts.debug) {
      res.breakpoint_scores.push_back(Snapshot(pt.time, verdict.score * kBreakpointScoreFactor));
    }

    if (verdict.major_gap && verdict.major_change) {
      return absl::InternalError("Verdict indicated both major_gap and major_change");
    }

    if (opts.debug && verdict.ShouldBreak(opts.mean_shift_threshold)) {
      // Insert a 0 after a break to reset the graph.
      res.breakpoint_scores.push_back(Snapshot(pt.time, 0));
    }

    if (verdict.major_gap) {
      // Start the next segment at this point.
      res.segments.push_back(state.current_run());
      state = DetectorState(seq.time_offset, opts);
      if (auto next_res = state.AddPoint(pt, window); !next_res.ok()) {
        return next_res.status();
      } else if (next_res->ShouldBreak(opts.mean_shift_threshold)) {
        return absl::InternalError("Attempted to break immediately after major gap");
      }
    } else if (verdict.major_change) {
      // Start the next segment at the end of last segment.
      res.segments.push_back(state.current_run());
      state = DetectorState(seq.time_offset, opts);
      for (const auto& [pt_to_add, window_to_add] : {
               std::pair{seq.points[i - 1], smoothed_diffs[i - 1]},
               std::pair{pt, window},
           }) {
        if (auto next_res = state.AddPoint(pt_to_add, window_to_add); !next_res.ok()) {
          return next_res.status();
        } else if (next_res->ShouldBreak(opts.mean_shift_threshold)) {
          return absl::InternalError("Attempted to break immediately after major change");
        }
      }
    } else if (verdict.ShouldBreak(opts.mean_shift_threshold)) {
      // Add the segment up to the detection delay.
      // TODO: better breakpoint selection instead of just fixed delay.
      if (state.current_run().size() < kDetectionDelay) {
        return absl::InternalError(
            "Attempted to break with insufficient points for detection delay.");
      }
      res.segments.push_back(seq.CopyWithNewPoints(
          std::vector(state.current_run().begin(),
                      state.current_run().begin() + state.current_run().size() - kDetectionDelay)));
      // Roll back time by detection delay and start over.
      state = DetectorState(seq.time_offset, opts, kDetectionDelay + 1);
      i -= kDetectionDelay + 1;
    }
  }
  res.segments.push_back(state.current_run());
  return res;
}

}  // namespace

RunSegments::RunSegments(std::vector<Sequence> run_segments, Sequence breakpoint_scores,
                         Sequence smoothed_diffs) {
  // Debug info
  breakpoint_scores_ = std::move(breakpoint_scores);
  smoothed_diffs_ = std::move(smoothed_diffs);

  // Get breakpoints
  if (run_segments.empty()) return;
  constexpr std::size_t kPadding = 10;
  breakpoints_ = breakpoint_scores_.CopyEmptyAndReserve(run_segments.size() + kPadding);
  for (const Sequence& seq : run_segments) {
    if (breakpoints_.empty() || breakpoints_.back().time != seq.front().time) {
      breakpoints_.push_back(seq.front());
    }
    breakpoints_.push_back(seq.back());
  }

  // Classify segments
  for (Sequence& sequence : run_segments) {
    if (sequence.diff() == 0) {
      inactive_segments_.push_back(std::move(sequence));
    } else {
      active_segments_.push_back(std::move(sequence));
    }
  }
}

absl::StatusOr<RunSegments> SegmentRuns(const Sequence& sequence,
                                        const SegmentationOptions& options) {
  std::vector<Window> diffs = GetSmoothedDiffs(sequence, options.window);
  Sequence smoothed_diffs =
      options.debug ? SmoothedDiffsToSequence(diffs, sequence.time_offset) : Sequence{};
  ASSIGN_OR_RETURN(DetectionResult res, BreakpointDetection(sequence, diffs, options));
  return RunSegments(res.segments, res.breakpoint_scores, smoothed_diffs);
}

}  // namespace sekai::run_analysis
