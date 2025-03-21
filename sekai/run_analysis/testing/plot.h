#pragma once

#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "imgui.h"
#include "sekai/run_analysis/segmentation.h"
#include "sekai/run_analysis/snapshot.h"

namespace sekai::run_analysis {

struct PlotOptions {};

class PlotBase {
 public:
  ~PlotBase() = default;

  virtual void Draw(const PlotOptions& options) const = 0;
};

template <typename T>
class ConditionalPlot : public PlotBase {
 public:
  ConditionalPlot(const bool* enable, T inner_plot)
      : enable_(enable), inner_plot_(std::move(inner_plot)) {}

  void Draw(const PlotOptions& options) const override {
    if (*enable_) {
      inner_plot_.Draw(options);
    }
  }

 private:
  const bool* enable_;
  T inner_plot_;
};

class MarkersPlot : public PlotBase {
 public:
  // Input must outlive this class.
  MarkersPlot(std::string_view title, const Sequence& points,
              std::optional<ImVec4> color = std::nullopt)
      : points_(points), title_(title), color_(color) {}

  void Draw(const PlotOptions& options) const override;

 private:
  const Sequence& points_;
  std::string title_;
  std::optional<ImVec4> color_;
};

class PointsLineGraph : public PlotBase {
 public:
  // Input must outlive this class.
  PointsLineGraph(std::string_view title, const Sequence& points,
                  std::optional<ImVec4> color = std::nullopt)
      : points_(points), title_(title), color_(color) {}

  void Draw(const PlotOptions& options) const override;

 private:
  const Sequence& points_;
  std::string title_;
  std::optional<ImVec4> color_;
};

template <typename Plot>
class SegmentsPlot : public PlotBase {
 public:
  // Input must outlive this class.
  SegmentsPlot(std::string_view prefix, std::span<const Sequence> segments,
               std::span<const ImVec4> colors = {})
      : prefix_(prefix) {
    segment_graphs_.reserve(segments.size());
    int counter = 0;
    for (const Sequence& segment : segments) {
      std::optional<ImVec4> color;
      if (!colors.empty()) color = colors[counter % colors.size()];
      segment_graphs_.emplace_back(absl::StrCat(prefix_, " ", counter++), segment, color);
    }
  }

  void Draw(const PlotOptions& options) const override {
    for (const Plot& graph : segment_graphs_) {
      graph.Draw(options);
    }
  }

 private:
  std::string prefix_;
  std::vector<Plot> segment_graphs_;
};

struct HistogramOptions {
  bool drop_zeros = true;
  int bins = 200;
};

class HistogramPlot : public PlotBase {
 public:
  // Input must outlive this class.
  HistogramPlot(std::string_view title, std::span<const int> points, HistogramOptions options = {});

  void Draw(const PlotOptions& options) const override;

 private:
  std::string title_;
  HistogramOptions options_;
  std::vector<int> points_;
};

}  // namespace sekai::run_analysis
