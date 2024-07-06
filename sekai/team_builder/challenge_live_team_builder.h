#pragma once

#include <optional>
#include <vector>

#include "absl/log/absl_check.h"
#include "absl/time/time.h"
#include "sekai/array_size.h"
#include "sekai/estimator_base.h"
#include "sekai/event_bonus.h"
#include "sekai/profile.h"
#include "sekai/team.h"
#include "sekai/team_builder/neighbor_teams.h"
#include "sekai/team_builder/simulated_annealing_team_builder.h"
#include "sekai/team_builder/team_builder_base.h"

namespace sekai {

class ChallengeLiveTeamBuilder : public TeamBuilderBase {
 public:
  explicit ChallengeLiveTeamBuilder(int char_id) : ChallengeLiveTeamBuilder({}, char_id) {}
  ChallengeLiveTeamBuilder(const SimulatedAnnealingTeamBuilder::Options& opts, int char_id)
      : char_id_(char_id), opts_(opts) {
    ABSL_CHECK_LT(char_id, CharacterArraySize());
  }

 protected:
  int char_id_;
  SimulatedAnnealingTeamBuilder::Options opts_;

  std::vector<Team> RecommendTeamsImpl(std::span<const Card* const> filtered_pool,
                                       const Profile& profile, const EventBonus& event_bonus,
                                       const EstimatorBase& estimator,
                                       std::optional<absl::Time> deadline = std::nullopt) override;
};

}  // namespace sekai
