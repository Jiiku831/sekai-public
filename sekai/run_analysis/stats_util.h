#pragma once

#include <algorithm>
#include <cmath>
#include <functional>
#include <numeric>
#include <ranges>
#include <utility>

#include <boost/math/distributions/normal.hpp>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "sekai/ranges_util.h"
#include "sekai/run_analysis/proto/service.pb.h"

namespace sekai::run_analysis {

class DistributionBase {
 public:
  virtual ~DistributionBase() = default;
  virtual double Pdf(double x) const = 0;
  virtual double Cdf(double x) const = 0;
  virtual double Ppf(double x) const = 0;
  virtual double Mean() const = 0;
  virtual double Stdev() const = 0;
  virtual double offset() const = 0;
  virtual double scale() const = 0;
};

template <typename Dist>
class Distribution : public DistributionBase {
 public:
  Distribution(Dist dist, double offset = 0, double scale = 1)
      : dist_(std::move(dist)),
        offset_(offset),
        scale_(scale),
        mu_(boost::math::mean(dist_) + offset),
        sigma_(boost::math::standard_deviation(dist_) * scale) {
    std::tie(min_, max_) = boost::math::range(dist_);
  }

  double Pdf(double x) const override { return boost::math::pdf(dist_, Map(x)) * scale_; }
  double Cdf(double x) const override { return boost::math::cdf(dist_, Map(x)) * scale_; }
  double Ppf(double x) const override { return MapInv(boost::math::quantile(dist_, x)); }
  double Mean() const override { return mu_; }
  double Stdev() const override { return sigma_; }
  double offset() const override { return offset_; }
  double scale() const override { return scale_; }

 private:
  double Map(double x) const { return std::clamp(scale_ * (x - offset_), min_, max_); }
  double MapInv(double x) const { return x / scale_ + offset_; }

  Dist dist_;
  double offset_, scale_;
  double mu_, sigma_;
  double min_, max_;
};

template <typename T, typename Cont>
T mean(const Cont& cont) {
  if (cont.empty()) return 0;
  const std::size_t n = cont.size();
  auto r = cont | std::views::transform([&](const auto& v) { return static_cast<T>(v) / n; });
  return static_cast<T>(std::accumulate(r.begin(), r.end(), static_cast<T>(0)));
}

// Input is container of pairs (val, weight).
template <typename T, typename Cont>
T weighted_mean(const Cont& cont) {
  if (cont.empty()) return 0;
  auto weights =
      cont | std::views::transform([&](const auto& v) { return static_cast<T>(v.second); });
  const T total_weight = std::accumulate(weights.begin(), weights.end(), static_cast<T>(0));
  auto r = cont | std::views::transform([&](const auto& v) {
             return static_cast<T>(v.first) * v.second / total_weight;
           });
  return static_cast<T>(std::accumulate(r.begin(), r.end(), static_cast<T>(0)));
}

template <typename T, typename Cont>
T variance(const Cont& cont, const T mu) {
  if (cont.empty()) return 0;
  return mean<T>(cont | std::views::transform([&](const auto& v) {
                   T d = v - mu;
                   return d * d;
                 }));
}

// Input is container of pairs (val, weight).
template <typename T, typename Cont>
T weighted_variance(const Cont& cont, const T mu) {
  if (cont.empty()) return 0;
  return weighted_mean<T>(cont | std::views::transform([&](const auto& v) {
                            T d = v.first - mu;
                            return std::make_pair(d * d, v.second);
                          }));
}

template <typename T, typename Cont>
T stdev(const Cont& cont, const T mu) {
  if (cont.empty()) return 0;
  return std::sqrt(variance<T>(cont, mu));
}

// Input is container of pairs (val, weight).
template <typename T, typename Cont>
T weighted_stdev(const Cont& cont, const T mu) {
  if (cont.empty()) return 0;
  return std::sqrt(weighted_variance<T>(cont, mu));
}

class EpanechnikovKernel {
 public:
  explicit EpanechnikovKernel(float bandwidth) : bandwidth_(bandwidth) {}

  float operator()(float x) const {
    x /= bandwidth_;
    if (std::abs(x) <= 1) return 0.75 * (1 - x * x) / bandwidth_;
    return 0;
  }

 private:
  float bandwidth_;
};

// Input is sorted container of pairs (val, weight).
template <typename Kernel, typename Cont>
absl::StatusOr<ValueDist2> ComputeDist(const Kernel& kernel, const Cont& sorted_vals, int n_samples,
                                       float outliers = 0.99) {
  ValueDist2 res;
  if (sorted_vals.empty()) return res;
  const float total_weight = std::ranges::fold_left(
      sorted_vals |
          std::views::transform([](const std::pair<float, float>& v) { return v.second; }),
      0, std::plus<float>());
  if (total_weight == 0) return res;
  float seen_weight = 0;
  const float min_weight = ((1 - outliers) / 2) * total_weight;
  const float max_weight = outliers * total_weight;
  std::optional<float> min_val;
  std::optional<float> max_val;
  for (const std::pair<float, float>& v : sorted_vals) {
    seen_weight += v.second;
    while (seen_weight > static_cast<float>(res.percentiles_size()) / 100 * total_weight) {
      res.add_percentiles(v.first);
    }
    if (seen_weight < min_weight) continue;
    if (!min_val.has_value()) {
      min_val = v.first;
    }
    if (!max_val.has_value() && seen_weight >= max_weight) {
      max_val = v.first;
    }
  }
  res.add_percentiles(sorted_vals.rbegin()->first);
  // This should never happen.
  if (!min_val.has_value() || !max_val.has_value()) {
    return absl::InternalError("No min or max value.");
  }

  const float step = (*max_val - *min_val) / (n_samples - 1);
  for (int i = 0; i < n_samples; ++i) {
    ValueDist2::Point& point = *res.add_points();
    const float x = *min_val + step * i;
    point.set_val(x);
    point.set_pdf(weighted_mean<float>(RangesTo<std::vector>(
        sorted_vals | std::views::transform([&](const std::pair<float, float>& v) {
          return std::make_pair(kernel(x - v.first), v.second);
        }))));
  }
  return res;
}

}  // namespace sekai::run_analysis
