#include "sekai/team_builder/skill_selector.h"

#include <cstddef>

#include "absl/base/nullability.h"
#include "sekai/card.h"
#include "sekai/combinations.h"
#include "sekai/estimator_base.h"
#include "sekai/event_bonus.h"
#include "sekai/profile.h"
#include "sekai/team.h"

namespace sekai {

Team OptimizeSkillSelection(std::span<const Card* const> cards, const Profile& profile,
                            const EventBonus& event_bonus, const EstimatorBase& estimator) {
  std::vector<std::vector<const Card*>> pools(5);
  for (std::size_t i = 0; i < std::min(cards.size(), pools.size()); ++i) {
    const Card* regular_card = profile.GetCard(cards[i]->card_id());
    if (regular_card == nullptr) {
      // Should only happen in tests. In this case, just skip optimization.
      pools[i].push_back(cards[i]);
      continue;
    }
    pools[i].push_back(profile.GetCard(cards[i]->card_id()));
    absl::Nullable<const Card*> secondary_card = profile.GetSecondaryCard(cards[i]->card_id());
    if (secondary_card != nullptr) {
      pools[i].push_back(secondary_card);
    }
  }

  Team best_team{cards};
  double best_val = estimator.ExpectedValue(profile, event_bonus, best_team);
  std::vector<std::span<const Card* const>> pool_spans(pools.size());
  for (std::size_t i = 0; i < cards.size(); ++i) {
    pool_spans[i] = pools[i];
  }
  Product<const Card*, 5>(pool_spans, [&](std::span<const Card* const> new_cards) {
    Team new_team{new_cards};
    double new_val = estimator.ExpectedValue(profile, event_bonus, new_team);
    if (new_val > best_val) {
      best_team = new_team;
      best_val = new_val;
    }
    return true;
  })();
  return best_team;
}

}  // namespace sekai
