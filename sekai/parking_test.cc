#include "sekai/parking.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "absl/log/log.h"
#include "sekai/db/master_db.h"
#include "testing/util.h"

namespace sekai {
namespace {

using ::testing::Pair;
using ::testing::ProtoEquals;
using ::testing::UnorderedElementsAre;

int GetSum(const absl::flat_hash_map<int, int> values) {
  int sum = 0;
  for (auto [val, count] : values) {
    // LOG(INFO) << val << "x" << count << "=" << val * count;
    sum += val * count;
  }
  return sum;
}

int GetCount(const absl::flat_hash_map<int, int> values) {
  int sum = 0;
  for (auto [val, count] : values) {
    sum += count;
  }
  return sum;
}

TEST(SolveSubsetSumWithRepetitionTest, TestExample1) {
  std::array values = {200, 202, 204, 206, 208, 210, 212, 214, 216, 218, 220, 222, 224,
                       226, 228, 230, 232, 234, 236, 238, 240, 242, 244, 246, 248, 250,
                       252, 254, 256, 258, 260, 262, 264, 266, 268, 270, 272, 274, 276,
                       278, 280, 282, 284, 286, 288, 290, 292, 294, 296, 298, 300};
  constexpr int kTarget = 5900;
  auto result = SolveSubsetSumWithRepetition(values, kTarget);
  ASSERT_TRUE(result.ok()) << result.status();
  EXPECT_EQ(GetSum(*result), kTarget);
  EXPECT_EQ(GetCount(*result), 20);
}

TEST(SolveSubsetSumWithRepetitionTest, TestExample2) {
  std::array values = {200, 202, 204, 206, 208, 210, 212, 214, 216, 218, 220, 222, 224,
                       226, 228, 230, 232, 234, 236, 238, 240, 242, 244, 246, 248, 250,
                       252, 254, 256, 258, 260, 262, 264, 266, 268, 270, 272, 274, 276,
                       278, 280, 282, 284, 286, 288, 290, 292, 294, 296, 298, 300};
  constexpr int kTarget = 590100;
  auto result = SolveSubsetSumWithRepetition(values, kTarget);
  ASSERT_TRUE(result.ok()) << result.status();
  EXPECT_EQ(GetSum(*result), kTarget);
  EXPECT_EQ(GetCount(*result), 1967);
}

TEST(SolveSubsetSumWithRepetitionTest, TestExample3) {
  std::array values = {200, 202, 204, 206, 208, 210, 212, 214, 216, 218, 220, 222, 224,
                       226, 228, 230, 232, 234, 236, 238, 240, 242, 244, 246, 248, 250,
                       252, 254, 256, 258, 260, 262, 264, 266, 268, 270, 272, 274, 276,
                       278, 280, 282, 284, 286, 288, 290, 292, 294, 296, 298, 300};
  constexpr int kTarget = 590098;
  auto result = SolveSubsetSumWithRepetition(values, kTarget);
  ASSERT_TRUE(result.ok()) << result.status();
  EXPECT_EQ(GetSum(*result), kTarget);
}

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

Team MakeTeam(std::span<const Card> cards) {
  std::vector<const Card*> card_ptrs;
  for (const Card& card : cards) {
    card_ptrs.push_back(&card);
  }
  return Team{card_ptrs};
}

class AnnotateTeamProtoWithMultiTurnParkingStrategyTest : public ::testing::Test {
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
};

TEST_F(AnnotateTeamProtoWithMultiTurnParkingStrategyTest, AnnotateExample1) {
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
  TeamProto team_proto;
  AnnotateTeamProtoWithMultiTurnParkingStrategy(team, profile_, bonus, /*accuracy=*/0.9, 7'180'000,
                                                team_proto);
  int total_mult = 0;
  int total_ep = 0;
  int total_plays = 0;
  for (const auto& park : team_proto.multi_turn_parking()) {
    total_mult += park.total_multiplier();
    EXPECT_EQ(park.total_multiplier(), park.multiplier() * park.plays());
    total_ep += park.total_ep();
    EXPECT_EQ(park.total_ep(), park.total_multiplier() * park.base_ep());
    total_plays += park.plays();
  }
  EXPECT_EQ(total_plays, 286) << team_proto.DebugString();
  EXPECT_EQ(total_mult, 10'000) << team_proto.DebugString();
  EXPECT_EQ(total_ep, 7'180'000) << team_proto.DebugString();
}

TEST_F(AnnotateTeamProtoWithMultiTurnParkingStrategyTest, AnnotateExample2) {
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
  TeamProto team_proto;
  AnnotateTeamProtoWithMultiTurnParkingStrategy(team, profile_, bonus, /*accuracy=*/0.9, 88'888,
                                                team_proto);
  int total_mult = 0;
  int total_ep = 0;
  int total_plays = 0;
  for (const auto& park : team_proto.multi_turn_parking()) {
    total_mult += park.total_multiplier();
    EXPECT_EQ(park.total_multiplier(), park.multiplier() * park.plays());
    total_ep += park.total_ep();
    EXPECT_EQ(park.total_ep(), park.total_multiplier() * park.base_ep());
    total_plays += park.plays();
  }
  EXPECT_EQ(total_plays, 5) << team_proto.DebugString();
  EXPECT_EQ(total_mult, 124) << team_proto.DebugString();
  EXPECT_EQ(total_ep, 88'888) << team_proto.DebugString();
}

}  // namespace
}  // namespace sekai
