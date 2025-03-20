#include "sekai/run_analysis/sequence_util.h"

#include <cmath>
#include <vector>

#include "absl/time/time.h"
#include "sekai/run_analysis/snapshot.h"

namespace sekai::run_analysis {
namespace {

bool DurationClose(absl::Duration x, absl::Duration y) {
  constexpr absl::Duration kTol = absl::Seconds(1);
  return absl::AbsDuration(x - y) < kTol;
}

}  // namespace

Sequence AlignSequence(const Sequence& sequence, absl::Duration interval) {
  Sequence new_sequence = sequence.CopyEmptyAndReserve(sequence.size());
  for (const Snapshot& snapshot : sequence) {
    absl::Duration nearest_interval = std::round((snapshot.time * 10 / interval) / 10.0) * interval;
    if (!new_sequence.empty() && DurationClose(nearest_interval, new_sequence.back().time)) {
      continue;
    }
    new_sequence.push_back(Snapshot(nearest_interval, snapshot.points));
  }
  return new_sequence;
}

Sequence InterpolateSequence(const Sequence& sequence, absl::Duration interval,
                             absl::Duration max_gap) {
  Sequence new_sequence = sequence.CopyEmptyAndReserve(sequence.size());
  for (const Snapshot& snapshot : sequence) {
    if (new_sequence.empty()) {
      new_sequence.push_back(snapshot);
      continue;
    }

    const Snapshot& last = new_sequence.back();
    int intervals = std::round(((snapshot.time - last.time) * 10 / interval) / 10.0);
    if (snapshot.time - last.time > max_gap) {
      intervals = 1;
    }
    int step = (snapshot.points - last.points) / intervals;
    for (int i = 1; i < intervals; ++i) {
      new_sequence.push_back(Snapshot{
          .time = last.time + interval * i,
          .points = last.points + step * i,
      });
    }
    new_sequence.push_back(snapshot);
  }
  return new_sequence;
}

Sequence ProcessSequence(const Sequence& sequence, absl::Duration interval,
                         absl::Duration max_gap) {
  Sequence processed_seq =
      InterpolateSequence(AlignSequence(sequence, interval), interval, max_gap);
  for (std::size_t i = 1; i < processed_seq.size(); ++i) {
    processed_seq.points[i].diff =
        processed_seq.points[i].points - processed_seq.points[i - 1].points;
  }
  return processed_seq;
}

}  // namespace sekai::run_analysis
