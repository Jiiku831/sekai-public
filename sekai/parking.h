#pragma once

#include <span>

#include "absl/container/flat_hash_map.h"
#include "absl/status/statusor.h"
#include "sekai/event_bonus.h"
#include "sekai/profile.h"
#include "sekai/team.h"

namespace sekai {

// Precondition: values are sorted and all positive.
absl::StatusOr<absl::flat_hash_map<int, int>> SolveSubsetSumWithRepetition(
    std::span<const int> values, int target);

void AnnotateTeamProtoWithMultiTurnParkingStrategy(const Team& team, const Profile& profile,
                                                   const EventBonus& event_bonus, float accuracy,
                                                   int target, TeamProto& team_proto);

}  // namespace sekai
