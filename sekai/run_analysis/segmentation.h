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

}  // namespace sekai::run_analysis
