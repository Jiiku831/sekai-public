#pragma once

#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "imgui.h"
#include "implot.h"
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

struct NormalDistributionOptions {
  std::optional<double> clamp_min;
  std::optional<double> clamp_max;
  bool draw_mu = false;
};

class NormalDistributionPdf : public PlotBase {
 public:
  NormalDistributionPdf(std::string_view title, double mu, double sigma,
                        std::optional<ImVec4> color = std::nullopt,
                        NormalDistributionOptions options = {})
      : mu_(mu), sigma_(sigma), title_(title), color_(color), options_(options) {}

  void Draw(const PlotOptions& options) const override;

 private:
  double mu_;
  double sigma_;
  std::string title_;
  std::optional<ImVec4> color_;
  NormalDistributionOptions options_;
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

template <typename T>
class HistogramPlot : public PlotBase {
 public:
  HistogramPlot(std::string_view title, std::span<const T> points, HistogramOptions options = {})
      : title_(title), options_(std::move(options)) {
    if (options.drop_zeros) {
      points_ =
          RangesTo<std::vector>(points | std::views::filter([](const int x) { return x != 0; }));
    } else {
      points_ = {points.begin(), points.end()};
    }
  }

  void Draw(const PlotOptions& options) const override {
    ImPlot::PlotHistogram(title_.c_str(), points_.data(), points_.size(), options_.bins,
                          /*bar_scale=*/1.0, ImPlotRange(), ImPlotHistogramFlags_Density);
  }

 private:
  std::string title_;
  HistogramOptions options_;
  std::vector<T> points_;
};

template <typename T>
HistogramPlot(std::string_view, const std::vector<T>&) -> HistogramPlot<T>;

}  // namespace sekai::run_analysis
