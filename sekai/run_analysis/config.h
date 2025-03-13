#pragma once

#include "absl/time/time.h"

namespace sekai::run_analysis {

inline constexpr int kMinSegmentLength = 3;
inline constexpr absl::Duration kInterval = absl::Minutes(5);
inline constexpr absl::Duration kMaxSegmentGap = absl::Minutes(17);
inline constexpr int kWindow = 3;

}  // namespace sekai::run_analysis
