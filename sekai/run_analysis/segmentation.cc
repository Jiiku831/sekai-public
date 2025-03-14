#include "sekai/run_analysis/segmentation.h"

#include <cstddef>
#include <cstdint>
#include <queue>
#include <ranges>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/log/log.h"
#include "absl/time/time.h"
#include "sekai/ranges_util.h"
#include "snapshot.h"

namespace sekai::run_analysis {

namespace {

constexpr int kDetectionDelay = 6;
constexpr int kMinRunLength = 4;

struct DetectionResult {
  Sequence breakpoint_scores;
  std::vector<absl::Duration> breakpoints;
};

class DetectorState {
 public:
  DetectorState(float shift) : shift_(shift) {}
  std::pair<float, float> AddObservation(const Snapshot& pt) {
    std::pair<float, float> res{0.0f, 0.0f};
    if (current_run_.size() >= kMinRunLength) {
      float mean = total_ / current_run_.size();
      // res = {
      //     std::max(0.0f, pt.points - (mean + shift_) + last_res_.first),
      //     std::max(0.0f, (mean - shift_) - pt.points + last_res_.second),
      // };
      res = {
          std::max(0.0f, static_cast<float>(pt.points - mean) * shift_ / mean + last_res_.first),
          std::max(0.0f, static_cast<float>(mean - pt.points) * shift_ / mean + last_res_.second),
      };
    }

    current_run_.push_back(pt);
    total_ += pt.points;
    last_res_ = res;
    return res;
  }

  int size() const { return current_run_.size(); }
  const Sequence& current_run() const { return current_run_; }

 private:
  float shift_ = 0;
  uint64_t total_ = 0;
  std::pair<float, float> last_res_ = {0, 0};
  Sequence current_run_;
};

void BreakpointDetection(Sequence run, float threshold, float shift, absl::Duration interval,
                         DetectionResult& result) {
  result.breakpoints.push_back(run.front().time);
  DetectorState state(shift);
  for (std::size_t i = 0; i < run.size(); ++i) {
    const auto [upper, lower] = state.AddObservation(run.points[i]);

    if (state.size() < kMinRunLength) {
      result.breakpoint_scores.push_back(Snapshot{run.points[i].time, 0});
      continue;
    }
    auto bp_score = std::max(upper, lower);
    result.breakpoint_scores.points.emplace_back(run.points[i].time, bp_score * 1000);

    if (bp_score > threshold) {
      result.breakpoints.push_back(run.points[i].time - kDetectionDelay * interval);
      state = DetectorState(shift);
    }
  }
}

}  // namespace

std::vector<Sequence> SplitIntoSegments(const Sequence& sequence, std::size_t min_segment_length,
                                        absl::Duration max_segment_gap) {
  std::vector<Sequence> segments;
  Sequence current_segment;
  for (const Snapshot& point : sequence.points) {
    if (!current_segment.empty() &&
        max_segment_gap < point.time - current_segment.points.back().time) {
      if (current_segment.size() >= min_segment_length) {
        segments.push_back(std::move(current_segment));
      }
      current_segment = sequence.CopyEmpty();
    }

    current_segment.points.push_back(point);
  }
  if (current_segment.size() >= min_segment_length) {
    segments.push_back(std::move(current_segment));
  }
  return segments;
}

Runs SegmentRuns(const Sequence& sequence, int window, float breakpoint_shift,
                 float breakpoint_threshold_low, float breakpoint_threshold_high,
                 absl::Duration interval) {
  if (sequence.size() < 3) {
    return {
        .breakpoints = sequence,
        .initial_splits = {sequence},
        .runs = {sequence},
        .breakpoint_scores = sequence.CopyEmpty(),
    };
  }
  std::vector<Sequence> initial_splits;
  // First detect start/stop and big changes.
  const std::size_t end_index = sequence.size() - 3;
  Sequence current_seq = sequence.CopyWithNewPoints({sequence.points[0]});
  current_seq.reserve(128);
  for (std::size_t i = 0; i < end_index; ++i) {
    const Snapshot& l = sequence.points[i];
    const Snapshot& m = sequence.points[i + 1];
    const Snapshot& r = sequence.points[i + 2];
    current_seq.push_back(m);

    int lhs = m.points - l.points;
    int rhs = r.points - m.points;
    if (!(lhs * rhs == 0 && lhs == rhs) &&
        (std::abs(static_cast<float>(lhs) / (rhs + 1)) < breakpoint_threshold_high ||
         std::abs(static_cast<float>(rhs) / (lhs + 1)) < breakpoint_threshold_high)) {
      initial_splits.push_back(current_seq);
      current_seq = sequence.CopyWithNewPoints({m});
    }
  }
  current_seq.push_back(sequence.back());
  initial_splits.push_back(current_seq);

  DetectionResult result;
  result.breakpoint_scores = sequence.CopyEmptyAndReserve(sequence.size());
  for (const Sequence& run : initial_splits) {
    if (run.size() > static_cast<std::size_t>(window)) {
      auto smooth_seq = run.CopyWithNewPoints(RangesTo<std::vector<Snapshot>>(
          std::views::zip(std::span(run.begin(), run.end() - window),
                          std::span(run.begin() + window, run.end())) |
          std::views::transform([&](const auto& window) {
            return Snapshot{
                std::get<0>(window).time,
                static_cast<int>((std::get<1>(window).points - std::get<0>(window).points)),
            };
          })));
      BreakpointDetection(smooth_seq, breakpoint_threshold_low, breakpoint_shift, interval, result);
    } else {
      result.breakpoints.push_back(run.front().time);
    }
  }

  std::priority_queue<absl::Duration, std::vector<absl::Duration>, std::greater<absl::Duration>>
      final_break_times{result.breakpoints.begin(), result.breakpoints.end()};

  std::vector<Sequence> final_runs;
  final_runs.reserve(result.breakpoints.size());
  Sequence current_sequence = sequence.CopyEmptyAndReserve(128);
  for (const Snapshot& pt : sequence) {
    while (!final_break_times.empty() && pt.time >= final_break_times.top()) {
      if (!current_sequence.empty()) {
        current_sequence.push_back(pt);
        final_runs.push_back(current_sequence);
        current_sequence = sequence.CopyEmptyAndReserve(128);
      }
      final_break_times.pop();
    }

    current_sequence.push_back(pt);
  }
  current_sequence.push_back(sequence.back());
  final_runs.push_back(current_sequence);

  Sequence breakpoints = sequence.CopyWithNewPoints(RangesTo<std::vector<Snapshot>>(
      final_runs | std::views::transform([](const auto& seq) { return seq.front(); })));
  breakpoints.push_back(final_runs.back().back());

  Runs runs = {
      .breakpoints = breakpoints,
      .initial_splits = initial_splits,
      .runs = final_runs,
      .breakpoint_scores = result.breakpoint_scores,
  };
  return runs;
}

}  // namespace sekai::run_analysis
