#include "sekai/team.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

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

using ::testing::Eq;
using ::testing::ParseTextProto;
using ::testing::Pointwise;

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
                  int skill_level = 1) {
    CardState state;
    state.set_level(level);
    state.set_master_rank(master_rank);
    state.set_skill_level(skill_level);
    state.set_special_training(true);
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
  std::array expected = {174455, 97100, 7925, 270};
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
  std::array expected = {170495, 99142, 8514, 270};
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
  std::array expected = {176264, 95381, 8282, 270};
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
  std::array expected = {154589, 115112, 7719, 270};
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
  std::array expected = {177477, 159725, 8865, 270};
  EXPECT_THAT(team.PowerDetailed(profile_), Pointwise(Eq(), expected));
  // Actual team power is 346337
  EXPECT_EQ(team.Power(profile_), 346337);
  EXPECT_EQ(team.Power(profile_), team.PowerDetailed(profile_).sum());
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
  constraints1.AddKizunaPair({cards[2].character_id(), cards[4].character_id()});
  EXPECT_TRUE(team.SatisfiesConstraints(constraints1));

  Constraints constraints2;
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
