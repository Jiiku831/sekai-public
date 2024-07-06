#include "sekai/team_builder/pool_utils.h"

#include <array>
#include <span>
#include <utility>

#include "sekai/array_size.h"
#include "sekai/card.h"
#include "sekai/db/proto/enums.pb.h"

namespace sekai {
namespace {

struct GreaterCharByBonus {
  const EventBonus& event_bonus;
  bool operator()(const int lhs, const int rhs) {
    return event_bonus.MaxDeckBonusForChar(lhs) > event_bonus.MaxDeckBonusForChar(rhs);
  }
};

struct GreaterCardByBonus {
  bool operator()(const Card* lhs, const Card* rhs) {
    return lhs->event_bonus() > rhs->event_bonus();
  }
};

struct GreaterCardBySupportBonus {
  bool operator()(const Card* lhs, const Card* rhs) {
    return lhs->support_bonus() > rhs->support_bonus();
  }
};

struct GreaterCardByPower {
  bool attr_match;
  bool primary_unit_match;
  bool secondary_unit_match;

  bool operator()(const Card* lhs, const Card* rhs) {
    return lhs->precomputed_power(attr_match, primary_unit_match, secondary_unit_match) >
           rhs->precomputed_power(attr_match, primary_unit_match, secondary_unit_match);
  }
};

}  // namespace

std::array<std::vector<const Card*>, kCharacterArraySize> PartitionCardPoolByCharacters(
    std::span<const Card* const> pool) {
  std::array<std::vector<const Card*>, kCharacterArraySize> char_pools;
  for (const Card* card : pool) {
    char_pools[card->character_id()].push_back(card);
  }
  return char_pools;
}

std::array<std::vector<const Card*>, db::Attr_ARRAYSIZE> PartitionCardPoolByAttribute(
    std::span<const Card* const> pool) {
  std::array<std::vector<const Card*>, db::Attr_ARRAYSIZE> char_pools;
  for (const Card* card : pool) {
    char_pools[card->db_attr()].push_back(card);
  }
  return char_pools;
}

std::vector<int> SortCharactersByMaxEventBonus(const EventBonus& event_bonus) {
  std::vector<int> char_ids = UniqueCharacterIds();
  std::stable_sort(char_ids.begin(), char_ids.end(), GreaterCharByBonus{event_bonus});
  return char_ids;
}

std::vector<const Card*> SortCardsByBonus(std::span<const Card* const> pool) {
  std::vector<const Card*> new_pool = {pool.begin(), pool.end()};
  std::stable_sort(new_pool.begin(), new_pool.end(), GreaterCardByBonus());
  return new_pool;
}

std::vector<const Card*> SortCardsByPower(std::span<const Card* const> pool, bool attr_match,
                                          bool primary_unit_match, bool secondary_unit_match) {
  std::vector<const Card*> new_pool = {pool.begin(), pool.end()};
  std::stable_sort(new_pool.begin(), new_pool.end(),
                   GreaterCardByPower{
                       .attr_match = attr_match,
                       .primary_unit_match = primary_unit_match,
                       .secondary_unit_match = secondary_unit_match,
                   });
  return new_pool;
}

std::vector<const Card*> FilterCardsByUnit(db::Unit unit, std::span<const Card* const> pool) {
  std::vector<const Card*> new_pool;
  for (const Card* card : pool) {
    if (card->IsUnit(unit)) {
      new_pool.push_back(card);
    }
  }
  return new_pool;
}

std::vector<const Card*> FilterCardsByAttr(db::Attr attr, std::span<const Card* const> pool) {
  std::vector<const Card*> new_pool;
  for (const Card* card : pool) {
    if (card->db_attr() == attr) {
      new_pool.push_back(card);
    }
  }
  return new_pool;
}

std::vector<const Card*> GetSortedSupportPool(std::span<const Card* const> pool) {
  std::vector<const Card*> new_pool;
  new_pool.reserve(pool.size());
  for (const Card* card : pool) {
    if (card->support_bonus() > 0) {
      new_pool.push_back(card);
    }
  }
  std::stable_sort(new_pool.begin(), new_pool.end(), GreaterCardBySupportBonus{});
  return new_pool;
}

}  // namespace sekai
