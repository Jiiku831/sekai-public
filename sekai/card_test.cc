#include "sekai/card.h"

#include <array>

#include <Eigen/Eigen>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "absl/flags/declare.h"
#include "absl/log/log.h"
#include "sekai/db/master_db.h"
#include "sekai/db/proto/all.h"
#include "sekai/event_bonus.h"
#include "sekai/profile.h"
#include "sekai/proto/card_state.pb.h"
#include "sekai/proto/profile.pb.h"
#include "sekai/unit_count.h"
#include "testing/util.h"

ABSL_DECLARE_FLAG(float, leader_card_bonus_rate);

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

TEST(CardTest, TestCard1PowerWithCanvas) {
  const db::Card& card1 = MasterDb::FindFirst<db::Card>(1);
  auto state = ParseTextProto<CardState>(R"pb(
    level: 20
    master_rank: 3
    card_episodes_read: 1
    card_episodes_read: 2
    special_training: false
    canvas_crafted: true
  )pb");
  Card card{card1, state};
  EXPECT_EQ(card.power_vec().sum(), 7575 + (300 + 150) * 3 + 300);
  EXPECT_EQ(card.power_vec(), Eigen::Vector3i(2663 + 300 + 150 + 100, 2325 + 300 + 150 + 100,
                                              2587 + 300 + 150 + 100));
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

TEST(CardTest, TestCard801PowerWithCanvas) {
  const db::Card& card1 = MasterDb::FindFirst<db::Card>(801);
  auto state = ParseTextProto<CardState>(R"pb(
    level: 60
    master_rank: 5
    card_episodes_read: 1599
    card_episodes_read: 1600
    special_training: true
    canvas_crafted: true
  )pb");
  Card card{card1, state};
  EXPECT_EQ(card.power_vec().sum(), 37292 + 1500);
  EXPECT_EQ(card.power_vec(),
            Eigen::Vector3i(9254 + 1000 + 850 + 400 + 500, 10129 + 1000 + 850 + 400 + 500,
                            11159 + 1000 + 850 + 400 + 500));
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
    EXPECT_EQ(card.SkillValue(0, unit_count), expected[i]);
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
  EXPCET_TRUE(card787.is_event_card());

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
  EXPECT_FALSE(card637.is_event_card());

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

TEST(CardTest, TestEvent112Chap1EventBonusLeadBonus) {
  absl::SetFlag(&FLAGS_leader_card_bonus_rate, 20);
  auto event_id = ParseTextProto<EventId>(R"pb(event_id: 112 chapter_id: 1)pb");
  EventBonus event_bonus(event_id);
  SupportUnitEventBonus support_bonus(event_id);

  CardState state;
  // Event ena 4*
  Card card787{MasterDb::FindFirst<db::Card>(787), state};
  card787.ApplyEventBonus(event_bonus, support_bonus);
  EXPECT_EQ(card787.event_bonus(), 20 + 25 + 10);
  EXPECT_EQ(card787.support_bonus(), 10);
  EXPCET_TRUE(card787.is_event_card());
  EXPCET_EQ(card787.lead_bonus(), 20);

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
  EXPECT_FALSE(card637.is_event_card());
  EXPCET_EQ(card637.lead_bonus(), 0);

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
  EXPECT_EQ(card784.event_bonus(), 25 + 15);
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
  card_special.UseSecondarySkill(true);
  Card card_wxs{MasterDb::FindFirst<db::Card>(707), state};
  Card card_ln{MasterDb::FindFirst<db::Card>(622), state};
  Card card_vbs{MasterDb::FindFirst<db::Card>(600), state};

  std::vector<const Card*> cards = {
      &card_special, &card_wxs, &card_wxs, &card_vbs, &card_ln,
  };
  ProfileProto profile_proto = TestProfile();
  profile_proto.set_character_ranks(25, 82);
  Profile profile(profile_proto);
  card_special.ApplyProfilePowerBonus(profile);
  UnitCount unit_count{cards};
  EXPECT_EQ(card_special.SkillValue(0, unit_count), 151);
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
  EXPECT_EQ(card_special.SkillValue(0, unit_count), 150);
  EXPECT_EQ(card_special.MaxSkillValue(), 150);
}

TEST(CardTest, SecondarySkillWithReferenceBoostPrimary) {
  CardState state;
  state.set_skill_level(4);
  state.set_special_training(true);

  Card card_special{MasterDb::FindFirst<db::Card>(949), state};
  card_special.UseSecondarySkill(true);
  Card card_1{MasterDb::FindFirst<db::Card>(4), state};    // 100%
  Card card_2{MasterDb::FindFirst<db::Card>(88), state};   // 120%
  Card card_3{MasterDb::FindFirst<db::Card>(622), state};  // 150%
  Card card_4{MasterDb::FindFirst<db::Card>(802), state};  // 100%

  std::vector<const Card*> cards = {
      &card_special, &card_1, &card_2, &card_3, &card_4,
  };
  ProfileProto profile_proto = TestProfile();
  profile_proto.set_character_ranks(17, 56);
  Profile profile(profile_proto);
  card_special.ApplyProfilePowerBonus(profile);
  UnitCount unit_count{cards};
  EXPECT_EQ(card_special.SkillValue(0, unit_count), 80 + 58);
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
  EXPECT_FLOAT_EQ(card_special.SkillValue(0, unit_count),
                  80 + static_cast<float>(100 + 120 + 140 + 100) / 8);
  EXPECT_EQ(card_special.MaxSkillValue(), 150);
}

TEST(CardTest, ApplyProfileGateBonus) {
  // Ichika
  CardState state;
  state.set_level(60);
  state.add_card_episodes_read(7);
  state.add_card_episodes_read(8);
  state.set_special_training(true);
  Card card{MasterDb::FindFirst<db::Card>(4), state};

  ProfileProto profile_proto = TestProfile();
  profile_proto.mutable_mysekai_gate_levels()->Resize(6, 0);
  profile_proto.set_mysekai_gate_levels(1, 7);
  Profile profile(profile_proto);
  card.ApplyProfilePowerBonus(profile);
  EXPECT_EQ(card.gate_power_bonus(), 240);
  EXPECT_EQ(card.power(), 34295);
  EXPECT_EQ(card.precomputed_power(false, false, false), 56585 + 240);
}

TEST(CardTest, ApplyProfileGateBonusUnitVS) {
  // LN Miku
  CardState state;
  state.set_level(60);
  state.add_card_episodes_read(395);
  state.add_card_episodes_read(396);
  state.set_master_rank(5);
  state.set_special_training(true);
  state.set_canvas_crafted(true);
  Card card{MasterDb::FindFirst<db::Card>(198), state};

  ProfileProto profile_proto = TestProfile();
  profile_proto.mutable_mysekai_gate_levels()->Resize(6, 0);
  profile_proto.set_mysekai_gate_levels(1, 18);
  profile_proto.set_mysekai_gate_levels(2, 1);
  profile_proto.set_mysekai_gate_levels(3, 20);
  profile_proto.set_mysekai_gate_levels(4, 3);
  profile_proto.set_mysekai_gate_levels(5, 4);
  Profile profile(profile_proto);
  card.ApplyProfilePowerBonus(profile);
  EXPECT_EQ(card.gate_power_bonus(), 703);
  EXPECT_EQ(card.power(), 39098);
  constexpr int kInaccuracy = 1;
  EXPECT_EQ(card.precomputed_power(false, false, false), 39098 + 23457 + 1953 + 703 + kInaccuracy);
}

TEST(CardTest, ApplyProfileGateBonusSubunitlessVS) {
  // Miku
  CardState state;
  state.set_level(60);
  state.add_card_episodes_read(1015);
  state.add_card_episodes_read(1016);
  state.set_master_rank(5);
  state.set_special_training(true);
  state.set_canvas_crafted(true);
  Card card{MasterDb::FindFirst<db::Card>(509), state};

  ProfileProto profile_proto = TestProfile();
  profile_proto.mutable_mysekai_gate_levels()->Resize(6, 0);
  profile_proto.set_mysekai_gate_levels(1, 1);
  profile_proto.set_mysekai_gate_levels(2, 2);
  profile_proto.set_mysekai_gate_levels(3, 18);  // Use highest level gate
  profile_proto.set_mysekai_gate_levels(4, 4);
  profile_proto.set_mysekai_gate_levels(5, 3);
  Profile profile(profile_proto);
  card.ApplyProfilePowerBonus(profile);
  EXPECT_EQ(card.gate_power_bonus(), 698);
  EXPECT_EQ(card.power(), 38797);
  constexpr int kInaccuracy = 3;
  EXPECT_EQ(card.precomputed_power(false, false, false), 38797 + 23276 + 1938 + 698 + kInaccuracy);
}

TEST(CardTest, ApplyProfileFixtureBonus) {
  // Miku
  CardState state;
  state.set_level(60);
  state.add_card_episodes_read(1015);
  state.add_card_episodes_read(1016);
  state.set_master_rank(5);
  state.set_special_training(true);
  state.set_canvas_crafted(true);
  Card card{MasterDb::FindFirst<db::Card>(509), state};

  ProfileProto profile_proto = TestProfile();
  (*profile_proto.mutable_mysekai_fixture_crafted())[402] = true;
  (*profile_proto.mutable_mysekai_fixture_crafted())[403] = true;
  (*profile_proto.mutable_mysekai_fixture_crafted())[404] = true;
  (*profile_proto.mutable_mysekai_fixture_crafted())[510] = true;
  (*profile_proto.mutable_mysekai_fixture_crafted())[511] = true;
  (*profile_proto.mutable_mysekai_fixture_crafted())[512] = true;
  Profile profile(profile_proto);
  card.ApplyProfilePowerBonus(profile);
  EXPECT_EQ(card.fixture_power_bonus(), 775);
  EXPECT_EQ(card.power(), 38797);
  constexpr int kInaccuracy = 3;
  EXPECT_EQ(card.precomputed_power(false, false, false), 38797 + 23276 + 1938 + 775 + kInaccuracy);
}

TEST(CardTest, ApplyProfileFixtureAndGateBonus) {
  // Miku
  CardState state;
  state.set_level(60);
  state.add_card_episodes_read(1015);
  state.add_card_episodes_read(1016);
  state.set_master_rank(5);
  state.set_special_training(true);
  state.set_canvas_crafted(true);
  Card card{MasterDb::FindFirst<db::Card>(509), state};

  ProfileProto profile_proto = TestProfile();
  (*profile_proto.mutable_mysekai_fixture_crafted())[402] = true;
  (*profile_proto.mutable_mysekai_fixture_crafted())[403] = true;
  (*profile_proto.mutable_mysekai_fixture_crafted())[404] = true;
  (*profile_proto.mutable_mysekai_fixture_crafted())[510] = true;
  (*profile_proto.mutable_mysekai_fixture_crafted())[511] = true;
  (*profile_proto.mutable_mysekai_fixture_crafted())[512] = true;
  profile_proto.mutable_mysekai_gate_levels()->Resize(6, 0);
  profile_proto.set_mysekai_gate_levels(1, 1);
  profile_proto.set_mysekai_gate_levels(2, 2);
  profile_proto.set_mysekai_gate_levels(3, 18);  // Use highest level gate
  profile_proto.set_mysekai_gate_levels(4, 4);
  profile_proto.set_mysekai_gate_levels(5, 3);
  Profile profile(profile_proto);
  card.ApplyProfilePowerBonus(profile);
  EXPECT_EQ(card.gate_power_bonus(), 698);
  EXPECT_EQ(card.fixture_power_bonus(), 775);
  EXPECT_EQ(card.power(), 38797);
  constexpr int kInaccuracy = 3;
  EXPECT_EQ(card.precomputed_power(false, false, false),
            38797 + 23276 + 1938 + 775 + 698 + kInaccuracy);
}

TEST(CardTest, InitialSpecialTrainingStatusIgnoresCardStateStatus) {
  CardState trained_state, untrained_state;
  trained_state.set_special_training(true);
  trained_state.set_level(60);
  untrained_state.set_level(60);
  Card card_trained{MasterDb::FindFirst<db::Card>(1167), trained_state};
  Card card_untrained{MasterDb::FindFirst<db::Card>(1167), untrained_state};
  EXPECT_EQ(card_trained.power_vec(), card_untrained.power_vec());
}

}  // namespace
}  // namespace sekai
