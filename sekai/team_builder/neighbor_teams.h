#pragma once

#include <random>
#include <vector>

#include "absl/base/attributes.h"
#include "sekai/array_size.h"
#include "sekai/card.h"
#include "sekai/config.h"
#include "sekai/team.h"
#include "sekai/team_builder/constraints.h"

namespace sekai {

class NeighborTeams {
 public:
  virtual ~NeighborTeams() = default;

  virtual std::vector<Team> GetNeighbors(const Team& team) const = 0;
  virtual std::optional<Team> GetRandomNeighbor(const Team& team, std::mt19937& g) const = 0;
};

class SimpleNeighbors : public NeighborTeams {
 public:
  SimpleNeighbors(std::span<const Card* const> pool ABSL_ATTRIBUTE_LIFETIME_BOUND,
                  const Constraints& constraints ABSL_ATTRIBUTE_LIFETIME_BOUND)
      : pool_(pool), constraints_(constraints) {}

  std::vector<Team> GetNeighbors(const Team& team) const override;

  std::optional<Team> GetRandomNeighbor(const Team& team, std::mt19937& g) const override;

 private:
  std::span<const Card* const> pool_;
  const Constraints& constraints_;
};

}  // namespace sekai
