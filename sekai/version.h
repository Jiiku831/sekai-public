#pragma once

#include <array>
#include <compare>
#include <cstddef>
#include <utility>

#include "absl/log/absl_check.h"
#include "absl/log/log.h"
#include "absl/strings/numbers.h"
#include "absl/strings/str_join.h"
#include "absl/strings/str_split.h"
#include "absl/time/time.h"

namespace sekai {

template <std::size_t N>
class Version {
 public:
  explicit Version(absl::string_view version_string) {
    version_.fill(0);
    std::vector<std::string> parts = absl::StrSplit(version_string, ".");
    ABSL_CHECK_LE(parts.size(), N)
        << "Expected at most " << N << " parts in version string: " << version_string;
    if (parts.size() < N) {
      LOG(WARNING) << "Got less than " << N << " parts in version string: " << version_string;
    }
    for (std::size_t i = 0; i < parts.size(); ++i) {
      ABSL_CHECK(absl::SimpleAtoi(parts[i], &version_[i]))
          << "Failed to parse \"" << parts[i] << "\" as an integer.";
    }
  }

  constexpr explicit Version(std::array<int, N> version) : version_(std::move(version)) {}

  template <std::size_t M>
  friend std::strong_ordering operator<=>(const Version<M>& lhs, const Version<M>& rhs);

  template <typename Sink, std::size_t M>
  friend void AbslStringify(Sink& sink, const Version<M>& version);

 private:
  std::array<int, N> version_;
};

template <std::size_t N>
std::strong_ordering operator<=>(const Version<N>& lhs, const Version<N>& rhs) {
  for (std::size_t i = 0; i < N; ++i) {
    if (lhs.version_[i] < rhs.version_[i]) {
      return std::strong_ordering::less;
    }
    if (lhs.version_[i] > rhs.version_[i]) {
      return std::strong_ordering::greater;
    }
  }
  return std::strong_ordering::equal;
}

template <std::size_t N>
bool operator==(const Version<N>& lhs, const Version<N>& rhs) {
  return std::strong_ordering::equal == (lhs <=> rhs);
}

template <typename Sink, std::size_t N>
void AbslStringify(Sink& sink, const Version<N>& version) {
  sink.Append(absl::StrJoin(version.version_, "."));
}

const Version<3>& GetCurrentAppVersion();
const Version<4>& GetCurrentAssetVersion();
const Version<4>& GetCurrentDataVersion();

constexpr Version<4> kPreAnni2AssetVersion({2, 2, 0, 0});
constexpr Version<4> kAnni2AssetVersion({2, 3, 0, 0});
constexpr Version<4> kAnni3AssetVersion({3, 0, 0, 0});
constexpr Version<4> kEndOfWlAssetVersion({3, 8, 0, 30});
constexpr Version<4> kAnni4AssetVersion({4, 0, 0, 0});
constexpr Version<4> kMovieAssetVersion({5, 0, 0, 21});
constexpr Version<4> kAnni4p5AssetVersion({5, 2, 0, 0});
constexpr Version<4> kAnni5AssetVersion({6, 0, 0, 0});
constexpr Version<4> kNewYear5AssetVersion({6, 2, 0, 30});

absl::Time Get4thAnniResetTime();
absl::Time Get4thAnniReleaseTime();
absl::Time Get5thAnniReleaseTime();
Version<4> GetAssetVersionAt(absl::Time time);

}  // namespace sekai
