#include "sekai/run_analysis/testing/plot.h"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <numbers>
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
  std::vector<float> x;
  std::vector<float> y;

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
    seq.x.push_back((point.time) / absl::Minutes(1));
    seq.y.push_back(point.points == std::numeric_limits<int>::min()
                        ? std::numeric_limits<float>::quiet_NaN()
                        : point.points);
  }
  return seq;
}

}  // namespace

void MarkersPlot::Draw(const PlotOptions& options) const {
  if (points_.empty()) return;
  PointSequence seq = SplitSequence(points_);
  if (color_.has_value()) {
    ImPlot::SetNextFillStyle(*color_);
    ImPlot::SetNextLineStyle(*color_);
  }
  ImPlot::PlotScatter(title_.c_str(), seq.x.data(), seq.y.data(), seq.size());
}

void PointsLineGraph::Draw(const PlotOptions& options) const {
  if (points_.empty()) return;
  PointSequence seq = SplitSequence(points_);
  if (color_.has_value()) {
    ImPlot::SetNextFillStyle(*color_);
    ImPlot::SetNextLineStyle(*color_);
  }
  ImPlot::PlotLine(title_.c_str(), seq.x.data(), seq.y.data(), seq.size());
}

void DistributionPlot::Draw(const PlotOptions& options) const {
  std::vector<double> x(options_.samples), y(options_.samples);
  double min = dist_.Ppf(options_.min_quantile);
  double max = dist_.Ppf(options_.max_quantile);
  const double kStep = (max - min) / (options_.samples - 1);
  int actual_samples = options_.samples;
  bool past_min = false;
  double actual_mu = 0;
  for (int i = 0; i < options_.samples; ++i) {
    double pt = min + kStep * i;
    x[i] = pt;
    y[i] = dist_.Pdf(pt);
    if (options_.clamp_min.has_value() && pt <= *options_.clamp_min) {
      y[i] = 0;
    } else if (options_.clamp_min.has_value() && !past_min) {
      past_min = true;
      y[i] = i == 0 ? 0 : dist_.Cdf(pt) / kStep;
    }
    actual_mu += pt * kStep * y[i];
    if (options_.clamp_max.has_value() && pt >= *options_.clamp_max) {
      y[i] = (1.0 - dist_.Cdf(pt)) / kStep;
      actual_samples = i + 1;
      actual_mu += pt * kStep * y[i];
      break;
    }
  }
  if (color_.has_value()) {
    ImPlot::SetNextFillStyle(*color_);
    ImPlot::SetNextLineStyle(*color_);
  }
  ImPlot::PlotLine(title_.c_str(), x.data(), y.data(), actual_samples);
  if (options_.draw_mu) {
    ImPlot::DragLineX(0, &actual_mu, color_.value_or(ImVec4(1, 0, 0, 1)), /*thickness=*/1,
                      ImPlotDragToolFlags_NoInputs);
    ImPlot::TagX(actual_mu, color_.value_or(ImVec4(1, 0, 0, 1)), "%.1f", actual_mu);
  }
}

}  // namespace sekai::run_analysis
