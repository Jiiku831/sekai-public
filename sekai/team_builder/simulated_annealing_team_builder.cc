#include "sekai/team_builder/simulated_annealing_team_builder.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>
#include <random>
#include <vector>

#include <indicators/progress_bar.hpp>

#include "absl/log/absl_check.h"
#include "sekai/array_size.h"
#include "sekai/bitset.h"
#include "sekai/combinations.h"
#include "sekai/team.h"
#include "sekai/team_builder/constraints.h"
#include "sekai/team_builder/neighbor_teams.h"
#include "sekai/team_builder/optimization_objective.h"
#include "sekai/team_builder/pool_utils.h"

namespace sekai {

namespace {

constexpr int kTeamSize = 5;

std::optional<Team> GetRandomTeam(std::span<const Card* const> pool, const Constraints& constraints,
                                  std::mt19937& g) {
  std::array<std::vector<const Card*>, kCharacterArraySize> char_pools =
      PartitionCardPoolByCharacters(pool);

  std::vector<int> shuffled_ids = UniqueCharacterIds();
  std::shuffle(shuffled_ids.begin(), shuffled_ids.end(), g);

  std::optional<Team> team;
  Combinations<int, 5>{
      shuffled_ids,
      [&](std::span<const int> candidate_chars) {
        Character chars_present;
        for (int char_id : candidate_chars) {
          chars_present.set(char_id);
        }
        if (!constraints.CharacterSetSatisfiesConstraint(chars_present)) {
          return true;
        }
        Character lead_chars = constraints.GetCharactersEligibleForLead(chars_present);

        std::array<std::span<const Card* const>, 5> current_pool;
        ABSL_CHECK_EQ(static_cast<int>(candidate_chars.size()), 5);
        for (int i = 0; i < static_cast<int>(candidate_chars.size()); ++i) {
          current_pool[i] = char_pools[candidate_chars[i]];
          if (current_pool[i].empty()) {
            return true;
          }
        }

        // Go down the list of candidate cards for each possible lead until
        // we find a match.
        for (int i = 0; i < static_cast<int>(candidate_chars.size()); ++i) {
          int candidate_char = candidate_chars[i];
          if (!lead_chars.test(candidate_char)) {
            continue;
          }
          std::array<const Card*, 5> candidate_cards;
          for (int j = 0; j < static_cast<int>(candidate_chars.size()); ++j) {
            candidate_cards[j] = current_pool[j][0];
          }
          std::span<const Card* const> candidate_pool = current_pool[i];
          for (int j = 0; j < static_cast<int>(candidate_pool.size()); ++j) {
            candidate_cards[i] = candidate_pool[j];

            Team candidate_team{candidate_cards};
            bool constraints_satisfied = !constraints.HasLeadSkillConstraint();
            if (!constraints_satisfied) {
              Team::SkillValueDetail skill_value =
                  candidate_team.ConstrainedMaxSkillValue(lead_chars);
              constraints_satisfied =
                  constraints.LeadSkillSatisfiesConstraint(skill_value.lead_skill);
            }

            if (constraints_satisfied) {
              team = candidate_team;
              return false;
            }
          }
        }

        return true;
      },
  }();
  return team;
}

float TransitionProbability(double start_val, double candidate_val, double temp) {
  return std::exp((candidate_val - start_val) / temp);
}

}  // namespace

std::vector<Team> SimulatedAnnealingTeamBuilder::RecommendTeamsImpl(
    std::span<const Card* const> pool, const Profile& profile, const EventBonus& event_bonus,
    const Estimator& estimator, std::optional<absl::Time> deadline) {
  std::random_device rd;
  std::mt19937 g{rd()};
  std::vector<const Card*> shuffled_pool = {pool.begin(), pool.end()};
  std::shuffle(shuffled_pool.begin(), shuffled_pool.end(), g);
  std::uniform_real_distribution<float> prob;

  std::unique_ptr<indicators::ProgressBar> bar;
  uint64_t max_progress = opts_.max_steps;
  if (opts_.enable_progress) {
    bar = std::make_unique<indicators::ProgressBar>(
        indicators::option::BarWidth{60}, indicators::option::ShowElapsedTime{true},
        indicators::option::ShowRemainingTime{true}, indicators::option::ShowPercentage{true},
        indicators::option::MaxProgress{max_progress},
        indicators::option::PostfixText{absl::StrFormat("0/%llu", max_progress)});
  }

  SimpleNeighbors neighbors_gen(shuffled_pool, constraints_);
  ObjectiveFunction objective = GetObjectiveFunction(obj_);
  std::optional<Team> best_team = GetRandomTeam(shuffled_pool, constraints_, g);

  if (!best_team.has_value()) {
    return {};
  }

  Team current_team = *best_team;
  double current_val =
      objective(current_team, profile, event_bonus, estimator,
                constraints_.GetCharactersEligibleForLead(current_team.chars_present()));
  double best_val = current_val;
  int steps_since_best_val_changed = 0;
  float current_temp = opts_.initial_temp;
  ++stats_.teams_evaluated;
  bool early_exit = false;
  for (int i = 0; i < opts_.max_steps; ++i) {
    if (bar != nullptr && i % 10000 == 0) {
      bar->set_progress(i);
      bar->set_option(
          indicators::option::PostfixText{absl::StrFormat("%llu/%llu", i, max_progress)});
    }

    std::optional<Team> new_team = neighbors_gen.GetRandomNeighbor(current_team, g);
    if (new_team.has_value()) {
      ++stats_.teams_evaluated;
      float new_val =
          objective(*new_team, profile, event_bonus, estimator,
                    constraints_.GetCharactersEligibleForLead(new_team->chars_present()));

      if (new_val > best_val) {
        best_val = new_val;
        best_team = new_team;
        steps_since_best_val_changed = 0;
      }

      float transition_prob = TransitionProbability(current_val, new_val, current_temp);
      if (prob(g) < transition_prob) {
        current_team = *new_team;
        current_val = new_val;
      } else if (transition_prob = TransitionProbability(best_val, new_val, current_temp) * 0.1;
                 prob(g) < transition_prob) {
        // Always small chance to return to best state.
        current_team = *best_team;
        current_val = best_val;
      }
    }

    // Cooling
    if (i > opts_.steps_before_cooling) {
      current_temp = std::exp((i - opts_.steps_before_cooling) / opts_.decay_half_life);
    }
    if (current_temp <= opts_.min_temp) {
      current_temp = opts_.min_temp;
    }

    // Early exit
    ++steps_since_best_val_changed;
    if (opts_.early_exit_steps > 0 && steps_since_best_val_changed > opts_.early_exit_steps) {
      if (bar != nullptr) {
        bar->set_option(indicators::option::PostfixText{absl::StrFormat("Early exit at %llu", i)});
        bar->mark_as_completed();
      }
      early_exit = true;
      break;
    }
  }
  if (bar != nullptr && !early_exit) {
    bar->set_option(
        indicators::option::PostfixText{absl::StrFormat("%llu/%llu", max_progress, max_progress)});
    bar->set_progress(max_progress);
  }

  if (best_team.has_value()) {
    return {*best_team};
  }
  return {};
}

}  // namespace sekai
