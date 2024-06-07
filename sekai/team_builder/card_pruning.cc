#include "sekai/team_builder/card_pruning.h"

#include "absl/log/log.h"

namespace sekai {

ShouldPrune::ShouldPrune(const CardPruningOptions& opts) : opts_(opts) {
  if (opts.min_ep > 0) {
    min_bonus_ = opts.estimator.MinRequiredBonus(opts.min_ep, opts.max_power, opts.max_skill) -
                 opts.max_bonus + opts.min_bonus_in_max_bonus_team;
    min_power_ = opts.estimator.MinRequiredPower(opts.min_ep, opts.max_bonus, opts.max_skill) -
                 opts.max_power + opts.min_power_in_max_power_team;
    if (min_power_ <= 0) min_power_ = 0;
    if (min_bonus_ <= 0) min_bonus_ = 0;
  }
}

bool ShouldPrune::operator()(const Card* card) const {
  if (min_bonus_ == 0 && min_power_ == 0) return false;

  if (card->max_power() < min_power_) return true;
  if (card->event_bonus() < min_bonus_) return true;

  return false;
}

}  // namespace sekai
