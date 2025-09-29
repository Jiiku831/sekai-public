#include "sekai/version.h"

#include <span>

#include "absl/base/no_destructor.h"
#include "sekai/db/master_db.h"
#include "sekai/db/proto/all.h"

namespace sekai {
namespace {

using ::sekai::db::MasterDb;

const db::Version& GetCurrentDbVersion() {
  std::span<const db::Version> versions = MasterDb::GetAll<db::Version>();
  ABSL_CHECK(!versions.empty());
  return versions.front();
}

}  // namespace

const Version<3>& GetCurrentAppVersion() {
  static const absl::NoDestructor<Version<3>> kVersion(GetCurrentDbVersion().app_version());
  return *kVersion;
}

const Version<4>& GetCurrentAssetVersion() {
  static const absl::NoDestructor<Version<4>> kVersion(GetCurrentDbVersion().asset_version());
  return *kVersion;
}

const Version<4>& GetCurrentDataVersion() {
  static const absl::NoDestructor<Version<4>> kVersion(GetCurrentDbVersion().data_version());
  return *kVersion;
}

absl::Time Get2ndAnniReleaseTime() {
  return absl::FromCivil(absl::CivilSecond(2022, 9, 27, 15, 0, 0), absl::UTCTimeZone());
}

absl::Time Get3rdAnniReleaseTime() {
  return absl::FromCivil(absl::CivilSecond(2023, 9, 27, 15, 0, 0), absl::UTCTimeZone());
}

absl::Time GetEndOfWlReleaseTime() {
  return absl::FromCivil(absl::CivilSecond(2024, 8, 30, 2, 0, 0), absl::UTCTimeZone());
}

absl::Time Get4thAnniResetTime() {
  return absl::FromCivil(absl::CivilSecond(2024, 9, 27, 19, 0, 0), absl::UTCTimeZone());
}

absl::Time Get4thAnniReleaseTime() {
  return absl::FromCivil(absl::CivilSecond(2024, 9, 27, 2, 0, 0), absl::UTCTimeZone());
}

absl::Time GetMovieReleaseTime() {
  return absl::FromCivil(absl::CivilSecond(2025, 1, 17, 2, 0, 0), absl::UTCTimeZone());
}

absl::Time Get4p5AnniReleaseTime() {
  return absl::FromCivil(absl::CivilSecond(2024, 4, 7, 2, 0, 0), absl::UTCTimeZone());
}

absl::Time Get5thAnniReleaseTime() {
  return absl::FromCivil(absl::CivilSecond(2025, 9, 29, 15, 0, 0), absl::UTCTimeZone());
}

Version<4> GetAssetVersionAt(absl::Time time) {
  // TODO: implement properly
  if (time < Get2ndAnniReleaseTime()) {
    return kPreAnni2AssetVersion;
  }
  if (time < Get3rdAnniReleaseTime()) {
    return kAnni2AssetVersion;
  }
  if (time < GetEndOfWlReleaseTime()) {
    return kAnni3AssetVersion;
  }
  if (time < Get4thAnniReleaseTime()) {
    return kEndOfWlAssetVersion;
  }
  if (time < GetMovieReleaseTime()) {
    return kAnni4AssetVersion;
  }
  if (time < Get4p5AnniReleaseTime()) {
    return kMovieAssetVersion;
  }
  if (time < Get5thAnniReleaseTime()) {
    return kAnni4p5AssetVersion;
  }
  if (time > Get5thAnniReleaseTime()) {
    return kAnni5AssetVersion;
  }
  return kAnni5AssetVersion;
}

}  // namespace sekai
