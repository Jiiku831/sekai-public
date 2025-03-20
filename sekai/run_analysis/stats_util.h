#pragma once

#include <numeric>
#include <ranges>
#include <utility>

namespace sekai::run_analysis {

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
