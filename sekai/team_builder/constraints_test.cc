#include "sekai/team_builder/constraints.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "sekai/array_size.h"
#include "sekai/bitset.h"
#include "sekai/db/master_db.h"
#include "sekai/db/proto/all.h"
#include "sekai/proto/constraints.pb.h"
#include "testing/util.h"

namespace sekai {
namespace {

using ::sekai::db::MasterDb;
using ::testing::ElementsAre;
using ::testing::IsEmpty;
using ::testing::ParseTextProto;
using ::testing::UnorderedElementsAre;

TEST(ConstraintsTest, EmptyConstraintsIsEmpty) {
  Constraints constraints;
  EXPECT_TRUE(constraints.empty());
}

TEST(ConstraintsTest, ConstraintsFromEmptyProtoIsEmpty) {
  TeamConstraints team_constraints;
  Constraints constraints(team_constraints);
  EXPECT_TRUE(constraints.empty());
}

TEST(ConstraintsTest, ConstraintsWithLeadConstraintsIsNotEmpty) {
  Constraints constraints(ParseTextProto<TeamConstraints>(R"pb(
    lead_char_ids: 1
  )pb"));
  EXPECT_FALSE(constraints.empty());
}

TEST(ConstraintsTest, ConstraintsWithKizunaConstraintsIsNotEmpty) {
  Constraints constraints(ParseTextProto<TeamConstraints>(R"pb(
    kizuna_pairs {char_1: 1 char_2: 2}
  )pb"));
  EXPECT_FALSE(constraints.empty());
}

TEST(ConstraintsTest, ConstraintsWithLeadSkillConstraintsIsNotEmpty) {
  Constraints constraints(ParseTextProto<TeamConstraints>(R"pb(
    min_lead_skill: 1
  )pb"));
  EXPECT_FALSE(constraints.empty());
}

TEST(ConstraintsTest, ConstraintsWithLeadSkillConstraints0IsEmpty) {
  Constraints constraints(ParseTextProto<TeamConstraints>(R"pb(
    min_lead_skill: 0
  )pb"));
  EXPECT_TRUE(constraints.empty());
}

TEST(ConstraintsTest, ConstraintsWithExcludedRaritiesIsNotEmpty) {
  Constraints constraints(ParseTextProto<TeamConstraints>(R"pb(
    exclude_rarities: RARITY_4
  )pb"));
  EXPECT_FALSE(constraints.empty());
}

TEST(ConstraintsTest, CharSetSatisfiesEmptyConstraint) {
  Constraints constraints;
  Character chars;
  EXPECT_TRUE(constraints.CharacterSetSatisfiesConstraint(chars));
}

TEST(ConstraintsTest, TestCharacterSetLeadConstraints) {
  Constraints constraints(ParseTextProto<TeamConstraints>(R"pb(
    lead_char_ids: 1 lead_char_ids: 2
  )pb"));
  Character chars;
  chars.set(1);
  EXPECT_TRUE(constraints.CharacterSetSatisfiesConstraint(chars));

  chars.reset();
  chars.set(5);
  EXPECT_FALSE(constraints.CharacterSetSatisfiesConstraint(chars));

  chars.set(2);
  EXPECT_TRUE(constraints.CharacterSetSatisfiesConstraint(chars));
}

TEST(ConstraintsTest, TestCharacterSetKizunaConstraints) {
  Constraints constraints(ParseTextProto<TeamConstraints>(R"pb(
    kizuna_pairs {char_1: 1 char_2: 2}
    kizuna_pairs {char_1: 1 char_2: 3}
  )pb"));
  Character chars;
  chars.set(1);
  EXPECT_FALSE(constraints.CharacterSetSatisfiesConstraint(chars));

  chars.set(5);
  EXPECT_FALSE(constraints.CharacterSetSatisfiesConstraint(chars));

  chars.set(2);
  EXPECT_TRUE(constraints.CharacterSetSatisfiesConstraint(chars));

  chars.reset(2);
  chars.set(3);
}

TEST(ConstraintsTest, GetCharactersEligibleForLeadWithEmptyConstraint) {
  Character chars_present;
  Constraints constraints;
  Character eligible_leads = constraints.GetCharactersEligibleForLead(chars_present);

  Character expected_leads;
  expected_leads.set();
  expected_leads.reset(0);
  EXPECT_EQ(eligible_leads, expected_leads);
}

TEST(ConstraintsTest, GetCharactersEligibleForLeadWithFullLeadsConstraint) {
  Character chars_present;
  Constraints constraints;
  for (int i = 1; i < CharacterArraySize(); ++i) {
    constraints.AddLeadChar(i);
  }
  Character eligible_leads = constraints.GetCharactersEligibleForLead(chars_present);

  Character expected_leads;
  expected_leads.set();
  expected_leads.reset(0);
  EXPECT_EQ(eligible_leads, expected_leads);
}

TEST(ConstraintsTest, GetCharactersEligibleForLeadWithLeadsConstraint) {
  Constraints constraints(ParseTextProto<TeamConstraints>(R"pb(
    lead_char_ids: 1 lead_char_ids: 2
  )pb"));

  Character chars_present;
  Character eligible_leads = constraints.GetCharactersEligibleForLead(chars_present);

  Character expected_leads;
  expected_leads.set(1);
  expected_leads.set(2);
  EXPECT_EQ(eligible_leads, expected_leads);
}

TEST(ConstraintsTest, GetCharactersEligibleForLeadWithKizunaConstraint) {
  Constraints constraints(ParseTextProto<TeamConstraints>(R"pb(
    kizuna_pairs {char_1: 1 char_2: 2}
    kizuna_pairs {char_1: 1 char_2: 3}
    kizuna_pairs {char_1: 1 char_2: 4}
    kizuna_pairs {char_1: 1 char_2: 5}
  )pb"));
  Character chars_present;
  chars_present.set(1);
  chars_present.set(2);
  chars_present.set(3);

  Character eligible_leads = constraints.GetCharactersEligibleForLead(chars_present);

  Character expected_leads;
  expected_leads.set(1);
  expected_leads.set(2);
  expected_leads.set(3);

  EXPECT_EQ(eligible_leads, expected_leads);
}

TEST(ConstraintsTest, GetCharactersEligibleForLeadWithLeadAndKizunaConstraint1) {
  Constraints constraints(ParseTextProto<TeamConstraints>(R"pb(
    lead_char_ids: 1
    lead_char_ids: 2
    lead_char_ids: 4
    kizuna_pairs {char_1: 1 char_2: 2}
    kizuna_pairs {char_1: 1 char_2: 3}
  )pb"));
  Character chars_present;
  chars_present.set(1);
  chars_present.set(2);
  chars_present.set(3);

  Character eligible_leads = constraints.GetCharactersEligibleForLead(chars_present);

  Character expected_leads;
  expected_leads.set(1);
  expected_leads.set(2);

  EXPECT_EQ(eligible_leads, expected_leads);
}

TEST(ConstraintsTest, GetCharactersEligibleForLeadWithLeadAndKizunaConstraint2) {
  Constraints constraints(ParseTextProto<TeamConstraints>(R"pb(
    lead_char_ids: 1
    lead_char_ids: 2
    lead_char_ids: 4
    kizuna_pairs {char_1: 1 char_2: 2}
    kizuna_pairs {char_1: 1 char_2: 3}
    kizuna_pairs {char_1: 2 char_2: 4}
    kizuna_pairs {char_1: 2 char_2: 5}
  )pb"));
  Character chars_present;
  chars_present.set(1);
  chars_present.set(2);
  chars_present.set(3);

  Character eligible_leads = constraints.GetCharactersEligibleForLead(chars_present);

  Character expected_leads;
  expected_leads.set(1);
  expected_leads.set(2);

  EXPECT_EQ(eligible_leads, expected_leads);
}

TEST(ConstraintsTest, GetCharactersEligibleForLeadWithLeadAndKizunaConstraint3) {
  Constraints constraints(ParseTextProto<TeamConstraints>(R"pb(
    lead_char_ids: 1
    lead_char_ids: 2
    lead_char_ids: 4
    kizuna_pairs {char_1: 1 char_2: 3}
    kizuna_pairs {char_1: 2 char_2: 4}
    kizuna_pairs {char_1: 2 char_2: 5}
  )pb"));
  Character chars_present;
  chars_present.set(1);
  chars_present.set(3);

  Character eligible_leads = constraints.GetCharactersEligibleForLead(chars_present);

  Character expected_leads;
  expected_leads.set(1);

  EXPECT_EQ(eligible_leads, expected_leads);
}

TEST(ConstraintsTest, TestLeadSkillConstraints) {
  Constraints constraints(ParseTextProto<TeamConstraints>(R"pb(
    min_lead_skill: 100
  )pb"));
  EXPECT_TRUE(constraints.LeadSkillSatisfiesConstraint(100));
  EXPECT_FALSE(constraints.LeadSkillSatisfiesConstraint(99));
}

TEST(ConstraintsTest, TestFilterCardPool) {
  CardState state;
  state.set_master_rank(5);
  state.set_skill_level(4);

  Card rarity_2{MasterDb::FindFirst<db::Card>(809), state};
  Card rarity_4{MasterDb::FindFirst<db::Card>(844), state};
  Card rarity_bd{MasterDb::FindFirst<db::Card>(784), state};

  std::vector<const Card*> pool = {&rarity_2, &rarity_4, &rarity_bd};
  Constraints constraints_empty;
  EXPECT_THAT(constraints_empty.FilterCardPool(pool),
              UnorderedElementsAre(&rarity_2, &rarity_4, &rarity_bd));

  pool = {&rarity_2, &rarity_4, &rarity_bd};
  Constraints constraints_1(ParseTextProto<TeamConstraints>(R"pb(
    exclude_rarities: RARITY_2
  )pb"));
  EXPECT_THAT(constraints_1.FilterCardPool(pool), UnorderedElementsAre(&rarity_4, &rarity_bd));

  pool = {&rarity_2, &rarity_4, &rarity_bd};
  Constraints constraints_2(ParseTextProto<TeamConstraints>(R"pb(
    exclude_rarities: RARITY_2
    exclude_rarities: RARITY_4
  )pb"));
  EXPECT_THAT(constraints_2.FilterCardPool(pool), UnorderedElementsAre(&rarity_bd));
}

TEST(ConstraintsTest, TestHasLeadSkillConstraint) {
  Constraints constraints(ParseTextProto<TeamConstraints>(R"pb(
    min_lead_skill: 100
  )pb"));
  EXPECT_TRUE(constraints.HasLeadSkillConstraint());
  constraints.SetMinLeadSkill(0);
  EXPECT_FALSE(constraints.HasLeadSkillConstraint());
}

}  // namespace
}  // namespace sekai
