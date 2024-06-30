#include "sekai/team_builder/max_power_team_builder.h"

#include <array>
#include <bitset>
#include <functional>
#include <limits>
#include <map>
#include <vector>

#include "absl/log/absl_check.h"
#include "sekai/array_size.h"
#include "sekai/bitset.h"
#include "sekai/card.h"
#include "sekai/combinations.h"
#include "sekai/team.h"
#include "sekai/team_builder/constraints.h"
#include "sekai/team_builder/greedy_team_builder.h"
#include "sekai/team_builder/optimization_objective.h"
#include "sekai/team_builder/pool_utils.h"
#include "sekai/team_builder/team_builder.h"

namespace sekai {
namespace {

struct CandidateState {
  db::Attr attr;
  db::Unit primary_unit;
  db::Unit secondary_unit;
};

std::vector<Team> RecommendTeamsForState(const CandidateState& state,
                                         std::span<const Card* const> pool, const Profile& profile,
                                         const EventBonus& event_bonus, const Estimator& estimator,
                                         const Constraints& constraints,
                                         std::optional<absl::Time> deadline,
                                         TeamBuilder::Stats& stats) {
  std::vector<const Card*> filtered_pool = {pool.begin(), pool.end()};
  bool attr_match = false;
  if (state.attr != db::ATTR_UNKNOWN) {
    attr_match = true;
    filtered_pool = FilterCardsByAttr(state.attr, filtered_pool);
  }

  bool primary_unit_match = false;
  if (state.primary_unit != db::UNIT_NONE) {
    primary_unit_match = true;
    filtered_pool = FilterCardsByUnit(state.primary_unit, filtered_pool);
  }

  bool secondary_unit_match = false;
  if (state.secondary_unit != db::UNIT_NONE) {
    secondary_unit_match = true;
    filtered_pool = FilterCardsByUnit(state.secondary_unit, filtered_pool);
  }

  std::vector<const Card*> sorted_pool =
      SortCardsByPower(filtered_pool, attr_match, primary_unit_match, secondary_unit_match);
  GreedyTeamBuilder team_builder(OptimizationObjective::kOptimizePower);
  team_builder.AddConstraints(constraints);
  std::vector<Team> teams =
      team_builder.RecommendTeams(sorted_pool, profile, event_bonus, estimator, deadline);
  stats += team_builder.stats();
  return teams;
}

}  // namespace

std::vector<Team> MaxPowerTeamBuilder::RecommendTeamsImpl(std::span<const Card* const> pool,
                                                          const Profile& profile,
                                                          const EventBonus& event_bonus,
                                                          const Estimator& estimator,
                                                          std::optional<absl::Time> deadline) {
  std::optional<Team> best_team;
  for (int attr = 0; attr <= db::Attr_MAX; ++attr) {
    for (int unit = 0; unit <= db::Unit_MAX; ++unit) {
      for (db::Unit secondary_unit : {db::UNIT_NONE, db::UNIT_VS}) {
        ABSL_CHECK(db::Unit_IsValid(unit));
        ABSL_CHECK(db::Attr_IsValid(attr));

        CandidateState state = {
            .attr = static_cast<db::Attr>(attr),
            .primary_unit = static_cast<db::Unit>(unit),
            .secondary_unit = secondary_unit,
        };
        std::vector<Team> candidate_teams = RecommendTeamsForState(
            state, pool, profile, event_bonus, estimator, constraints_, deadline, stats_);
        for (const Team& team : candidate_teams) {
          if (!best_team.has_value() || best_team->Power(profile) < team.Power(profile)) {
            best_team = team;
          }
        }
      }
    }
  }
  if (!best_team.has_value()) {
    return {};
  }
  return {*best_team};
}

}  // namespace sekai
