#include "sekai/team_builder/naive_team_builder.h"

#include <limits>

#include "absl/log/absl_check.h"
#include "sekai/array_size.h"
#include "sekai/bitset.h"
#include "sekai/combinations.h"
#include "sekai/team.h"
#include "sekai/team_builder/constraints.h"

namespace sekai {
namespace {

absl::AnyInvocable<double(const Team&, const Profile&, const EventBonus&, const Estimator&,
                          const Constraints&) const>
ObjectiveFn(NaiveTeamBuilder::Objective obj) {
  switch (obj) {
    case NaiveTeamBuilder::Objective::kOptimizePoints:
      return [](const Team& team, const Profile& profile, const EventBonus& event_bonus,
                const Estimator& estimator, const Constraints& constraints) {
        int power = team.Power(profile);
        float eb = team.EventBonus(event_bonus);
        int skill_value = team.ConstrainedMaxSkillValue(constraints).skill_value;
        return estimator.ExpectedEp(power, eb, skill_value, skill_value);
      };
    case NaiveTeamBuilder::Objective::kOptimizePower:
      return [](const Team& team, const Profile& profile, const EventBonus& event_bonus,
                const Estimator& estimator, const Constraints& constraints) {
        return static_cast<double>(team.Power(profile));
      };
    case NaiveTeamBuilder::Objective::kOptimizeBonus:
      return [](const Team& team, const Profile& profile, const EventBonus& event_bonus,
                const Estimator& estimator, const Constraints& constraints) {
        return static_cast<double>(team.EventBonus(event_bonus));
      };
    case NaiveTeamBuilder::Objective::kOptimizeSkill:
      return [](const Team& team, const Profile& profile, const EventBonus& event_bonus,
                const Estimator& estimator, const Constraints& constraints) {
        return static_cast<double>(team.ConstrainedMaxSkillValue(constraints).skill_value);
      };
    default:
      ABSL_CHECK(false) << "unhandled case";
      return [](const Team&, const Profile& profile, const EventBonus& event_bonus,
                const Estimator& estimator, const Constraints& constraints) { return 0.0; };
  }
}

}  // namespace

std::vector<Team> NaiveTeamBuilder::RecommendTeamsImpl(std::span<const Card* const> pool,
                                                       const Profile& profile,
                                                       const EventBonus& event_bonus,
                                                       const Estimator& estimator,
                                                       std::optional<absl::Time> deadline) {
  absl::AnyInvocable<double(const Team&, const Profile&, const EventBonus&, const Estimator&,
                            const Constraints&) const>
      objective = ObjectiveFn(obj_);
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

        Team candidate_team{candidate_cards};
        if (constraints_.HasLeadSkillConstraint()) {
          Team::SkillValueDetail skill_value =
              candidate_team.ConstrainedMaxSkillValue(constraints_);
          if (!constraints_.LeadSkillSatisfiesConstraint(skill_value.lead_skill)) {
            return true;
          }
        }
        ++stats_.teams_evaluated;
        double candidate_val =
            objective(candidate_team, profile, event_bonus, estimator, constraints_);
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
