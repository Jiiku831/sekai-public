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
#include "sekai/team_builder/team_builder_base.h"

namespace sekai {

class MaxPowerTeamBuilder : public TeamBuilderBase {
 public:
  std::vector<Team> RecommendTeamsImpl(std::span<const Card* const> pool, const Profile& profile,
                                       const EventBonus& event_bonus, const Estimator& estimator,
                                       std::optional<absl::Time> deadline = std::nullopt) override;
};

}  // namespace sekai
