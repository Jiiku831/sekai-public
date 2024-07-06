#include "sekai/team_builder/challenge_live_team_builder.h"

#include "absl/log/log.h"
#include "sekai/array_size.h"
#include "sekai/card.h"
#include "sekai/combinations.h"
#include "sekai/estimator_base.h"
#include "sekai/event_bonus.h"
#include "sekai/profile.h"
#include "sekai/team_builder/neighbor_teams.h"
#include "sekai/team_builder/pool_utils.h"
#include "sekai/team_builder/simulated_annealing_team_builder.h"
#include "sekai/team_builder/team_builder.h"

namespace sekai {
namespace {

std::vector<const Card*> GetCharacterPool(std::span<const Card* const> pool, int char_id) {
  std::array<std::vector<const Card*>, kCharacterArraySize> char_pools =
      PartitionCardPoolByCharacters(pool);
  return char_pools[char_id];
}

std::vector<Team> NaiveRecommendTeams(std::span<const Card* const> pool, const Profile& profile,
                                      const EventBonus& event_bonus, const EstimatorBase& estimator,
                                      TeamBuilder::Stats& stats) {
  double best_val = 0;
  std::optional<Team> best_team = std::nullopt;

  Combinations<const Card*, 5>{
      pool,
      [&](std::span<const Card* const> candidate_cards) {
        ++stats.teams_considered;
        Team candidate_team{candidate_cards};
        ++stats.teams_evaluated;
        double candidate_val = estimator.ExpectedValue(profile, event_bonus, candidate_team);
        if (candidate_val > best_val || !best_team.has_value()) {
          best_val = candidate_val;
          best_team = candidate_team;
        }
        return true;
      },
  }();
  if (best_team.has_value()) {
    return {*best_team};
  }
  return {};
}

}  // namespace

std::vector<Team> ChallengeLiveTeamBuilder::RecommendTeamsImpl(
    std::span<const Card* const> filtered_pool, const Profile& profile,
    const EventBonus& event_bonus, const EstimatorBase& estimator,
    std::optional<absl::Time> deadline) {
  std::vector<const Card*> pool = GetCharacterPool(filtered_pool, char_id_);
  // LOG(INFO) << "Character " << char_id_ << " pool size: " << pool.size();
  if (pool.size() < 70) {
    // LOG(INFO) << "Using exhaustive search";
    return NaiveRecommendTeams(pool, profile, event_bonus, estimator, stats_);
  }
  // LOG(INFO) << "Using simulated annealing";
  SimulatedAnnealingTeamBuilder::Options opts = opts_;
  opts.neighbor_gen = NeighborStrategy::kChallengeLive;
  opts.allow_repeat_chars = true;
  opts.initial_temp = 100'000;
  SimulatedAnnealingTeamBuilder builder{opts};
  std::vector<Team> results =
      builder.RecommendTeams(pool, profile, event_bonus, estimator, deadline);
  stats_ += builder.stats();
  return results;
};

}  // namespace sekai
