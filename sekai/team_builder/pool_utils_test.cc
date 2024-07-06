#include "sekai/team_builder/pool_utils.h"

#include <array>
#include <span>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "sekai/array_size.h"
#include "sekai/card.h"
#include "sekai/db/master_db.h"
#include "sekai/db/proto/all.h"
#include "sekai/event_bonus.h"
#include "sekai/profile.h"
#include "sekai/proto/event_id.pb.h"

namespace sekai {
namespace {

using ::sekai::db::MasterDb;
using ::testing::ElementsAre;
using ::testing::ElementsAreArray;

std::vector<const Card*> GetCardPtrs(std::span<const Card> cards) {
  std::vector<const Card*> card_ptrs;
  for (const Card& card : cards) {
    card_ptrs.push_back(&card);
  }
  return card_ptrs;
}

TEST(PartitionCardPoolByCharactersTest, SortsCardsByCharacter) {
  CardState state;
  state.set_master_rank(5);
  state.set_skill_level(4);

  Card card_ichika_1{MasterDb::FindFirst<db::Card>(4), state};
  Card card_ichika_2{MasterDb::FindFirst<db::Card>(157), state};
  Card card_ichika_3{MasterDb::FindFirst<db::Card>(279), state};
  Card card_saki_1{MasterDb::FindFirst<db::Card>(109), state};
  Card card_saki_2{MasterDb::FindFirst<db::Card>(209), state};
  Card card_saki_3{MasterDb::FindFirst<db::Card>(244), state};
  Card card_miku_1{MasterDb::FindFirst<db::Card>(88), state};
  Card card_miku_2{MasterDb::FindFirst<db::Card>(116), state};
  Card card_miku_3{MasterDb::FindFirst<db::Card>(126), state};

  std::vector<const Card*> pool = {
      &card_ichika_1, &card_ichika_2, &card_ichika_3, &card_saki_1, &card_saki_2,
      &card_saki_3,   &card_miku_1,   &card_miku_2,   &card_miku_3,
  };

  std::array<std::vector<const Card*>, kCharacterArraySize> sorted_pools =
      PartitionCardPoolByCharacters(pool);

  EXPECT_THAT(sorted_pools[1], ElementsAre(&card_ichika_1, &card_ichika_2, &card_ichika_3));
  EXPECT_THAT(sorted_pools[2], ElementsAre(&card_saki_1, &card_saki_2, &card_saki_3));
  EXPECT_THAT(sorted_pools[21], ElementsAre(&card_miku_1, &card_miku_2, &card_miku_3));
}

TEST(PartitionCardPoolByAttributeTest, SortsCardsByAttribute) {
  CardState state;
  state.set_master_rank(5);
  state.set_skill_level(4);

  Card card_ichika_1{MasterDb::FindFirst<db::Card>(4), state};    // Cute
  Card card_ichika_2{MasterDb::FindFirst<db::Card>(157), state};  // Happy
  Card card_ichika_3{MasterDb::FindFirst<db::Card>(279), state};  // Myst
  Card card_saki_1{MasterDb::FindFirst<db::Card>(109), state};    // Myst
  Card card_saki_2{MasterDb::FindFirst<db::Card>(209), state};    // Cool
  Card card_saki_3{MasterDb::FindFirst<db::Card>(244), state};    // Happy
  Card card_miku_1{MasterDb::FindFirst<db::Card>(88), state};     // Cute
  Card card_miku_2{MasterDb::FindFirst<db::Card>(116), state};    // Cool
  Card card_miku_3{MasterDb::FindFirst<db::Card>(126), state};    // Pure

  std::vector<const Card*> pool = {
      &card_ichika_1, &card_ichika_2, &card_ichika_3, &card_saki_1, &card_saki_2,
      &card_saki_3,   &card_miku_1,   &card_miku_2,   &card_miku_3,
  };

  std::array<std::vector<const Card*>, db::Attr_ARRAYSIZE> sorted_pools =
      PartitionCardPoolByAttribute(pool);

  EXPECT_THAT(sorted_pools[db::ATTR_PURE], ElementsAre(&card_miku_3));
  EXPECT_THAT(sorted_pools[db::ATTR_COOL], ElementsAre(&card_saki_2, &card_miku_2));
  EXPECT_THAT(sorted_pools[db::ATTR_MYST], ElementsAre(&card_ichika_3, &card_saki_1));
  EXPECT_THAT(sorted_pools[db::ATTR_HAPPY], ElementsAre(&card_ichika_2, &card_saki_3));
  EXPECT_THAT(sorted_pools[db::ATTR_CUTE], ElementsAre(&card_ichika_1, &card_miku_1));
}

TEST(SortCharactersByMaxEventBonusTest, SortCharacterByMaxEventBonus) {
  SimpleEventBonus simple_event_bonus = sekai::ParseTextProto<SimpleEventBonus>(R"pb(
    attr: ATTR_PURE
    chars {char_id: 17}
    chars {char_id: 18}
    chars {char_id: 19}
    chars {char_id: 20}
    chars {char_id: 21 unit: UNIT_25}
    chars {char_id: 22 unit: UNIT_25}
    chars {char_id: 23 unit: UNIT_25}
    chars {char_id: 24 unit: UNIT_25}
    chars {char_id: 25 unit: UNIT_25}
    chars {char_id: 26 unit: UNIT_25}
  )pb");
  auto event_bonus = EventBonus(simple_event_bonus);
  EXPECT_THAT(SortCharactersByMaxEventBonus(event_bonus),
              ElementsAre(17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
                          12, 13, 14, 15, 16));
}

TEST(SortCardsByBonusTest, SortsCardsByBonus) {
  CardState state;
  state.set_master_rank(5);
  state.set_skill_level(4);

  auto event_id = ParseTextProto<EventId>(R"pb(event_id: 134)pb");
  EventBonus event_bonus(event_id);

  std::vector<Card> cards = {
      {MasterDb::FindFirst<db::Card>(749), state},  // 0 - 41%
      {MasterDb::FindFirst<db::Card>(97), state},   // 1 - 40.5%
      {MasterDb::FindFirst<db::Card>(824), state},  // 2 - 41%
      {MasterDb::FindFirst<db::Card>(152), state},  // 3 - 65%
      {MasterDb::FindFirst<db::Card>(196), state},  // 4 - 65%
      {MasterDb::FindFirst<db::Card>(217), state},  // 5 - 65%
      {MasterDb::FindFirst<db::Card>(943), state},  // 6 - 85%
      {MasterDb::FindFirst<db::Card>(944), state},  // 7 - 85%
      {MasterDb::FindFirst<db::Card>(945), state},  // 8 - 85%
  };

  std::vector<int> expected_order = {6, 7, 8, 3, 4, 5, 0, 2, 1};

  std::vector<const Card*> expected_pool;
  for (int index : expected_order) {
    expected_pool.push_back(&cards[index]);
  }

  for (Card& card : cards) {
    card.ApplyEventBonus(event_bonus);
  }

  std::vector<const Card*> sorted_pool = SortCardsByBonus(GetCardPtrs(cards));

  EXPECT_THAT(sorted_pool, ElementsAreArray(expected_pool));
}

TEST(SortCardsByPowerTest, SortsCardsByPower) {
  CardState state;
  state.set_master_rank(0);
  state.set_skill_level(1);

  std::vector<Card> cards = {
      {MasterDb::FindFirst<db::Card>(2), state},  // 0 - 8288
      {MasterDb::FindFirst<db::Card>(3), state},  // 1 - 11340
      {MasterDb::FindFirst<db::Card>(1), state},  // 2 - 3780
      {MasterDb::FindFirst<db::Card>(2), state},  // 3 - 8288
  };

  std::vector<int> expected_order = {1, 0, 3, 2};

  std::vector<const Card*> expected_pool;
  for (int index : expected_order) {
    expected_pool.push_back(&cards[index]);
  }

  Profile profile;
  for (Card& card : cards) {
    card.ApplyProfilePowerBonus(profile);
  }

  std::vector<const Card*> sorted_pool = SortCardsByPower(GetCardPtrs(cards), /*attr_match=*/false,
                                                          /*primary_unit_match=*/false,
                                                          /*secondary_unit_match=*/false);

  EXPECT_THAT(sorted_pool, ElementsAreArray(expected_pool));
}

TEST(FilterCardsByUnitTest, FiltersVs) {
  CardState state;
  state.set_master_rank(0);
  state.set_skill_level(1);

  std::vector<Card> cards = {
      {MasterDb::FindFirst<db::Card>(4), state},    // 0 - Ichika
      {MasterDb::FindFirst<db::Card>(88), state},   // 1 - Miku - MMJ
      {MasterDb::FindFirst<db::Card>(92), state},   // 2 - Rin - VBS
      {MasterDb::FindFirst<db::Card>(109), state},  // 3 - Saki
      {MasterDb::FindFirst<db::Card>(126), state},  // 4 - Miku - WXS
      {MasterDb::FindFirst<db::Card>(289), state},  // 5 - Miku - Subunitless
  };

  std::vector<int> expected_order = {1, 2, 4, 5};

  std::vector<const Card*> expected_pool;
  for (int index : expected_order) {
    expected_pool.push_back(&cards[index]);
  }

  std::vector<const Card*> filtered_pool = FilterCardsByUnit(db::UNIT_VS, GetCardPtrs(cards));
  EXPECT_THAT(filtered_pool, ElementsAreArray(expected_pool));
}

TEST(FilterCardsByUnitTest, FiltersNonVs1) {
  CardState state;
  state.set_master_rank(0);
  state.set_skill_level(1);

  std::vector<Card> cards = {
      {MasterDb::FindFirst<db::Card>(4), state},    // 0 - Ichika
      {MasterDb::FindFirst<db::Card>(88), state},   // 1 - Miku - MMJ
      {MasterDb::FindFirst<db::Card>(92), state},   // 2 - Rin - VBS
      {MasterDb::FindFirst<db::Card>(109), state},  // 3 - Saki
      {MasterDb::FindFirst<db::Card>(126), state},  // 4 - Miku - WXS
      {MasterDb::FindFirst<db::Card>(289), state},  // 5 - Miku - Subunitless
      {MasterDb::FindFirst<db::Card>(130), state},  // 6 - Minori
      {MasterDb::FindFirst<db::Card>(134), state},  // 7 - An
  };

  std::vector<int> expected_order = {1, 6};

  std::vector<const Card*> expected_pool;
  for (int index : expected_order) {
    expected_pool.push_back(&cards[index]);
  }

  std::vector<const Card*> filtered_pool = FilterCardsByUnit(db::UNIT_MMJ, GetCardPtrs(cards));
  EXPECT_THAT(filtered_pool, ElementsAreArray(expected_pool));
}

TEST(FilterCardsByUnitTest, FiltersNonVs2) {
  CardState state;
  state.set_master_rank(0);
  state.set_skill_level(1);

  std::vector<Card> cards = {
      {MasterDb::FindFirst<db::Card>(4), state},    // 0 - Ichika
      {MasterDb::FindFirst<db::Card>(88), state},   // 1 - Miku - MMJ
      {MasterDb::FindFirst<db::Card>(92), state},   // 2 - Rin - VBS
      {MasterDb::FindFirst<db::Card>(109), state},  // 3 - Saki
      {MasterDb::FindFirst<db::Card>(126), state},  // 4 - Miku - WXS
      {MasterDb::FindFirst<db::Card>(289), state},  // 5 - Miku - Subunitless
      {MasterDb::FindFirst<db::Card>(130), state},  // 6 - Minori
      {MasterDb::FindFirst<db::Card>(134), state},  // 7 - An
  };

  std::vector<int> expected_order = {0, 3};

  std::vector<const Card*> expected_pool;
  for (int index : expected_order) {
    expected_pool.push_back(&cards[index]);
  }

  std::vector<const Card*> filtered_pool = FilterCardsByUnit(db::UNIT_LN, GetCardPtrs(cards));
  EXPECT_THAT(filtered_pool, ElementsAreArray(expected_pool));
}

TEST(FilterCardsByAttrTest, FiltersByAttrCute) {
  CardState state;
  state.set_master_rank(0);
  state.set_skill_level(1);

  std::vector<Card> cards = {
      {MasterDb::FindFirst<db::Card>(4), state},    // 0 - Cute
      {MasterDb::FindFirst<db::Card>(88), state},   // 1 - Cute
      {MasterDb::FindFirst<db::Card>(92), state},   // 2 - Pure
      {MasterDb::FindFirst<db::Card>(109), state},  // 3 - Myst
      {MasterDb::FindFirst<db::Card>(126), state},  // 4 - Pure
      {MasterDb::FindFirst<db::Card>(289), state},  // 5 - Myst
      {MasterDb::FindFirst<db::Card>(130), state},  // 6 - Cute
      {MasterDb::FindFirst<db::Card>(134), state},  // 7 - Cool
  };

  std::vector<int> expected_order = {0, 1, 6};

  std::vector<const Card*> expected_pool;
  for (int index : expected_order) {
    expected_pool.push_back(&cards[index]);
  }

  std::vector<const Card*> filtered_pool = FilterCardsByAttr(db::ATTR_CUTE, GetCardPtrs(cards));
  EXPECT_THAT(filtered_pool, ElementsAreArray(expected_pool));
}

TEST(FilterCardsByAttrTest, FiltersByAttrPuree) {
  CardState state;
  state.set_master_rank(0);
  state.set_skill_level(1);

  std::vector<Card> cards = {
      {MasterDb::FindFirst<db::Card>(4), state},    // 0 - Cute
      {MasterDb::FindFirst<db::Card>(88), state},   // 1 - Cute
      {MasterDb::FindFirst<db::Card>(92), state},   // 2 - Pure
      {MasterDb::FindFirst<db::Card>(109), state},  // 3 - Myst
      {MasterDb::FindFirst<db::Card>(126), state},  // 4 - Pure
      {MasterDb::FindFirst<db::Card>(289), state},  // 5 - Myst
      {MasterDb::FindFirst<db::Card>(130), state},  // 6 - Cute
      {MasterDb::FindFirst<db::Card>(134), state},  // 7 - Cool
  };

  std::vector<int> expected_order = {2, 4};

  std::vector<const Card*> expected_pool;
  for (int index : expected_order) {
    expected_pool.push_back(&cards[index]);
  }

  std::vector<const Card*> filtered_pool = FilterCardsByAttr(db::ATTR_PURE, GetCardPtrs(cards));
  EXPECT_THAT(filtered_pool, ElementsAreArray(expected_pool));
}

TEST(GetSortedSupportPoolTest, FiltersIneligibleAndSortsBySupportBonus) {
  CardState state;
  state.set_master_rank(0);
  state.set_skill_level(1);

  std::vector<Card> cards = {
      {MasterDb::FindFirst<db::Card>(74), state},   // 0 - Ena 2* - 2%
      {MasterDb::FindFirst<db::Card>(69), state},   // 1 - Mafuyu 1* - 6%
      {MasterDb::FindFirst<db::Card>(71), state},   // 2 - Mafuyu 3* - 8%
      {MasterDb::FindFirst<db::Card>(73), state},   // 3 - Ena 1* - 1%
      {MasterDb::FindFirst<db::Card>(70), state},   // 4 - Mafuyu 2* - 7%
      {MasterDb::FindFirst<db::Card>(75), state},   // 5 - Ena 3* - 3%
      {MasterDb::FindFirst<db::Card>(198), state},  // 6 - LN Miku 4* - X
      {MasterDb::FindFirst<db::Card>(152), state},  // 7 - Ena 4* - 10%
      {MasterDb::FindFirst<db::Card>(114), state},  // 8 - Mafuyu 4* - 15%
      {MasterDb::FindFirst<db::Card>(116), state},  // 9 - 25 Miku 4* - 10%
  };

  EventId event_id;
  event_id.set_event_id(112);
  event_id.set_chapter_id(1);
  EventBonus event_bonus(event_id);
  for (Card& card : cards) {
    card.ApplyEventBonus(event_bonus);
  }

  std::vector<int> expected_order = {8, 7, 9, 2, 4, 1, 5, 0, 3};

  std::vector<const Card*> expected_pool;
  for (int index : expected_order) {
    expected_pool.push_back(&cards[index]);
  }

  std::vector<const Card*> filtered_pool = GetSortedSupportPool(GetCardPtrs(cards));
  EXPECT_THAT(filtered_pool, ElementsAreArray(expected_pool));
}

}  // namespace
}  // namespace sekai
