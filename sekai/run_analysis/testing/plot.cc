#include "sekai/run_analysis/testing/plot.h"

#include <cstdint>
#include <span>
#include <vector>

#include "absl/strings/str_cat.h"
#include "implot.h"
#include "sekai/run_analysis/snapshot.h"

namespace sekai::run_analysis {

void PointsLineGraph::Draw(const PlotOptions& options) const {
  if (points_.empty()) return;
  std::vector<int> x;
  std::vector<int> y;
  x.reserve(points_.size());
  y.reserve(points_.size());

  for (const Snapshot& point : points_) {
    x.push_back(point.timestamp);
    y.push_back(point.points);
  }

  ImPlot::PlotLine(title_.c_str(), x.data(), y.data(), x.size());
}

SegmentsLineGraph::SegmentsLineGraph(std::span<const Segment> segments) {
  segment_graphs_.reserve(segments.size());
  int counter = 0;
  for (const Segment& segment : segments) {
    segment_graphs_.emplace_back(segment.points, absl::StrCat("Segment ", counter++));
  }
}

void SegmentsLineGraph::Draw(const PlotOptions& options) const {
  for (const PointsLineGraph& graph : segment_graphs_) {
    graph.Draw(options);
  }
}

}  // namespace sekai::run_analysis
