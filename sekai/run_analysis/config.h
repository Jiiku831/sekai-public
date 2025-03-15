#pragma once

#include "absl/time/time.h"

namespace sekai::run_analysis {

inline constexpr int kMinSegmentLength = 4;
inline constexpr absl::Duration kInterval = absl::Minutes(5);
inline constexpr absl::Duration kMaxSegmentGap = absl::Minutes(17);
inline constexpr int kWindow = 2;
inline constexpr float kBreakpointShift = 1.7;
inline constexpr float kBreakpointDecay = 0.9;
inline constexpr float kBreakpointThresholdLow = 5;
inline constexpr float kBreakpointThresholdHigh = 0.2;

// TODO: move to flag
inline constexpr bool kRunAnalysisDebug = true;

}  // namespace sekai::run_analysis
