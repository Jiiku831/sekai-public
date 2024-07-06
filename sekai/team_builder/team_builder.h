#pragma once

#include <optional>
#include <span>
#include <vector>

#include "absl/time/time.h"
#include "sekai/card.h"
#include "sekai/estimator_base.h"
#include "sekai/event_bonus.h"
#include "sekai/profile.h"
#include "sekai/team.h"

namespace sekai {

class TeamBuilder {
 public:
  virtual ~TeamBuilder() = default;

  virtual std::vector<Team> RecommendTeams(std::span<const Card* const> pool,
                                           const Profile& profile, const EventBonus& event_bonus,
                                           const EstimatorBase& estimator,
                                           std::optional<absl::Time> deadline = std::nullopt) = 0;

  struct Stats {
    uint64_t teams_total = 0;
    uint64_t teams_considered = 0;
    uint64_t teams_evaluated = 0;
    uint64_t cards_pruned = 0;

    void operator+=(const Stats& other) {
      teams_total += other.teams_total;
      teams_considered += other.teams_considered;
      teams_evaluated += other.teams_evaluated;
      cards_pruned += other.cards_pruned;
    }
  };

  virtual const TeamBuilder::Stats& stats() const = 0;
};

}  // namespace sekai
