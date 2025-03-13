#include "sekai/run_analysis/segmentation.h"

#include <cstddef>
#include <vector>

#include "absl/time/time.h"

namespace sekai::run_analysis {

std::vector<Sequence> SplitIntoSegments(const Sequence& sequence, std::size_t min_segment_length,
                                        absl::Duration max_segment_gap) {
  std::vector<Sequence> segments;
  Sequence current_segment;
  for (const Snapshot& point : sequence.points) {
    if (!current_segment.empty() &&
        max_segment_gap < point.time - current_segment.points.back().time) {
      if (current_segment.size() >= min_segment_length) {
        segments.push_back(std::move(current_segment));
      }
      current_segment = sequence.CopyEmpty();
    }

    current_segment.points.push_back(point);
  }
  if (current_segment.size() >= min_segment_length) {
    segments.push_back(std::move(current_segment));
  }
  return segments;
}

}  // namespace sekai::run_analysis
