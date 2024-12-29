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
  OptimizeExactPoints objective(315);
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
  OptimizeExactPoints objective(314);
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
  OptimizeExactPoints objective(536);
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
  OptimizeExactPoints objective(620);
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

TEST_F(OptimizeExactPointsTest, PointsAreExpectedExample1) {
  std::array expected = {
      308, 311, 314, 317, 320, 323, 326, 329, 332, 335, 338, 341, 344, 348, 351, 354, 357, 360,
      363, 366, 369, 372, 375, 378, 381, 385, 388, 391, 394, 397, 400, 403, 406, 409, 412, 415,
      418, 421, 425, 428, 431, 434, 437, 440, 443, 446, 449, 452, 455, 458, 462, 465, 468, 471,
      474, 477, 480, 483, 486, 489, 492, 495, 498, 502, 505, 508, 511, 514, 517, 520, 523, 526,
      529, 532, 535, 539, 542, 545, 548, 551, 554, 557, 560, 563, 566, 569, 572, 575, 579, 582,
      585, 588, 591, 594, 597, 600, 603, 606, 609, 612, 616, 619, 622, 625, 628, 631, 634, 637,
      640, 643, 646, 649, 652, 656, 659, 662, 665, 668, 671, 674, 677, 680, 683, 686, 689};
  constexpr int kEb = 208;
  constexpr int kStep = 20000;
  for (int i = 0; static_cast<std::size_t>(i) < expected.size(); ++i) {
    for (int j = 0; j < kStep; ++j) {
      ASSERT_EQ(OptimizeExactPoints::BaseEp(i * kStep + j, kEb), expected[i])
          << "score: " << i * kStep + j;
    }
  }
}

TEST_F(OptimizeExactPointsTest, PointsAreExpectedExample2) {
  std::array expected = {
      497,  501,  506,  511,  516,  521,  526,  531,  536,  541,  546,  551,  556,  561,
      566,  571,  576,  581,  586,  591,  596,  601,  606,  611,  616,  621,  626,  631,
      636,  641,  646,  651,  656,  661,  665,  670,  675,  680,  685,  690,  695,  700,
      705,  710,  715,  720,  725,  730,  735,  740,  745,  750,  755,  760,  765,  770,
      775,  780,  785,  790,  795,  800,  805,  810,  815,  820,  825,  829,  834,  839,
      844,  849,  854,  859,  864,  869,  874,  879,  884,  889,  894,  899,  904,  909,
      914,  919,  924,  929,  934,  939,  944,  949,  954,  959,  964,  969,  974,  979,
      984,  989,  994,  998,  1003, 1008, 1013, 1018, 1023, 1028, 1033, 1038, 1043, 1048,
      1053, 1058, 1063, 1068, 1073, 1078, 1083, 1088, 1093, 1098, 1103, 1108, 1113};
  constexpr int kEb = 397;
  constexpr int kStep = 20000;
  for (int i = 0; static_cast<std::size_t>(i) < expected.size(); ++i) {
    for (int j = 0; j < kStep; ++j) {
      ASSERT_EQ(OptimizeExactPoints::BaseEp(i * kStep + j, kEb), expected[i])
          << "score: " << i * kStep + j;
    }
  }
}

TEST_F(OptimizeExactPointsTest, PointsAreExpectedExample3) {
  std::array expected = {
      681,  687,  694,  701,  708,  715,  721,  728,  735,  742,  749,  755,  762,  769,
      776,  783,  789,  796,  803,  810,  817,  824,  830,  837,  844,  851,  858,  864,
      871,  878,  885,  892,  898,  905,  912,  919,  926,  932,  939,  946,  953,  960,
      967,  973,  980,  987,  994,  1001, 1007, 1014, 1021, 1028, 1035, 1041, 1048, 1055,
      1062, 1069, 1075, 1082, 1089, 1096, 1103, 1110, 1116, 1123, 1130, 1137, 1144, 1150,
      1157, 1164, 1171, 1178, 1184, 1191, 1198, 1205, 1212, 1218, 1225, 1232, 1239, 1246,
      1253, 1259, 1266, 1273, 1280, 1287, 1293, 1300, 1307, 1314, 1321, 1327, 1334, 1341,
      1348, 1355, 1362, 1368, 1375, 1382, 1389, 1396, 1402, 1409, 1416, 1423, 1430, 1436,
      1443, 1450, 1457, 1464, 1470, 1477, 1484, 1491, 1498, 1505, 1511, 1518, 1525};
  constexpr int kEb = 581;
  constexpr int kStep = 20000;
  for (int i = 0; static_cast<std::size_t>(i) < expected.size(); ++i) {
    for (int j = 0; j < kStep; ++j) {
      ASSERT_EQ(OptimizeExactPoints::BaseEp(i * kStep + j, kEb), expected[i])
          << "score: " << i * kStep + j;
    }
  }
}

}  // namespace
}  // namespace sekai
