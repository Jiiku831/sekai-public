#include "sekai/team_builder/skill_selector.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "sekai/card.h"
#include "sekai/db/master_db.h"
#include "sekai/estimator.h"
#include "sekai/event_bonus.h"
#include "sekai/profile.h"
#include "sekai/proto/card_state.pb.h"
#include "sekai/proto/profile.pb.h"
#include "sekai/team.h"
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
    cards {
      key: 950
      value {
        skill_level: 4
        special_training: true
      }
    }
    cards {
      key: 622
      value {
        skill_level: 4
        special_training: true
      }
    }
    cards {
      key: 707
      value {
        skill_level: 4
        special_training: true
      }
    }
    cards {
      key: 600
      value {
        skill_level: 4
        special_training: true
      }
    }
    cards {
      key: 802
      value {
        skill_level: 4
        special_training: true
      }
    }
    cards {
      key: 4
      value {
        skill_level: 4
        special_training: true
      }
    }
    cards {
      key: 949
      value {
        skill_level: 4
        special_training: true
      }
    }
    cards {
      key: 88
      value {
        skill_level: 4
        special_training: true
      }
    }
    cards {
      key: 622
      value {
        skill_level: 4
        special_training: true
      }
    }
  )pb");
  // clang-format on
}

TEST(OptimizeSkillSelectionTest, SecondarySkillWithUnitCountPrimary) {
  ProfileProto profile_proto = TestProfile();
  EventBonus event_bonus;
  Estimator estimator = RandomExEstimator(Estimator::Mode::kSolo);
  {
    Profile profile(profile_proto);
    Team team = OptimizeSkillSelection(
        std::array{
            profile.GetSecondaryCard(950),
            profile.GetCard(707),
            profile.GetCard(707),
            profile.GetCard(622),
            profile.GetCard(600),
        },
        profile, event_bonus, estimator);
    UnitCount unit_count{team.cards()};
    EXPECT_FALSE(team.cards()[0]->UseSecondarySkill());
    EXPECT_EQ(team.cards()[0]->SkillValue(0, unit_count), 150);
    EXPECT_EQ(team.cards()[0]->MaxSkillValue(), 150);
  }

  profile_proto.set_character_ranks(25, 82);
  {
    Profile profile(profile_proto);
    Team team = OptimizeSkillSelection(
        std::array{
            profile.GetCard(950),
            profile.GetCard(707),
            profile.GetCard(707),
            profile.GetCard(622),
            profile.GetCard(600),
        },
        profile, event_bonus, estimator);
    UnitCount unit_count{team.cards()};
    EXPECT_TRUE(team.cards()[0]->UseSecondarySkill());
    EXPECT_EQ(team.cards()[0]->SkillValue(0, unit_count), 151);
    EXPECT_EQ(team.cards()[0]->MaxSkillValue(), 160);
  }
}

TEST(OptimizeSkillSelectionTest, SecondarySkillWithUnitCountPrimaryNoSpecialTraining) {
  ProfileProto profile_proto = TestProfile();
  for (auto& [id, state] : *profile_proto.mutable_cards()) {
    state.set_special_training(false);
  }
  profile_proto.set_character_ranks(25, 82);
  Profile profile(profile_proto);
  EventBonus event_bonus;
  Estimator estimator = RandomExEstimator(Estimator::Mode::kSolo);
  Team team = OptimizeSkillSelection(
      std::array{
          profile.GetCard(950),
          profile.GetCard(707),
          profile.GetCard(707),
          profile.GetCard(622),
          profile.GetCard(600),
      },
      profile, event_bonus, estimator);
  UnitCount unit_count{team.cards()};
  EXPECT_FALSE(team.cards()[0]->UseSecondarySkill());
  EXPECT_EQ(team.cards()[0]->SkillValue(0, unit_count), 150);
  EXPECT_EQ(team.cards()[0]->MaxSkillValue(), 150);
}

TEST(OptimizeSkillSelectionTest, SecondarySkillWithReferenceBoostPrimary) {
  ProfileProto profile_proto = TestProfile();
  EventBonus event_bonus;
  Estimator estimator = RandomExEstimator(Estimator::Mode::kSolo);
  {
    Profile profile(profile_proto);
    Team team = OptimizeSkillSelection(
        std::array{
            profile.GetSecondaryCard(949),
            profile.GetCard(4),    // 100%
            profile.GetCard(88),   // 120%
            profile.GetCard(622),  // 150%
            profile.GetCard(802),  // 100%
        },
        profile, event_bonus, estimator);
    UnitCount unit_count{team.cards()};
    EXPECT_FALSE(team.cards()[0]->UseSecondarySkill());
    EXPECT_FLOAT_EQ(team.cards()[0]->SkillValue(0, unit_count), 80 + 57.5);
    EXPECT_EQ(team.cards()[0]->MaxSkillValue(), 150);
  }

  profile_proto.set_character_ranks(17, 90);
  {
    Profile profile(profile_proto);
    Team team = OptimizeSkillSelection(
        std::array{
            profile.GetCard(949),
            profile.GetCard(4),    // 100%
            profile.GetCard(88),   // 120%
            profile.GetCard(622),  // 150%
            profile.GetCard(802),  // 100%
        },
        profile, event_bonus, estimator);
    UnitCount unit_count{team.cards()};
    EXPECT_TRUE(team.cards()[0]->UseSecondarySkill());
    EXPECT_EQ(team.cards()[0]->SkillValue(0, unit_count), 110 + 45);
    EXPECT_EQ(team.cards()[0]->MaxSkillValue(), 160);
  }
}

TEST(OptimizeSkillSelectionTest, SecondarySkillWithReferenceBoostPrimaryNoSpecialTraining) {
  ProfileProto profile_proto = TestProfile();
  for (auto& [id, state] : *profile_proto.mutable_cards()) {
    state.set_special_training(false);
  }
  profile_proto.set_character_ranks(17, 56);
  Profile profile(profile_proto);
  EventBonus event_bonus;
  Estimator estimator = RandomExEstimator(Estimator::Mode::kSolo);
  Team team = OptimizeSkillSelection(
      std::array{
          profile.GetCard(949),
          profile.GetCard(4),    // 100%
          profile.GetCard(88),   // 120%
          profile.GetCard(622),  // 150%
          profile.GetCard(802),  // 100%
      },
      profile, event_bonus, estimator);
  UnitCount unit_count{team.cards()};
  EXPECT_FALSE(team.cards()[0]->UseSecondarySkill());
  EXPECT_FLOAT_EQ(team.cards()[0]->SkillValue(0, unit_count),
                  80 + static_cast<float>(100 + 120 + 140 + 100) / 8);
  EXPECT_EQ(team.cards()[0]->MaxSkillValue(), 150);
}

}  // namespace
}  // namespace sekai
