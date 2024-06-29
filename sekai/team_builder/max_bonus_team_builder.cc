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
#include "sekai/team_builder/team_builder.h"

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

std::vector<Team> RecommendTeamsImplNoConstraints(
    std::span<const Card* const> pool, const Profile& profile, const EventBonus& event_bonus,
    const Estimator& estimator, std::optional<absl::Time> deadline, TeamBuilder::Stats& stats) {
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
      ++stats.teams_considered;
      ++stats.teams_evaluated;
      return {Team{candidate_cards}};
    }
  }
  return {};
}

std::vector<Team> RecommendTeamsImplWithConstraints(
    std::span<const Card* const> pool, const Profile& profile, const EventBonus& event_bonus,
    const Estimator& estimator, std::optional<absl::Time> deadline, const Constraints& constraints,
    TeamBuilder::Stats& stats) {
  std::array<std::vector<const Card*>, kCharacterArraySize> char_pools =
      PartitionCardPoolByCharacters(SortCardsByBonusToVector(pool));
  std::optional<Team> best_team;
  std::vector<int> sorted_char_ids = SortCharactersByMaxEventBonus(event_bonus);
  // TODO: this doesnt work for world link
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
        Character lead_chars = constraints.GetCharactersEligibleForLead(chars_present);

        std::array<std::span<const Card* const>, 5> current_pool;
        ABSL_CHECK_EQ(static_cast<int>(candidate_chars.size()), 5);
        for (int i = 0; i < static_cast<int>(candidate_chars.size()); ++i) {
          current_pool[i] = char_pools[candidate_chars[i]];
          if (current_pool[i].empty()) {
            return true;
          }
        }

        // Go down the list of candidate cards for each possible lead until
        // we find a match.
        for (int i = 0; i < static_cast<int>(candidate_chars.size()); ++i) {
          int candidate_char = candidate_chars[i];
          if (!lead_chars.test(candidate_char)) {
            continue;
          }
          std::array<const Card*, 5> candidate_cards;
          for (int j = 0; j < static_cast<int>(candidate_chars.size()); ++j) {
            candidate_cards[j] = current_pool[j][0];
          }
          std::span<const Card* const> candidate_pool = current_pool[i];
          for (int j = 0; j < static_cast<int>(candidate_pool.size()); ++j) {
            candidate_cards[i] = candidate_pool[j];

            Team candidate_team{candidate_cards};
            ++stats.teams_considered;
            ++stats.teams_evaluated;
            bool constraints_satisfied = !constraints.HasLeadSkillConstraint();
            if (!constraints_satisfied) {
              Team::SkillValueDetail skill_value =
                  candidate_team.ConstrainedMaxSkillValue(lead_chars);
              constraints_satisfied =
                  constraints.LeadSkillSatisfiesConstraint(skill_value.lead_skill);
            }

            if (constraints_satisfied) {
              if (!best_team.has_value() || best_team->EventBonus() < candidate_team.EventBonus()) {
                best_team = candidate_team;
              }
              break;
            }
          }
        }

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
    return RecommendTeamsImplNoConstraints(pool, profile, event_bonus, estimator, deadline, stats_);
  }
  return RecommendTeamsImplWithConstraints(pool, profile, event_bonus, estimator, deadline,
                                           constraints_, stats_);
}

}  // namespace sekai
