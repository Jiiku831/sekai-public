#pragma once

namespace sekai {

#ifdef __cpp_lib_ranges_to_container
#warning C++23 ranges::to is available to replace this crap.
#endif
template <typename Container, typename Range>
auto RangesTo(Range&& r) {
  return Container(r.begin(), r.end());
}

}  // namespace sekai
