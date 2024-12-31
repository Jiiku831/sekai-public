#include "sekai/profile.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "absl/log/log.h"
#include "sekai/config.h"
#include "sekai/proto_util.h"
#include "sekai/unit_count.h"
#include "testing/util.h"

namespace sekai {
namespace {

using ::testing::_;
using ::testing::DescribeMatcher;
using ::testing::ElementsAre;
using ::testing::FloatEq;
using ::testing::IsEmpty;
using ::testing::ParseTextProto;

MATCHER_P2(BonusRateIs, rate, matching_rate,
           "rate that " + DescribeMatcher<float>(rate, negation) + (negation ? " or" : " and") +
               " matching rate that " + DescribeMatcher<float>(matching_rate, negation)) {
  return testing::ExplainMatchResult(rate, arg.rate, result_listener) &&
         testing::ExplainMatchResult(matching_rate, arg.matching_rate, result_listener);
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

TEST(ProfileTest, LoadCardsFromCsv) {
  Profile profile(TestProfile());
  profile.LoadCardsFromCsv(SekaiRunfilesRoot() / "data/profile/cards.csv");
  EXPECT_THAT(profile.PrimaryCardPool(), IsEmpty());
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

}  // namespace
}  // namespace sekai
