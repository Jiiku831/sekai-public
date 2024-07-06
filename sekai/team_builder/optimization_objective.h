#pragma once

#include "absl/functional/any_invocable.h"
#include "sekai/bitset.h"
#include "sekai/card.h"
#include "sekai/estimator_base.h"
#include "sekai/event_bonus.h"
#include "sekai/profile.h"
#include "sekai/team.h"

namespace sekai {

enum class OptimizationObjective {
  kOptimizePoints,
  kOptimizePower,
  kOptimizeBonus,
  kOptimizeSkill,
};

using ObjectiveFunction = absl::AnyInvocable<double(const Team&, const Profile&, const EventBonus&,
                                                    const EstimatorBase&, Character) const>;

ObjectiveFunction GetObjectiveFunction(OptimizationObjective obj);

}  // namespace sekai
