#include "sekai/run_analysis/testing/plot.h"

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

void PointsLineGraph::Draw(const PlotOptions& options) const {
  if (points_.empty()) return;
  std::vector<int> x;
  std::vector<int> y;
  x.reserve(points_.size());
  y.reserve(points_.size());

  for (const Snapshot& point : points_.points) {
    x.push_back(static_cast<int>((point.time) / absl::Minutes(1)));
    y.push_back(point.points);
  }

  ImPlot::PlotLine(title_.c_str(), x.data(), y.data(), x.size());
}

SegmentsLineGraph::SegmentsLineGraph(std::span<const Sequence> segments) {
  segment_graphs_.reserve(segments.size());
  int counter = 0;
  for (const Sequence& segment : segments) {
    segment_graphs_.emplace_back(absl::StrCat("Segment ", counter++), segment);
  }
}

void SegmentsLineGraph::Draw(const PlotOptions& options) const {
  for (const PointsLineGraph& graph : segment_graphs_) {
    graph.Draw(options);
  }
}

HistogramPlot::HistogramPlot(std::string_view title, std::span<const int> points,
                             HistogramOptions options)
    : title_(title), options_(std::move(options)) {
  if (options.drop_zeros) {
    points_ = ToVector(points | std::views::filter([](const int x) { return x != 0; }));
  } else {
    points_ = {points.begin(), points.end()};
  }
}

void HistogramPlot::Draw(const PlotOptions& options) const {
  ImPlot::PlotHistogram(title_.c_str(), points_.data(), points_.size(), options_.bins,
                        /*bar_scale=*/1.0, ImPlotRange(), ImPlotHistogramFlags_Density);
}

}  // namespace sekai::run_analysis
