#include "sekai/team_builder/optimization_objective.h"

#include <limits>

#include "absl/log/absl_check.h"
#include "sekai/bitset.h"
#include "sekai/estimator.h"
#include "sekai/event_bonus.h"
#include "sekai/profile.h"
#include "sekai/team.h"

namespace sekai {

ObjectiveFunction GetObjectiveFunction(OptimizationObjective obj) {
  switch (obj) {
    using enum OptimizationObjective;
    case kOptimizePoints:
      return [](const Team& team, const Profile& profile, const EventBonus& event_bonus,
                const Estimator& estimator, Character lead_chars) {
        int power = team.Power(profile);
        float eb = team.EventBonus(event_bonus);
        int skill_value = team.ConstrainedMaxSkillValue(lead_chars).skill_value;
        return estimator.ExpectedEp(power, eb, skill_value, skill_value);
      };
    case kOptimizePower:
      return [](const Team& team, const Profile& profile, const EventBonus& event_bonus,
                const Estimator& estimator,
                Character lead_chars) { return static_cast<double>(team.Power(profile)); };
    case kOptimizeBonus:
      return [](const Team& team, const Profile& profile, const EventBonus& event_bonus,
                const Estimator& estimator,
                Character lead_chars) { return static_cast<double>(team.EventBonus(event_bonus)); };
    case kOptimizeSkill:
      return [](const Team& team, const Profile& profile, const EventBonus& event_bonus,
                const Estimator& estimator, Character lead_chars) {
        return static_cast<double>(team.ConstrainedMaxSkillValue(lead_chars).skill_value);
      };
    default:
      ABSL_CHECK(false) << "unhandled case";
      return [](const Team&, const Profile& profile, const EventBonus& event_bonus,
                const Estimator& estimator, Character lead_chars) { return 0.0; };
  }
}

}  // namespace sekai
