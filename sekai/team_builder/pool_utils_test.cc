#include "sekai/team_builder/pool_utils.h"

#include <array>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "gmock/gmock.h"
#include "sekai/array_size.h"
#include "sekai/db/master_db.h"
#include "sekai/db/proto/all.h"
#include "sekai/event_bonus.h"
#include "sekai/proto/event_bonus.pb.h"

namespace sekai {
namespace {

using ::sekai::db::MasterDb;
using ::testing::ElementsAre;

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

}  // namespace
}  // namespace sekai
