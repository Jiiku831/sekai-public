#include "sekai/team_builder/neighbor_teams.h"

#include <algorithm>
#include <bitset>
#include <numeric>
#include <random>
#include <vector>

#include "sekai/array_size.h"
#include "sekai/bitset.h"
#include "sekai/team.h"
#include "sekai/team_builder/constraints.h"

namespace sekai {
namespace {

constexpr int kMaxAttempts = 100;

class SimpleNeighborState {
 public:
  SimpleNeighborState(Team team, std::size_t index_to_replace,
                      const Constraints& constraints ABSL_ATTRIBUTE_LIFETIME_BOUND)
      : index_to_replace_(index_to_replace),
        cards_(team.cards().begin(), team.cards().end()),
        constraints_(constraints) {
    for (std::size_t i = 0; i < cards_.size(); ++i) {
      const Card* card = cards_[i];
      cards_present_.set(card->card_id());
      if (i != index_to_replace) {
        chars_present_.set(card->character_id());
      }
    }
  }

  bool ReplacementViable(const Card* card) const {
    if (cards_present_.test(card->card_id()) || chars_present_.test(card->character_id())) {
      return false;
    }
    Character new_chars = chars_present_;
    new_chars.set(card->character_id());
    return constraints_.CharacterSetSatisfiesConstraint(new_chars);
  }

  Team MakeNewTeam(const Card* new_card) {
    cards_[index_to_replace_] = new_card;
    return Team{cards_};
  }

 private:
  std::size_t index_to_replace_;
  std::vector<const Card*> cards_;
  std::bitset<kCardArraySize> cards_present_;
  Character chars_present_;
  const Constraints& constraints_;
};

std::optional<Team> MakeTeamIfViable(const Card* candidate, const Constraints& constraints,
                                     SimpleNeighborState& state) {
  if (!state.ReplacementViable(candidate)) {
    return std::nullopt;
  }
  Team new_team = state.MakeNewTeam(candidate);
  if (!new_team.SatisfiesConstraints(constraints)) {
    return std::nullopt;
  }
  return new_team;
}

}  // namespace

std::vector<Team> SimpleNeighbors::GetNeighbors(const Team& team) const {
  std::vector<Team> teams;
  for (std::size_t i = 0; i < team.cards().size(); ++i) {
    SimpleNeighborState state{team, i, constraints_};
    for (const Card* candidate : pool_) {
      std::optional<Team> new_team = MakeTeamIfViable(candidate, constraints_, state);
      if (new_team.has_value()) {
        teams.push_back(*new_team);
      }
    }
  }
  return teams;
}

std::optional<Team> SimpleNeighbors::GetRandomNeighbor(const Team& team, std::mt19937& g) const {
  std::vector<std::size_t> positions(team.cards().size());
  std::iota(positions.begin(), positions.end(), 0);
  std::shuffle(positions.begin(), positions.end(), g);
  std::uniform_int_distribution<int> card_dist(0, pool_.size() - 1);

  for (std::size_t i = 0; i < team.cards().size(); ++i) {
    SimpleNeighborState state{team, positions[i], constraints_};
    for (int j = 0; j < kMaxAttempts; ++j) {
      const Card* candidate = pool_[card_dist(g)];
      std::optional<Team> new_team = MakeTeamIfViable(candidate, constraints_, state);
      if (new_team.has_value()) {
        return new_team;
      }
    }
  }
  // TODO: maybe fallback to exhaustive gen
  return std::nullopt;
}

}  // namespace sekai
