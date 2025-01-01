#include "sekai/team_builder/optimization_objective.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include "absl/base/no_destructor.h"
#include "absl/container/flat_hash_map.h"
#include "absl/log/absl_check.h"
#include "absl/log/log.h"
#include "sekai/bitset.h"
#include "sekai/challenge_live_estimator.h"
#include "sekai/config.h"
#include "sekai/estimator_base.h"
#include "sekai/event_bonus.h"
#include "sekai/points.h"
#include "sekai/profile.h"
#include "sekai/proto/team.pb.h"
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
            Character lead_chars) { return static_cast<double>(team.Power(profile)) / 100; };
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

OptimizeExactPoints::OptimizeExactPoints(int target, float accuracy)
    : target_(target), accuracy_(accuracy) {
  absl::flat_hash_map<int, int> min_multiplier_pts;
  for (int i = kBoostMultipliers.size() - 1; i >= 0; --i) {
    if (target % kBoostMultipliers[i] != 0) {
      continue;
    }
    int base_target = target / kBoostMultipliers[i];
    if (base_target < 100) {
      continue;
    }
    min_multiplier_pts[base_target] = i;
  }

  for (int eb = 0; eb < kMaxEventBonus; ++eb) {
    for (int score = 0; score < kMaxScore; score += kSoloScoreStep) {
      const int point = SoloEbiPoints(score, eb);
      auto min_multiplier = min_multiplier_pts.find(point);
      if (min_multiplier != min_multiplier_pts.end()) {
        viable_bonuses_.push_back(eb);
        min_score_[eb] = score;
        min_multiplier_[eb] = min_multiplier->second;
        break;
      }
    }
  }
}

ObjectiveFunction OptimizeExactPoints::GetObjectiveFunction() const {
  return [this](const Team& team, const Profile& profile, const EventBonus& event_bonus,
                const EstimatorBase& estimator, Character lead_chars) -> double {
    if (viable_bonuses_.empty()) {
      return -std::numeric_limits<double>::infinity();
    }
    const float team_event_bonus = team.EventBonus(event_bonus);
    const auto eb_lb =
        std::lower_bound(viable_bonuses_.begin(), viable_bonuses_.end(), team_event_bonus);
    int closest_viable_eb = viable_bonuses_.front();
    if (eb_lb != viable_bonuses_.end()) {
      // Otherwise, all viable bonuses are greater than the team event bonus.
      // So the closest is the first value.
      const auto eb_ub = std::next(eb_lb);
      if (eb_ub == viable_bonuses_.end()) {
        closest_viable_eb = *eb_lb;
      } else {
        int dist_to_lb = team_event_bonus - *eb_lb;
        int dist_to_ub = *eb_ub - team_event_bonus;
        closest_viable_eb = dist_to_lb < dist_to_ub ? *eb_lb : *eb_ub;
      }
    }

    int min_score = min_score_.at(closest_viable_eb);
    double max_team_score =
        SoloEbiMasEstimator().MaxExpectedValue(profile, event_bonus, team, lead_chars) * accuracy_;
    // Optimize to ensure that event bonus is exact and team score is at least the minimum needed.
    double eb_penalty = 10 * std::abs(closest_viable_eb - team_event_bonus);
    double score_penalty = std::max(0.0, min_score - max_team_score) / 1000;

    if (eb_penalty + score_penalty > 0) {
      return -1.0 * (eb_penalty + score_penalty);
    }

    return min_score <= 0 ? 100.0 : 100.0 * (max_team_score - min_score) / max_team_score;
  };
}

std::vector<OptimizeExactPoints::ViableStrategy> OptimizeExactPoints::GetViableStrategies(
    const Team& team, const Profile& profile, const EventBonus& event_bonus) const {
  std::vector<ViableStrategy> strategies;
  double max_team_score =
      SoloEbiMasEstimator().ExpectedValue(profile, event_bonus, team) * accuracy_;
  const float team_event_bonus = team.EventBonus(event_bonus);
  for (int i = 0; static_cast<std::size_t>(i) < kBoostMultipliers.size(); ++i) {
    if (target_ % kBoostMultipliers[i] != 0) {
      continue;
    }
    int base_target = target_ / kBoostMultipliers[i];
    for (int score = 0; score < max_team_score; score += kSoloScoreStep) {
      if (SoloEbiPoints(score, team_event_bonus) == base_target) {
        strategies.push_back({
            .boost = i,
            .multiplier = kBoostMultipliers[i],
            .base_ep = base_target,
            .ep = target_,
            .score_lb = score,
            .score_ub = score + kSoloScoreStep - 1,
        });
      }
    }
  }
  return strategies;
}

void OptimizeExactPoints::AnnotateTeamProto(const Team& team, const Profile& profile,
                                            const EventBonus& event_bonus,
                                            TeamProto& team_proto) const {
  std::vector<ViableStrategy> strategies = GetViableStrategies(team, profile, event_bonus);
  team_proto.set_target_ep(target_);
  team_proto.set_max_solo_ebi_score(
      SoloEbiMasEstimator().ExpectedValue(profile, event_bonus, team) * accuracy_);
  for (const ViableStrategy& strategy : strategies) {
    ParkingStrategy& strategy_proto = *team_proto.add_parking_strategies();
    strategy_proto.set_boost(strategy.boost);
    strategy_proto.set_multiplier(strategy.multiplier);
    strategy_proto.set_base_ep(strategy.base_ep);
    strategy_proto.set_total_ep(strategy.ep);
    strategy_proto.set_score_lb(strategy.score_lb);
    strategy_proto.set_score_ub(strategy.score_ub);
  }
}

ObjectiveFunction OptimizeFillTeam::GetObjectiveFunction() const {
  return [min_power = min_power_](const Team& team, const Profile& profile,
                                  const EventBonus& event_bonus, const EstimatorBase& estimator,
                                  Character lead_chars) {
    int team_power = team.Power(profile);
    if (team_power < min_power) {
      return static_cast<double>(team_power - min_power) / 100;
    }

    double skill_factor =
        10.0 * static_cast<int>(team.ConstrainedMaxSkillValue(lead_chars).skill_value);
    double point_factor =
        estimator.MaxExpectedValue(profile, event_bonus, team, lead_chars) / kMaxBaseEventPoint;

    return skill_factor + point_factor;
  };
}

ObjectiveFunction GetObjectiveFunction(const OptimizationObjective& obj) {
  return obj.GetObjectiveFunction();
}

}  // namespace sekai
