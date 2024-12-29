#pragma once

#include <optional>
#include <span>
#include <vector>

#include "absl/time/time.h"
#include "sekai/card.h"
#include "sekai/estimator_base.h"
#include "sekai/event_bonus.h"
#include "sekai/profile.h"
#include "sekai/team.h"
#include "sekai/team_builder/optimization_objective.h"
#include "sekai/team_builder/team_builder_base.h"

namespace sekai {

class NaiveTeamBuilder : public TeamBuilderBase {
 public:
  explicit NaiveTeamBuilder(const OptimizationObjective& obj = OptimizePoints::Get()) : obj_(obj) {}

 protected:
  const OptimizationObjective& obj_;

  std::vector<Team> RecommendTeamsImpl(std::span<const Card* const> pool, const Profile& profile,
                                       const EventBonus& event_bonus,
                                       const EstimatorBase& estimator,
                                       std::optional<absl::Time> deadline = std::nullopt) override;
};

class NaivePowerTeamBuilder : public NaiveTeamBuilder {
 public:
  explicit NaivePowerTeamBuilder() : NaiveTeamBuilder(OptimizePower::Get()) {}
};

class NaiveBonusTeamBuilder : public NaiveTeamBuilder {
 public:
  explicit NaiveBonusTeamBuilder() : NaiveTeamBuilder(OptimizeBonus::Get()) {}
};

class NaiveSkillTeamBuilder : public NaiveTeamBuilder {
 public:
  explicit NaiveSkillTeamBuilder() : NaiveTeamBuilder(OptimizeSkill::Get()) {}
};

}  // namespace sekai
