#include "sekai/run_analysis/histogram.h"

#include <algorithm>
#include <functional>
#include <iterator>
#include <numeric>
#include <ranges>
#include <vector>
#include <version>

#include "absl/time/time.h"
#include "sekai/ranges_util.h"
#include "sekai/run_analysis/snapshot.h"

namespace sekai::run_analysis {

namespace {

Histograms ComputeForSegment(int smoothing_window, absl::Duration interval,
                             const Sequence& segment) {
  Histograms histograms;
  histograms.smoothing_window = smoothing_window;
  histograms.steps = ToVector(std::views::zip(std::span(segment.begin(), segment.end() - 1),
                                              std::span(segment.begin() + 1, segment.end())) |
                              std::views::transform([&](const auto& window) {
                                return (std::get<1>(window).points - std::get<0>(window).points);
                              }));
  histograms.speeds =
      ToVector(histograms.steps | std::views::transform([&](int val) {
                 return static_cast<int>(val * (60.0f / (interval / absl::Minutes(1))));
               }));
  if (segment.size() > smoothing_window) {
#ifdef __cpp_lib_ranges_slide
#warning C++23 ranges::slide is available to replace this crap.
#endif
    histograms.smoothed_speeds = ToVector(
        std::views::zip(std::span(segment.begin(), segment.end() - smoothing_window),
                        std::span(segment.begin() + smoothing_window, segment.end())) |
        std::views::transform([&](const auto& window) {
          return static_cast<int>((std::get<1>(window).points - std::get<0>(window).points) *
                                  (60.0f / (smoothing_window * interval / absl::Minutes(1))));
        }));
  }
  return histograms;
}

}  // namespace

Histograms Histograms::Join(std::span<const Histograms> others) {
  Histograms histograms;
  if (others.empty()) {
    return histograms;
  }
  histograms.smoothing_window = others[0].smoothing_window;
  auto sizes = others | std::views::transform([](const Histograms& h) { return h.steps.size(); });
  histograms.reserve(std::accumulate(sizes.begin(), sizes.end(), 0));
  for (const Histograms& other : others) {
    histograms.steps.insert(histograms.steps.end(), other.steps.begin(), other.steps.end());
    histograms.speeds.insert(histograms.speeds.end(), other.speeds.begin(), other.speeds.end());
    histograms.smoothed_speeds.insert(histograms.smoothed_speeds.end(),
                                      other.smoothed_speeds.begin(), other.smoothed_speeds.end());
  }
  return histograms;
}

Histograms ComputeHistograms(std::span<const Sequence> segments, int smoothing_window,
                             absl::Duration interval) {
  return Histograms::Join(ToVector(segments | std::views::transform(std::bind_front(
                                                  ComputeForSegment, smoothing_window, interval))));
}

}  // namespace sekai::run_analysis
