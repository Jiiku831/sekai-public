#include "sekai/run_analysis/segmentation.h"

#include <cstddef>
#include <cstdint>
#include <deque>
#include <limits>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/log/log.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/time/time.h"
#include "base/util.h"
#include "segmentation.h"
#include "sekai/ranges_util.h"
#include "sekai/run_analysis/clustering.h"
#include "sekai/run_analysis/segmentation.h"
#include "sekai/run_analysis/snapshot.h"

namespace sekai::run_analysis {

namespace {

constexpr int kDetectionDelay = 2;
constexpr int kMaxClusters = 3;

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

    template <typename Sink>
    friend void AbslStringify(Sink& sink, const Verdict& v) {
      if (v.major_change && v.major_gap) {
        absl::Format(&sink, "Verdict{MAJOR_CHANGE, MAJOR_GAP, %.0f}", v.score);
        return;
      }
      if (v.major_change) {
        absl::Format(&sink, "Verdict{MAJOR_CHANGE, %.0f}", v.score);
        return;
      }
      if (v.major_gap) {
        absl::Format(&sink, "Verdict{MAJOR_GAP, %.0f}", v.score);
        return;
      }
      absl::Format(&sink, "Verdict{%.0f}", v.score);
    }
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

void InsertSplitAtPt(const Snapshot& pt, SegmentationV2DebugData& debug_data) {
  for (std::size_t i = 0; i < debug_data.cluster_means.size(); ++i) {
    debug_data.cluster_means[i].push_back(Snapshot{
        .time = pt.time,
        .points = std::numeric_limits<int>::min(),
    });
    debug_data.cluster_lbs[i].push_back(Snapshot{
        .time = pt.time,
        .points = std::numeric_limits<int>::min(),
    });
    debug_data.cluster_ubs[i].push_back(Snapshot{
        .time = pt.time,
        .points = std::numeric_limits<int>::min(),
    });
  }
}

class DetectorStateV2 {
 public:
  DetectorStateV2(absl::Time ref, const SegmentationOptions& opts,
                  SegmentationV2DebugData* debug_data, std::size_t disallow_breaks = 0)
      : min_segment_length_(std::max(opts.min_segment_length, disallow_breaks)),
        max_segment_gap_(opts.max_segment_gap),
        major_shift_threshold_(opts.major_shift_threshold),
        mean_shift_threshold_(opts.mean_shift_threshold),
        decay_(opts.shift_detection_decay),
        outlier_threshold_(opts.shift_detection_factor),
        debug_(opts.debug),
        debug_data_(debug_data) {
    constexpr std::size_t kAverageSegmentLength = 64;
    current_run_.time_offset = ref;
    current_run_.reserve(kAverageSegmentLength);
  }

  absl::StatusOr<DetectorState::Verdict> AddPoint(const Snapshot& pt, const Window& window) {
    if (absl::AbsDuration(pt.time - window.end.time) > absl::Seconds(1)) {
      return absl::InvalidArgumentError(
          absl::StrFormat("Expected time of pt (%v) and end of window (%v) to be equal.", pt.time,
                          window.end.time));
    }

    // 1. If current_run has no point, always add the point.
    if (current_run_.empty()) {
      current_run_.push_back(pt);
      return DetectorState::Verdict{};
    }

    // 2. If gap to previous point too large, break the sequence.
    if (pt.time - current_run_.back().time > max_segment_gap_) {
      return DetectorState::Verdict{
          .major_gap = true,
          .score = -2,
      };
    }

    // 3. Check for major changes if there is at least one segment.
    if (current_run_.size() >= 2 &&
        DiffChangeFactor(current_run_.back(), pt) < major_shift_threshold_) {
      return DetectorState::Verdict{
          .major_change = true,
          .score = -1,
      };
    }

    // 4. Skip clustering the size of the min_segment_length, but add to the segment.
    if (current_run_.size() < min_segment_length_) {
      current_run_.push_back(pt);
      if (current_run_.size() == min_segment_length_) Recluster();
      return DetectorState::Verdict{};
    }

    // 5. Assign clusters online, keeping track of outliers.
    float score = 0;
    constexpr std::size_t kOutlierTrackingWindow = 6;
    float stdev_deviation = UpdateClusters(pt);
    bool is_outlier = stdev_deviation > 0;
    last_outliers_.push_back(is_outlier);
    if (last_outliers_.size() > kOutlierTrackingWindow) {
      last_outliers_.pop_front();
    }
    int outlier_run = 0;
    int outlier_count = 0;
    bool outlier_seen = true;
    for (auto it = last_outliers_.rbegin(); it != last_outliers_.rend(); ++it) {
      bool outlier = *it;
      if (outlier) ++outlier_count;
      if (outlier_seen && outlier) {
        ++outlier_run;
      } else {
        outlier_seen = false;
      }
    }
    float outlier_ratio = static_cast<float>(outlier_count) / last_outliers_.size();

    if (is_outlier) {
      last_res_ = stdev_deviation * (0.5 + outlier_ratio) + last_res_ * decay_;
      constexpr float kMaxOutlierRatio = 0.4;
      if (outlier_run > kDetectionDelay || outlier_ratio > kMaxOutlierRatio) {
        score = last_res_;
      }
    } else {
      last_res_ *= decay_;
    }

    // 6. Recluster as needed.
    constexpr int kReclusterEveryNSamples = 5;
    if (current_run_.size() > min_segment_length_ * 2 &&
        (current_run_.size() - min_segment_length_) % kReclusterEveryNSamples == 0) {
      // std::vector<float> prev_means = RangesTo<std::vector<float>>(
      //     latest_clusters_ |
      //     std::views::transform([](const auto& cluster) { return cluster.mean; }));
      Recluster(min_segment_length_);
    }

    current_run_.push_back(pt);
    return DetectorState::Verdict{
        .score = score,
    };
  }

  int size() const { return current_run_.size(); }
  const Sequence& current_run() const { return current_run_; }

 private:
  static constexpr float kMinStdev = 2000;
  static constexpr float kMaxStdev = 40000;

  float length_factor(std::size_t n) const {
    static constexpr double kDiscount = 3;
    return kDiscount / std::min(std::pow(static_cast<float>(n), 0.5), kDiscount);
  }

  float cluster_window(std::size_t cluster_index) const {
    std::size_t cluster_size = latest_clusters_[cluster_index].vals.size();
    return outlier_threshold_ * length_factor(cluster_size) *
           std::min(kMaxStdev, std::max(latest_clusters_[cluster_index].stdev, kMinStdev));
  }

  // Returns how many stdev was the point from the closest cluster. 0 if not considered an outlier.
  float UpdateClusters(const Snapshot& pt) {
    int closest_cluster = 0;
    for (std::size_t i = 1; i < latest_clusters_.size() - 1; ++i) {
      if (std::abs(pt.diff - latest_clusters_[i].mean) <
          std::abs(pt.diff - latest_clusters_[closest_cluster].mean)) {
        closest_cluster = i;
      }
    }
    auto& closest_mean = latest_clusters_[closest_cluster].mean;
    std::size_t cluster_size = latest_clusters_[closest_cluster].vals.size();
    float stdev_deviation = std::abs(closest_mean - pt.diff) / cluster_window(closest_cluster);
    if (stdev_deviation > 1) {
      closest_cluster = kMaxClusters;
    } else {
      closest_mean = closest_mean + (1.0 / cluster_size) * (pt.diff - closest_mean);
      latest_clusters_[closest_cluster].vals.push_back(pt);
      stdev_deviation = 0;
    }
    if (debug_) {
      debug_data_->cluster_assignments[closest_cluster].push_back(pt);
      UpdateClusterDebug(pt);
    }
    return stdev_deviation;
  }

  void Recluster(std::size_t drop = 0) {
    bool initial_cluster = latest_clusters_.empty();
    constexpr std::size_t kDropInitialSegment = 1;
    latest_clusters_ = FindClusters(current_run_ | std::views::drop(kDropInitialSegment) |
                                        std::views::take(current_run_.size() - drop),
                                    kMinClusterSizeRatio,
                                    /*outlier_iterations=*/2,
                                    /*outlier_rejection_threshold=*/outlier_threshold_);
    if (initial_cluster && debug_) {
      for (const Snapshot& pt : current_run_) {
        UpdateClusterDebug(pt);
      }
      for (std::size_t i = 0; i < latest_clusters_.size() - 1; ++i) {
        for (std::size_t j = 0; j < latest_clusters_[i].vals.size(); ++j) {
          debug_data_->cluster_assignments[i].push_back(latest_clusters_[i].vals[j]);
        }
      }
    }
  }

  void UpdateClusterDebug(const Snapshot& pt) {
    for (std::size_t i = 0; i < kMaxClusters; ++i) {
      if (i < latest_clusters_.size() - 1) {
        debug_data_->cluster_means[i].push_back(Snapshot{
            .time = pt.time,
            .points = static_cast<int>(latest_clusters_[i].mean),
        });
        debug_data_->cluster_lbs[i].push_back(Snapshot{
            .time = pt.time,
            .points = static_cast<int>(latest_clusters_[i].mean - cluster_window(i)),
        });
        debug_data_->cluster_ubs[i].push_back(Snapshot{
            .time = pt.time,
            .points = static_cast<int>(latest_clusters_[i].mean + cluster_window(i)),
        });
      } else {
        debug_data_->cluster_means[i].push_back(Snapshot{
            .time = pt.time,
            .points = std::numeric_limits<int>::min(),
        });
        debug_data_->cluster_lbs[i].push_back(Snapshot{
            .time = pt.time,
            .points = std::numeric_limits<int>::min(),
        });
        debug_data_->cluster_ubs[i].push_back(Snapshot{
            .time = pt.time,
            .points = std::numeric_limits<int>::min(),
        });
      }
    }
  }

  std::size_t min_segment_length_ = 0;
  absl::Duration max_segment_gap_ = absl::InfiniteDuration();
  float major_shift_threshold_ = 0;
  float mean_shift_threshold_ = 0;
  float decay_ = 0;
  float last_res_ = 0;
  float outlier_threshold_ = 0;
  std::deque<bool> last_outliers_;
  bool debug_ = false;
  Sequence current_run_;
  SegmentationV2DebugData* debug_data_;
  std::vector<Cluster> latest_clusters_;
};

struct DetectionResult {
  std::vector<Sequence> segments;
  Sequence breakpoint_scores;
  SegmentationV2DebugData debug_data;
};

void InsertIntoCluster0(const Sequence& seq, SegmentationV2DebugData& debug_data) {
  for (const Snapshot& pt : seq) {
    debug_data.cluster_assignments[0].push_back(pt);
  }
}

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
    res.debug_data = {
        .cluster_assignments = {kMaxClusters + 1, seq.CopyEmpty()},
        .cluster_means = {kMaxClusters, seq.CopyEmpty()},
        .cluster_lbs = {kMaxClusters, seq.CopyEmpty()},
        .cluster_ubs = {kMaxClusters, seq.CopyEmpty()},
    };
  }
  res.segments.reserve(kAverageSegmentCount);
  DetectorStateV2 state{seq.time_offset, opts, &res.debug_data};
  for (std::size_t i = 0; i < seq.size(); ++i) {
    const Snapshot& pt = seq.points[i];
    const Window& window = smoothed_diffs[i];
    ASSIGN_OR_RETURN(const DetectorState::Verdict verdict, state.AddPoint(pt, window));
    if (opts.debug) {
      if (verdict.major_gap) {
        res.breakpoint_scores.push_back(Snapshot(pt.time, 0));
      }
      res.breakpoint_scores.push_back(Snapshot(
          pt.time,
          std::min(verdict.score, opts.mean_shift_threshold + 2) * kBreakpointScoreFactor));
    }

    if (verdict.major_gap && verdict.major_change) {
      return absl::InternalError("Verdict indicated both major_gap and major_change");
    }

    if (opts.debug && verdict.ShouldBreak(opts.mean_shift_threshold)) {
      res.breakpoint_scores.push_back(Snapshot(pt.time, std::numeric_limits<int>::min()));
    }

    if (verdict.major_gap) {
      // Start the next segment at this point.
      res.segments.push_back(state.current_run());
      state = DetectorStateV2(seq.time_offset, opts, &res.debug_data);
      if (opts.debug) {
        InsertIntoCluster0(res.segments.back(), res.debug_data);
        InsertSplitAtPt(pt, res.debug_data);
      }
      if (auto next_res = state.AddPoint(pt, window); !next_res.ok()) {
        return next_res.status();
      } else if (next_res->ShouldBreak(opts.mean_shift_threshold)) {
        return absl::InternalError(
            absl::StrCat("Attempted to break immediately after major gap: ", *next_res));
      }
    } else if (verdict.major_change) {
      // Start the next segment at the end of last segment.
      res.segments.push_back(state.current_run());
      state = DetectorStateV2(seq.time_offset, opts, &res.debug_data);
      if (opts.debug) {
        InsertIntoCluster0(res.segments.back(), res.debug_data);
        InsertSplitAtPt(pt, res.debug_data);
      }
      for (const auto& [pt_to_add, window_to_add] : {
               std::pair{seq.points[i - 1], smoothed_diffs[i - 1]},
               std::pair{pt, window},
           }) {
        if (auto next_res = state.AddPoint(pt_to_add, window_to_add); !next_res.ok()) {
          return next_res.status();
        } else if (next_res->ShouldBreak(opts.mean_shift_threshold)) {
          return absl::InternalError(
              absl::StrCat("Attempted to break immediately after major change: ", *next_res));
        }
      }
    } else if (verdict.ShouldBreak(opts.mean_shift_threshold)) {
      // Add the segment up to the detection delay.
      // TODO: better breakpoint selection instead of just fixed delay.
      // TODO: need to erase debug data if rolling back.
      if (opts.debug) InsertSplitAtPt(pt, res.debug_data);
      // res.segments.push_back(state.current_run().CopyWithNewPoints(
      //     {state.current_run().begin(),
      //      state.current_run().begin() + state.current_run().size() - 1}));
      // state = DetectorStateV2(seq.time_offset, opts, &res.debug_data);
      // for (const auto& [pt_to_add, window_to_add] : {
      //          std::pair{seq.points[i - 1], smoothed_diffs[i - 1]},
      //          std::pair{pt, window},
      //      }) {
      //   if (auto next_res = state.AddPoint(pt_to_add, window_to_add); !next_res.ok()) {
      //     return next_res.status();
      //   } else if (next_res->ShouldBreak(opts.mean_shift_threshold)) {
      //     return absl::InternalError(
      //         absl::StrCat("Attempted to break immediately after split: ", *next_res));
      //   }
      // }
      if (state.current_run().size() < kDetectionDelay) {
        return absl::InternalError(
            "Attempted to break with insufficient points for detection delay.");
      }
      res.segments.push_back(seq.CopyWithNewPoints(
          std::vector(state.current_run().begin(),
                      state.current_run().begin() + state.current_run().size() - kDetectionDelay)));
      // Roll back time by detection delay and start over.
      state = DetectorStateV2(seq.time_offset, opts, &res.debug_data, kDetectionDelay + 1);
      i -= kDetectionDelay + 1;
    }
  }
  res.segments.push_back(state.current_run());
  return res;
}

}  // namespace

RunSegments::RunSegments(std::vector<Sequence> run_segments, Sequence breakpoint_scores,
                         Sequence smoothed_diffs, SegmentationV2DebugData debug_data) {
  // Debug info
  breakpoint_scores_ = std::move(breakpoint_scores);
  smoothed_diffs_ = std::move(smoothed_diffs);
  segment_speeds_.reserve(run_segments.size() * 2);
  debug_data_ = std::move(debug_data);

  // Get breakpoints
  if (run_segments.empty()) return;
  constexpr std::size_t kPadding = 10;
  breakpoints_ = breakpoint_scores_.CopyEmptyAndReserve(run_segments.size() + kPadding);
  for (const Sequence& seq : run_segments) {
    if (breakpoints_.empty() || breakpoints_.back().time != seq.front().time) {
      breakpoints_.push_back(seq.front());
    }
    breakpoints_.push_back(seq.back());
    if (seq.size() > 1) {
      int speed = (seq.back().points - seq.front().points) *
                  (60.0f / ((seq.back().time - seq.front().time) / absl::Minutes(1)));
      segment_speeds_.push_back(breakpoint_scores.CopyWithNewPoints({
          Snapshot{seq.front().time, speed},
          Snapshot{seq.back().time, speed},
      }));
    }
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
  return RunSegments(res.segments, res.breakpoint_scores, smoothed_diffs, res.debug_data);
}

}  // namespace sekai::run_analysis
