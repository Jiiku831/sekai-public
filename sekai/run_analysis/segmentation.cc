#include "sekai/run_analysis/segmentation.h"

#include <cstddef>

namespace sekai::run_analysis {

std::vector<Segment> SplitIntoSegments(std::span<const Snapshot> points,
                                       std::size_t min_segment_length, int max_segment_gap) {
  std::vector<Segment> segments;
  Segment current_segment;
  for (const Snapshot& point : points) {
    if (!current_segment.empty() &&
        current_segment.points.back().timestamp + max_segment_gap < point.timestamp) {
      if (current_segment.size() >= min_segment_length) {
        segments.push_back(std::move(current_segment));
      }
      current_segment = Segment();
    }

    current_segment.points.push_back(point);
  }
  if (current_segment.size() >= min_segment_length) {
    segments.push_back(std::move(current_segment));
  }
  return segments;
}

}  // namespace sekai::run_analysis
