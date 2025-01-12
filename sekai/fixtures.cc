#include "sekai/fixtures.h"

#include <span>
#include <vector>

#include "sekai/db/master_db.h"
#include "sekai/db/proto/all.h"

namespace sekai {

using ::sekai::db::MasterDb;

std::span<const db::MySekaiFixture* const> FixturesWithCharBonuses() {
  static const auto* const kFixtures = [] {
    auto* fixtures = new std::vector<const db::MySekaiFixture*>;
    for (const db::MySekaiFixture& fixture : MasterDb::GetAll<db::MySekaiFixture>()) {
      if (!fixture.has_mysekai_fixture_game_character_group_performance_bonus_id()) continue;
      fixtures->push_back(&fixture);
    }
    return fixtures;
  }();
  return *kFixtures;
}

}  // namespace sekai
