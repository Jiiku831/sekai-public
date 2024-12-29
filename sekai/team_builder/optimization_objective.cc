#include "sekai/team_builder/optimization_objective.h"

#include <limits>

#include "absl/base/no_destructor.h"
#include "absl/log/absl_check.h"
#include "sekai/bitset.h"
#include "sekai/estimator_base.h"
#include "sekai/event_bonus.h"
#include "sekai/profile.h"
#include "sekai/team.h"

namespace sekai {

ObjectiveFunction OptimizePoints::GetObjectiveFunction() const {
  return [](const Team& team, const Profile& profile, const EventBonus& event_bonus,
            const EstimatorBase& estimator, Character lead_chars) {
    return estimator.MaxExpectedValue(profile, event_bonus, team, lead_chars);
  };
}

const OptimizationObjective& OptimizePoints::Get() {
  static const absl::NoDestructor<OptimizePoints> kObj;
  return *kObj;
}

ObjectiveFunction OptimizePower::GetObjectiveFunction() const {
  return [](const Team& team, const Profile& profile, const EventBonus& event_bonus,
            const EstimatorBase& estimator,
            Character lead_chars) { return static_cast<double>(team.Power(profile)); };
}

const OptimizationObjective& OptimizePower::Get() {
  static const absl::NoDestructor<OptimizePower> kObj;
  return *kObj;
}

ObjectiveFunction OptimizeBonus::GetObjectiveFunction() const {
  return [](const Team& team, const Profile& profile, const EventBonus& event_bonus,
            const EstimatorBase& estimator,
            Character lead_chars) { return static_cast<double>(team.EventBonus(event_bonus)); };
}

const OptimizationObjective& OptimizeBonus::Get() {
  static const absl::NoDestructor<OptimizeBonus> kObj;
  return *kObj;
}

ObjectiveFunction OptimizeSkill::GetObjectiveFunction() const {
  return [](const Team& team, const Profile& profile, const EventBonus& event_bonus,
            const EstimatorBase& estimator, Character lead_chars) {
    return static_cast<double>(team.ConstrainedMaxSkillValue(lead_chars).skill_value);
  };
}

const OptimizationObjective& OptimizeSkill::Get() {
  static const absl::NoDestructor<OptimizeSkill> kObj;
  return *kObj;
}

ObjectiveFunction GetObjectiveFunction(const OptimizationObjective& obj) {
  return obj.GetObjectiveFunction();
}

}  // namespace sekai
