#pragma once

#include <optional>
#include <span>
#include <vector>

#include "absl/time/time.h"
#include "sekai/card.h"
#include "sekai/estimator.h"
#include "sekai/event_bonus.h"
#include "sekai/profile.h"
#include "sekai/team.h"
#include "sekai/team_builder/optimization_objective.h"
#include "sekai/team_builder/team_builder_base.h"

namespace sekai {

class NaiveTeamBuilder : public TeamBuilderBase {
 public:
  explicit NaiveTeamBuilder(OptimizationObjective obj = OptimizationObjective::kOptimizePoints)
      : obj_(obj) {}

  std::vector<Team> RecommendTeamsImpl(std::span<const Card* const> pool, const Profile& profile,
                                       const EventBonus& event_bonus, const Estimator& estimator,
                                       std::optional<absl::Time> deadline = std::nullopt) override;

 protected:
  OptimizationObjective obj_;
};

class NaivePowerTeamBuilder : public NaiveTeamBuilder {
 public:
  explicit NaivePowerTeamBuilder() : NaiveTeamBuilder(OptimizationObjective::kOptimizePower) {}
};

class NaiveBonusTeamBuilder : public NaiveTeamBuilder {
 public:
  explicit NaiveBonusTeamBuilder() : NaiveTeamBuilder(OptimizationObjective::kOptimizeBonus) {}
};

class NaiveSkillTeamBuilder : public NaiveTeamBuilder {
 public:
  explicit NaiveSkillTeamBuilder() : NaiveTeamBuilder(OptimizationObjective::kOptimizeSkill) {}
};

}  // namespace sekai
