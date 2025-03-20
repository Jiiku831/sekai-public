#include "sekai/profile.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "sekai/array_size.h"
#include "sekai/config.h"
#include "sekai/proto_util.h"
#include "sekai/unit_count.h"
#include "testing/util.h"

namespace sekai {
namespace {

using ::testing::_;
using ::testing::DescribeMatcher;
using ::testing::ElementsAre;
using ::testing::Eq;
using ::testing::FloatEq;
using ::testing::IsEmpty;
using ::testing::ParseTextProto;
using ::testing::Pointee;
using ::testing::ProtoEquals;
using ::testing::SizeIs;

MATCHER_P2(BonusRateIs, rate, matching_rate,
           "rate that " + DescribeMatcher<float>(rate, negation) + (negation ? " or" : " and") +
               " matching rate that " + DescribeMatcher<float>(matching_rate, negation)) {
  return testing::ExplainMatchResult(rate, arg.rate, result_listener) &&
         testing::ExplainMatchResult(matching_rate, arg.matching_rate, result_listener);
}

MATCHER_P(CardStateThat, matcher,
          "card state that " + DescribeMatcher<CardState>(matcher, negation)) {
  return testing::ExplainMatchResult(matcher, arg.state(), result_listener);
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
    mysekai_gate_levels: [0, 5, 10, 20, 30, 40]
    # Ichika S/M/L
    mysekai_fixture_crafted {
    key: 342
    value: true
    }
    mysekai_fixture_crafted {
    key: 343
    value: true
    }
    mysekai_fixture_crafted {
    key: 344
    value: true
    }
    # Saki S/M/L
    mysekai_fixture_crafted {
    key: 345
    value: false
    }
    mysekai_fixture_crafted {
    key: 346
    value: false
    }
    mysekai_fixture_crafted {
    key: 347
    value: true
    }
  )pb");
  // clang-format on
}

TEST(ProfileTest, CheckTestProfileAttrBonus) {
  Profile profile(TestProfile());
  std::vector<BonusRate> attr_bonus = {profile.attr_bonus().begin(), profile.attr_bonus().end()};
  EXPECT_THAT(attr_bonus, ElementsAre(_,                                        // Offset
                                      BonusRateIs(FloatEq(9), FloatEq(18)),     // Cool
                                      BonusRateIs(FloatEq(15), FloatEq(30)),    // Cute
                                      BonusRateIs(FloatEq(12.5), FloatEq(25)),  // Happy
                                      BonusRateIs(FloatEq(8.5), FloatEq(17)),   // Myst
                                      BonusRateIs(FloatEq(15), FloatEq(30))     // Pure
                                      ));
}

TEST(ProfileTest, CheckTestProfileCharBonus) {
  Profile profile(TestProfile());
  std::vector<BonusRate> char_bonus = {profile.char_bonus().begin(), profile.char_bonus().end()};
  EXPECT_THAT(char_bonus, ElementsAre(_,
                                      // LN
                                      BonusRateIs(FloatEq(30), FloatEq(0)),  //
                                      BonusRateIs(FloatEq(30), FloatEq(0)),  //
                                      BonusRateIs(FloatEq(30), FloatEq(0)),  //
                                      BonusRateIs(FloatEq(30), FloatEq(0)),  //
                                      // MMJ
                                      BonusRateIs(FloatEq(20), FloatEq(0)),  //
                                      BonusRateIs(FloatEq(20), FloatEq(0)),  //
                                      BonusRateIs(FloatEq(20), FloatEq(0)),  //
                                      BonusRateIs(FloatEq(20), FloatEq(0)),  //
                                      // VBS
                                      BonusRateIs(FloatEq(20), FloatEq(0)),  //
                                      BonusRateIs(FloatEq(20), FloatEq(0)),  //
                                      BonusRateIs(FloatEq(20), FloatEq(0)),  //
                                      BonusRateIs(FloatEq(20), FloatEq(0)),  //
                                      // WXS
                                      BonusRateIs(FloatEq(20), FloatEq(0)),  //
                                      BonusRateIs(FloatEq(20), FloatEq(0)),  //
                                      BonusRateIs(FloatEq(20), FloatEq(0)),  //
                                      BonusRateIs(FloatEq(20), FloatEq(0)),  //
                                      // 25ji
                                      BonusRateIs(FloatEq(20), FloatEq(0)),  //
                                      BonusRateIs(FloatEq(20), FloatEq(0)),  //
                                      BonusRateIs(FloatEq(20), FloatEq(0)),  //
                                      BonusRateIs(FloatEq(20), FloatEq(0)),  //
                                      // VS
                                      BonusRateIs(FloatEq(30), FloatEq(0)),  //
                                      BonusRateIs(FloatEq(30), FloatEq(0)),  //
                                      BonusRateIs(FloatEq(30), FloatEq(0)),  //
                                      BonusRateIs(FloatEq(30), FloatEq(0)),  //
                                      BonusRateIs(FloatEq(30), FloatEq(0)),  //
                                      BonusRateIs(FloatEq(30), FloatEq(0))   //
                                      ));
}

TEST(ProfileTest, CheckTestProfileCrBonus) {
  Profile profile(TestProfile());
  std::vector<BonusRate> cr_bonus = {profile.cr_bonus().begin(), profile.cr_bonus().end()};
  EXPECT_THAT(cr_bonus, ElementsAre(_,
                                    // LN
                                    BonusRateIs(FloatEq(5), FloatEq(0)),  //
                                    BonusRateIs(FloatEq(5), FloatEq(0)),  //
                                    BonusRateIs(FloatEq(5), FloatEq(0)),  //
                                    BonusRateIs(FloatEq(5), FloatEq(0)),  //
                                    // MMJ
                                    BonusRateIs(FloatEq(4.4), FloatEq(0)),  //
                                    BonusRateIs(FloatEq(4.7), FloatEq(0)),  //
                                    BonusRateIs(FloatEq(4.4), FloatEq(0)),  //
                                    BonusRateIs(FloatEq(4.1), FloatEq(0)),  //
                                    // VBS
                                    BonusRateIs(FloatEq(4.7), FloatEq(0)),  //
                                    BonusRateIs(FloatEq(4.1), FloatEq(0)),  //
                                    BonusRateIs(FloatEq(4.3), FloatEq(0)),  //
                                    BonusRateIs(FloatEq(4.1), FloatEq(0)),  //
                                    // WXS
                                    BonusRateIs(FloatEq(4.3), FloatEq(0)),  //
                                    BonusRateIs(FloatEq(4.3), FloatEq(0)),  //
                                    BonusRateIs(FloatEq(4.2), FloatEq(0)),  //
                                    BonusRateIs(FloatEq(4.1), FloatEq(0)),  //
                                    // 25ji
                                    BonusRateIs(FloatEq(4.5), FloatEq(0)),  //
                                    BonusRateIs(FloatEq(4.2), FloatEq(0)),  //
                                    BonusRateIs(FloatEq(4.4), FloatEq(0)),  //
                                    BonusRateIs(FloatEq(4.6), FloatEq(0)),  //
                                    // VS
                                    BonusRateIs(FloatEq(5), FloatEq(0)),  //
                                    BonusRateIs(FloatEq(5), FloatEq(0)),  //
                                    BonusRateIs(FloatEq(5), FloatEq(0)),  //
                                    BonusRateIs(FloatEq(5), FloatEq(0)),  //
                                    BonusRateIs(FloatEq(5), FloatEq(0)),  //
                                    BonusRateIs(FloatEq(5), FloatEq(0))   //
                                    ));
}

TEST(ProfileTest, CheckTestProfileUnitBonus) {
  Profile profile(TestProfile());
  std::vector<BonusRate> unit_bonus = {profile.unit_bonus().begin(), profile.unit_bonus().end()};
  EXPECT_THAT(unit_bonus, ElementsAre(_,                                      // Offset
                                      BonusRateIs(FloatEq(15), FloatEq(30)),  // LN
                                      BonusRateIs(FloatEq(10), FloatEq(20)),  // MMJ
                                      BonusRateIs(FloatEq(10), FloatEq(20)),  // VBS
                                      BonusRateIs(FloatEq(10), FloatEq(20)),  // WXS
                                      BonusRateIs(FloatEq(10), FloatEq(20)),  // 25ji
                                      BonusRateIs(FloatEq(15), FloatEq(30))   // VS
                                      ));
}

TEST(ProfileTest, CheckTestProfileFixtureBonus) {
  Profile profile(TestProfile());
  std::vector<float> fixture_bonus = {profile.mysekai_fixture_bonus().begin(),
                                      profile.mysekai_fixture_bonus().end()};
  EXPECT_EQ(fixture_bonus.size(), kCharacterArraySize);
  EXPECT_EQ(fixture_bonus[0], 0);
  EXPECT_EQ(fixture_bonus[1], 1 + 3 + 6);
  EXPECT_EQ(fixture_bonus[2], 6);
  for (int i = 3; i < kCharacterArraySize; ++i) {
    EXPECT_EQ(fixture_bonus[i], 0) << i;
  }
}

TEST(ProfileTest, CheckTestProfileGateBonus) {
  Profile profile(TestProfile());
  std::vector<float> gate_bonus = {profile.mysekai_gate_bonus().begin(),
                                   profile.mysekai_gate_bonus().end()};
  EXPECT_THAT(gate_bonus,
              ElementsAre(_,             // Padding
                          FloatEq(0.5),  // LN
                          FloatEq(1.0),  // MMJ
                          FloatEq(2.0),  // VBS
                          FloatEq(3.0),  // WxS
                          FloatEq(4.0),  // Niigo
                          FloatEq(4.0)   // VSQkj:w
                                         //
                          ));
}

TEST(ProfileTest, LoadCardsFromCsv) {
  Profile profile(TestProfile());
  profile.LoadCardsFromCsv(SekaiRunfilesRoot() / "data/profile/cards.csv");
  EXPECT_THAT(profile.PrimaryCardPool(), IsEmpty());
}

TEST(ProfileTest, LoadCardsFromCsvV2Format) {
  Profile profile(TestProfile());
  profile.LoadCardsFromCsv(SekaiRunfilesRoot() / "data/profile/cards_v2.csv");
  EXPECT_THAT(profile.PrimaryCardPool(), SizeIs(2));
  auto expected_card1_state = ParseTextProto<CardState>(
      R"pb(
        level: 20
        master_rank: 1
        skill_level: 2
        card_episodes_read: 1
        card_episodes_read: 2
        canvas_crafted: false
      )pb");
  EXPECT_THAT(profile.GetCard(1), Pointee(CardStateThat(ProtoEquals(expected_card1_state))));
  EXPECT_THAT(profile.GetCard(2), Eq(nullptr));
  auto expected_card3_state = ParseTextProto<CardState>(R"pb(
    level: 50
    master_rank: 2
    skill_level: 3
    card_episodes_read: 5
    card_episodes_read: 6
    special_training: true
    canvas_crafted: true
  )pb");
  EXPECT_THAT(profile.GetCard(3), Pointee(CardStateThat(ProtoEquals(expected_card3_state))));
  EXPECT_THAT(profile.GetCard(4), Eq(nullptr));
}

TEST(ProfileTest, ApplySkillCharacterRanks) {
  auto profile_proto = ParseTextProto<ProfileProto>(R"pb(
    cards: {
      key: 950
      value: {level: 60 master_rank: 5 skill_level: 4 special_training: true}
    }
  )pb");
  profile_proto.MergeFrom(TestProfile());
  profile_proto.set_character_ranks(25, 120);
  Profile profile(profile_proto);
  const Card* card = profile.GetSecondaryCard(950);
  ASSERT_NE(card, nullptr);

  std::vector<const Card*> cards = {card};
  UnitCount unit_count{cards};
  EXPECT_FLOAT_EQ(card->SkillValue(0, unit_count), 160);
}

TEST(ProfileTest, NoSecondaryCardIfNoSpecialTraining) {
  auto profile_proto = ParseTextProto<ProfileProto>(R"pb(
    cards: {
      key: 950
      value: {level: 60 master_rank: 5 skill_level: 4 special_training: false}
    }
  )pb");
  profile_proto.MergeFrom(TestProfile());
  profile_proto.set_character_ranks(25, 120);
  Profile profile(profile_proto);
  const Card* card = profile.GetSecondaryCard(950);
  EXPECT_EQ(card, nullptr);
}

TEST(ProfileTest, SortedSupport) {
  auto event_id = ParseTextProto<EventId>(R"pb(event_id: 112 chapter_id: 1)pb");
  EventBonus bonus(event_id);

  auto profile_proto = ParseTextProto<ProfileProto>(R"pb(
    cards: {
      key: 71
      value: {level: 1 master_rank: 0 skill_level: 1}
    }
    cards: {
      key: 74
      value: {level: 1 master_rank: 0 skill_level: 1}
    }
    cards: {
      key: 116
      value: {level: 1 master_rank: 0 skill_level: 1}
    }
    cards: {
      key: 198
      value: {level: 1 master_rank: 0 skill_level: 1}
    }
  )pb");
  profile_proto.MergeFrom(TestProfile());
  Profile profile{profile_proto};
  profile.ApplyEventBonus(bonus);

  std::vector<int> sorted_support_ids;
  for (const Card* card : profile.sorted_support()) {
    sorted_support_ids.push_back(card->card_id());
  }
  EXPECT_THAT(sorted_support_ids, ElementsAre(116, 71, 74));
}

TEST(ProfileTest, MinProfileSuccess) { EXPECT_EQ(Profile::Min().bonus_power(), 0); }

TEST(ProfileTest, MaxProfileSuccess) { EXPECT_EQ(Profile::Max().bonus_power(), kMaxTitleBonus); }

}  // namespace
}  // namespace sekai
