#include "sekai/team_builder/pool_utils.h"

#include <array>
#include <span>
#include <utility>

#include "sekai/array_size.h"
#include "sekai/card.h"

namespace sekai {

std::array<std::vector<const Card*>, kCharacterArraySize> PartitionCardPoolByCharacters(
    std::span<const Card* const> pool) {
  std::array<std::vector<const Card*>, kCharacterArraySize> char_pools;
  for (const Card* card : pool) {
    char_pools[card->character_id()].push_back(card);
  }
  return char_pools;
}

std::vector<int> SortCharactersByMaxEventBonus(const EventBonus& event_bonus) {
  std::multimap<std::pair<float, int>, int, std::greater<>> sorted_ids;
  for (int char_id : UniqueCharacterIds()) {
    sorted_ids.emplace(std::make_pair(event_bonus.MaxDeckBonusForChar(char_id), -char_id), char_id);
  }
  std::vector<int> ids;
  for (const auto& [unused, id] : sorted_ids) {
    ids.push_back(id);
  }
  return ids;
}

}  // namespace sekai
