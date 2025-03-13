#pragma once

namespace sekai {

#ifdef __cpp_lib_ranges_to_container
#warning C++23 ranges::to is available to replace this crap.
#endif
template <typename Range>
auto ToVector(Range&& r) {
  return std::vector(r.begin(), r.end());
}

}  // namespace sekai
