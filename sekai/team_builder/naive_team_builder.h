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
#include "sekai/team_builder/team_builder.h"

namespace sekai {

class NaiveTeamBuilder : public TeamBuilder {
 public:
  enum class Objective {
    kOptimizePoints,
    kOptimizePower,
    kOptimizeBonus,
    kOptimizeSkill,
  };
  explicit NaiveTeamBuilder(Objective obj = Objective::kOptimizePoints) : obj_(obj) {}

  std::vector<Team> RecommendTeams(std::span<const Card* const> pool, const Profile& profile,
                                   const EventBonus& event_bonus, const Estimator& estimator,
                                   std::optional<absl::Time> deadline = std::nullopt) override;

 protected:
  Objective obj_;
};

}  // namespace sekai
