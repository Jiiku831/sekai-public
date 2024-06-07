#include "sekai/event_bonus.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "absl/log/log.h"
#include "sekai/proto/event_bonus.pb.h"
#include "sekai/proto/event_id.pb.h"
#include "testing/util.h"

namespace sekai {
namespace {

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
    chars { char_id: 2 }
    chars { char_id: 4 }
    chars { char_id: 18 }
    chars { char_id: 20 }
    chars { char_id: 25 unit: UNIT_LN }
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

TEST(EventBonusTest, WorldBloomEventFromSimpleEventBonus) {
  auto event_bonus = ParseTextProto<SimpleEventBonus>(R"pb(
    chars { char_id: 17 }
    chars { char_id: 18 }
    chars { char_id: 19 }
    chars { char_id: 20 }
    cards: 785
    cards: 786
    cards: 787
    cards: 788
  )pb");
  EventBonus bonus(event_bonus);
  EventBonusProto bonus_proto = bonus.ToProto();
  auto expected_proto =
      ParseTextProtoFile<EventBonusProto>("sekai/testdata/event_bonus_112.textproto");
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

TEST(SupportUnitEventBonusTest, WorldBloomEventFromSimpleEventBonus) {
  auto event_bonus = ParseTextProto<SimpleEventBonus>(R"pb(
    chars { char_id: 17 }
    chars { char_id: 18 }
    chars { char_id: 19 }
    chars { char_id: 20 }
    cards: 785
    cards: 786
    cards: 787
    cards: 788
    chapter_char_id: 18
  )pb");
  SupportUnitEventBonus bonus(event_bonus);
  EventBonusProto bonus_proto = bonus.ToProto();
  auto expected_proto =
      ParseTextProtoFile<EventBonusProto>("sekai/testdata/support_event_bonus_112_1.textproto");
  EXPECT_THAT(bonus_proto, ProtoEquals(expected_proto));
}

}  // namespace
}  // namespace sekai
