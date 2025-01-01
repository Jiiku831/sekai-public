#pragma once

#include "sekai/team_builder/constraints.h"
#include "sekai/team_builder/pool_utils.h"
#include "sekai/team_builder/skill_selector.h"
#include "sekai/team_builder/team_builder.h"

namespace sekai {

class TeamBuilderBase : public TeamBuilder {
 public:
  void AddConstraints(const Constraints& constraints) { constraints_ = constraints; }

  std::vector<Team> RecommendTeams(std::span<const Card* const> pool, const Profile& profile,
                                   const EventBonus& event_bonus, const EstimatorBase& estimator,
                                   std::optional<absl::Time> deadline = std::nullopt) override {
    support_pool_ = GetSortedSupportPool(pool);
    std::vector<Team> teams = RecommendTeamsImpl(constraints_.FilterCardPool(pool), profile,
                                                 event_bonus, estimator, deadline);
    std::vector<Team> finalized_teams;
    finalized_teams.reserve(teams.size());
    for (const Team& team : teams) {
      finalized_teams.push_back(
          OptimizeSkillSelection(team.cards(), profile, event_bonus, estimator));
    }
    return finalized_teams;
  }

  const TeamBuilder::Stats& stats() const override { return stats_; }

  const Constraints& constraints() const { return constraints_; }

 protected:
  TeamBuilder::Stats stats_;
  Constraints constraints_;
  std::vector<const Card*> support_pool_;

  virtual std::vector<Team> RecommendTeamsImpl(
      std::span<const Card* const> filtered_pool, const Profile& profile,
      const EventBonus& event_bonus, const EstimatorBase& estimator,
      std::optional<absl::Time> deadline = std::nullopt) = 0;
};

}  // namespace sekai
