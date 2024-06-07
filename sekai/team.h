#pragma once

#include <span>
#include <vector>

#include <Eigen/Eigen>

#include "sekai/bitset.h"
#include "sekai/card.h"
#include "sekai/estimator.h"
#include "sekai/event_bonus.h"
#include "sekai/profile.h"
#include "sekai/proto/team.pb.h"

namespace sekai {

// A team is just a collection of cards. No validation is done on this. The
// cards within a team must outlive the object.
class Team {
 public:
  explicit Team(std::span<const Card* const> cards);

  // For testing only.
  explicit Team(std::span<const Card> cards);

  std::span<const Card* const> cards() const { return cards_; }

  int Power(const Profile& profile) const;
  Eigen::Vector4i PowerDetailed(const Profile& profile) const;

  float EventBonus() const { return event_bonus_base_; }
  float EventBonus(const class EventBonus& event_bonus) const;
  float MinBonusContrib() const;

  int SkillValue() const;
  int MaxSkillValue() const;
  void ReorderTeamForOptimalSkillValue();

  TeamProto ToProto(const Profile& profile, const class EventBonus& event_bonus,
                    const Estimator& estimator) const;

 private:
  int CardPowerContrib(const Card* card) const;
  float CardBonusContrib(const Card* card) const;
  int CardSkillContrib(const Card* card, bool& unit_count_populated,
                       std::array<int, db::Unit_ARRAYSIZE>& unit_count) const;

  std::vector<const Card*> cards_;
  Attr attrs_;
  int attrs_count_;
  Unit primary_units_;
  Unit secondary_units_;
  float event_bonus_base_ = 0;
  bool attr_match_ = false;
  bool primary_units_match_ = false;
  bool secondary_units_match_ = false;
};

}  // namespace sekai
