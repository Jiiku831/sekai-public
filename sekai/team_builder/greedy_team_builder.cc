#include "sekai/team_builder/greedy_team_builder.h"

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
#include "sekai/estimator_base.h"
#include "sekai/team.h"
#include "sekai/team_builder/optimization_objective.h"
#include "sekai/team_builder/pool_utils.h"
#include "sekai/team_builder/team_builder.h"

namespace sekai {
namespace {

std::vector<Team> RecommendTeamsImplNoConstraints(
    std::span<const Card* const> sorted_pool, std::span<const Card* const> support_pool,
    const Profile& profile, const EventBonus& event_bonus, const EstimatorBase& estimator,
    std::optional<absl::Time> deadline, TeamBuilder::Stats& stats) {
  std::vector<const Card*> candidate_cards;
  std::bitset<kCharacterArraySize> chars_present;
  Attr attrs_present;
  for (const Card* card : sorted_pool) {
    if (chars_present.test(card->character_id())) continue;
    candidate_cards.push_back(card);
    chars_present.set(card->character_id());
    attrs_present.set(card->db_attr());
    if (candidate_cards.size() == 5) {
      ++stats.teams_considered;
      ++stats.teams_evaluated;
      Team team{candidate_cards};
      if (!support_pool.empty()) {
        team.FillSupportCards(support_pool);
      }
      return {team};
    }
  }
  return {};
}

std::vector<Team> RecommendTeamsImplWithConstraints(
    std::span<const Card* const> sorted_pool, std::span<const Card* const> support_pool,
    const Profile& profile, const EventBonus& event_bonus, const EstimatorBase& estimator,
    std::optional<absl::Time> deadline, const Constraints& constraints,
    const OptimizationObjective& obj, TeamBuilder::Stats& stats) {
  ObjectiveFunction obj_fn = GetObjectiveFunction(obj);
  std::array<std::vector<const Card*>, kCharacterArraySize> char_pools =
      PartitionCardPoolByCharacters(sorted_pool);
  std::optional<Team> best_team;
  Combinations<int, 5>{
      UniqueCharacterIds(),
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
              if (!support_pool.empty()) {
                candidate_team.FillSupportCards(support_pool);
              }
              if (!best_team.has_value() ||
                  obj_fn(*best_team, profile, event_bonus, estimator, lead_chars) <
                      obj_fn(candidate_team, profile, event_bonus, estimator, lead_chars)) {
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

std::vector<Team> GreedyTeamBuilder::RecommendTeamsImpl(std::span<const Card* const> pool,
                                                        const Profile& profile,
                                                        const EventBonus& event_bonus,
                                                        const EstimatorBase& estimator,
                                                        std::optional<absl::Time> deadline) {
  if (constraints_.empty()) {
    return RecommendTeamsImplNoConstraints(pool, support_pool_, profile, event_bonus, estimator,
                                           deadline, stats_);
  }
  return RecommendTeamsImplWithConstraints(pool, support_pool_, profile, event_bonus, estimator,
                                           deadline, constraints_, obj_, stats_);
}

}  // namespace sekai
