#include "sekai/run_analysis/histogram.h"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <iterator>
#include <numeric>
#include <ranges>
#include <vector>
#include <version>

#include "absl/time/time.h"
#include "sekai/ranges_util.h"
#include "sekai/run_analysis/snapshot.h"

namespace sekai::run_analysis {  // namespace

Histograms Histograms::Join(std::span<const Histograms> others) {
  Histograms histograms;
  if (others.empty()) {
    return histograms;
  }
  histograms.smoothing_window = others[0].smoothing_window;
  auto sizes = others | std::views::transform([](const Histograms& h) { return h.steps.size(); });
  histograms.reserve(std::accumulate(sizes.begin(), sizes.end(), 0));
  for (const Histograms& other : others) {
    histograms.step_seq.points.insert(histograms.step_seq.points.end(),
                                      other.step_seq.points.begin(), other.step_seq.points.end());
    histograms.steps.insert(histograms.steps.end(), other.steps.begin(), other.steps.end());
    histograms.speeds.insert(histograms.speeds.end(), other.speeds.begin(), other.speeds.end());
    histograms.smoothed_speeds.insert(histograms.smoothed_speeds.end(),
                                      other.smoothed_speeds.begin(), other.smoothed_speeds.end());
    histograms.smooth_seq.points.insert(histograms.smooth_seq.points.end(),
                                        other.smooth_seq.points.begin(),
                                        other.smooth_seq.points.end());
    histograms.segment_speeds.insert(histograms.segment_speeds.end(), other.segment_speeds.begin(),
                                     other.segment_speeds.end());
  }
  return histograms;
}

Histograms ComputeHistograms(const Sequence& segment, int smoothing_window,
                             absl::Duration interval) {
  Histograms histograms;
  histograms.smoothing_window = smoothing_window;
  histograms.step_seq = segment.CopyWithNewPoints(RangesTo<std::vector<Snapshot>>(
      std::views::zip(std::span(segment.begin(), segment.end() - 1),
                      std::span(segment.begin() + 1, segment.end())) |
      std::views::transform([&](const auto& window) {
        return Snapshot{
            std::get<1>(window).time,
            std::get<1>(window).points - std::get<0>(window).points,
        };
      })));
  histograms.steps = RangesTo<std::vector<int>>(
      histograms.step_seq |
      std::views::transform([](const Snapshot& snapshot) { return snapshot.points; }));
  histograms.speeds = RangesTo<std::vector<int>>(
      histograms.steps | std::views::transform([&](int val) {
        return static_cast<int>(val * (60.0f / (interval / absl::Minutes(1))));
      }));
  if (segment.size() > static_cast<uint32_t>(smoothing_window)) {
#ifdef __cpp_lib_ranges_slide
#warning C++23 ranges::slide is available to replace this crap.
#endif
    histograms.smooth_seq = segment.CopyWithNewPoints(RangesTo<std::vector<Snapshot>>(
        std::views::zip(std::span(segment.begin(), segment.end() - smoothing_window),
                        std::span(segment.begin() + smoothing_window, segment.end())) |
        std::views::transform([&](const auto& window) {
          return Snapshot{
              std::get<1>(window).time,
              static_cast<int>((std::get<1>(window).points - std::get<0>(window).points) *
                               (60.0f / (smoothing_window * interval / absl::Minutes(1)))),
          };
        })));
    histograms.smoothed_speeds = RangesTo<std::vector<int>>(
        histograms.smooth_seq |
        std::views::transform([](const Snapshot& snapshot) { return snapshot.points; }));
    if (segment.size() > 1) {
      histograms.segment_speeds.insert(
          histograms.segment_speeds.end(),
          int((segment.back().time - segment.front().time) / interval),
          (segment.back().points - segment.front().points) *
              (60.0f / ((segment.back().time - segment.front().time) / absl::Minutes(1))));
    }
  }
  return histograms;
}

}  // namespace sekai::run_analysis
