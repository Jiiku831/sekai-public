#pragma once

#include "sekai/bitset.h"
#include "sekai/event_bonus.h"
#include "sekai/profile.h"
#include "sekai/proto/team.pb.h"

namespace sekai {

class Team;

class EstimatorBase {
 public:
  ~EstimatorBase() = default;

  virtual double ExpectedValue(const Profile& profile, const EventBonus& event_bonus,
                               const Team& team) const = 0;
  virtual double MaxExpectedValue(const Profile& profile, const EventBonus& event_bonus,
                                  const Team& team, Character lead_chars) const = 0;
  virtual double SmoothOptimizationObjective(const Profile& profile, const EventBonus& event_bonus,
                                             const Team& team, Character lead_chars) const = 0;

  virtual void AnnotateTeamProto(const Profile& profile, const EventBonus& event_bonus,
                                 const Team& team, TeamProto& team_proto) const = 0;
};

}  // namespace sekai
