#pragma once

#include <cstddef>
#include <span>
#include <vector>

#include "absl/status/statusor.h"
#include "absl/time/time.h"
#include "sekai/run_analysis/config.h"
#include "sekai/run_analysis/snapshot.h"

namespace sekai::run_analysis {

struct SegmentationV2DebugData {
  std::vector<Sequence> cluster_assignments;
  std::vector<Sequence> cluster_means;
  std::vector<Sequence> cluster_lbs;
  std::vector<Sequence> cluster_ubs;
};

class RunSegments {
 public:
  RunSegments() = default;
  RunSegments(std::vector<Sequence> segments, Sequence breakpoint_scores, Sequence smoothed_diffs,
              SegmentationV2DebugData debug_data);

  const std::vector<Sequence>& active_segments() const { return active_segments_; }
  const std::vector<Sequence>& segment_speeds() const { return segment_speeds_; }
  const Sequence& breakpoints() const { return breakpoints_; }
  const Sequence& breakpoint_scores() const { return breakpoint_scores_; }
  const Sequence& smoothed_diffs() const { return smoothed_diffs_; }
  const SegmentationV2DebugData& debug() const { return debug_data_; }

 private:
  std::vector<Sequence> active_segments_;
  std::vector<Sequence> inactive_segments_;
  std::vector<Sequence> segment_speeds_;
  Sequence breakpoints_;
  Sequence breakpoint_scores_;
  Sequence smoothed_diffs_;
  SegmentationV2DebugData debug_data_;
};

struct SegmentationOptions {
  // Initial segmentation parameters
  absl::Duration interval = kInterval;
  absl::Duration max_segment_gap = kMaxSegmentGap;
  std::size_t min_segment_length = kMinSegmentLength;

  // Smoothing window to apply to breakpoint detection.
  int window = kWindow;

  // Breakpoint detection parameters.
  float shift_detection_decay = kBreakpointDecay;
  float shift_detection_factor = kBreakpointShift;
  float mean_shift_threshold = kBreakpointThresholdLow;

  // The threshold at which a change indicates a major shift in strategy.
  float major_shift_threshold = kBreakpointThresholdHigh;

  bool debug = kRunAnalysisDebug;
};

absl::StatusOr<RunSegments> SegmentRuns(const Sequence& sequence,
                                        const SegmentationOptions& options = {});

}  // namespace sekai::run_analysis
