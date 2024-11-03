#include "sekai/version.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "sekai/db/master_db.h"

namespace sekai {
namespace {

using ::sekai::db::MasterDb;

TEST(VersionTest, CompareEqual) { EXPECT_EQ(Version<4>("1.2.3.4"), Version<4>({1, 2, 3, 4})); }

TEST(VersionTest, CompareLess) {
  EXPECT_LT(Version<4>("1.2.3.4"), Version<4>("1.2.3.5"));
  EXPECT_LT(Version<4>("1.2.3.4"), Version<4>("1.2.4.4"));
  EXPECT_LT(Version<4>("1.2.3.4"), Version<4>("1.3.3.4"));
  EXPECT_LT(Version<4>("1.2.3.4"), Version<4>("2.2.3.4"));
}

TEST(VersionTest, CompareGreater) {
  EXPECT_GT(Version<4>("1.2.3.5"), Version<4>("1.2.3.4"));
  EXPECT_GT(Version<4>("1.2.4.4"), Version<4>("1.2.3.4"));
  EXPECT_GT(Version<4>("1.3.3.4"), Version<4>("1.2.3.4"));
  EXPECT_GT(Version<4>("2.2.3.4"), Version<4>("1.2.3.4"));
}

TEST(DbVersionsTest, ExactlyOneVersion) {
  std::span<const db::Version> versions = MasterDb::GetAll<db::Version>();
  EXPECT_EQ(versions.size(), 1);
}

TEST(GetCurrentAppVersionTest, Returns) {
  LOG(INFO) << "Current App Version: " << GetCurrentAppVersion();
  EXPECT_NE(GetCurrentAppVersion(), Version<3>({0, 0, 0}));
}

TEST(GetCurrentAssetVersionTest, Returns) {
  LOG(INFO) << "Current Asset Version: " << GetCurrentAssetVersion();
  EXPECT_NE(GetCurrentAssetVersion(), Version<4>({0, 0, 0, 0}));
}

TEST(GetCurrentDataVersionTest, Returns) {
  LOG(INFO) << "Current Data Version: " << GetCurrentDataVersion();
  EXPECT_NE(GetCurrentDataVersion(), Version<4>({0, 0, 0, 0}));
}

TEST(GetAssetVersionAt, ReturnsCurrentAssetVersionAtNow) {
  EXPECT_EQ(GetAssetVersionAt(absl::Now()), GetCurrentAssetVersion());
}

}  // namespace
}  // namespace sekai
