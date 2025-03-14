#include "sekai/run_analysis/testing/plot.h"

#include <cstddef>
#include <cstdint>
#include <ranges>
#include <span>
#include <vector>

#include "absl/log/log.h"
#include "absl/strings/str_cat.h"
#include "absl/time/time.h"
#include "implot.h"
#include "plot.h"
#include "sekai/ranges_util.h"
#include "sekai/run_analysis/snapshot.h"

namespace sekai::run_analysis {
namespace {

struct PointSequence {
  std::vector<int> x;
  std::vector<int> y;

  void reserve(std::size_t n) {
    x.reserve(n);
    y.reserve(n);
  }

  std::size_t size() const { return std::min(x.size(), y.size()); }
};

PointSequence SplitSequence(const Sequence& sequence) {
  PointSequence seq;
  seq.reserve(sequence.size());
  for (const Snapshot& point : sequence) {
    seq.x.push_back(static_cast<int>((point.time) / absl::Minutes(1)));
    seq.y.push_back(point.points);
  }
  return seq;
}

}  // namespace

void MarkersPlot::Draw(const PlotOptions& options) const {
  if (points_.empty()) return;
  PointSequence seq = SplitSequence(points_);
  ImPlot::PlotScatter(title_.c_str(), seq.x.data(), seq.y.data(), seq.size());
}

void PointsLineGraph::Draw(const PlotOptions& options) const {
  if (points_.empty()) return;
  PointSequence seq = SplitSequence(points_);
  ImPlot::PlotLine(title_.c_str(), seq.x.data(), seq.y.data(), seq.size());
}

HistogramPlot::HistogramPlot(std::string_view title, std::span<const int> points,
                             HistogramOptions options)
    : title_(title), options_(std::move(options)) {
  if (options.drop_zeros) {
    points_ =
        RangesTo<std::vector<int>>(points | std::views::filter([](const int x) { return x != 0; }));
  } else {
    points_ = {points.begin(), points.end()};
  }
}

void HistogramPlot::Draw(const PlotOptions& options) const {
  ImPlot::PlotHistogram(title_.c_str(), points_.data(), points_.size(), options_.bins,
                        /*bar_scale=*/1.0, ImPlotRange(), ImPlotHistogramFlags_Density);
}

}  // namespace sekai::run_analysis
