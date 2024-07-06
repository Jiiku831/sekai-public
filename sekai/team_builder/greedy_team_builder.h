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

// Returns the first feasible team.
class GreedyTeamBuilder : public TeamBuilderBase {
 public:
  GreedyTeamBuilder(OptimizationObjective obj) : obj_(obj) {}

 private:
  OptimizationObjective obj_;

  // Assumes that the pool is already sorted.
  std::vector<Team> RecommendTeamsImpl(std::span<const Card* const> pool, const Profile& profile,
                                       const EventBonus& event_bonus,
                                       const EstimatorBase& estimator,
                                       std::optional<absl::Time> deadline = std::nullopt) override;
};

}  // namespace sekai
