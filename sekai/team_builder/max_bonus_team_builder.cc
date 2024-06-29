#include "sekai/team_builder/max_bonus_team_builder.h"

#include <array>
#include <bitset>
#include <functional>
#include <limits>
#include <map>
#include <vector>

#include "absl/log/absl_check.h"
#include "sekai/array_size.h"
#include "sekai/bitset.h"
#include "sekai/card.h"
#include "sekai/combinations.h"
#include "sekai/team.h"
#include "sekai/team_builder/pool_utils.h"

namespace sekai {
namespace {

std::multimap<float, const Card*, std::greater<>> SortCardsByBonus(
    std::span<const Card* const> pool) {
  std::multimap<float, const Card*, std::greater<>> sorted_pool;
  for (const Card* card : pool) {
    sorted_pool.emplace(card->event_bonus(), card);
  }
  return sorted_pool;
}

std::vector<const Card*> SortCardsByBonusToVector(std::span<const Card* const> pool) {
  std::multimap<float, const Card*, std::greater<>> sorted_cards = SortCardsByBonus(pool);
  std::vector<const Card*> new_pool;
  for (const auto& [unused, card] : sorted_cards) {
    new_pool.push_back(card);
  }
  return new_pool;
}

std::vector<Team> RecommendTeamsImplNoConstraints(std::span<const Card* const> pool,
                                                  const Profile& profile,
                                                  const EventBonus& event_bonus,
                                                  const Estimator& estimator,
                                                  std::optional<absl::Time> deadline) {
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

std::vector<Team> RecommendTeamsImplWithConstraints(std::span<const Card* const> pool,
                                                    const Profile& profile,
                                                    const EventBonus& event_bonus,
                                                    const Estimator& estimator,
                                                    std::optional<absl::Time> deadline,
                                                    const Constraints& constraints) {
  std::array<std::vector<const Card*>, kCharacterArraySize> char_pools =
      PartitionCardPoolByCharacters(SortCardsByBonusToVector(pool));
  std::optional<Team> best_team;
  std::vector<int> sorted_char_ids = SortCharactersByMaxEventBonus(event_bonus);
  // TODO: add variant that search near the first
  Combinations<int, 5>{
      sorted_char_ids,
      [&](std::span<const int> candidate_chars) {
        Character chars_present;
        for (int char_id : candidate_chars) {
          chars_present.set(char_id);
        }
        if (!constraints.CharacterSetSatisfiesConstraint(chars_present)) {
          return true;
        }

        std::array<std::span<const Card* const>, 5> current_pool;
        ABSL_CHECK_EQ(static_cast<int>(candidate_chars.size()), 5);
        for (int i = 0; i < static_cast<int>(candidate_chars.size()); ++i) {
          current_pool[i] = char_pools[candidate_chars[i]];
          if (current_pool[i].empty()) {
            return true;
          }
        }

        std::array<int, 5> current_index = {0, 0, 0, 0, 0};
        bool at_end = false;
        do {
          int index_with_lowest_eb = -1;
          at_end = true;
          std::array<const Card*, 5> candidate_cards;
          float min_drop = std::numeric_limits<float>::infinity();
          // Fill in candidate cards and choose index to increment.
          for (int i = 0; i < static_cast<int>(current_pool.size()); ++i) {
            candidate_cards[i] = current_pool[i][current_index[i]];
            if (
                // If the current position can be incremented.
                current_index[i] + 1 < static_cast<int>(current_pool[i].size()) &&
                // Then update the index to increment if one of the following is true.
                (
                    // This is the first index that can be incremented.
                    index_with_lowest_eb < 0 ||
                    // Or, this is the index that will result in the lowest
                    // eb drop.
                    current_pool[i][current_index[i]]->event_bonus() -
                            current_pool[i][current_index[i] + 1]->event_bonus() <
                        min_drop)) {
              min_drop = current_pool[i][current_index[i]]->event_bonus() -
                         current_pool[i][current_index[i] + 1]->event_bonus();
              index_with_lowest_eb = i;
              at_end = false;
            }
          }

          Team candidate_team{candidate_cards};
          bool constraints_satisfied = !constraints.HasLeadSkillConstraint();
          if (!constraints_satisfied) {
            Team::SkillValueDetail skill_value =
                candidate_team.ConstrainedMaxSkillValue(constraints);
            constraints_satisfied =
                constraints.LeadSkillSatisfiesConstraint(skill_value.lead_skill);
          }

          if (constraints_satisfied) {
            if (!best_team.has_value() || best_team->EventBonus() < candidate_team.EventBonus()) {
              best_team = candidate_team;
            }
            break;
          }

          ++current_index[index_with_lowest_eb];
        } while (!at_end);

        return true;
      },
  }();
  if (best_team.has_value()) {
    return {*best_team};
  }
  return {};
}

}  // namespace

std::vector<Team> MaxBonusTeamBuilder::RecommendTeamsImpl(std::span<const Card* const> pool,
                                                          const Profile& profile,
                                                          const EventBonus& event_bonus,
                                                          const Estimator& estimator,
                                                          std::optional<absl::Time> deadline) {
  if (constraints_.empty()) {
    return RecommendTeamsImplNoConstraints(pool, profile, event_bonus, estimator, deadline);
  }
  return RecommendTeamsImplWithConstraints(pool, profile, event_bonus, estimator, deadline,
                                           constraints_);
}

}  // namespace sekai
