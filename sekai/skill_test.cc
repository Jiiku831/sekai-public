#include "sekai/skill.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "sekai/card.h"
#include "sekai/config.h"
#include "sekai/db/master_db.h"
#include "sekai/db/proto/all.h"
#include "sekai/unit_count.h"

namespace sekai {
namespace {

using ::sekai::db::MasterDb;

TEST(SkillTest, TestSkillScoreUp) {
  const db::Skill& db_skill = MasterDb::FindFirst<db::Skill>(1);
  std::array expected = {20, 25, 30, 40};
  for (int skill_level = 1; skill_level <= 4; ++skill_level) {
    Skill skill{db_skill, skill_level};
    EXPECT_EQ(skill.raw_skill_value(), expected[skill_level - 1]);
    EXPECT_EQ(skill.MaxSkillValue(), expected[skill_level - 1]);
  }
}

TEST(SkillTest, TestSkillScoreUpConditionLife) {
  const db::Skill& db_skill = MasterDb::FindFirst<db::Skill>(12);
  std::array expected = {120, 125, 130, 140};
  for (int skill_level = 1; skill_level <= 4; ++skill_level) {
    Skill skill{db_skill, skill_level};
    EXPECT_EQ(skill.raw_skill_value(), expected[skill_level - 1]);
    EXPECT_EQ(skill.MaxSkillValue(), expected[skill_level - 1]);
  }
}

TEST(SkillTest, TestSkillScoreUpKeep) {
  const db::Skill& db_skill = MasterDb::FindFirst<db::Skill>(13);
  std::array expected = {120, 125, 130, 140};
  for (int skill_level = 1; skill_level <= 4; ++skill_level) {
    Skill skill{db_skill, skill_level};
    EXPECT_EQ(skill.raw_skill_value(), expected[skill_level - 1]);
    EXPECT_EQ(skill.MaxSkillValue(), expected[skill_level - 1]);
  }
}

TEST(SkillTest, TestSkillWithSubUnit) {
  const db::Skill& db_skill = MasterDb::FindFirst<db::Skill>(15);
  std::array expected = {80, 85, 90, 100};
  std::array expected_max = {130, 135, 140, 150};
  for (int skill_level = 1; skill_level <= 4; ++skill_level) {
    Skill skill{db_skill, skill_level};
    EXPECT_EQ(skill.raw_skill_value(), expected[skill_level - 1]);
    EXPECT_EQ(skill.MaxSkillValue(), expected_max[skill_level - 1]);
  }
}

TEST(SkillTest, TestSkillEnhanceUnitBonus) {
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
    EXPECT_EQ(card.MaxSkillValue(), 150);
    EXPECT_EQ(unit_count.CharacterCount(db::UNIT_WXS), i + 1);
    EXPECT_EQ(unit_count.CharacterCount(db::UNIT_LN), 4 - i);
  }
}

TEST(SkillTest, TestSkillScoreUpUnitCount0) {
  CardState state;
  state.set_skill_level(4);
  Card card_vbs{MasterDb::FindFirst<db::Card>(600), state};
  const db::Skill& db_skill = MasterDb::FindFirst<db::Skill>(24);

  std::array expected = {70, 75, 80, 90};
  std::array expected_max = {130, 135, 140, 150};
  for (int skill_level = 1; skill_level <= 4; ++skill_level) {
    std::vector<const Card*> cards = {
        &card_vbs, &card_vbs, &card_vbs, &card_vbs, &card_vbs,
    };

    UnitCount unit_count(cards);
    Skill skill{db_skill, skill_level};
    EXPECT_EQ(skill.SkillValue(0, unit_count), expected[skill_level - 1]);
    EXPECT_EQ(skill.MaxSkillValue(), expected_max[skill_level - 1]);
  }
}

TEST(SkillTest, TestSkillScoreUpUnitCount1) {
  CardState state;
  state.set_skill_level(4);
  Card card_wxs{MasterDb::FindFirst<db::Card>(707), state};
  Card card_vbs{MasterDb::FindFirst<db::Card>(600), state};
  const db::Skill& db_skill = MasterDb::FindFirst<db::Skill>(24);

  std::array expected = {100, 105, 110, 120};
  std::array expected_max = {130, 135, 140, 150};
  for (int skill_level = 1; skill_level <= 4; ++skill_level) {
    std::vector<const Card*> cards = {
        &card_wxs, &card_vbs, &card_wxs, &card_vbs, &card_vbs,
    };

    UnitCount unit_count(cards);
    Skill skill{db_skill, skill_level};
    EXPECT_EQ(skill.SkillValue(0, unit_count), expected[skill_level - 1]);
    EXPECT_EQ(skill.MaxSkillValue(), expected_max[skill_level - 1]);
  }
}

TEST(SkillTest, TestSkillScoreUpUnitCount2) {
  CardState state;
  state.set_skill_level(4);
  Card card_wxs{MasterDb::FindFirst<db::Card>(707), state};
  Card card_ln{MasterDb::FindFirst<db::Card>(622), state};
  Card card_vbs{MasterDb::FindFirst<db::Card>(600), state};
  const db::Skill& db_skill = MasterDb::FindFirst<db::Skill>(24);

  std::array expected = {130, 135, 140, 150};
  std::array expected_max = {130, 135, 140, 150};
  for (int skill_level = 1; skill_level <= 4; ++skill_level) {
    std::vector<const Card*> cards = {
        &card_wxs, &card_vbs, &card_wxs, &card_vbs, &card_ln,
    };

    UnitCount unit_count(cards);
    Skill skill{db_skill, skill_level};
    EXPECT_EQ(skill.SkillValue(0, unit_count), expected[skill_level - 1]);
    EXPECT_EQ(skill.MaxSkillValue(), expected_max[skill_level - 1]);
  }
}

TEST(SkillTest, TestSkillScoreUpUnitCount3) {
  CardState state;
  state.set_skill_level(4);
  Card card_wxs{MasterDb::FindFirst<db::Card>(707), state};
  Card card_ln{MasterDb::FindFirst<db::Card>(622), state};
  Card card_vbs{MasterDb::FindFirst<db::Card>(600), state};
  Card card_mmj{MasterDb::FindFirst<db::Card>(88), state};
  const db::Skill& db_skill = MasterDb::FindFirst<db::Skill>(24);

  std::array expected = {130, 135, 140, 150};
  std::array expected_max = {130, 135, 140, 150};
  for (int skill_level = 1; skill_level <= 4; ++skill_level) {
    std::vector<const Card*> cards = {
        &card_wxs, &card_vbs, &card_wxs, &card_mmj, &card_ln,
    };

    UnitCount unit_count(cards);
    Skill skill{db_skill, skill_level};
    EXPECT_EQ(skill.SkillValue(0, unit_count), expected[skill_level - 1]);
    EXPECT_EQ(skill.MaxSkillValue(), expected_max[skill_level - 1]);
  }
}

TEST(SkillTest, TestCharacterRankEnhanceValue) {
  const db::Skill& db_skill = MasterDb::FindFirst<db::Skill>(22);

  std::array expected_base = {90, 95, 100, 110};
  std::array expected_max = {140, 145, 150, 160};
  for (int skill_level = 1; skill_level <= 4; ++skill_level) {
    for (int cr = 0; cr <= 120; ++cr) {
      int expected = static_cast<int>(std::min(cr, 100) / 2) + expected_base[skill_level - 1];
      std::vector<const Card*> cards;
      UnitCount unit_count(cards);
      Skill skill{db_skill, skill_level};
      skill.ApplyCharacterRank(cr);
      EXPECT_EQ(skill.SkillValue(0, unit_count), expected);
      EXPECT_EQ(skill.MaxSkillValue(), expected_max[skill_level - 1]);
    }
  }
}

TEST(SkillTest, TestScoreUpReferenceBase) {
  const db::Skill& db_skill = MasterDb::FindFirst<db::Skill>(23);
  std::array expected = {60, 65, 70, 80};
  std::array expected_max = {120, 130, 140, 150};
  for (int skill_level = 1; skill_level <= 4; ++skill_level) {
    Skill skill{db_skill, skill_level};
    EXPECT_EQ(skill.raw_skill_value(), expected[skill_level - 1]);
    EXPECT_EQ(skill.MaxSkillValue(), expected_max[skill_level - 1]);
  }
}

TEST(SkillTest, TestScoreUpReferenceAddsOtherAverage) {
  CardState state;
  state.set_skill_level(4);
  Card card_1{MasterDb::FindFirst<db::Card>(4), state};    // 100%
  Card card_2{MasterDb::FindFirst<db::Card>(88), state};   // 120%
  Card card_3{MasterDb::FindFirst<db::Card>(622), state};  // 150%
  Card card_4{MasterDb::FindFirst<db::Card>(802), state};  // 100%
  // Expected boost is (50 + 60 + 70 + 50) / 4 = 57.5
  const db::Skill& db_skill = MasterDb::FindFirst<db::Skill>(23);

  std::array expected = {
      0.0f,
      60.f + static_cast<float>(100 + 120 + 120 + 100) / 8,
      65.f + static_cast<float>(100 + 120 + 130 + 100) / 8,
      70.f + 57.5f,
      80.f + 57.5f,
  };
  std::array expected_avg = {
      0.0f,
      static_cast<float>(100 + 120 + 120 + 100) / 4,
      static_cast<float>(100 + 120 + 130 + 100) / 4,
      57.5f * 2,
      57.5f * 2,
  };
  std::array expected_max = {0.0f, 120.f, 130.f, 140.f, 150.f};
  for (int skill_level = 1; skill_level <= 4; ++skill_level) {
    CardState skill_state;
    skill_state.set_skill_level(skill_level);
    Card card_special{MasterDb::FindFirst<db::Card>(949), skill_state};

    std::vector<const Card*> cards = {
        &card_special, &card_1, &card_2, &card_3, &card_4,
    };

    UnitCount unit_count(cards);
    Skill skill{db_skill, skill_level};
    EXPECT_FLOAT_EQ(skill.SkillValue(0, unit_count), expected[skill_level]);
    EXPECT_EQ(skill.MaxSkillValue(), expected_max[skill_level]);
    EXPECT_FLOAT_EQ(unit_count.ReferenceBoostAverageCappedSkillValue(0), expected_avg[skill_level]);
    EXPECT_EQ(card_special.ReferenceBoostCappedMaxSkillValue(skill_level),
              std::min(kReferenceScoreBoostCaps[skill_level], expected_max[skill_level]));
    EXPECT_EQ(skill.ReferenceBoostCappedMaxSkillValue(skill_level),
              std::min(kReferenceScoreBoostCaps[skill_level], expected_max[skill_level]));
    EXPECT_EQ(skill.ReferenceBoostCappedMaxCardSkillValue(skill_level),
              std::min(kReferenceScoreBoostCaps[skill_level], expected_max[skill_level]));
  }
}

TEST(SkillTest, TestScoreUpReferenceAddsOtherAverageExample2) {
  CardState sl1, sl4;
  sl1.set_skill_level(1);
  sl4.set_skill_level(4);
  Card card_1{MasterDb::FindFirst<db::Card>(1058), sl4};  // 150
  Card card_2{MasterDb::FindFirst<db::Card>(227), sl1};   // 100
  Card card_3{MasterDb::FindFirst<db::Card>(671), sl4};   // 140
  Card card_4{MasterDb::FindFirst<db::Card>(1057), sl1};  // 120
  Card card_5{MasterDb::FindFirst<db::Card>(1061), sl1};  // 130
  const db::Skill& db_skill = MasterDb::FindFirst<db::Skill>(23);

  std::vector<const Card*> cards = {&card_1, &card_2, &card_3, &card_4, &card_5};

  {
    constexpr int kSkillLevel = 1;
    UnitCount unit_count(cards);
    Skill skill{db_skill, kSkillLevel};
    EXPECT_FLOAT_EQ(skill.SkillValue(3, unit_count),
                    60 + static_cast<float>(120 + 100 + 120 + 120) / 8);
    EXPECT_EQ(skill.MaxSkillValue(), 120);
    EXPECT_FLOAT_EQ(unit_count.ReferenceBoostAverageCappedSkillValue(3),
                    static_cast<float>(120 + 100 + 120 + 120) / 4);
    EXPECT_EQ(card_4.ReferenceBoostCappedMaxSkillValue(kSkillLevel), 120);
    EXPECT_EQ(skill.ReferenceBoostCappedMaxSkillValue(kSkillLevel), 120);
    EXPECT_EQ(skill.ReferenceBoostCappedMaxCardSkillValue(kSkillLevel), 120);
  }

  {
    constexpr int kSkillLevel = 4;
    UnitCount unit_count(cards);
    Skill skill{db_skill, kSkillLevel};
    EXPECT_FLOAT_EQ(skill.SkillValue(0, unit_count),
                    80 + static_cast<float>(100 + 140 + 120 + 130) / 8);
    EXPECT_EQ(skill.MaxSkillValue(), 150);
    EXPECT_FLOAT_EQ(unit_count.ReferenceBoostAverageCappedSkillValue(0),
                    static_cast<float>(100 + 140 + 120 + 130) / 4);
    EXPECT_EQ(card_1.ReferenceBoostCappedMaxSkillValue(kSkillLevel), 140);
    EXPECT_EQ(skill.ReferenceBoostCappedMaxSkillValue(kSkillLevel), 140);
    EXPECT_EQ(skill.ReferenceBoostCappedMaxCardSkillValue(kSkillLevel), 140);
  }
}

TEST(SkillTest, TestScoreUpReferenceCap) {
  const db::Skill& db_skill = MasterDb::FindFirst<db::Skill>(23);
  for (int skill_level = 1; skill_level < kSkillLevelArraySize; ++skill_level) {
    Skill skill{db_skill, skill_level};
    EXPECT_EQ(skill.ReferenceBoostCappedMaxSkillValue(skill_level),
              kReferenceScoreBoostCaps[skill_level]);
  }
}

TEST(SkillTest, TestScoreUpReferenceAddsOtherAverageCapped) {
  CardState state;
  state.set_skill_level(4);
  Card card{MasterDb::FindFirst<db::Card>(622), state};  // 150%
  const db::Skill& db_skill = MasterDb::FindFirst<db::Skill>(23);

  std::array expected_max = {120, 130, 140, 150};
  for (int skill_level = 1; skill_level <= 4; ++skill_level) {
    CardState skill_state;
    skill_state.set_skill_level(skill_level);
    Card card_special{MasterDb::FindFirst<db::Card>(949), skill_state};
    std::vector<const Card*> cards = {
        &card_special, &card, &card, &card, &card,
    };

    UnitCount unit_count(cards);
    Skill skill{db_skill, skill_level};
    EXPECT_FLOAT_EQ(skill.SkillValue(0, unit_count), expected_max[skill_level - 1]);
    EXPECT_EQ(skill.MaxSkillValue(), expected_max[skill_level - 1]);
  }
}

TEST(SkillTest, TestScoreUpReferenceSucceedsWithNoOtherCards) {
  CardState state;
  state.set_skill_level(4);
  Card card{MasterDb::FindFirst<db::Card>(622), state};  // 150%
  const db::Skill& db_skill = MasterDb::FindFirst<db::Skill>(23);

  std::array expected = {60, 65, 70, 80};
  for (int skill_level = 1; skill_level <= 4; ++skill_level) {
    CardState skill_state;
    skill_state.set_skill_level(skill_level);
    Card card_special{MasterDb::FindFirst<db::Card>(949), skill_state};
    std::vector<const Card*> cards = {
        &card_special,
    };

    UnitCount unit_count(cards);
    Skill skill{db_skill, skill_level};
    EXPECT_FLOAT_EQ(skill.SkillValue(0, unit_count), expected[skill_level - 1]);
  }
}

}  // namespace
}  // namespace sekai
