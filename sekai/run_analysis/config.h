#pragma once

#include "absl/time/time.h"

namespace sekai::run_analysis {

inline constexpr int kMinSegmentLength = 6;
inline constexpr absl::Duration kInterval = absl::Minutes(5);
inline constexpr absl::Duration kMaxSegmentGap = absl::Minutes(17);
inline constexpr int kWindow = 2;
inline constexpr float kBreakpointShift = 3;
inline constexpr float kBreakpointDecay = 0.9;
inline constexpr float kBreakpointThresholdLow = 5;
inline constexpr float kBreakpointThresholdHigh = 0.2;

// Reject clusters smaller than this amount of the population.
inline constexpr float kMinClusterSizeRatio = 0.10;
inline constexpr int kClusteringOutlierIterations = 3;
inline constexpr float kClusteringOutlierRejectionThresh = 2;

bool DebugEnabled();

}  // namespace sekai::run_analysis
