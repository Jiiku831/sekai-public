#include "sekai/run_analysis/testing/plot.h"

#include <cstdint>
#include <span>
#include <vector>

#include "absl/strings/str_cat.h"
#include "absl/time/time.h"
#include "implot.h"
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

}  // namespace sekai::run_analysis
