#pragma once

#include <span>

#include "sekai/card.h"
#include "sekai/config.h"
#include "sekai/estimator.h"

namespace sekai {

struct CardPruningOptions {
  const Estimator& estimator;
  int min_ep = 0;

  int max_power = kMaxPower;
  int min_power_in_max_power_team = 0;

  int max_bonus = kMaxEventBonus;
  int min_bonus_in_max_bonus_team = 0;

  int max_skill = kMaxSkillValue;

  // TODO: consider pruning by 1st order approximation as well
};

struct ShouldPrune {
 public:
  explicit ShouldPrune(const CardPruningOptions& opts);

  bool operator()(const Card* card) const;

 private:
  CardPruningOptions opts_;
  int min_power_ = 0;
  int min_bonus_ = 0;
};

// Returns the number of cards removed.
template <typename T>
int PruneCards(const CardPruningOptions& opts, T& range) {
  std::size_t size_before = range.size();
  std::erase_if(range, ShouldPrune{opts});
  return size_before - range.size();
}

}  // namespace sekai
