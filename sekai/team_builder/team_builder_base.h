#pragma once

#include "sekai/team_builder/constraints.h"
#include "sekai/team_builder/team_builder.h"

namespace sekai {

class TeamBuilderBase : public TeamBuilder {
 public:
  void AddConstraints(const Constraints& constraints) { constraints_ = constraints; }

  std::vector<Team> RecommendTeams(std::span<const Card* const> pool, const Profile& profile,
                                   const EventBonus& event_bonus, const Estimator& estimator,
                                   std::optional<absl::Time> deadline = std::nullopt) override {
    return RecommendTeamsImpl(constraints_.FilterCardPool(pool), profile, event_bonus, estimator,
                              deadline);
  }

  virtual std::vector<Team> RecommendTeamsImpl(
      std::span<const Card* const> filtered_pool, const Profile& profile,
      const EventBonus& event_bonus, const Estimator& estimator,
      std::optional<absl::Time> deadline = std::nullopt) = 0;

  struct Stats {
    uint64_t teams_total = 0;
    uint64_t teams_considered = 0;
    uint64_t teams_evaluated = 0;
    uint64_t cards_pruned = 0;
  };

  const Stats& stats() const { return stats_; }

 protected:
  Stats stats_;
  Constraints constraints_;
};

}  // namespace sekai
