#include "sekai/card.h"

#include <array>

#include <Eigen/Eigen>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "absl/log/log.h"
#include "sekai/db/master_db.h"
#include "sekai/db/proto/all.h"
#include "sekai/event_bonus.h"
#include "sekai/proto/card_state.pb.h"
#include "testing/util.h"

namespace sekai {
namespace {

using ::sekai::db::MasterDb;
using ::testing::ParseTextProto;

TEST(CardTest, TestCard1Power) {
  const db::Card& card1 = MasterDb::FindFirst<db::Card>(1);
  auto state = ParseTextProto<CardState>(R"pb(
    level: 20
    master_rank: 3
    card_episodes_read: 1
    card_episodes_read: 2
    special_training: false
  )pb");
  Card card{card1, state};
  EXPECT_EQ(card.power_vec().sum(), 7575 + (300 + 150) * 3);
  EXPECT_EQ(card.power_vec(),
            Eigen::Vector3i(2663 + 300 + 150, 2325 + 300 + 150, 2587 + 300 + 150));
}

TEST(CardTest, TestCard801Power) {
  const db::Card& card1 = MasterDb::FindFirst<db::Card>(801);
  auto state = ParseTextProto<CardState>(R"pb(
    level: 60
    master_rank: 5
    card_episodes_read: 1599
    card_episodes_read: 1600
    special_training: true
  )pb");
  Card card{card1, state};
  EXPECT_EQ(card.power_vec().sum(), 37292);
  EXPECT_EQ(card.power_vec(), Eigen::Vector3i(9254 + 1000 + 850 + 400, 10129 + 1000 + 850 + 400,
                                              11159 + 1000 + 850 + 400));
}

TEST(CardTest, TestCard1Skill) {
  const db::Card& card1 = MasterDb::FindFirst<db::Card>(1);
  std::array expected = {20, 25, 30, 40};
  CardState state;
  for (int i = 1; i <= 4; ++i) {
    state.set_skill_level(i);
    Card card{card1, state};
    EXPECT_EQ(card.raw_skill_value(), expected[i - 1]);
  }
}

TEST(CardTest, TestCard289SkillConditionLife) {
  const db::Card& card1 = MasterDb::FindFirst<db::Card>(289);
  std::array expected = {120, 125, 130, 140};
  CardState state;
  for (int i = 1; i <= 4; ++i) {
    state.set_skill_level(i);
    Card card{card1, state};
    EXPECT_EQ(card.raw_skill_value(), expected[i - 1]);
  }
}

TEST(CardTest, TestCard290SkillKeep) {
  const db::Card& card1 = MasterDb::FindFirst<db::Card>(289);
  std::array expected = {120, 125, 130, 140};
  CardState state;
  for (int i = 1; i <= 4; ++i) {
    state.set_skill_level(i);
    Card card{card1, state};
    EXPECT_EQ(card.raw_skill_value(), expected[i - 1]);
  }
}

TEST(CardTest, TestCard707SkillUnit) {
  const db::Card& card1 = MasterDb::FindFirst<db::Card>(707);
  std::array expected = {80, 85, 90, 100};
  CardState state;
  for (int i = 1; i <= 4; ++i) {
    state.set_skill_level(i);
    Card card{card1, state};
    EXPECT_EQ(card.raw_skill_value(), expected[i - 1]);
  }
}

TEST(CardTest, TestCard707SkillUnitBonus) {
  CardState state;
  state.set_skill_level(4);
  Card card{MasterDb::FindFirst<db::Card>(707), state};
  Card card_no_match{MasterDb::FindFirst<db::Card>(622), state};

  std::array expected = {100, 110, 120, 130, 150};
  for (int i = 0; i < 5; ++i) {
    bool unit_count_populated = false;
    std::array<int, db::Unit_ARRAYSIZE> unit_count{};
    std::vector<const Card*> cards;

    for (int j = 0; j < 5; j++) {
      cards.push_back((j <= i) ? &card : &card_no_match);
    }
    EXPECT_EQ(card.SkillValue(UnitCount{unit_count_populated, unit_count, cards}), expected[i]);
    EXPECT_TRUE(unit_count_populated);
    EXPECT_EQ(unit_count[db::UNIT_WXS], i + 1);
    EXPECT_EQ(unit_count[db::UNIT_LN], 4 - i);
  }
}

TEST(CardTest, TestCard289Units) {
  const db::Card& card1 = MasterDb::FindFirst<db::Card>(289);
  CardState state;
  Card card{card1, state};
  EXPECT_EQ(card.primary_unit(), Unit(db::UNIT_VS));
  EXPECT_EQ(card.secondary_unit(), Unit(db::UNIT_VS));
}

TEST(CardTest, TestCard707Units) {
  const db::Card& card1 = MasterDb::FindFirst<db::Card>(707);
  CardState state;
  Card card{card1, state};
  EXPECT_EQ(card.primary_unit(), Unit(db::UNIT_WXS));
  EXPECT_EQ(card.secondary_unit(), Unit(db::UNIT_VS));
}

TEST(CardTest, TestEvent112Chap1EventBonus) {
  auto event_id = ParseTextProto<EventId>(R"pb(event_id: 112 chapter_id: 1)pb");
  EventBonus event_bonus(event_id);
  SupportUnitEventBonus support_bonus(event_id);

  CardState state;
  // Event ena 4*
  Card card787{MasterDb::FindFirst<db::Card>(787), state};
  card787.ApplyEventBonus(event_bonus, support_bonus);
  EXPECT_EQ(card787.event_bonus(), 20 + 25 + 10);
  EXPECT_EQ(card787.support_bonus(), 10);

  state.set_master_rank(5);
  state.set_skill_level(4);
  Card card787_2{MasterDb::FindFirst<db::Card>(787), state};
  card787_2.ApplyEventBonus(event_bonus, support_bonus);
  EXPECT_EQ(card787_2.event_bonus(), 20 + 25 + 25);
  EXPECT_EQ(card787_2.support_bonus(), 12.5 + 2.5);

  // Non-event mafuyu
  Card card637{MasterDb::FindFirst<db::Card>(637), state};
  card637.ApplyEventBonus(event_bonus, support_bonus);
  EXPECT_EQ(card637.event_bonus(), 25 + 25);
  EXPECT_EQ(card637.support_bonus(), 5 + 12.5 + 2.5);

  // 25 miku
  Card card404{MasterDb::FindFirst<db::Card>(404), state};
  card404.ApplyEventBonus(event_bonus, support_bonus);
  EXPECT_EQ(card404.event_bonus(), 25);
  EXPECT_EQ(card404.support_bonus(), 12.5 + 2.5);

  // Subunitless miku
  Card card801{MasterDb::FindFirst<db::Card>(801), state};
  card801.ApplyEventBonus(event_bonus, support_bonus);
  EXPECT_EQ(card801.event_bonus(), 25);
  // No guarantees of support bonus for ineligible cards.
}

TEST(CardTest, TestEvent117Bonus) {
  auto event_id = ParseTextProto<EventId>(R"pb(event_id: 117)pb");
  EventBonus event_bonus(event_id);

  CardState state;
  state.set_master_rank(5);
  state.set_skill_level(4);

  // Non-event sbibo 2*
  Card card809{MasterDb::FindFirst<db::Card>(809), state};
  card809.ApplyEventBonus(event_bonus);
  EXPECT_EQ(card809.event_bonus(), 25 + 25 + 1);
  EXPECT_EQ(card809.support_bonus(), 0);

  // Event MEIKO myst 4*
  Card card844{MasterDb::FindFirst<db::Card>(844), state};
  card844.ApplyEventBonus(event_bonus);
  EXPECT_EQ(card844.event_bonus(), 20 + 25 + 25 + 25);
  EXPECT_EQ(card844.support_bonus(), 0);

  // Subunitless MEIKO cool BD
  Card card784{MasterDb::FindFirst<db::Card>(784), state};
  card784.ApplyEventBonus(event_bonus);
  EXPECT_EQ(card784.event_bonus(), 15 + 15);
  EXPECT_EQ(card784.support_bonus(), 0);
}

}  // namespace
}  // namespace sekai
