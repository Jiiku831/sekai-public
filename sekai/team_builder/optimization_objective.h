#pragma once

#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/functional/any_invocable.h"
#include "sekai/bitset.h"
#include "sekai/card.h"
#include "sekai/estimator_base.h"
#include "sekai/event_bonus.h"
#include "sekai/profile.h"
#include "sekai/proto/team.pb.h"
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

// Optimize for exact points gained by ebi.
class OptimizeExactPoints : public OptimizationObjective {
 public:
  OptimizeExactPoints(int target);

  ObjectiveFunction GetObjectiveFunction() const override;

  struct ViableStrategy {
    int boost = 0;
    int multiplier = 0;
    int base_ep = 0;
    int ep = 0;
    int score_lb = 0;
    int score_ub = 0;
    int max_score = 0;
  };
  std::vector<ViableStrategy> GetViableStrategies(const Team& team, const Profile& profile,
                                                  const EventBonus& event_bonus) const;

  void AnnotateTeamProto(const Team& team, const Profile& profile, const EventBonus& event_bonus,
                         TeamProto& team_proto) const;

  static int BaseEp(int score, double eb);

 private:
  int target_;
  absl::flat_hash_map<int, int> min_score_;
  absl::flat_hash_map<int, int> min_multiplier_;
  std::vector<int> viable_bonuses_;

  static constexpr int kBaseFactor = 100;
  static constexpr int kScoreStep = 20'000;
  static constexpr int kMinScoreMargin = 10'000;
};

ObjectiveFunction GetObjectiveFunction(const OptimizationObjective& obj);

}  // namespace sekai
