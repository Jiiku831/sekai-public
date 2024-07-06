#pragma once

#include <functional>
#include <optional>
#include <span>
#include <vector>

#include "absl/time/time.h"
#include "sekai/card.h"
#include "sekai/estimator_base.h"
#include "sekai/event_bonus.h"
#include "sekai/profile.h"
#include "sekai/team.h"
#include "sekai/team_builder/constraints.h"
#include "sekai/team_builder/neighbor_teams.h"
#include "sekai/team_builder/optimization_objective.h"
#include "sekai/team_builder/team_builder_base.h"

namespace sekai {

class SimulatedAnnealingTeamBuilder : public TeamBuilderBase {
 public:
  struct Options {
    int max_steps = 10'000'000;

    float initial_temp = 100;
    int steps_before_cooling = 100'000;
    int decay_half_life = 500'000;
    float min_temp = 0.0001;

    int early_exit_steps = 1'000'000;

    bool enable_progress = false;

    NeighborStrategy neighbor_gen = NeighborStrategy::kSimple;
    // When this is true, then constraints are not respected in the initial team. Which may result
    // in no teams.
    bool allow_repeat_chars = false;
  };
  explicit SimulatedAnnealingTeamBuilder(
      OptimizationObjective obj = OptimizationObjective::kOptimizePoints)
      : obj_(obj) {}
  explicit SimulatedAnnealingTeamBuilder(
      const Options& options, OptimizationObjective obj = OptimizationObjective::kOptimizePoints)
      : opts_(options), obj_(obj) {}

 protected:
  Options opts_;
  OptimizationObjective obj_;

  std::vector<Team> RecommendTeamsImpl(std::span<const Card* const> pool, const Profile& profile,
                                       const EventBonus& event_bonus,
                                       const EstimatorBase& estimator,
                                       std::optional<absl::Time> deadline = std::nullopt) override;
};

class SimulatedAnnealingPowerTeamBuilder : public SimulatedAnnealingTeamBuilder {
 public:
  explicit SimulatedAnnealingPowerTeamBuilder()
      : SimulatedAnnealingTeamBuilder(OptimizationObjective::kOptimizePower) {}
};

class SimulatedAnnealingBonusTeamBuilder : public SimulatedAnnealingTeamBuilder {
 public:
  explicit SimulatedAnnealingBonusTeamBuilder()
      : SimulatedAnnealingTeamBuilder(OptimizationObjective::kOptimizeBonus) {}
};

class SimulatedAnnealingSkillTeamBuilder : public SimulatedAnnealingTeamBuilder {
 public:
  explicit SimulatedAnnealingSkillTeamBuilder()
      : SimulatedAnnealingTeamBuilder(OptimizationObjective::kOptimizeSkill) {}
};

}  // namespace sekai
