#include "sekai/team_builder/max_bonus_team_builder.h"

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
#include "sekai/estimator_base.h"
#include "sekai/team.h"
#include "sekai/team_builder/greedy_team_builder.h"
#include "sekai/team_builder/optimization_objective.h"
#include "sekai/team_builder/pool_utils.h"
#include "sekai/team_builder/team_builder.h"

namespace sekai {

std::vector<Team> MaxBonusTeamBuilder::RecommendTeamsImpl(std::span<const Card* const> pool,
                                                          const Profile& profile,
                                                          const EventBonus& event_bonus,
                                                          const EstimatorBase& estimator,
                                                          std::optional<absl::Time> deadline) {
  // TODO: this doesnt work for world link. need optimizer that looks only at rainbow teams.
  std::vector<const Card*> sorted_pool = SortCardsByBonus(pool);
  GreedyTeamBuilder team_builder{OptimizeBonus::Get()};
  team_builder.AddConstraints(constraints_);
  std::vector<Team> teams =
      team_builder.RecommendTeams(sorted_pool, profile, event_bonus, estimator, deadline);
  stats_ = team_builder.stats();
  return teams;
}

}  // namespace sekai
