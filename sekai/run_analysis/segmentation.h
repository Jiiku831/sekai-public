#pragma once

#include <cstddef>
#include <span>
#include <vector>

#include "absl/time/time.h"
#include "sekai/run_analysis/config.h"
#include "sekai/run_analysis/snapshot.h"

namespace sekai::run_analysis {

// Assumes points are sorted by time.
std::vector<Sequence> SplitIntoSegments(const Sequence& sequence, std::size_t min_segment_length,
                                        absl::Duration max_segment_gap = kMaxSegmentGap);

struct Runs {
  Sequence breakpoints;
  std::vector<Sequence> initial_splits;
  std::vector<Sequence> runs;
  Sequence breakpoint_scores;
};
Runs SegmentRuns(const Sequence& sequence, int window, float breakpoint_shift = kBreakpointShift,
                 float breakpoint_threshold_low = kBreakpointThresholdLow,
                 float breakpoint_threshold_high = kBreakpointThresholdHigh,
                 absl::Duration interval = kInterval);

}  // namespace sekai::run_analysis
