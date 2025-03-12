#pragma once

#include <cstdint>

namespace sekai::run_analysis {

struct Snapshot {
  int timestamp;
  int points;

  auto operator<=>(const Snapshot&) const = default;
};

}  // namespace sekai::run_analysis
