#include "sekai/parking.h"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <map>
#include <random>
#include <tuple>

#include "absl/container/btree_map.h"
#include "absl/container/flat_hash_map.h"
#include "absl/log/absl_check.h"
#include "absl/log/log.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "base/util.h"
#include "sekai/config.h"
#include "sekai/event_bonus.h"
#include "sekai/profile.h"
#include "sekai/team.h"

namespace sekai {
namespace {

absl::flat_hash_map<int, int> MakeSolution(std::span<const int> path) {
  absl::flat_hash_map<int, int> sol;
  for (int val : path) {
    ++sol[val];
  }
  return sol;
}

// Assumes values is sorted.
absl::flat_hash_map<int, int> ExhaustiveSubsetSumWithRepetition(std::span<const int> values,
                                                                int target) {
  absl::btree_map<int, std::vector<int>> last, current;
  current[0] = {};
  last = current;
  for (int i = values.size() - 1; i >= 0; --i) {
    const int val = values[i];
    if (val > target) {
      continue;
    }

    int breakpoint = target / val;
    for (const auto& [old_val, old_path] : last) {
      for (int j = 1; j <= breakpoint; ++j) {
        const int new_val = old_val + val * j;
        if (new_val > target) {
          break;
        }
        std::vector<int>& new_path = current[new_val];
        if (!new_path.empty() && new_path.size() <= old_path.size() + j) {
          continue;
        }
        new_path = old_path;
        new_path.insert(new_path.end(), j, val);
      }
    }
    if (const auto& solution = current[target]; !solution.empty()) {
      return MakeSolution(solution);
    }
    last = current;
  }
  return {};
}

class Sum {
 public:
  void Add(int val) {
    ++values_[val];
    ++count_;
    sum_ += val;
  }

  void Add(int val, int n) {
    values_[val] += n;
    count_ += n;
    sum_ += n * val;
  }

  void Remove(absl::flat_hash_map<int, int>::iterator it) {
    ABSL_CHECK(it != values_.end());
    ABSL_CHECK_GT(it->second, 0);
    --(it->second);
    --count_;
    sum_ -= it->first;
    if (it->second <= 0) {
      values_.erase(it);
    }
  }

  int sum() const { return sum_; }
  int count() const { return count_; }
  const absl::flat_hash_map<int, int>& values() const { return values_; }

  Sum Mutate(std::span<const int> values, int target, std::uniform_real_distribution<float>& prob,
             std::uniform_int_distribution<int>& value_dist, std::mt19937& g) {
    Sum new_sum = *this;
    if (!new_sum.values_.empty() && prob(g) < 0.5) {
      std::uniform_int_distribution<int> key_dist(0, new_sum.values_.size() - 1);
      int pos = key_dist(g);
      auto it = std::next(new_sum.values_.begin(), pos);
      new_sum.Remove(it);
    } else {
      new_sum.Add(values[value_dist(g)]);
    }
    return new_sum;
  }

 private:
  absl::flat_hash_map<int, int> values_;
  int sum_ = 0;
  int count_ = 0;
};

Sum FillSumWithBound(std::span<const int> values, int target) {
  Sum sum;
  sum.Add(values.back(), target / values.back());
  return sum;
}

float GetValue(const Sum& sum, int target, int max_count, int value_count) {
  int target_penalty = std::abs(sum.sum() - target);
  if (target_penalty > 0) {
    return target_penalty * -1;
  }

  int count_goodness = 10 * static_cast<float>(max_count - sum.count()) / max_count;
  int complexity_goodness = static_cast<float>(value_count - sum.values().size()) / value_count;
  return count_goodness = complexity_goodness;
}

float TransitionProb(float old_val, float new_val, float temp) {
  return std::exp((new_val - old_val) / temp);
}

absl::StatusOr<absl::flat_hash_map<int, int>> ProbabilisticSubsetSumWithRepetition(
    std::span<const int> values, int target) {
  std::random_device rd;
  std::mt19937 g{rd()};
  std::uniform_real_distribution<float> prob;
  std::uniform_int_distribution<int> value_dist(0, values.size() - 1);

  int max_count = target / values.front() + 1;
  int value_count = values.size();
  Sum best_sum = FillSumWithBound(values, target);
  float best_val = GetValue(best_sum, target, max_count, value_count);
  Sum current_sum = best_sum;
  float current_val = best_val;
  int steps_since_val_changed = 0;
  float current_temp = 100;
  int steps_before_cooling = 100'000;
  int half_life = 500'000;
  int max_steps = 10'000'000;
  int early_exit_steps = 500'000;
  float min_temp = 0.0001;
  float return_prob = 0.05;
  for (int i = 0; i < max_steps; ++i) {
    Sum next_sum = current_sum.Mutate(values, target, prob, value_dist, g);
    float next_val = GetValue(next_sum, target, max_count, value_count);

    if (next_val > best_val) {
      best_val = next_val;
      best_sum = next_sum;
      current_val = next_val;
      current_sum = next_sum;
      steps_since_val_changed = 0;
    } else {
      if (float transition_prob = TransitionProb(current_val, next_val, current_temp);
          prob(g) < transition_prob) {
        current_val = next_val;
        current_sum = next_sum;
      } else if (float transition_prob =
                     TransitionProb(current_val, next_val, current_temp) * return_prob;
                 prob(g) < transition_prob) {
        current_val = best_val;
        current_sum = best_sum;
      }
    }

    // Cooling
    if (i > steps_before_cooling) {
      current_temp = std::exp((i - steps_before_cooling) / half_life);
    }
    if (current_temp <= min_temp) {
      current_temp = min_temp;
    }

    if (steps_since_val_changed >= early_exit_steps) {
      break;
    }
    ++steps_since_val_changed;
  }

  if (best_val >= 0) {
    return best_sum.values();
  }
  return absl::ResourceExhaustedError("Compute budget exhausted.");
}

}  // namespace

absl::StatusOr<absl::flat_hash_map<int, int>> SolveSubsetSumWithRepetition(
    std::span<const int> values, int target) {
  absl::flat_hash_map<int, int> solution_map;
  if (target == 0) {
    return solution_map;
  }
  if (target < 0 || values.empty()) {
    return absl::NotFoundError("No solution.");
  }
  const int bound = values.back();
  for (int i = 0; i < static_cast<int>(values.size()) - 1; ++i) {
    if (values[i] > bound) {
      return absl::FailedPreconditionError("Not all values are bounded by last element.");
    }
    if (values[i] > values[i + 1]) {
      return absl::FailedPreconditionError("Values are not sorted.");
    }
    if (values[i] <= 0) {
      return absl::FailedPreconditionError("Not all values are positive.");
    }
  }
  if (target <= 100'000) {
    return ExhaustiveSubsetSumWithRepetition(values, target);
  }
  return ProbabilisticSubsetSumWithRepetition(values, target);
}

void AnnotateTeamProtoWithMultiTurnParkingStrategy(const Team& team, const Profile& profile,
                                                   const EventBonus& event_bonus, float accuracy,
                                                   int target, TeamProto& team_proto) {
  absl::flat_hash_map<int, int> mult_to_boost;
  for (int i = 0; i < static_cast<int>(kBoostMultipliers.size()); ++i) {
    mult_to_boost[kBoostMultipliers[i]] = i;
  }
  Team::SoloEbiBasePoints base_points = team.GetSoloEbiBasePoints(profile, event_bonus, accuracy);
  std::vector<ParkingStrategy> turns;
  ASSIGN_OR_RETURN_VOID(auto sol,
                        SolveSubsetSumWithRepetition(base_points.possible_points, target));
  for (auto [points, count] : sol) {
    absl::StatusOr<absl::flat_hash_map<int, int>> mult_sol =
        SolveSubsetSumWithRepetition(kBoostMultipliers, count);
    if (!mult_sol.ok()) {
      // Shouldn't happen
      LOG(WARNING) << "Failed to find a boost solution for " << count;
      mult_sol = absl::flat_hash_map<int, int>{{{1, count}}};
    }
    ABSL_CHECK_OK(mult_sol);

    const int score_lb = base_points.score_threshold.at(points);
    for (auto [mult, plays] : *mult_sol) {
      ParkingStrategy strategy;
      strategy.set_boost(mult_to_boost.at(mult));
      strategy.set_multiplier(mult);
      strategy.set_base_ep(points);
      strategy.set_total_ep(points * mult * plays);
      strategy.set_score_lb(score_lb);
      strategy.set_score_ub(score_lb + kSoloScoreStep - 1);
      strategy.set_plays(plays);
      strategy.set_total_multiplier(mult * plays);
      turns.push_back(std::move(strategy));
    }
  }
  std::sort(turns.begin(), turns.end(), [](const ParkingStrategy& lhs, const ParkingStrategy& rhs) {
    return std::tuple(lhs.boost(), lhs.base_ep()) > std::tuple(rhs.boost(), rhs.base_ep());
  });
  *team_proto.mutable_multi_turn_parking() = {turns.begin(), turns.end()};
}

}  // namespace sekai
