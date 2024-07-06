#include "sekai/team_builder/optimization_objective.h"

#include <limits>

#include "absl/log/absl_check.h"
#include "sekai/bitset.h"
#include "sekai/estimator_base.h"
#include "sekai/event_bonus.h"
#include "sekai/profile.h"
#include "sekai/team.h"

namespace sekai {

ObjectiveFunction GetObjectiveFunction(OptimizationObjective obj) {
  switch (obj) {
    using enum OptimizationObjective;
    case kOptimizePoints:
      return [](const Team& team, const Profile& profile, const EventBonus& event_bonus,
                const EstimatorBase& estimator, Character lead_chars) {
        return estimator.MaxExpectedValue(profile, event_bonus, team, lead_chars);
      };
    case kOptimizePower:
      return [](const Team& team, const Profile& profile, const EventBonus& event_bonus,
                const EstimatorBase& estimator,
                Character lead_chars) { return static_cast<double>(team.Power(profile)); };
    case kOptimizeBonus:
      return [](const Team& team, const Profile& profile, const EventBonus& event_bonus,
                const EstimatorBase& estimator,
                Character lead_chars) { return static_cast<double>(team.EventBonus(event_bonus)); };
    case kOptimizeSkill:
      return [](const Team& team, const Profile& profile, const EventBonus& event_bonus,
                const EstimatorBase& estimator, Character lead_chars) {
        return static_cast<double>(team.ConstrainedMaxSkillValue(lead_chars).skill_value);
      };
    default:
      ABSL_CHECK(false) << "unhandled case";
      return [](const Team&, const Profile& profile, const EventBonus& event_bonus,
                const EstimatorBase& estimator, Character lead_chars) { return 0.0; };
  }
}

}  // namespace sekai
