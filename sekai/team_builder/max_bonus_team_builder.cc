#include "sekai/team_builder/max_bonus_team_builder.h"

#include <array>
#include <bitset>
#include <functional>
#include <map>
#include <vector>

#include "absl/log/absl_check.h"
#include "sekai/array_size.h"
#include "sekai/bitset.h"
#include "sekai/card.h"
#include "sekai/combinations.h"
#include "sekai/team.h"

namespace sekai {
namespace {

constexpr int kCharacterArraySize = 32;

std::multimap<float, const Card*, std::greater<>> SortCardsByBonus(
    std::span<const Card* const> pool) {
  std::multimap<float, const Card*, std::greater<>> sorted_pool;
  for (const Card* card : pool) {
    sorted_pool.emplace(card->event_bonus(), card);
  }
  return sorted_pool;
}

}  // namespace

std::vector<Team> MaxBonusTeamBuilder::RecommendTeams(std::span<const Card* const> pool,
                                                      const Profile& profile,
                                                      const EventBonus& event_bonus,
                                                      const Estimator& estimator,
                                                      std::optional<absl::Time> deadline) {
  ABSL_CHECK_LE(CharacterArraySize(), kCharacterArraySize);
  std::vector<const Card*> candidate_cards;
  std::multimap<float, const Card*, std::greater<>> sorted_cards = SortCardsByBonus(pool);
  std::bitset<kCharacterArraySize> chars_present;
  Attr attrs_present;
  for (const auto& [unused, card] : sorted_cards) {
    if (chars_present.test(card->character_id())) continue;
    if (event_bonus.has_diff_attr_bonus() && attrs_present.test(card->db_attr())) continue;
    candidate_cards.push_back(card);
    chars_present.set(card->character_id());
    attrs_present.set(card->db_attr());
    if (candidate_cards.size() == 5) {
      return {Team{candidate_cards}};
    }
  }
  return {};
}

}  // namespace sekai
