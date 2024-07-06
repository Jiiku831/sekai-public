#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "sekai/card.h"
#include "sekai/db/master_db.h"
#include "sekai/db/proto/all.h"
#include "sekai/estimator.h"
#include "sekai/profile.h"
#include "sekai/proto/event_id.pb.h"
#include "sekai/proto/profile.pb.h"
#include "sekai/team.h"
#include "sekai/team_builder/constraints.h"
#include "sekai/team_builder/event_team_builder.h"
#include "sekai/team_builder/max_bonus_team_builder.h"
#include "sekai/team_builder/max_power_team_builder.h"
#include "sekai/team_builder/naive_team_builder.h"
#include "sekai/team_builder/simulated_annealing_team_builder.h"
#include "testing/util.h"

namespace sekai {
namespace {

using ::sekai::db::MasterDb;
using ::testing::IsEmpty;
using ::testing::Not;
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

Estimator MakeEstimator() {
  std::vector<const db::MusicMeta*> metas =
      db::MasterDb::GetIf<db::MusicMeta>([](const db::MusicMeta& meta) {
        return meta.difficulty() == "expert" && meta.music_id() <= 100;
      });
  return Estimator{Estimator::Mode::kMulti, metas};
}

template <typename T>
class ConstraintCorrectnessTest : public testing::Test {
 protected:
  std::vector<Team> GetTeams() {
    std::vector<const Card*> pool;
    for (const Card& card : cards_) {
      pool.push_back(&card);
    }
    team_builder_.AddConstraints(constraints_);
    return team_builder_.RecommendTeams(pool, profile_, event_bonus_, estimator_);
  }

  void AddCard(int id) {
    CardState state;
    state.set_master_rank(5);
    state.set_skill_level(4);
    cards_.emplace_back(MasterDb::FindFirst<db::Card>(id), state);
  }

  T team_builder_;
  Profile profile_{TestProfile()};
  EventBonus event_bonus_{ParseTextProto<EventId>(R"pb(event_id: 117)pb")};
  Estimator estimator_ = MakeEstimator();
  std::vector<Card> cards_;
  Constraints constraints_;
};

using TeamBuilderTypes =
    ::testing::Types<EventTeamBuilder, MaxBonusTeamBuilder, MaxPowerTeamBuilder, NaiveTeamBuilder,
                     NaivePowerTeamBuilder, NaiveBonusTeamBuilder, NaiveSkillTeamBuilder,
                     SimulatedAnnealingTeamBuilder>;
TYPED_TEST_SUITE(ConstraintCorrectnessTest, TeamBuilderTypes);

TYPED_TEST(ConstraintCorrectnessTest, DisallowedRarity) {
  this->AddCard(4);    // Ichika 4*
  this->AddCard(109);  // Saki 4*
  this->AddCard(111);  // Shiho 4*
  this->AddCard(116);  // Miku 4*
  this->AddCard(189);  // Honami 2*
  this->constraints_.AddExcludedRarity(db::RARITY_2);
  EXPECT_THAT(this->GetTeams(), IsEmpty());
}

TYPED_TEST(ConstraintCorrectnessTest, AllowedRarity) {
  this->AddCard(4);    // Ichika 4*
  this->AddCard(109);  // Saki 4*
  this->AddCard(111);  // Shiho 4*
  this->AddCard(116);  // Miku 4*
  this->AddCard(155);  // Honami 4*
  this->constraints_.AddExcludedRarity(db::RARITY_2);
  EXPECT_THAT(this->GetTeams(), Not(IsEmpty()));
}

TYPED_TEST(ConstraintCorrectnessTest, NoAllowedLeads) {
  this->AddCard(4);                    // Ichika
  this->AddCard(109);                  // Saki
  this->AddCard(111);                  // Shiho
  this->AddCard(125);                  // Haruka
  this->AddCard(129);                  // Airi
  this->AddCard(130);                  // Minori
  this->AddCard(135);                  // Kohane
  this->AddCard(155);                  // Honami
  this->AddCard(157);                  // Ichika
  this->constraints_.AddLeadChar(21);  // Miku
  EXPECT_THAT(this->GetTeams(), IsEmpty());
}

TYPED_TEST(ConstraintCorrectnessTest, HasAllowedLeads) {
  this->AddCard(4);                   // Ichika
  this->AddCard(109);                 // Saki
  this->AddCard(111);                 // Shiho
  this->AddCard(125);                 // Haruka
  this->AddCard(129);                 // Airi
  this->AddCard(130);                 // Minori
  this->AddCard(135);                 // Kohane
  this->AddCard(155);                 // Honami
  this->AddCard(157);                 // Ichika
  this->constraints_.AddLeadChar(1);  // Ichika
  EXPECT_THAT(this->GetTeams(), Not(IsEmpty()));
}

TYPED_TEST(ConstraintCorrectnessTest, NoKizunaPairs) {
  this->AddCard(4);                           // Ichika
  this->AddCard(109);                         // Saki
  this->AddCard(111);                         // Shiho
  this->AddCard(125);                         // Haruka
  this->AddCard(129);                         // Airi
  this->AddCard(130);                         // Minori
  this->AddCard(135);                         // Kohane
  this->AddCard(155);                         // Honami
  this->AddCard(157);                         // Ichika
  this->constraints_.AddKizunaPair({1, 21});  // Miku, Ichika
  this->constraints_.AddKizunaPair({2, 21});  // Miku, Saki
  EXPECT_THAT(this->GetTeams(), IsEmpty());
}

TYPED_TEST(ConstraintCorrectnessTest, HasKizunaPairs) {
  this->AddCard(4);                           // Ichika
  this->AddCard(109);                         // Saki
  this->AddCard(111);                         // Shiho
  this->AddCard(125);                         // Haruka
  this->AddCard(129);                         // Airi
  this->AddCard(130);                         // Minori
  this->AddCard(135);                         // Kohane
  this->AddCard(155);                         // Honami
  this->AddCard(157);                         // Ichika
  this->AddCard(116);                         // Miku
  this->constraints_.AddKizunaPair({1, 21});  // Miku, Ichika
  this->constraints_.AddKizunaPair({2, 21});  // Miku, Saki
  EXPECT_THAT(this->GetTeams(), Not(IsEmpty()));
}

TYPED_TEST(ConstraintCorrectnessTest, HasKizunaPairsButNoLead) {
  this->AddCard(4);                           // Ichika
  this->AddCard(111);                         // Shiho
  this->AddCard(125);                         // Haruka
  this->AddCard(129);                         // Airi
  this->AddCard(130);                         // Minori
  this->AddCard(135);                         // Kohane
  this->AddCard(155);                         // Honami
  this->AddCard(157);                         // Ichika
  this->AddCard(116);                         // Miku
  this->constraints_.AddKizunaPair({1, 21});  // Miku, Ichika
  this->constraints_.AddKizunaPair({2, 21});  // Miku, Saki
  this->constraints_.AddLeadChar(2);          // Saki
  EXPECT_THAT(this->GetTeams(), IsEmpty());
}

TYPED_TEST(ConstraintCorrectnessTest, HasKizunaPairsAndLead) {
  this->AddCard(4);                           // Ichika
  this->AddCard(111);                         // Shiho
  this->AddCard(125);                         // Haruka
  this->AddCard(129);                         // Airi
  this->AddCard(130);                         // Minori
  this->AddCard(135);                         // Kohane
  this->AddCard(155);                         // Honami
  this->AddCard(157);                         // Ichika
  this->AddCard(116);                         // Miku
  this->constraints_.AddKizunaPair({1, 21});  // Miku, Ichika
  this->constraints_.AddKizunaPair({2, 21});  // Miku, Saki
  this->constraints_.AddLeadChar(21);         // Miku
  EXPECT_THAT(this->GetTeams(), Not(IsEmpty()));
}

TYPED_TEST(ConstraintCorrectnessTest, NoMinLeadSkill) {
  this->AddCard(2);   // Ichika 2*
  this->AddCard(6);   // Saki 2*
  this->AddCard(10);  // Honami 2*
  this->AddCard(14);  // Shiho 2*
  this->AddCard(18);  // Minori 2*
  this->constraints_.SetMinLeadSkill(100);
  EXPECT_THAT(this->GetTeams(), IsEmpty());
}

TYPED_TEST(ConstraintCorrectnessTest, HasMinLeadSkill) {
  this->AddCard(2);    // Ichika 2*
  this->AddCard(6);    // Saki 2*
  this->AddCard(10);   // Honami 2*
  this->AddCard(14);   // Shiho 2*
  this->AddCard(622);  // Miku LN uscorer
  this->constraints_.SetMinLeadSkill(100);
  EXPECT_THAT(this->GetTeams(), Not(IsEmpty()));
}

TYPED_TEST(ConstraintCorrectnessTest, HasMinLeadSkillButNoLead) {
  this->AddCard(2);    // Ichika 2*
  this->AddCard(6);    // Saki 2*
  this->AddCard(10);   // Honami 2*
  this->AddCard(14);   // Shiho 2*
  this->AddCard(622);  // Miku LN uscorer
  this->constraints_.SetMinLeadSkill(100);
  this->constraints_.AddLeadChar(2);  // Saki
  EXPECT_THAT(this->GetTeams(), IsEmpty());
}

TYPED_TEST(ConstraintCorrectnessTest, HasMinLeadSkillAndLead) {
  this->AddCard(2);    // Ichika 2*
  this->AddCard(6);    // Saki 2*
  this->AddCard(10);   // Honami 2*
  this->AddCard(14);   // Shiho 2*
  this->AddCard(622);  // Miku LN uscorer
  this->constraints_.SetMinLeadSkill(100);
  this->constraints_.AddLeadChar(21);  // Miku
  EXPECT_THAT(this->GetTeams(), Not(IsEmpty()));
}

TYPED_TEST(ConstraintCorrectnessTest, HasMinLeadSkillButNoKizuna) {
  this->AddCard(2);    // Ichika 2*
  this->AddCard(6);    // Saki 2*
  this->AddCard(10);   // Honami 2*
  this->AddCard(14);   // Shiho 2*
  this->AddCard(622);  // Miku LN uscorer
  this->constraints_.SetMinLeadSkill(100);
  this->constraints_.AddKizunaPair({1, 2});  // Saki, Ichika
  EXPECT_THAT(this->GetTeams(), IsEmpty());
}

TYPED_TEST(ConstraintCorrectnessTest, HasMinLeadSkillAndKizuna) {
  this->AddCard(2);    // Ichika 2*
  this->AddCard(6);    // Saki 2*
  this->AddCard(10);   // Honami 2*
  this->AddCard(14);   // Shiho 2*
  this->AddCard(622);  // Miku LN uscorer
  this->constraints_.SetMinLeadSkill(100);
  this->constraints_.AddKizunaPair({21, 2});  // Miku, Saki
  EXPECT_THAT(this->GetTeams(), Not(IsEmpty()));
}

}  // namespace
}  // namespace sekai
