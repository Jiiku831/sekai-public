#include "sekai/event_bonus.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "absl/log/log.h"
#include "sekai/card.h"
#include "sekai/db/master_db.h"
#include "sekai/proto/card_state.pb.h"
#include "sekai/proto/event_bonus.pb.h"
#include "sekai/proto/event_id.pb.h"
#include "testing/util.h"

namespace sekai {
namespace {

using ::sekai::db::MasterDb;
using ::testing::ParseTextProto;
using ::testing::ParseTextProtoFile;
using ::testing::ProtoEquals;

TEST(EventBonusTest, RegularEvent) {
  auto event_id = ParseTextProto<EventId>(R"pb(event_id: 117)pb");
  EventBonus bonus(event_id);
  EventBonusProto bonus_proto = bonus.ToProto();
  auto expected_proto =
      ParseTextProtoFile<EventBonusProto>("sekai/testdata/event_bonus_117.textproto");
  EXPECT_THAT(bonus_proto, ProtoEquals(expected_proto));
}

TEST(EventBonusTest, RegularEventFromSimpleEventBonus) {
  auto event_bonus = ParseTextProto<SimpleEventBonus>(R"pb(
    attr: ATTR_MYST
    chars {char_id: 2}
    chars {char_id: 4}
    chars {char_id: 18}
    chars {char_id: 20}
    chars {char_id: 25 unit: UNIT_LN}
    cards: 842
    cards: 843
    cards: 844
  )pb");
  EventBonus bonus(event_bonus);
  EventBonusProto bonus_proto = bonus.ToProto();
  auto expected_proto =
      ParseTextProtoFile<EventBonusProto>("sekai/testdata/event_bonus_117.textproto");
  EXPECT_THAT(bonus_proto, ProtoEquals(expected_proto));
}

TEST(EventBonusTest, WorldBloomEvent) {
  auto event_id = ParseTextProto<EventId>(R"pb(event_id: 112 chapter_id: 1)pb");
  EventBonus bonus(event_id);
  EventBonusProto bonus_proto = bonus.ToProto();
  auto expected_proto =
      ParseTextProtoFile<EventBonusProto>("sekai/testdata/event_bonus_112.textproto");
  EXPECT_THAT(bonus_proto, ProtoEquals(expected_proto));
}

TEST(EventBonusTest, WorldBloomEventFromSimpleEventBonusVersion1) {
  auto event_bonus = ParseTextProto<SimpleEventBonus>(R"pb(
    chars {char_id: 17}
    chars {char_id: 18}
    chars {char_id: 19}
    chars {char_id: 20}
    cards: 785
    cards: 786
    cards: 787
    cards: 788
    chapter_char_id: 18
  )pb");
  EventBonus bonus(event_bonus, WORLD_BLOOM_VERSION_1);
  EventBonusProto bonus_proto = bonus.ToProto();
  auto expected_proto =
      ParseTextProtoFile<EventBonusProto>("sekai/testdata/event_bonus_112.textproto");
  EXPECT_THAT(bonus_proto, ProtoEquals(expected_proto));
}

TEST(EventBonusTest, WorldBloomEventSupportUnit) {
  auto event_id = ParseTextProto<EventId>(R"pb(event_id: 112 chapter_id: 1)pb");
  EventBonus bonus(event_id);
  ASSERT_NE(bonus.SupportUnitBonus(), nullptr);
  EventBonusProto bonus_proto = bonus.SupportUnitBonus()->ToProto();
  auto expected_proto =
      ParseTextProtoFile<EventBonusProto>("sekai/testdata/support_event_bonus_112_1.textproto");
  EXPECT_THAT(bonus_proto, ProtoEquals(expected_proto));
}

TEST(EventBonusTest, WorldBloomEventSupportUnitVersion1) {
  auto event_id = ParseTextProto<EventId>(R"pb(event_id: 112 chapter_id: 1)pb");
  EventBonus bonus(event_id, WORLD_BLOOM_VERSION_1);
  ASSERT_NE(bonus.SupportUnitBonus(), nullptr);
  EventBonusProto bonus_proto = bonus.SupportUnitBonus()->ToProto();
  auto expected_proto =
      ParseTextProtoFile<EventBonusProto>("sekai/testdata/support_event_bonus_112_1.textproto");
  EXPECT_THAT(bonus_proto, ProtoEquals(expected_proto));
}

TEST(EventBonusTest, WorldBloomEventSupportUnitVs) {
  auto event_id = ParseTextProto<EventId>(R"pb(event_id: 140 chapter_id: 1)pb");
  EventBonus bonus(event_id);
  ASSERT_NE(bonus.SupportUnitBonus(), nullptr);
  EventBonusProto bonus_proto = bonus.SupportUnitBonus()->ToProto();
  auto expected_proto =
      ParseTextProtoFile<EventBonusProto>("sekai/testdata/support_event_bonus_140_1.textproto");
  EXPECT_THAT(bonus_proto, ProtoEquals(expected_proto));
}

TEST(EventBonusTest, WorldBloomEventSupportUnitVsVersion1) {
  auto event_id = ParseTextProto<EventId>(R"pb(event_id: 140 chapter_id: 1)pb");
  EventBonus bonus(event_id, WORLD_BLOOM_VERSION_1);
  ASSERT_NE(bonus.SupportUnitBonus(), nullptr);
  EventBonusProto bonus_proto = bonus.SupportUnitBonus()->ToProto();
  auto expected_proto =
      ParseTextProtoFile<EventBonusProto>("sekai/testdata/support_event_bonus_140_1.textproto");
  EXPECT_THAT(bonus_proto, ProtoEquals(expected_proto));
}

TEST(EventBonusTest, WorldBloomEventSupportUnitFromSimpleEventBonusVersion1) {
  auto event_bonus = ParseTextProto<SimpleEventBonus>(R"pb(
    chars {char_id: 17}
    chars {char_id: 18}
    chars {char_id: 19}
    chars {char_id: 20}
    cards: 785
    cards: 786
    cards: 787
    cards: 788
    chapter_char_id: 18
  )pb");
  EventBonus bonus(event_bonus, WORLD_BLOOM_VERSION_1);
  ASSERT_NE(bonus.SupportUnitBonus(), nullptr);
  EventBonusProto bonus_proto = bonus.SupportUnitBonus()->ToProto();
  auto expected_proto =
      ParseTextProtoFile<EventBonusProto>("sekai/testdata/support_event_bonus_112_1.textproto");
  EXPECT_THAT(bonus_proto, ProtoEquals(expected_proto));
}

TEST(EventBonusTest, WorldBloomEventSupportUnitVsFromSimpleEventBonusVersion1) {
  auto event_bonus = ParseTextProto<SimpleEventBonus>(R"pb(
    chars {char_id: 21}
    chars {char_id: 22}
    chars {char_id: 23}
    chars {char_id: 24}
    chars {char_id: 25}
    chars {char_id: 26}
    cards: 979
    cards: 980
    cards: 981
    cards: 982
    cards: 983
    cards: 984
    chapter_char_id: 24
  )pb");
  EventBonus bonus(event_bonus, WORLD_BLOOM_VERSION_1);
  ASSERT_NE(bonus.SupportUnitBonus(), nullptr);
  EventBonusProto bonus_proto = bonus.SupportUnitBonus()->ToProto();
  auto expected_proto =
      ParseTextProtoFile<EventBonusProto>("sekai/testdata/support_event_bonus_140_1.textproto");
  EXPECT_THAT(bonus_proto, ProtoEquals(expected_proto));
}

TEST(EventBonusTest, WorldBloomEventSupportUnitVsFromSimpleEventBonusVersion2) {
  auto event_bonus = ParseTextProto<SimpleEventBonus>(R"pb(
    chars {char_id: 21}
    chars {char_id: 22}
    chars {char_id: 23}
    chars {char_id: 24}
    chars {char_id: 25}
    chars {char_id: 26}
    cards: 979
    cards: 980
    cards: 981
    cards: 982
    cards: 983
    cards: 984
    chapter_char_id: 24
  )pb");
  EventBonus bonus(event_bonus, WORLD_BLOOM_VERSION_2);
  ASSERT_NE(bonus.SupportUnitBonus(), nullptr);
  EventBonusProto bonus_proto = bonus.SupportUnitBonus()->ToProto();
  auto expected_proto =
      ParseTextProtoFile<EventBonusProto>("sekai/testdata/support_event_bonus_vs_v2.textproto");
  EXPECT_THAT(bonus_proto, ProtoEquals(expected_proto));
}

TEST(SupportUnitEventBonusTest, WorldBloomEvent) {
  auto event_id = ParseTextProto<EventId>(R"pb(event_id: 112 chapter_id: 1)pb");
  SupportUnitEventBonus bonus(event_id);
  EventBonusProto bonus_proto = bonus.ToProto();
  auto expected_proto =
      ParseTextProtoFile<EventBonusProto>("sekai/testdata/support_event_bonus_112_1.textproto");
  EXPECT_THAT(bonus_proto, ProtoEquals(expected_proto));
}

TEST(SupportUnitEventBonusTest, WorldBloomEventFromSimpleEventBonusVersion1) {
  auto event_bonus = ParseTextProto<SimpleEventBonus>(R"pb(
    chars {char_id: 17}
    chars {char_id: 18}
    chars {char_id: 19}
    chars {char_id: 20}
    cards: 785
    cards: 786
    cards: 787
    cards: 788
    chapter_char_id: 18
  )pb");
  SupportUnitEventBonus bonus(event_bonus, WORLD_BLOOM_VERSION_1);
  EventBonusProto bonus_proto = bonus.ToProto();
  auto expected_proto =
      ParseTextProtoFile<EventBonusProto>("sekai/testdata/support_event_bonus_112_1.textproto");
  EXPECT_THAT(bonus_proto, ProtoEquals(expected_proto));
}

TEST(SupportUnitEventBonusTest, NonChapterUnitBonus) {
  auto event_bonus = ParseTextProto<SimpleEventBonus>(R"pb(
    chars {char_id: 17}
    chars {char_id: 18}
    chars {char_id: 19}
    chars {char_id: 20}
    cards: 785
    cards: 786
    cards: 787
    cards: 788
    chapter_char_id: 18
  )pb");
  EventBonus bonus(event_bonus, WORLD_BLOOM_VERSION_1);

  CardState card_state;
  Card card_miku_ln{MasterDb::FindFirst<db::Card>(198), card_state};
  Card card_miku_25{MasterDb::FindFirst<db::Card>(116), card_state};
  Card card_miku_vs{MasterDb::FindFirst<db::Card>(81), card_state};
  card_miku_ln.ApplyEventBonus(bonus);
  card_miku_25.ApplyEventBonus(bonus);
  card_miku_vs.ApplyEventBonus(bonus);

  EXPECT_FLOAT_EQ(card_miku_ln.support_bonus(), 0);
  EXPECT_FLOAT_EQ(card_miku_25.support_bonus(), 10);
  EXPECT_FLOAT_EQ(card_miku_vs.support_bonus(), 0);
}

TEST(SupportUnitEventBonusTest, NonChapterUnitBonus2) {
  auto event_id = ParseTextProto<EventId>(R"pb(event_id: 112 chapter_id: 1)pb");
  EventBonus bonus(event_id);

  CardState card_state;
  Card card_miku_ln{MasterDb::FindFirst<db::Card>(198), card_state};
  Card card_miku_25{MasterDb::FindFirst<db::Card>(116), card_state};
  Card card_miku_vs{MasterDb::FindFirst<db::Card>(81), card_state};
  card_miku_ln.ApplyEventBonus(bonus);
  card_miku_25.ApplyEventBonus(bonus);
  card_miku_vs.ApplyEventBonus(bonus);

  EXPECT_FLOAT_EQ(card_miku_ln.support_bonus(), 0);
  EXPECT_FLOAT_EQ(card_miku_25.support_bonus(), 10);
  EXPECT_FLOAT_EQ(card_miku_vs.support_bonus(), 0);
}

TEST(SupportUnitEventBonusTest, VsUnitBonus) {
  auto event_bonus = ParseTextProto<SimpleEventBonus>(R"pb(
    chars {char_id: 17}
    chars {char_id: 18}
    chars {char_id: 19}
    chars {char_id: 20}
    cards: 785
    cards: 786
    cards: 787
    cards: 788
    chapter_char_id: 21
  )pb");
  EventBonus bonus(event_bonus, WORLD_BLOOM_VERSION_1);

  CardState card_state;
  Card card_miku_ln{MasterDb::FindFirst<db::Card>(198), card_state};
  Card card_miku_25{MasterDb::FindFirst<db::Card>(116), card_state};
  Card card_miku_vs{MasterDb::FindFirst<db::Card>(509), card_state};
  Card card_meiko_wxs{MasterDb::FindFirst<db::Card>(146), card_state};
  Card card_meiko_vs{MasterDb::FindFirst<db::Card>(760), card_state};
  Card card_saki{MasterDb::FindFirst<db::Card>(109), card_state};
  card_miku_ln.ApplyEventBonus(bonus);
  card_miku_25.ApplyEventBonus(bonus);
  card_miku_vs.ApplyEventBonus(bonus);
  card_meiko_wxs.ApplyEventBonus(bonus);
  card_meiko_vs.ApplyEventBonus(bonus);
  card_saki.ApplyEventBonus(bonus);

  EXPECT_FLOAT_EQ(card_miku_ln.support_bonus(), 15);
  EXPECT_FLOAT_EQ(card_miku_25.support_bonus(), 15);
  EXPECT_FLOAT_EQ(card_miku_vs.support_bonus(), 15);
  EXPECT_FLOAT_EQ(card_meiko_wxs.support_bonus(), 10);
  EXPECT_FLOAT_EQ(card_meiko_vs.support_bonus(), 10);
  EXPECT_FLOAT_EQ(card_saki.support_bonus(), 0);
}

TEST(SupportUnitEventBonusTest, WorldBloomCardBonus) {
  auto event_bonus = ParseTextProto<SimpleEventBonus>(R"pb(
    chars {char_id: 17}
    chars {char_id: 18}
    chars {char_id: 19}
    chars {char_id: 20}
    cards: 785
    cards: 786
    cards: 787
    cards: 788
    chapter_char_id: 18
  )pb");
  EventBonus bonus(event_bonus, WORLD_BLOOM_VERSION_2);

  CardState card_state;
  Card card_mafuyu{MasterDb::FindFirst<db::Card>(786), card_state};
  Card card_miku_25{MasterDb::FindFirst<db::Card>(116), card_state};
  Card card_ena{MasterDb::FindFirst<db::Card>(787), card_state};
  card_mafuyu.ApplyEventBonus(bonus);
  card_miku_25.ApplyEventBonus(bonus);
  card_ena.ApplyEventBonus(bonus);

  EXPECT_FLOAT_EQ(card_mafuyu.support_bonus(), 20 + 5 + 7.5);
  EXPECT_FLOAT_EQ(card_miku_25.support_bonus(), 7.5);
  EXPECT_FLOAT_EQ(card_ena.support_bonus(), 7.5);
}

}  // namespace
}  // namespace sekai
