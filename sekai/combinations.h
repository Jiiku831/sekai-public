#pragma once

#include <iostream>
#include <span>
#include <vector>

#include "absl/base/nullability.h"
#include "absl/functional/any_invocable.h"
#include "absl/log/absl_check.h"

namespace sekai {

template <typename T>
class CombinationsBase {
 public:
  CombinationsBase(std::span<const T> pool,
                   absl::AnyInvocable<bool(std::span<const T>) const> loop_body)
      : pool_(pool), loop_body_(std::move(loop_body)) {}

 protected:
  std::span<const T> pool_;
  absl::AnyInvocable<bool(std::span<const T>) const> loop_body_;
};

template <typename T, int N>
class [[nodiscard]] Combinations : public CombinationsBase<T> {};

template <typename T>
class [[nodiscard]] Combinations<T, 5> : public CombinationsBase<T> {
 public:
  Combinations(std::span<const T> pool,
               absl::AnyInvocable<bool(std::span<const T>) const> loop_body)
      : CombinationsBase<T>(pool, std::move(loop_body)) {}

  void operator()() {
    std::span<const T>& pool = CombinationsBase<T>::pool_;
    absl::AnyInvocable<bool(std::span<const T>) const>& loop_body = CombinationsBase<T>::loop_body_;
    int pool_size = pool.size();
    std::array<T, 5> seq;
    for (int i0 = 0; i0 < pool_size; ++i0) {
      seq[0] = pool[i0];
      for (int i1 = i0 + 1; i1 < pool_size; ++i1) {
        seq[1] = pool[i1];
        for (int i2 = i1 + 1; i2 < pool_size; ++i2) {
          seq[2] = pool[i2];
          for (int i3 = i2 + 1; i3 < pool_size; ++i3) {
            seq[3] = pool[i3];
            for (int i4 = i3 + 1; i4 < pool_size; ++i4) {
              seq[4] = pool[i4];
              if (!loop_body(seq)) {
                return;
              }
            }
          }
        }
      }
    }
  }
};

template <typename T>
class ProductBase {
 public:
  ProductBase(std::span<std::span<const T>> pools,
              absl::AnyInvocable<bool(std::span<const T>) const> loop_body)
      : pools_(pools), loop_body_(std::move(loop_body)) {}

 protected:
  std::span<std::span<const T>> pools_;
  absl::AnyInvocable<bool(std::span<const T>) const> loop_body_;
};

template <typename T, int N>
class [[nodiscard]] Product : public ProductBase<T> {};

template <typename T>
class [[nodiscard]] Product<T, 5> : public ProductBase<T> {
 public:
  Product(std::span<std::span<const T>> pools,
          absl::AnyInvocable<bool(std::span<const T>) const> loop_body)
      : ProductBase<T>(pools, std::move(loop_body)) {
    ABSL_CHECK_EQ(static_cast<int64_t>(pools.size()), 5);
  }

  void operator()() {
    std::span<std::span<const T>>& pools = ProductBase<T>::pools_;
    absl::AnyInvocable<bool(std::span<const T>) const>& loop_body = ProductBase<T>::loop_body_;
    std::array<int, 5> pool_sizes;
    for (int i = 0; i < 5; ++i) {
      pool_sizes[i] = pools[i].size();
    }
    std::array<T, 5> seq;
    for (int i0 = 0; i0 < pool_sizes[0]; ++i0) {
      seq[0] = pools[0][i0];
      for (int i1 = 0; i1 < pool_sizes[1]; ++i1) {
        seq[1] = pools[1][i1];
        for (int i2 = 0; i2 < pool_sizes[2]; ++i2) {
          seq[2] = pools[2][i2];
          for (int i3 = 0; i3 < pool_sizes[3]; ++i3) {
            seq[3] = pools[3][i3];
            for (int i4 = 0; i4 < pool_sizes[4]; ++i4) {
              seq[4] = pools[4][i4];
              if (!loop_body(seq)) {
                return;
              }
            }
          }
        }
      }
    }
  }
};

struct Extent {
  std::size_t lo = 0;
  std::size_t hi = 0;
};

template <int N>
bool StepExtents(const std::size_t block_size, std::span<const std::size_t> pool_sizes,
                 std::array<Extent, N>& extents) {
  for (std::size_t i = 0; i < pool_sizes.size(); ++i) {
    extents[i].lo += block_size;
    extents[i].hi = std::min(extents[i].lo + block_size, pool_sizes[i]);
    if (extents[i].lo < pool_sizes[i]) {
      return true;
    }
    extents[i].lo = 0;
    extents[i].hi = std::min(block_size, pool_sizes[i]);
  }
  return false;
}

template <int N>
void PartitionedProduct(const std::size_t block_size, std::span<const std::size_t> pool_sizes,
                        absl::AnyInvocable<bool(const std::array<Extent, N>&) const> loop_body) {
  ABSL_CHECK_EQ(static_cast<int64_t>(pool_sizes.size()), N);
  std::array<Extent, N> extents{};
  for (std::size_t i = 0; i < pool_sizes.size(); ++i) {
    extents[i].lo = 0;
    extents[i].hi = std::min(block_size, pool_sizes[i]);
  }

  while (loop_body(extents) && StepExtents<N>(block_size, pool_sizes, extents)) {
  }
}

// Warning: copies values
template <int N, typename T>
void ForEachInBlock(const std::array<Extent, N>& extents,
                    const std::array<std::span<const T>, N>& values,
                    absl::AnyInvocable<bool(const std::array<T, N>&) const> loop_body) {
  std::array<std::size_t, N> indices;
  for (int i = 0; i < N; ++i) {
    indices[i] = extents[i].lo;
  }
  bool at_end = false;
  do {
    std::array<T, N> value;
    for (int i = 0; i < N; ++i) {
      ABSL_CHECK_LT(indices[i], values[i].size());
      value[i] = values[i][indices[i]];
    }
    if (!loop_body(value)) {
      break;
    }

    at_end = true;
    for (int i = 0; i < N; ++i) {
      ++indices[i];
      if (indices[i] != extents[i].hi) {
        at_end = false;
        break;
      }
      indices[i] = extents[i].lo;
    }
  } while (!at_end);
}

}  // namespace sekai
