#pragma once

#include <cstddef>
#include <vector>

#include "absl/time/time.h"
#include "sekai/run_analysis/snapshot.h"

namespace sekai::run_analysis {

struct Histograms {
  // Values of all steps.
  std::vector<int> steps;
  Sequence step_seq;

  // Values of hourly speeds computed from each step.
  std::vector<int> speeds;

  // Values of smoothed hourly speeds over the given window.
  int smoothing_window = 0;
  std::vector<int> smoothed_speeds;
  Sequence smooth_seq;

  // Values of smoothed hourly speeds over each segment;
  std::vector<int> segment_speeds;

  void reserve(std::size_t n) {
    step_seq.reserve(n);
    steps.reserve(n);
    speeds.reserve(n);
    smooth_seq.reserve(n);
    smoothed_speeds.reserve(n);
    segment_speeds.reserve(n);
  }

  static Histograms Join(std::span<const Histograms> others);
};

Histograms ComputeHistograms(const Sequence& segment, int smoothing_window,
                             absl::Duration interval);

}  // namespace sekai::run_analysis
