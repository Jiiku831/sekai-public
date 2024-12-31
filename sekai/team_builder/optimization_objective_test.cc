#include "sekai/team_builder/optimization_objective.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "sekai/bitset.h"
#include "sekai/card.h"
#include "sekai/db/master_db.h"
#include "sekai/db/proto/all.h"
#include "sekai/estimator.h"
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
      15, 15, 15, 15,
      # VBS chars
      15, 15, 15, 15,
      # WXS chars
      15, 15, 15, 15,
      # 25ji chars
      15, 15, 15, 15,
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
      15, 15,
      # VBS unit
      15, 15,
      # WXS unit
      15, 15,
      # 25ji unit
      15, 15,
      # VS unit
      15, 15, 15, 15, 15,
      # Cool plant (miyajo first)
      15, 15,
      # Cute plant
      15, 15,
      # Pure plant
      15, 15,
      # Happy plant
      15, 15,
      # Mysterious plant
      15, 15
    ]
    character_ranks: [
      # Offset (ignored)
      0,
      # LN
      50, 50, 50, 50,
      # MMJ
      50, 50, 50, 50,
      # VBS
      50, 50, 50, 50,
      # WXS
      50, 50, 50, 50,
      # 25ji
      50, 50, 50, 50,
      # VS
      50, 50, 50, 50, 50, 50
    ]
    bonus_power: 300
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

class OptimizeExactPointsTest : public ::testing::Test {
 protected:
  Card CreateCard(const Profile& profile, const EventBonus& event_bonus, int card_id, int level,
                  int master_rank = 0, int skill_level = 1) {
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
    card.ApplyEventBonus(event_bonus);
    return card;
  }

  void SetUp() override { all_chars_.set(); }

  Profile profile_{TestProfile()};
  Character all_chars_;
  Estimator estimator_ = RandomExEstimator(Estimator::Mode::kMulti);
};

TEST_F(OptimizeExactPointsTest, ExampleViableTeam1WorldLink) {
  auto event_bonus = ParseTextProto<SimpleEventBonus>(R"pb(
    chars {char_id: 21}
    chars {char_id: 22}
    chars {char_id: 23}
    chars {char_id: 24}
    chars {char_id: 25}
    chars {char_id: 26}
    chapter_char_id: 21
  )pb");
  EventBonus bonus(event_bonus);
  OptimizeExactPoints objective(315, 0.95);
  std::array cards = {
      CreateCard(profile_, bonus, /*card_id=*/289, /*level=*/60, /*master_rank=*/5),
      CreateCard(profile_, bonus, /*card_id=*/361, /*level=*/60),
      CreateCard(profile_, bonus, /*card_id=*/313, /*level=*/60),
      CreateCard(profile_, bonus, /*card_id=*/896, /*level=*/60),
      CreateCard(profile_, bonus, /*card_id=*/144, /*level=*/60),
  };
  Team team = MakeTeam(cards);
  EXPECT_GE(objective.GetObjectiveFunction()(team, profile_, bonus, estimator_, all_chars_), 0);
}

TEST_F(OptimizeExactPointsTest, ExampleNotViableTeam1WorldLink) {
  auto event_bonus = ParseTextProto<SimpleEventBonus>(R"pb(
    chars {char_id: 21}
    chars {char_id: 22}
    chars {char_id: 23}
    chars {char_id: 24}
    chars {char_id: 25}
    chars {char_id: 26}
    chapter_char_id: 21
  )pb");
  EventBonus bonus(event_bonus);
  OptimizeExactPoints objective(314, 0.95);
  std::array cards = {
      CreateCard(profile_, bonus, /*card_id=*/289, /*level=*/60, /*master_rank=*/5),
      CreateCard(profile_, bonus, /*card_id=*/361, /*level=*/60),
      CreateCard(profile_, bonus, /*card_id=*/313, /*level=*/60),
      CreateCard(profile_, bonus, /*card_id=*/896, /*level=*/60),
      CreateCard(profile_, bonus, /*card_id=*/144, /*level=*/60),
  };
  Team team = MakeTeam(cards);
  EXPECT_LT(objective.GetObjectiveFunction()(team, profile_, bonus, estimator_, all_chars_), 0);
}

TEST_F(OptimizeExactPointsTest, ExampleNotViableTeam2WorldLink) {
  auto event_bonus = ParseTextProto<SimpleEventBonus>(R"pb(
    chars {char_id: 21}
    chars {char_id: 22}
    chars {char_id: 23}
    chars {char_id: 24}
    chars {char_id: 25}
    chars {char_id: 26}
    chapter_char_id: 21
  )pb");
  EventBonus bonus(event_bonus);
  OptimizeExactPoints objective(536, 0.95);
  std::array cards = {
      CreateCard(profile_, bonus, /*card_id=*/289, /*level=*/60, /*master_rank=*/5),
      CreateCard(profile_, bonus, /*card_id=*/361, /*level=*/60),
      CreateCard(profile_, bonus, /*card_id=*/313, /*level=*/60),
      CreateCard(profile_, bonus, /*card_id=*/896, /*level=*/60),
      CreateCard(profile_, bonus, /*card_id=*/144, /*level=*/60),
  };
  Team team = MakeTeam(cards);
  EXPECT_LT(objective.GetObjectiveFunction()(team, profile_, bonus, estimator_, all_chars_), 0);
}

TEST_F(OptimizeExactPointsTest, ExampleNotViableTeam3WorldLink) {
  auto event_bonus = ParseTextProto<SimpleEventBonus>(R"pb(
    chars {char_id: 21}
    chars {char_id: 22}
    chars {char_id: 23}
    chars {char_id: 24}
    chars {char_id: 25}
    chars {char_id: 26}
    chapter_char_id: 21
  )pb");
  EventBonus bonus(event_bonus);
  OptimizeExactPoints objective(620, 0.95);
  std::array cards = {
      CreateCard(profile_, bonus, /*card_id=*/289, /*level=*/60, /*master_rank=*/5),
      CreateCard(profile_, bonus, /*card_id=*/361, /*level=*/60),
      CreateCard(profile_, bonus, /*card_id=*/313, /*level=*/60),
      CreateCard(profile_, bonus, /*card_id=*/896, /*level=*/60),
      CreateCard(profile_, bonus, /*card_id=*/144, /*level=*/60),
  };
  Team team = MakeTeam(cards);
  EXPECT_LT(objective.GetObjectiveFunction()(team, profile_, bonus, estimator_, all_chars_), 0);
}

}  // namespace
}  // namespace sekai
