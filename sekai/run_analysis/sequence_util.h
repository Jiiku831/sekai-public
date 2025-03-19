#pragma once

#include <vector>

#include "absl/time/time.h"
#include "sekai/run_analysis/config.h"
#include "sekai/run_analysis/snapshot.h"

namespace sekai::run_analysis {

// Round each point to the nearest interval, discarding conflicting points.
Sequence AlignSequence(const Sequence& sequence, absl::Duration interval);

// Interpolate points within max_gap at the specified interval. Assumes that each point is already
// aligned at the specified interval.
Sequence InterpolateSequence(const Sequence& sequence, absl::Duration interval,
                             absl::Duration max_gap);

Sequence ProcessSequence(const Sequence& sequence, absl::Duration interval = kInterval,
                         absl::Duration max_gap = kMaxSegmentGap);

}  // namespace sekai::run_analysis
