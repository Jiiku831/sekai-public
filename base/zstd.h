#pragma once

#include <string>
#include <type_traits>

#include "absl/log/absl_check.h"
#include "absl/log/log.h"

namespace zstd {
#include "lib/zstd.h"

static constexpr int kCompressionLevel = 5;  // Should be between 1-22.

template <typename T>
struct Compression final {
  using char_type = typename std::char_traits<T>::char_type;
  using ContainerType = std::basic_string<char_type>;

  inline auto operator()(const char_type* input, const size_t input_size) {
    ABSL_CHECK_LT(0u, input_size);

    const size_t required = ZSTD_compressBound(input_size);
    ContainerType block(required, 0x0);
    const size_t actual =
        ZSTD_compress(block.data(), block.size(), input, input_size, kCompressionLevel);
    ABSL_CHECK(!ZSTD_isError(actual));
    block.erase(block.begin() + actual, block.end());

    return block;
  }

  template <typename Container, typename IteratorValueType = typename std::iterator_traits<
                                    typename Container::iterator>::value_type>
  inline auto operator()(const Container& input)
      -> std::enable_if_t<std::is_same_v<char_type, IteratorValueType>, ContainerType> {
    return this->operator()(input.data(), input.size());
  }
};

using Compressor = Compression<char>;

template <typename T>
struct Decompression final {
  using char_type = typename std::char_traits<T>::char_type;
  using ContainerType = std::basic_string<char_type>;

  inline auto operator()(const char_type* input, const size_t input_size) {
    ABSL_CHECK_LT(0u, input_size);

    const size_t size = ZSTD_getFrameContentSize(input, input_size);
    ABSL_CHECK_NE(ZSTD_CONTENTSIZE_ERROR, size);
    ABSL_CHECK_NE(ZSTD_CONTENTSIZE_UNKNOWN, size);

    ContainerType block(size, 0x0);
    const size_t actual = ZSTD_decompress(block.data(), block.size(), input, input_size);
    ABSL_CHECK(!ZSTD_isError(actual));
    block.resize(actual);

    return block;
  }

  template <typename Container, typename IteratorValueType = typename std::iterator_traits<
                                    typename Container::iterator>::value_type>
  inline auto operator()(const Container& input)
      -> std::enable_if_t<std::is_same_v<char_type, IteratorValueType>, ContainerType> {
    return this->operator()(input.data(), input.size());
  }
};

using Decompressor = Decompression<char>;
}  // namespace zstd
