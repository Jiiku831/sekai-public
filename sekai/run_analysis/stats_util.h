#pragma once

#include <cmath>
#include <numeric>
#include <ranges>
#include <utility>

#include <boost/math/distributions/normal.hpp>

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

}  // namespace sekai::run_analysis
