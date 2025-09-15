#pragma once

#include <algorithm>
#include <ranges>
#include <type_traits>
#include <vector>

#include "absl/log/log.h"
#include "absl/status/statusor.h"
#include "base/util.h"

namespace sekai {

#ifdef __cpp_lib_ranges_to_container
#warning C++23 ranges::to is available to replace this crap.
#endif
template <template <typename> typename Container, typename Range,
          typename T = std::ranges::iterator_t<Range>::value_type>
Container<T> RangesTo(Range&& r) {
  return Container<T>(r.begin(), r.end());
}

inline auto LogAndDiscardErrors() {
  return std::views::transform([&](const auto& value_or) {
           if (!value_or.ok()) {
             LOG(ERROR) << value_or.status();
           }
           return value_or;
         }) |
         std::views::filter([](const auto& value_or) { return value_or.ok(); }) |
         std::views::transform([&](const auto& value_or) { return *value_or; });
}

template <typename Range, typename T = std::ranges::iterator_t<Range>::value_type::value_type>
absl::StatusOr<std::vector<T>> UnwrapStatusOr(Range&& r) {
  return std::ranges::fold_left(
      r, absl::StatusOr<std::vector<T>>(std::vector<T>()),
      [](absl::StatusOr<std::vector<T>>&& output,
         const absl::StatusOr<T>& next) -> absl::StatusOr<std::vector<T>> {
        RETURN_IF_ERROR(output.status());
        RETURN_IF_ERROR(next.status());
        output->push_back(*next);
        return output;
      });
}

template <template <typename> typename Container, typename Range,
          typename T = std::ranges::iterator_t<Range>::value_type::value_type>
absl::StatusOr<Container<T>> UnwrapStatusOrTo(Range&& r) {
  ASSIGN_OR_RETURN(std::vector<T> res, UnwrapStatusOr(r));
  if constexpr (std::is_same_v<Container<T>, std::vector<T>>) {
    return res;
  } else {
    return RangesTo<Container>(std::move(res));
  }
}

template <typename Fn>
auto RangeMapStatusOr(Fn fn) {
  return std::views::transform([&](auto v) { return MapStatusOr(fn, v); });
}

template <typename Fn>
auto RangeBindStatusOr(Fn fn) {
  return std::views::transform([&](auto v) { return BindStatusOr(fn, v); });
}

}  // namespace sekai
