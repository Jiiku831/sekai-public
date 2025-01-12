#include "sekai/bonus_limit.h"

#include <algorithm>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "sekai/db/master_db.h"
#include "sekai/db/proto/all.h"

namespace sekai {
namespace {

using ::sekai::db::MasterDb;

TEST(BonusLimitTest, TestMySekaiFixtureBonusLimit) {
  auto limits = MasterDb::GetAll<db::MySekaiFixtureGameCharacterPerformanceBonusLimit>();
  EXPECT_EQ(limits.size(), 1);
  EXPECT_EQ(limits[0].bonus_rate_limit(), kMaxMySekaiFixtureBonusLimit);
}

}  // namespace
}  // namespace sekai
