#pragma once

#include "sekai/card.h"
#include "sekai/estimator_base.h"
#include "sekai/event_bonus.h"
#include "sekai/profile.h"
#include "sekai/team.h"

namespace sekai {

// Optimizes the skill selection first 5 cards in the list.
Team OptimizeSkillSelection(std::span<const Card* const> cards, const Profile& profile,
                            const EventBonus& event_bonus, const EstimatorBase& estimator);

}  // namespace sekai
