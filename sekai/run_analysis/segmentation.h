#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "sekai/run_analysis/snapshot.h"

namespace sekai::run_analysis {

// A segment is a continuous sequence of points.
struct Segment {
  std::vector<Snapshot> points;

  bool empty() const { return points.empty(); }
  std::size_t size() const { return points.size(); }
};

// Assumes points are sorted by timestamp.
std::vector<Segment> SplitIntoSegments(std::span<const Snapshot> points,
                                       std::size_t min_segment_length,
                                       int max_segment_gap = 60 * 17);

}  // namespace sekai::run_analysis
