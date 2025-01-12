#include "sekai/max_level.h"

#include <algorithm>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "sekai/db/master_db.h"
#include "sekai/db/proto/all.h"

namespace sekai {
namespace {

using ::sekai::db::MasterDb;

TEST(MaxLevelTest, TestMySekaiGateMaxLevel) {
  for (const auto& gate : MasterDb::GetAll<db::MySekaiGate>()) {
    int max_level = 0;
    for (const auto* level : MasterDb::FindAll<db::MySekaiGateLevel>(gate.id())) {
      max_level = std::max(max_level, level->level());
    }
    EXPECT_EQ(kMaxMySekaiGateLevel, max_level);
  }
}

TEST(MaxLevelTest, TestAreaItemMaxLevel) {
  for (const auto& item : MasterDb::GetAll<db::AreaItem>()) {
    int max_level = 0;
    for (const auto* level : MasterDb::FindAll<db::AreaItemLevel>(item.id())) {
      max_level = std::max(max_level, level->level());
    }
    EXPECT_EQ(kMaxAreaItemLevel, max_level);
  }
}

}  // namespace
}  // namespace sekai
