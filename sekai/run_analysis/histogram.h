#pragma once

#include <cstddef>
#include <vector>

#include "absl/time/time.h"
#include "sekai/run_analysis/snapshot.h"

namespace sekai::run_analysis {

struct Histograms {
  // Values of all steps.
  std::vector<int> steps;

  // Values of hourly speeds computed from each step.
  std::vector<int> speeds;

  // Values of smoothed hourly speeds over the given window.
  int smoothing_window = 0;
  std::vector<int> smoothed_speeds;

  void reserve(std::size_t n) {
    steps.reserve(n);
    speeds.reserve(n);
    smoothed_speeds.reserve(n);
  }

  static Histograms Join(std::span<const Histograms> others);
};

Histograms ComputeHistograms(std::span<const Sequence> segments, int smoothing_window,
                             absl::Duration interval);

}  // namespace sekai::run_analysis
