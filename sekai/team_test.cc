#include "sekai/team.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "gmock/gmock.h"
#include "sekai/bitset.h"
#include "sekai/card.h"
#include "sekai/db/master_db.h"
#include "sekai/db/proto/all.h"
#include "sekai/event_bonus.h"
#include "sekai/profile.h"
#include "sekai/proto/profile.pb.h"
#include "testing/util.h"

namespace sekai {
namespace {

using ::testing::ElementsAreArray;
using ::testing::Eq;
using ::testing::Pair;
using ::testing::ParseTextProto;
using ::testing::Pointwise;
using ::testing::UnorderedElementsAre;

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

ProfileProto AltTestProfile() {
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
      15, 10,
      # Cute plant
      15, 15,
      # Pure plant
      15, 15,
      # Happy plant
      15, 15,
      # Mysterious plant
      10, 10
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

Team MakeTeam(std::span<const Card> cards) {
  std::vector<const Card*> card_ptrs;
  for (const Card& card : cards) {
    card_ptrs.push_back(&card);
  }
  return Team{card_ptrs};
}

class TeamTest : public ::testing::Test {
 protected:
  Card CreateCard(const Profile& profile, int card_id, int level, int master_rank = 0,
                  int skill_level = 1, bool canvas_crafted = false) {
    CardState state;
    state.set_level(level);
    state.set_master_rank(master_rank);
    state.set_skill_level(skill_level);
    state.set_special_training(true);
    state.set_canvas_crafted(canvas_crafted);
    for (const auto* episode : db::MasterDb::FindAll<db::CardEpisode>(card_id)) {
      state.add_card_episodes_read(episode->id());
    }
    Card card{db::MasterDb::FindFirst<db::Card>(card_id), state};
    card.ApplyProfilePowerBonus(profile);
    return card;
  }

  Profile profile_{TestProfile()};
  Profile alt_profile_{AltTestProfile()};
};

TEST_F(TeamTest, ExampleTeam1MinPowerContrib) {
  std::array cards = {
      CreateCard(alt_profile_, /*card_id=*/404, /*level=*/60, /*master_rank=*/5),
      CreateCard(alt_profile_, /*card_id=*/139, /*level=*/60),
      CreateCard(alt_profile_, /*card_id=*/115, /*level=*/60),
      CreateCard(alt_profile_, /*card_id=*/511, /*level=*/60),
      CreateCard(alt_profile_, /*card_id=*/787, /*level=*/60),
  };
  Team team = MakeTeam(cards);
  EXPECT_EQ(team.MinPowerContrib(alt_profile_), 53006);
}

TEST_F(TeamTest, ExampleTeam1Power) {
  std::array cards = {
      CreateCard(alt_profile_, /*card_id=*/404, /*level=*/60, /*master_rank=*/5),
      CreateCard(alt_profile_, /*card_id=*/139, /*level=*/60),
      CreateCard(alt_profile_, /*card_id=*/115, /*level=*/60),
      CreateCard(alt_profile_, /*card_id=*/511, /*level=*/60),
      CreateCard(alt_profile_, /*card_id=*/787, /*level=*/60),
  };
  Team team = MakeTeam(cards);
  // Actual area item bonus is 97097
  std::array expected = {174455, 97100, 7925, 270, 0, 0};
  EXPECT_THAT(team.PowerDetailed(alt_profile_), Pointwise(Eq(), expected));
  // Actual power is 279747
  EXPECT_EQ(team.Power(alt_profile_), 279750);
  EXPECT_EQ(team.Power(alt_profile_), team.PowerDetailed(alt_profile_).sum());
}

TEST_F(TeamTest, ExampleTeam2Power) {
  std::array cards = {
      CreateCard(profile_, /*card_id=*/427, /*level=*/60, /*master_rank=*/5),
      CreateCard(profile_, /*card_id=*/699, /*level=*/60),
      CreateCard(profile_, /*card_id=*/527, /*level=*/60),
      CreateCard(profile_, /*card_id=*/706, /*level=*/60, /*master_rank=*/5),
      CreateCard(profile_, /*card_id=*/355, /*level=*/60),
  };
  Team team = MakeTeam(cards);
  // Actual area item bonus is 99142
  std::array expected = {170495, 99142, 8514, 270, 0, 0};
  EXPECT_THAT(team.PowerDetailed(profile_), Pointwise(Eq(), expected));
  // Actual team power is 278421
  EXPECT_EQ(team.Power(profile_), 278421);
  EXPECT_EQ(team.Power(profile_), team.PowerDetailed(profile_).sum());
}

TEST_F(TeamTest, ExampleTeam3Power) {
  std::array cards = {
      CreateCard(profile_, /*card_id=*/116, /*level=*/60, /*master_rank=*/5),
      CreateCard(profile_, /*card_id=*/162, /*level=*/60),
      CreateCard(profile_, /*card_id=*/242, /*level=*/60),
      CreateCard(profile_, /*card_id=*/423, /*level=*/60, /*master_rank=*/2),
      CreateCard(profile_, /*card_id=*/483, /*level=*/60),
  };
  Team team = MakeTeam(cards);
  // Actual area item bonus is 95380
  std::array expected = {176264, 95381, 8282, 270, 0, 0};
  EXPECT_THAT(team.PowerDetailed(profile_), Pointwise(Eq(), expected));
  // Actual team power is 280196
  EXPECT_EQ(team.Power(profile_), 280197);
  EXPECT_EQ(team.Power(profile_), team.PowerDetailed(profile_).sum());
}

TEST_F(TeamTest, ExampleTeam4Power) {
  std::array cards = {
      CreateCard(profile_, /*card_id=*/427, /*level=*/60, /*master_rank=*/5),
      CreateCard(profile_, /*card_id=*/699, /*level=*/60),
      CreateCard(profile_, /*card_id=*/527, /*level=*/60),
      CreateCard(profile_, /*card_id=*/82, /*level=*/30, /*master_rank=*/5),
      CreateCard(profile_, /*card_id=*/355, /*level=*/60),
  };
  Team team = MakeTeam(cards);
  // Actual area item bonus is 115109
  std::array expected = {154589, 115112, 7719, 270, 0, 0};
  EXPECT_THAT(team.PowerDetailed(profile_), Pointwise(Eq(), expected));
  // Actual team power is 277687
  EXPECT_EQ(team.Power(profile_), 277690);
  EXPECT_EQ(team.Power(profile_), team.PowerDetailed(profile_).sum());
}

TEST_F(TeamTest, ExampleTeam5Power) {
  std::array cards = {
      CreateCard(profile_, /*card_id=*/198, /*level=*/60, /*master_rank=*/5),
      CreateCard(profile_, /*card_id=*/260, /*level=*/60, /*master_rank=*/2),
      CreateCard(profile_, /*card_id=*/259, /*level=*/60, /*master_rank=*/2),
      CreateCard(profile_, /*card_id=*/151, /*level=*/60),
      CreateCard(profile_, /*card_id=*/380, /*level=*/60),
  };
  Team team = MakeTeam(cards);
  // Actual area item bonus is 159725
  std::array expected = {177477, 159725, 8865, 270, 0, 0};
  EXPECT_THAT(team.PowerDetailed(profile_), Pointwise(Eq(), expected));
  // Actual team power is 346337
  EXPECT_EQ(team.Power(profile_), 346337);
  EXPECT_EQ(team.Power(profile_), team.PowerDetailed(profile_).sum());
}

TEST_F(TeamTest, ExampleTeam6Power) {
  std::array cards = {
      CreateCard(alt_profile_, /*card_id=*/509, /*level=*/60, /*master_rank=*/5, /*skill_level=*/4,
                 /*canvas_crafted=*/true),
      CreateCard(alt_profile_, /*card_id=*/875, /*level=*/60, /*master_rank=*/5, /*skill_level=*/4,
                 /*canvas_crafted=*/true),
      CreateCard(alt_profile_, /*card_id=*/198, /*level=*/60, /*master_rank=*/5, /*skill_level=*/4,
                 /*canvas_crafted=*/true),
      CreateCard(alt_profile_, /*card_id=*/149, /*level=*/60, /*master_rank=*/5, /*skill_level=*/4,
                 /*canvas_crafted=*/true),
      CreateCard(alt_profile_, /*card_id=*/126, /*level=*/60, /*master_rank=*/5, /*skill_level=*/4,
                 /*canvas_crafted=*/true),
  };
  Team team = MakeTeam(cards);
  // Actual area item bonus is 175106
  // Actual CR bonus is 9721
  std::array expected = {194573, 175110, 9722, 270, 0, 0};
  EXPECT_THAT(team.PowerDetailed(alt_profile_), Pointwise(Eq(), expected));
  EXPECT_EQ(team.Power(alt_profile_), 194573 + 175110 + 9722 + 270);
  EXPECT_EQ(team.Power(alt_profile_), team.PowerDetailed(alt_profile_).sum());
}

TEST_F(TeamTest, ExampleTeam6PowerWithFixtureBonus) {
  ProfileProto profile_proto = AltTestProfile();
  profile_proto.mutable_mysekai_gate_levels()->Resize(6, 0);
  profile_proto.set_mysekai_gate_levels(1, 18);
  profile_proto.set_mysekai_gate_levels(2, 2);
  profile_proto.set_mysekai_gate_levels(3, 2);
  profile_proto.set_mysekai_gate_levels(4, 9);
  profile_proto.set_mysekai_gate_levels(5, 2);
  (*profile_proto.mutable_mysekai_fixture_crafted())[402] = true;
  (*profile_proto.mutable_mysekai_fixture_crafted())[403] = true;
  (*profile_proto.mutable_mysekai_fixture_crafted())[404] = true;
  (*profile_proto.mutable_mysekai_fixture_crafted())[510] = true;
  (*profile_proto.mutable_mysekai_fixture_crafted())[511] = true;
  (*profile_proto.mutable_mysekai_fixture_crafted())[512] = true;
  Profile profile{profile_proto};
  std::array cards = {
      CreateCard(profile, /*card_id=*/509, /*level=*/60, /*master_rank=*/5, /*skill_level=*/4,
                 /*canvas_crafted=*/true),
      CreateCard(profile, /*card_id=*/875, /*level=*/60, /*master_rank=*/5, /*skill_level=*/4,
                 /*canvas_crafted=*/true),
      CreateCard(profile, /*card_id=*/198, /*level=*/60, /*master_rank=*/5, /*skill_level=*/4,
                 /*canvas_crafted=*/true),
      CreateCard(profile, /*card_id=*/149, /*level=*/60, /*master_rank=*/5, /*skill_level=*/4,
                 /*canvas_crafted=*/true),
      CreateCard(profile, /*card_id=*/126, /*level=*/60, /*master_rank=*/5, /*skill_level=*/4,
                 /*canvas_crafted=*/true),
  };
  Team team = MakeTeam(cards);
  // Actual area item bonus is 175106
  // Actual CR bonus is 9721
  // Actual fixture bonus is 3887
  // Actual gate bonus is 1905
  std::array expected = {194573, 175110, 9722, 270, 3887, 1905};
  EXPECT_THAT(team.PowerDetailed(profile), Pointwise(Eq(), expected));
  EXPECT_EQ(team.Power(profile), 194573 + 175110 + 9722 + 270 + 3887 + 1905);
  EXPECT_EQ(team.Power(profile), team.PowerDetailed(profile).sum());
}

TEST_F(TeamTest, ExampleTeam6PowerWithGateBonus) {
  ProfileProto profile_proto = AltTestProfile();
  profile_proto.mutable_mysekai_gate_levels()->Resize(6, 0);
  profile_proto.set_mysekai_gate_levels(1, 18);
  profile_proto.set_mysekai_gate_levels(2, 2);
  profile_proto.set_mysekai_gate_levels(3, 2);
  profile_proto.set_mysekai_gate_levels(4, 9);
  profile_proto.set_mysekai_gate_levels(5, 2);
  Profile profile{profile_proto};
  std::array cards = {
      CreateCard(profile, /*card_id=*/509, /*level=*/60, /*master_rank=*/5, /*skill_level=*/4,
                 /*canvas_crafted=*/true),
      CreateCard(profile, /*card_id=*/875, /*level=*/60, /*master_rank=*/5, /*skill_level=*/4,
                 /*canvas_crafted=*/true),
      CreateCard(profile, /*card_id=*/198, /*level=*/60, /*master_rank=*/5, /*skill_level=*/4,
                 /*canvas_crafted=*/true),
      CreateCard(profile, /*card_id=*/149, /*level=*/60, /*master_rank=*/5, /*skill_level=*/4,
                 /*canvas_crafted=*/true),
      CreateCard(profile, /*card_id=*/126, /*level=*/60, /*master_rank=*/5, /*skill_level=*/4,
                 /*canvas_crafted=*/true),
  };
  Team team = MakeTeam(cards);
  // Actual area item bonus is 175106
  // Actual CR bonus is 9721
  // Actual gate bonus is 1905
  std::array expected = {194573, 175110, 9722, 270, 0, 1905};
  EXPECT_THAT(team.PowerDetailed(profile), Pointwise(Eq(), expected));
  // Actual power is 374710
  EXPECT_EQ(team.Power(profile), 194573 + 175110 + 9722 + 270 + 1905);
  EXPECT_EQ(team.Power(profile), team.PowerDetailed(profile).sum());
}

TEST_F(TeamTest, ExampleTeam1EventBonus) {
  std::array cards = {
      CreateCard(profile_, /*card_id=*/116, /*level=*/60, /*master_rank=*/5),
      CreateCard(profile_, /*card_id=*/423, /*level=*/60, /*master_rank=*/2),
      CreateCard(profile_, /*card_id=*/162, /*level=*/60),
      CreateCard(profile_, /*card_id=*/242, /*level=*/60),
      CreateCard(profile_, /*card_id=*/483, /*level=*/60),
  };
  auto event_id = ParseTextProto<EventId>(R"pb(event_id: 111)pb");
  EventBonus bonus(event_id);
  EventBonusProto bonus_proto = bonus.ToProto();
  for (Card& card : cards) {
    card.ApplyEventBonus(bonus);
  }
  Team team = MakeTeam(cards);
  EXPECT_FLOAT_EQ(team.EventBonus(bonus), 295);
}

TEST_F(TeamTest, ExampleTeam2EventBonus) {
  std::array cards = {
      CreateCard(profile_, /*card_id=*/404, /*level=*/60, /*master_rank=*/5),
      CreateCard(profile_, /*card_id=*/219, /*level=*/60),
      CreateCard(profile_, /*card_id=*/239, /*level=*/60),
      CreateCard(profile_, /*card_id=*/115, /*level=*/60),
      CreateCard(profile_, /*card_id=*/787, /*level=*/60),
  };
  auto event_id = ParseTextProto<EventId>(R"pb(event_id: 112)pb");
  EventBonus bonus(event_id);
  EventBonusProto bonus_proto = bonus.ToProto();
  for (Card& card : cards) {
    card.ApplyEventBonus(bonus);
  }
  Team team = MakeTeam(cards);
  EXPECT_FLOAT_EQ(team.EventBonus(bonus), 310);
}

TEST_F(TeamTest, ExampleTeam1SoloEbiPoints) {
  std::array cards = {
      CreateCard(profile_, /*card_id=*/116, /*level=*/60, /*master_rank=*/5),
      CreateCard(profile_, /*card_id=*/423, /*level=*/60, /*master_rank=*/2),
      CreateCard(profile_, /*card_id=*/162, /*level=*/60),
      CreateCard(profile_, /*card_id=*/242, /*level=*/60),
      CreateCard(profile_, /*card_id=*/483, /*level=*/60),
  };
  auto event_id = ParseTextProto<EventId>(R"pb(event_id: 111)pb");
  EventBonus bonus(event_id);
  EventBonusProto bonus_proto = bonus.ToProto();
  for (Card& card : cards) {
    card.ApplyEventBonus(bonus);
  }
  Team team = MakeTeam(cards);
  EXPECT_FLOAT_EQ(team.EventBonus(bonus), 295);
  Team::SoloEbiBasePoints result = team.GetSoloEbiBasePoints(profile_, bonus, 0.9);
  EXPECT_THAT(
      result.possible_points,
      ElementsAreArray({395, 398, 402, 406, 410, 414, 418, 422, 426, 430, 434, 438, 442, 446,
                        450, 454, 458, 462, 466, 470, 474, 477, 481, 485, 489, 493, 497, 501,
                        505, 509, 513, 517, 521, 525, 529, 533, 537, 541, 545, 549, 553, 556,
                        560, 564, 568, 572, 576, 580, 584, 588, 592, 596, 600, 604, 608, 612,
                        616, 620, 624, 628, 632, 635, 639, 643, 647, 651, 655, 659, 663, 667,
                        671, 675, 679, 683, 687, 691, 695, 699, 703, 707, 711}));
  EXPECT_THAT(
      result.score_threshold,
      UnorderedElementsAre(
          Pair(395, 0), Pair(398, 20000), Pair(402, 40000), Pair(406, 60000), Pair(410, 80000),
          Pair(414, 100000), Pair(418, 120000), Pair(422, 140000), Pair(426, 160000),
          Pair(430, 180000), Pair(434, 200000), Pair(438, 220000), Pair(442, 240000),
          Pair(446, 260000), Pair(450, 280000), Pair(454, 300000), Pair(458, 320000),
          Pair(462, 340000), Pair(466, 360000), Pair(470, 380000), Pair(474, 400000),
          Pair(477, 420000), Pair(481, 440000), Pair(485, 460000), Pair(489, 480000),
          Pair(493, 500000), Pair(497, 520000), Pair(501, 540000), Pair(505, 560000),
          Pair(509, 580000), Pair(513, 600000), Pair(517, 620000), Pair(521, 640000),
          Pair(525, 660000), Pair(529, 680000), Pair(533, 700000), Pair(537, 720000),
          Pair(541, 740000), Pair(545, 760000), Pair(549, 780000), Pair(553, 800000),
          Pair(556, 820000), Pair(560, 840000), Pair(564, 860000), Pair(568, 880000),
          Pair(572, 900000), Pair(576, 920000), Pair(580, 940000), Pair(584, 960000),
          Pair(588, 980000), Pair(592, 1000000), Pair(596, 1020000), Pair(600, 1040000),
          Pair(604, 1060000), Pair(608, 1080000), Pair(612, 1100000), Pair(616, 1120000),
          Pair(620, 1140000), Pair(624, 1160000), Pair(628, 1180000), Pair(632, 1200000),
          Pair(635, 1220000), Pair(639, 1240000), Pair(643, 1260000), Pair(647, 1280000),
          Pair(651, 1300000), Pair(655, 1320000), Pair(659, 1340000), Pair(663, 1360000),
          Pair(667, 1380000), Pair(671, 1400000), Pair(675, 1420000), Pair(679, 1440000),
          Pair(683, 1460000), Pair(687, 1480000), Pair(691, 1500000), Pair(695, 1520000),
          Pair(699, 1540000), Pair(703, 1560000), Pair(707, 1580000), Pair(711, 1600000)));
}

TEST_F(TeamTest, FillSupportUnit) {
  std::array cards = {
      CreateCard(profile_, /*card_id=*/404, /*level=*/60, /*master_rank=*/5),
      CreateCard(profile_, /*card_id=*/219, /*level=*/60),
      CreateCard(profile_, /*card_id=*/239, /*level=*/60),
      CreateCard(profile_, /*card_id=*/115, /*level=*/60),
      CreateCard(profile_, /*card_id=*/787, /*level=*/60),
  };
  std::array support_pool = {
      cards[0],
      cards[1],
      cards[2],
      cards[3],
      cards[4],
      CreateCard(profile_, /*card_id=*/69, /*level=*/10),   // 6%
      CreateCard(profile_, /*card_id=*/70, /*level=*/10),   // 7%
      CreateCard(profile_, /*card_id=*/71, /*level=*/10),   // 8%
      CreateCard(profile_, /*card_id=*/114, /*level=*/10),  // 15%
      CreateCard(profile_, /*card_id=*/127, /*level=*/10),  // 8%
      CreateCard(profile_, /*card_id=*/176, /*level=*/10),  // 15%
      CreateCard(profile_, /*card_id=*/196, /*level=*/10),  // 15%
      CreateCard(profile_, /*card_id=*/206, /*level=*/10),  // 7%
      CreateCard(profile_, /*card_id=*/239, /*level=*/10),  // 15%
      CreateCard(profile_, /*card_id=*/278, /*level=*/10),  // 7%
      CreateCard(profile_, /*card_id=*/284, /*level=*/10),  // 15%
      CreateCard(profile_, /*card_id=*/311, /*level=*/10),  // 7%
      CreateCard(profile_, /*card_id=*/324, /*level=*/10),  // 15%
  };
  std::vector<const Card*> support_ptrs;
  for (const Card& card : support_pool) {
    support_ptrs.push_back(&card);
  }
  auto event_id = ParseTextProto<EventId>(R"pb(event_id: 112 chapter_id: 1)pb");
  EventBonus bonus(event_id);
  EventBonusProto bonus_proto = bonus.ToProto();
  for (Card& card : cards) {
    card.ApplyEventBonus(bonus);
  }
  for (Card& card : support_pool) {
    card.ApplyEventBonus(bonus);
  }
  Team team = MakeTeam(cards);
  EXPECT_FLOAT_EQ(team.EventBonus(bonus), 310);
  team.FillSupportCards(support_ptrs);
  EXPECT_FLOAT_EQ(team.EventBonus(bonus), 310 + 6 + 7 + 8 + 15 + 8 + 15 + 15 + 7 + 15 + 7 + 15 + 7);
}

TEST_F(TeamTest, ExampleTeam1SkillValue) {
  std::array cards = {
      CreateCard(profile_, /*card_id=*/622, /*level=*/60, /*master_rank=*/5, /*skill_level=*/4),
      CreateCard(profile_, /*card_id=*/495, /*level=*/60, /*master_rank=*/0, /*skill_level=*/1),
      CreateCard(profile_, /*card_id=*/535, /*level=*/60, /*master_rank=*/0, /*skill_level=*/2),
      CreateCard(profile_, /*card_id=*/259, /*level=*/60, /*master_rank=*/0, /*skill_level=*/3),
      CreateCard(profile_, /*card_id=*/224, /*level=*/60),
  };
  Team team = MakeTeam(cards);
  EXPECT_EQ(team.SkillValue(), 130 + (100 + 105 + 110 + 110) / 5);
}

TEST_F(TeamTest, ExampleTeam1MaxSkillValue) {
  std::array cards = {
      CreateCard(profile_, /*card_id=*/495, /*level=*/60, /*master_rank=*/0, /*skill_level=*/1),
      CreateCard(profile_, /*card_id=*/535, /*level=*/60, /*master_rank=*/0, /*skill_level=*/2),
      CreateCard(profile_, /*card_id=*/259, /*level=*/60, /*master_rank=*/0, /*skill_level=*/3),
      CreateCard(profile_, /*card_id=*/622, /*level=*/60, /*master_rank=*/5, /*skill_level=*/4),
      CreateCard(profile_, /*card_id=*/224, /*level=*/60),
  };
  Team team = MakeTeam(cards);
  EXPECT_EQ(team.SkillValue(), 100 + (105 + 110 + 130 + 110) / 5);
  EXPECT_EQ(team.MaxSkillValue(), 130 + (100 + 105 + 110 + 110) / 5);
}

TEST_F(TeamTest, ExampleTeam1ConstrainedMaxSkillValue) {
  std::array cards = {
      CreateCard(profile_, /*card_id=*/495, /*level=*/60, /*master_rank=*/0, /*skill_level=*/1),
      CreateCard(profile_, /*card_id=*/535, /*level=*/60, /*master_rank=*/0, /*skill_level=*/2),
      CreateCard(profile_, /*card_id=*/259, /*level=*/60, /*master_rank=*/0, /*skill_level=*/3),
      CreateCard(profile_, /*card_id=*/622, /*level=*/60, /*master_rank=*/5, /*skill_level=*/4),
      CreateCard(profile_, /*card_id=*/224, /*level=*/60),
  };
  Team team = MakeTeam(cards);
  Character eligible_leads;
  eligible_leads.set(cards[2].character_id());
  EXPECT_EQ(team.SkillValue(), 100 + (105 + 110 + 130 + 110) / 5);
  EXPECT_EQ(team.MaxSkillValue(), 130 + (100 + 105 + 110 + 110) / 5);

  Team::SkillValueDetail skill_value = team.ConstrainedMaxSkillValue(eligible_leads);
  EXPECT_EQ(skill_value.lead_skill, 110);
  EXPECT_EQ(skill_value.skill_value, 110 + (100 + 105 + 130 + 110) / 5);
}

TEST_F(TeamTest, ExampleTeam1ConstrainedMaxSkillValueAlt) {
  std::array cards = {
      CreateCard(profile_, /*card_id=*/495, /*level=*/60, /*master_rank=*/0, /*skill_level=*/1),
      CreateCard(profile_, /*card_id=*/535, /*level=*/60, /*master_rank=*/0, /*skill_level=*/2),
      CreateCard(profile_, /*card_id=*/259, /*level=*/60, /*master_rank=*/0, /*skill_level=*/3),
      CreateCard(profile_, /*card_id=*/622, /*level=*/60, /*master_rank=*/5, /*skill_level=*/4),
      CreateCard(profile_, /*card_id=*/224, /*level=*/60),
  };
  Team team = MakeTeam(cards);
  Character eligible_leads;
  eligible_leads.set(cards[2].character_id());
  eligible_leads.set(cards[3].character_id());
  EXPECT_EQ(team.SkillValue(), 100 + (105 + 110 + 130 + 110) / 5);
  EXPECT_EQ(team.MaxSkillValue(), 130 + (100 + 105 + 110 + 110) / 5);

  Team::SkillValueDetail skill_value = team.ConstrainedMaxSkillValue(eligible_leads);
  EXPECT_EQ(skill_value.lead_skill, 130);
  EXPECT_EQ(skill_value.skill_value, 130 + (100 + 105 + 110 + 110) / 5);
}

TEST_F(TeamTest, ExampleTeam1ReorderTeamWithConstraint) {
  std::array cards = {
      CreateCard(profile_, /*card_id=*/495, /*level=*/60, /*master_rank=*/0, /*skill_level=*/1),
      CreateCard(profile_, /*card_id=*/535, /*level=*/60, /*master_rank=*/0, /*skill_level=*/2),
      CreateCard(profile_, /*card_id=*/259, /*level=*/60, /*master_rank=*/0, /*skill_level=*/3),
      CreateCard(profile_, /*card_id=*/622, /*level=*/60, /*master_rank=*/5, /*skill_level=*/4),
      CreateCard(profile_, /*card_id=*/224, /*level=*/60),
  };
  Team team = MakeTeam(cards);
  Constraints constraints;
  constraints.AddLeadChar(cards[2].character_id());
  team.ReorderTeamForOptimalSkillValue(constraints);
  EXPECT_EQ(team.cards()[0], &cards[2]);
}

TEST_F(TeamTest, ExampleTeam1ReorderTeamWithConstraintAlt) {
  std::array cards = {
      CreateCard(profile_, /*card_id=*/495, /*level=*/60, /*master_rank=*/0, /*skill_level=*/1),
      CreateCard(profile_, /*card_id=*/535, /*level=*/60, /*master_rank=*/0, /*skill_level=*/2),
      CreateCard(profile_, /*card_id=*/259, /*level=*/60, /*master_rank=*/0, /*skill_level=*/3),
      CreateCard(profile_, /*card_id=*/622, /*level=*/60, /*master_rank=*/5, /*skill_level=*/4),
      CreateCard(profile_, /*card_id=*/224, /*level=*/60),
  };
  Team team = MakeTeam(cards);
  Constraints constraints;
  constraints.AddLeadChar(cards[2].character_id());
  constraints.AddLeadChar(cards[3].character_id());
  team.ReorderTeamForOptimalSkillValue(constraints);
  EXPECT_EQ(team.cards()[0], &cards[3]);
}

TEST_F(TeamTest, ExampleTeam1ReorderTeamWithKizunaConstraint) {
  std::array cards = {
      CreateCard(profile_, /*card_id=*/495, /*level=*/60, /*master_rank=*/0, /*skill_level=*/1),
      CreateCard(profile_, /*card_id=*/535, /*level=*/60, /*master_rank=*/0, /*skill_level=*/2),
      CreateCard(profile_, /*card_id=*/259, /*level=*/60, /*master_rank=*/0, /*skill_level=*/3),
      CreateCard(profile_, /*card_id=*/622, /*level=*/60, /*master_rank=*/5, /*skill_level=*/4),
      CreateCard(profile_, /*card_id=*/224, /*level=*/60),
  };
  Team team = MakeTeam(cards);
  Constraints constraints;
  constraints.AddLeadChar(cards[2].character_id());
  constraints.AddKizunaPair({cards[2].character_id(), cards[4].character_id()});
  team.ReorderTeamForOptimalSkillValue(constraints);
  EXPECT_EQ(team.cards()[0], &cards[2]);
  EXPECT_EQ(team.cards()[1], &cards[4]);
}

TEST_F(TeamTest, ExampleTeam1ReorderTeamWithKizunaConstraintAlt) {
  std::array cards = {
      CreateCard(profile_, /*card_id=*/495, /*level=*/60, /*master_rank=*/0, /*skill_level=*/1),
      CreateCard(profile_, /*card_id=*/535, /*level=*/60, /*master_rank=*/0, /*skill_level=*/2),
      CreateCard(profile_, /*card_id=*/259, /*level=*/60, /*master_rank=*/0, /*skill_level=*/3),
      CreateCard(profile_, /*card_id=*/622, /*level=*/60, /*master_rank=*/5, /*skill_level=*/4),
      CreateCard(profile_, /*card_id=*/224, /*level=*/60),
  };
  Team team = MakeTeam(cards);
  Constraints constraints;
  constraints.AddLeadChar(cards[2].character_id());
  constraints.AddKizunaPair({cards[2].character_id(), cards[4].character_id()});
  constraints.AddLeadChar(cards[3].character_id());
  constraints.AddKizunaPair({cards[3].character_id(), cards[4].character_id()});
  team.ReorderTeamForOptimalSkillValue(constraints);
  EXPECT_EQ(team.cards()[0], &cards[3]);
  EXPECT_EQ(team.cards()[1], &cards[4]);
}

TEST_F(TeamTest, TeamSatisfiesLeadConstraint) {
  std::array cards = {
      CreateCard(profile_, /*card_id=*/495, /*level=*/60, /*master_rank=*/0, /*skill_level=*/1),
      CreateCard(profile_, /*card_id=*/535, /*level=*/60, /*master_rank=*/0, /*skill_level=*/2),
      CreateCard(profile_, /*card_id=*/259, /*level=*/60, /*master_rank=*/0, /*skill_level=*/3),
      CreateCard(profile_, /*card_id=*/622, /*level=*/60, /*master_rank=*/5, /*skill_level=*/4),
      CreateCard(profile_, /*card_id=*/224, /*level=*/60),
  };
  Team team = MakeTeam(cards);
  Constraints constraints1;
  constraints1.AddLeadChar(cards[0].character_id());
  EXPECT_TRUE(team.SatisfiesConstraints(constraints1));

  Constraints constraints2;
  constraints2.AddLeadChar(20);
  EXPECT_FALSE(team.SatisfiesConstraints(constraints2));
}

TEST_F(TeamTest, TeamSatisfiesLeadKizunaConstraint) {
  std::array cards = {
      CreateCard(profile_, /*card_id=*/495, /*level=*/60, /*master_rank=*/0, /*skill_level=*/1),
      CreateCard(profile_, /*card_id=*/535, /*level=*/60, /*master_rank=*/0, /*skill_level=*/2),
      CreateCard(profile_, /*card_id=*/259, /*level=*/60, /*master_rank=*/0, /*skill_level=*/3),
      CreateCard(profile_, /*card_id=*/622, /*level=*/60, /*master_rank=*/5, /*skill_level=*/4),
      CreateCard(profile_, /*card_id=*/224, /*level=*/60),
  };
  Team team = MakeTeam(cards);
  Constraints constraints1;
  constraints1.AddLeadChar(cards[2].character_id());
  constraints1.AddLeadChar(cards[4].character_id());
  constraints1.AddKizunaPair({cards[2].character_id(), cards[4].character_id()});
  EXPECT_TRUE(team.SatisfiesConstraints(constraints1));

  Constraints constraints2;
  constraints2.AddLeadChar(1);
  constraints2.AddLeadChar(20);
  constraints2.AddKizunaPair({1, 20});
  EXPECT_FALSE(team.SatisfiesConstraints(constraints2));
}

TEST_F(TeamTest, TeamSatisfiesLeadSkillConstraint) {
  std::array cards = {
      CreateCard(profile_, /*card_id=*/495, /*level=*/60, /*master_rank=*/0, /*skill_level=*/1),
      CreateCard(profile_, /*card_id=*/535, /*level=*/60, /*master_rank=*/0, /*skill_level=*/2),
      CreateCard(profile_, /*card_id=*/259, /*level=*/60, /*master_rank=*/0, /*skill_level=*/3),
      CreateCard(profile_, /*card_id=*/622, /*level=*/60, /*master_rank=*/5, /*skill_level=*/4),
      CreateCard(profile_, /*card_id=*/15, /*level=*/60),
  };
  Team team = MakeTeam(cards);
  Constraints constraints1;
  constraints1.AddLeadChar(cards[3].character_id());
  constraints1.SetMinLeadSkill(150);
  EXPECT_TRUE(team.SatisfiesConstraints(constraints1));

  Constraints constraints2;
  constraints2.AddLeadChar(cards[0].character_id());
  constraints2.SetMinLeadSkill(150);
  EXPECT_FALSE(team.SatisfiesConstraints(constraints2));
}

TEST_F(TeamTest, TeamSatisfiesRarityConstraint) {
  std::array cards = {
      CreateCard(profile_, /*card_id=*/495, /*level=*/60, /*master_rank=*/0, /*skill_level=*/1),
      CreateCard(profile_, /*card_id=*/535, /*level=*/60, /*master_rank=*/0, /*skill_level=*/2),
      CreateCard(profile_, /*card_id=*/259, /*level=*/60, /*master_rank=*/0, /*skill_level=*/3),
      CreateCard(profile_, /*card_id=*/622, /*level=*/60, /*master_rank=*/5, /*skill_level=*/4),
      CreateCard(profile_, /*card_id=*/224, /*level=*/60),
  };
  Team team = MakeTeam(cards);
  Constraints constraints1;
  constraints1.AddExcludedRarity(db::RARITY_3);
  EXPECT_TRUE(team.SatisfiesConstraints(constraints1));

  Constraints constraints2;
  constraints2.AddExcludedRarity(db::RARITY_4);
  EXPECT_FALSE(team.SatisfiesConstraints(constraints2));
}

}  // namespace
}  // namespace sekai
