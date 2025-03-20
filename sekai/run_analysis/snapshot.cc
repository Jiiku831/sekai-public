#include "sekai/run_analysis/snapshot.h"

#include <cstddef>

namespace sekai::run_analysis {

Sequence Sequence::CopyWithNewPoints(std::vector<Snapshot> new_points) const {
  Sequence sequence = CopyEmpty();
  sequence.points = std::move(new_points);
  return sequence;
}

Sequence Sequence::CopyEmpty() const {
  return {
      .time_offset = time_offset,
  };
}

Sequence Sequence::CopyEmptyAndReserve(std::size_t size) const {
  Sequence sequence = CopyEmpty();
  sequence.reserve(size);
  return sequence;
}

}  // namespace sekai::run_analysis
