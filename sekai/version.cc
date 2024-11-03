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

Version<4> GetAssetVersionAt(absl::Time time) {
  // TODO: implement
  return GetCurrentAssetVersion();
}

}  // namespace sekai
