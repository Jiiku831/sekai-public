#pragma once

#include "absl/strings/str_format.h"
#include "absl/strings/str_join.h"
#include "absl/time/time.h"

namespace sekai::run_analysis {

struct Snapshot {
  absl::Duration time;
  int points = 0;
  int diff = 0;

  auto operator<=>(const Snapshot&) const = default;

  template <typename Sink>
  friend void AbslStringify(Sink& sink, const Snapshot& snapshot) {
    absl::Format(&sink, "Snapshot{%v, %d}", snapshot.time, snapshot.points);
  }
};

struct Sequence {
  absl::Time time_offset = absl::UnixEpoch();

  using container = std::vector<Snapshot>;
  container points;

  template <typename Sink>
  friend void AbslStringify(Sink& sink, const Sequence& seq) {
    absl::Format(&sink, "Sequence(offset=%v)\n%s", seq.time_offset,
                 absl::StrJoin(seq.points, "\n"));
  }

  Sequence CopyWithNewPoints(std::vector<Snapshot> new_points) const;
  Sequence CopyEmpty() const;
  Sequence CopyEmptyAndReserve(std::size_t size) const;

  int diff() const { return empty() ? 0 : (back().points - front().points); }

  bool empty() const { return points.empty(); }
  std::size_t size() const { return points.size(); }

  using iterator = container::iterator;
  using const_iterator = container::const_iterator;
  using reverse_iterator = container::reverse_iterator;
  using const_reverse_iterator = container::const_reverse_iterator;
  iterator begin() { return points.begin(); }
  iterator end() { return points.end(); }
  const_iterator begin() const { return points.begin(); }
  const_iterator end() const { return points.end(); }
  const_iterator cbegin() const { return points.cbegin(); }
  const_iterator cend() const { return points.cend(); }
  reverse_iterator rbegin() { return points.rbegin(); }
  reverse_iterator rend() { return points.rend(); }
  const_reverse_iterator rbegin() const { return points.rbegin(); }
  const_reverse_iterator rend() const { return points.rend(); }
  const_reverse_iterator crbegin() { return points.crbegin(); }
  const_reverse_iterator crend() { return points.crend(); }
  Snapshot& front() { return points.front(); }
  const Snapshot& front() const { return points.front(); }
  Snapshot& back() { return points.back(); }
  const Snapshot& back() const { return points.back(); }

  void reserve(std::size_t size) { return points.reserve(size); }
  void push_back(const Snapshot& v) { return points.push_back(std::move(v)); }
  void push_back(Snapshot&& v) { return points.push_back(std::move(v)); }
};

}  // namespace sekai::run_analysis
