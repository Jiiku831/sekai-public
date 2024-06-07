#include "sekai/db/master_db.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "absl/log/log.h"
#include "sekai/db/master_db.h"
#include "sekai/db/proto/all.h"

namespace sekai::db {
namespace {

using ::testing::Each;

MATCHER(IsOk, "") { return arg.ok(); }

TEST(MasterDbTest, TryLoad) {
  const auto& master_db = MasterDb::Get();
  EXPECT_THAT(master_db.Status(), Each(IsOk()));
}

TEST(MasterDbTest, CheckIndex) {
  const auto& master_db = MasterDb::Get();
  EXPECT_EQ(master_db.Get<db::GameCharacter>().FindAll(1).size(), 1);
}

}  // namespace
}  // namespace sekai::db
