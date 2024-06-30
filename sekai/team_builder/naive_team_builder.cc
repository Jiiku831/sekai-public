#include "sekai/team_builder/naive_team_builder.h"

#include <limits>

#include "absl/log/absl_check.h"
#include "sekai/array_size.h"
#include "sekai/bitset.h"
#include "sekai/combinations.h"
#include "sekai/team.h"
#include "sekai/team_builder/constraints.h"
#include "sekai/team_builder/optimization_objective.h"

namespace sekai {

std::vector<Team> NaiveTeamBuilder::RecommendTeamsImpl(std::span<const Card* const> pool,
                                                       const Profile& profile,
                                                       const EventBonus& event_bonus,
                                                       const Estimator& estimator,
                                                       std::optional<absl::Time> deadline) {
  ObjectiveFunction objective = GetObjectiveFunction(obj_);
  std::optional<Team> best_team;
  double best_val = -std::numeric_limits<double>::infinity();
  Combinations<const Card*, 5>{
      pool,
      [&](std::span<const Card* const> candidate_cards) {
        ++stats_.teams_considered;
        Character chars_present;
        for (const Card* card : candidate_cards) {
          if (chars_present.test(card->character_id())) {
            return true;
          }
          chars_present.set(card->character_id());
        }
        if (!constraints_.CharacterSetSatisfiesConstraint(chars_present)) {
          return true;
        }
        Character lead_chars = constraints_.GetCharactersEligibleForLead(chars_present);

        Team candidate_team{candidate_cards};
        if (constraints_.HasLeadSkillConstraint()) {
          Team::SkillValueDetail skill_value = candidate_team.ConstrainedMaxSkillValue(lead_chars);
          if (!constraints_.LeadSkillSatisfiesConstraint(skill_value.lead_skill)) {
            return true;
          }
        }
        ++stats_.teams_evaluated;
        double candidate_val =
            objective(candidate_team, profile, event_bonus, estimator, lead_chars);
        if (candidate_val > best_val) {
          best_val = candidate_val;
          best_team = candidate_team;
        }
        return true;
      },
  }();
  if (best_team.has_value()) {
    return {*best_team};
  }
  return {};
}

}  // namespace sekai
