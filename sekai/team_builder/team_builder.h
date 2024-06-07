#pragma once

#include <optional>
#include <span>
#include <vector>

#include "absl/time/time.h"
#include "sekai/card.h"
#include "sekai/estimator.h"
#include "sekai/event_bonus.h"
#include "sekai/profile.h"
#include "sekai/team.h"

namespace sekai {

class TeamBuilder {
 public:
  virtual ~TeamBuilder() = default;

  virtual std::vector<Team> RecommendTeams(std::span<const Card* const> pool,
                                           const Profile& profile, const EventBonus& event_bonus,
                                           const Estimator& estimator,
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
};

}  // namespace sekai
