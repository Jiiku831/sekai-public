#pragma once

#include <span>
#include <string>
#include <string_view>

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

class PointsLineGraph : public PlotBase {
 public:
  // Input must outlive this class.
  PointsLineGraph(std::string_view title, const Sequence& points)
      : points_(points), title_(title) {}

  void Draw(const PlotOptions& options) const override;

 private:
  const Sequence& points_;
  std::string title_;
};

class SegmentsLineGraph : public PlotBase {
 public:
  // Input must outlive this class.
  SegmentsLineGraph(std::span<const Sequence> segments);

  void Draw(const PlotOptions& options) const override;

 private:
  std::vector<PointsLineGraph> segment_graphs_;
};

struct HistogramOptions {
  bool drop_zeros = true;
  int bins = 100;
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
