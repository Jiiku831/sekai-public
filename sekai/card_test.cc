#include "sekai/card.h"

#include <array>

#include <Eigen/Eigen>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "absl/log/log.h"
#include "sekai/db/master_db.h"
#include "sekai/db/proto/all.h"
#include "sekai/event_bonus.h"
#include "sekai/profile.h"
#include "sekai/proto/card_state.pb.h"
#include "sekai/proto/profile.pb.h"
#include "sekai/unit_count.h"
#include "testing/util.h"

namespace sekai {
namespace {

using ::sekai::db::MasterDb;
using ::testing::ParseTextProto;

ProfileProto TestProfile() {
  // clang-format off
  return ParseTextProto<ProfileProto>(R"pb(
    area_item_levels: [
      # Offset (ignored)
      0,
      # LN chars
      15, 15, 15, 15,
      # MMJ chars
      10, 10, 10, 10,
      # VBS chars
      10, 10, 10, 10,
      # WXS chars
      10, 10, 10, 10,
      # 25ji chars
      10, 10, 10, 10,
      # Miku items
      15, 15, 15, 15, 15,
      # Rin item
      15,
      # Len item
      15,
      # Luka item
      15,
      # KAITO item
      15,
      # MEIKO item
      15,
      # LN unit
      15, 15,
      # MMJ unit
      10, 10,
      # VBS unit
      10, 10,
      # WXS unit
      10, 10,
      # 25ji unit
      10, 10,
      # VS unit
      15, 15, 15, 15, 15,
      # Cool plant (miyajo first)
      9, 9,
      # Cute plant
      15, 15,
      # Pure plant
      15, 15,
      # Happy plant
      15, 10,
      # Mysterious plant
      9, 8
    ]
    character_ranks: [
      # Offset (ignored)
      0,
      # LN
      55, 73, 54, 53,
      # MMJ
      44, 47, 44, 41,
      # VBS
      47, 41, 43, 41,
      # WXS
      43, 43, 42, 41,
      # 25ji
      45, 42, 44, 46,
      # VS
      109, 52, 52, 54, 53, 50
    ]
    bonus_power: 270
  )pb");
  // clang-format on
}

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
    EXPECT_EQ(card.MaxSkillValue(), expected[i - 1]);
  }
}

TEST(CardTest, TestCard289SkillConditionLife) {
  const db::Card& card1 = MasterDb::FindFirst<db::Card>(289);
  std::array expected = {120, 125, 130, 140};
  CardState state;
  for (int i = 1; i <= 4; ++i) {
    state.set_skill_level(i);
    Card card{card1, state};
    EXPECT_EQ(card.MaxSkillValue(), expected[i - 1]);
  }
}

TEST(CardTest, TestCard290SkillKeep) {
  const db::Card& card1 = MasterDb::FindFirst<db::Card>(289);
  std::array expected = {120, 125, 130, 140};
  CardState state;
  for (int i = 1; i <= 4; ++i) {
    state.set_skill_level(i);
    Card card{card1, state};
    EXPECT_EQ(card.MaxSkillValue(), expected[i - 1]);
  }
}

TEST(CardTest, TestCard707SkillUnit) {
  const db::Card& card1 = MasterDb::FindFirst<db::Card>(707);
  std::array expected = {130, 135, 140, 150};
  CardState state;
  for (int i = 1; i <= 4; ++i) {
    state.set_skill_level(i);
    Card card{card1, state};
    EXPECT_EQ(card.MaxSkillValue(), expected[i - 1]);
  }
}

TEST(CardTest, TestCard707SkillUnitBonus) {
  CardState state;
  state.set_skill_level(4);
  Card card{MasterDb::FindFirst<db::Card>(707), state};
  Card card_no_match{MasterDb::FindFirst<db::Card>(622), state};

  std::array expected = {100, 110, 120, 130, 150};
  for (int i = 0; i < 5; ++i) {
    std::vector<const Card*> cards;
    for (int j = 0; j < 5; j++) {
      cards.push_back((j <= i) ? &card : &card_no_match);
    }

    UnitCount unit_count(cards);
    EXPECT_EQ(card.SkillValue(unit_count), expected[i]);
    EXPECT_EQ(unit_count.CharacterCount(db::UNIT_WXS), i + 1);
    EXPECT_EQ(unit_count.CharacterCount(db::UNIT_LN), 4 - i);
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

TEST(CardTest, TestIsUnitRegularUnit) {
  CardState state;
  state.set_master_rank(5);
  state.set_skill_level(4);

  // Sbibo
  Card card809{MasterDb::FindFirst<db::Card>(809), state};
  EXPECT_TRUE(card809.IsUnit(db::UNIT_LN));
  EXPECT_FALSE(card809.IsUnit(db::UNIT_MMJ));
  EXPECT_FALSE(card809.IsUnit(db::UNIT_VS));

  // LN MEIKO
  Card card844{MasterDb::FindFirst<db::Card>(844), state};
  EXPECT_TRUE(card844.IsUnit(db::UNIT_LN));
  EXPECT_FALSE(card844.IsUnit(db::UNIT_MMJ));
  EXPECT_TRUE(card844.IsUnit(db::UNIT_VS));

  // Subunitless MEIKO
  Card card784{MasterDb::FindFirst<db::Card>(784), state};
  EXPECT_FALSE(card784.IsUnit(db::UNIT_LN));
  EXPECT_FALSE(card784.IsUnit(db::UNIT_MMJ));
  EXPECT_TRUE(card784.IsUnit(db::UNIT_VS));
}

TEST(CardTest, SecondarySkillWithUnitCountPrimary) {
  CardState state;
  state.set_skill_level(4);
  state.set_special_training(true);

  Card card_special{MasterDb::FindFirst<db::Card>(950), state};
  Card card_wxs{MasterDb::FindFirst<db::Card>(707), state};
  Card card_ln{MasterDb::FindFirst<db::Card>(622), state};
  Card card_vbs{MasterDb::FindFirst<db::Card>(600), state};

  std::vector<const Card*> cards = {
      &card_special, &card_wxs, &card_wxs, &card_vbs, &card_ln,
  };
  UnitCount unit_count(cards);
  EXPECT_EQ(card_special.SkillValue(unit_count), 150);
  EXPECT_EQ(card_special.MaxSkillValue(), 160);

  ProfileProto profile_proto = TestProfile();
  profile_proto.set_character_ranks(25, 82);
  Profile profile(profile_proto);
  card_special.ApplyProfilePowerBonus(profile);
  unit_count = UnitCount(cards);
  EXPECT_EQ(card_special.SkillValue(unit_count), 151);
  EXPECT_EQ(card_special.MaxSkillValue(), 160);
}

TEST(CardTest, SecondarySkillWithUnitCountPrimaryNoSpecialTraining) {
  CardState state;
  state.set_skill_level(4);
  state.set_special_training(false);

  Card card_special{MasterDb::FindFirst<db::Card>(950), state};
  Card card_wxs{MasterDb::FindFirst<db::Card>(707), state};
  Card card_ln{MasterDb::FindFirst<db::Card>(622), state};
  Card card_vbs{MasterDb::FindFirst<db::Card>(600), state};

  std::vector<const Card*> cards = {
      &card_special, &card_wxs, &card_wxs, &card_vbs, &card_ln,
  };
  UnitCount unit_count(cards);
  EXPECT_EQ(card_special.SkillValue(unit_count), 150);
  EXPECT_EQ(card_special.MaxSkillValue(), 150);

  ProfileProto profile_proto = TestProfile();
  profile_proto.set_character_ranks(25, 82);
  Profile profile(profile_proto);
  card_special.ApplyProfilePowerBonus(profile);
  unit_count = UnitCount(cards);
  EXPECT_EQ(card_special.SkillValue(unit_count), 150);
  EXPECT_EQ(card_special.MaxSkillValue(), 150);
}

TEST(CardTest, SecondarySkillWithReferenceBoostPrimary) {
  CardState state;
  state.set_skill_level(4);
  state.set_special_training(true);

  Card card_special{MasterDb::FindFirst<db::Card>(949), state};
  Card card_1{MasterDb::FindFirst<db::Card>(4), state};    // 100%
  Card card_2{MasterDb::FindFirst<db::Card>(88), state};   // 120%
  Card card_3{MasterDb::FindFirst<db::Card>(622), state};  // 150%
  Card card_4{MasterDb::FindFirst<db::Card>(802), state};  // 100%

  std::vector<const Card*> cards = {
      &card_special, &card_1, &card_2, &card_3, &card_4,
  };
  UnitCount unit_count(cards);
  EXPECT_FLOAT_EQ(card_special.SkillValue(unit_count), 80 + 57.5);
  EXPECT_EQ(card_special.MaxSkillValue(), 160);

  ProfileProto profile_proto = TestProfile();
  profile_proto.set_character_ranks(17, 56);
  Profile profile(profile_proto);
  card_special.ApplyProfilePowerBonus(profile);
  unit_count = UnitCount(cards);
  EXPECT_EQ(card_special.SkillValue(unit_count), 80 + 58);
  EXPECT_EQ(card_special.MaxSkillValue(), 160);
}

TEST(CardTest, SecondarySkillWithReferenceBoostPrimaryNoSpecialTraining) {
  CardState state;
  state.set_skill_level(4);
  state.set_special_training(false);

  Card card_special{MasterDb::FindFirst<db::Card>(949), state};
  Card card_1{MasterDb::FindFirst<db::Card>(4), state};    // 100%
  Card card_2{MasterDb::FindFirst<db::Card>(88), state};   // 120%
  Card card_3{MasterDb::FindFirst<db::Card>(622), state};  // 150%
  Card card_4{MasterDb::FindFirst<db::Card>(802), state};  // 100%

  std::vector<const Card*> cards = {
      &card_special, &card_1, &card_2, &card_3, &card_4,
  };
  UnitCount unit_count(cards);
  EXPECT_FLOAT_EQ(card_special.SkillValue(unit_count), 80 + 57.5);
  EXPECT_EQ(card_special.MaxSkillValue(), 150);

  ProfileProto profile_proto = TestProfile();
  profile_proto.set_character_ranks(17, 56);
  Profile profile(profile_proto);
  card_special.ApplyProfilePowerBonus(profile);
  unit_count = UnitCount(cards);
  EXPECT_FLOAT_EQ(card_special.SkillValue(unit_count), 80 + 57.5);
  EXPECT_EQ(card_special.MaxSkillValue(), 150);
}

}  // namespace
}  // namespace sekai
