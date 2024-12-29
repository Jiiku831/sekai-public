#pragma once

#include "absl/functional/any_invocable.h"
#include "sekai/bitset.h"
#include "sekai/card.h"
#include "sekai/estimator_base.h"
#include "sekai/event_bonus.h"
#include "sekai/profile.h"
#include "sekai/team.h"

namespace sekai {

using ObjectiveFunction = absl::AnyInvocable<double(const Team&, const Profile&, const EventBonus&,
                                                    const EstimatorBase&, Character) const>;

class OptimizationObjective {
 public:
  ~OptimizationObjective() = default;

  virtual ObjectiveFunction GetObjectiveFunction() const = 0;
};

class OptimizePoints : public OptimizationObjective {
 public:
  OptimizePoints() = default;

  ObjectiveFunction GetObjectiveFunction() const override;

  static const OptimizationObjective& Get();
};

class OptimizePower : public OptimizationObjective {
 public:
  OptimizePower() = default;

  ObjectiveFunction GetObjectiveFunction() const override;

  static const OptimizationObjective& Get();
};

class OptimizeBonus : public OptimizationObjective {
 public:
  OptimizeBonus() = default;

  ObjectiveFunction GetObjectiveFunction() const override;

  static const OptimizationObjective& Get();
};

class OptimizeSkill : public OptimizationObjective {
 public:
  OptimizeSkill() = default;

  ObjectiveFunction GetObjectiveFunction() const override;

  static const OptimizationObjective& Get();
};

ObjectiveFunction GetObjectiveFunction(const OptimizationObjective& obj);

}  // namespace sekai
