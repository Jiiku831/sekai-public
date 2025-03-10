#pragma once

#include <span>
#include <vector>

#include <Eigen/Eigen>

#include "absl/container/flat_hash_map.h"
#include "sekai/bitset.h"
#include "sekai/card.h"
#include "sekai/config.h"
#include "sekai/estimator_base.h"
#include "sekai/event_bonus.h"
#include "sekai/profile.h"
#include "sekai/proto/team.pb.h"
#include "sekai/proto/world_bloom.pb.h"
#include "sekai/team_builder/constraints.h"
#include "sekai/unit_count.h"

namespace sekai {

// A team is just a collection of cards. No validation is done on this. The
// cards within a team must outlive the object.
class Team {
 public:
  // Cards must outlive this object.
  explicit Team(std::span<const Card* const> cards);
  explicit Team(std::span<const Card> cards);

  std::span<const Card* const> cards() const { return cards_; }

  int Power(const Profile& profile) const;
  static constexpr int kPowerDetailComponents = 6;
  Eigen::Vector<int, kPowerDetailComponents> PowerDetailed(const Profile& profile) const;
  int MinPowerContrib(const Profile& profile) const;

  float EventBonus(const class EventBonus& event_bonus) const;
  float MinBonusContrib() const;

  float SkillValue() const;
  float MaxSkillValue() const;
  void ReorderTeamForOptimalSkillValue();
  void ReorderTeamForOptimalSkillValue(const Constraints& constraints);
  void ReorderTeamForOptimalSkillValue(Character eligible_leads);
  void ReorderTeamForKizuna(std::span<const Character> kizuna_pairs);
  std::vector<int> GetSkillValues() const;

  struct SkillValueDetail {
    float lead_skill;
    float skill_value;
  };
  SkillValueDetail ConstrainedMaxSkillValue(const Constraints& constraints) const;
  SkillValueDetail ConstrainedMaxSkillValue(Character eligible_leads) const;

  Character chars_present() const { return chars_present_; }
  bool SatisfiesConstraints(const Constraints& constraints) const;

  TeamProto ToProto(const Profile& profile, const class EventBonus& event_bonus,
                    const EstimatorBase& estimator) const;

  void FillSupportCards(std::span<const Card* const> sorted_pool,
                        WorldBloomVersion version = kDefaultWorldBloomVersion);

  // Guaranteed to be sorted.
  struct SoloEbiBasePoints {
    std::vector<int> possible_points;
    absl::flat_hash_map<int, int> score_threshold;
  };
  SoloEbiBasePoints GetSoloEbiBasePoints(const Profile& profile,
                                         const class EventBonus& event_bonus, float accuracy) const;

  template <typename Sink>
  friend void AbslStringify(Sink& sink, const Team& team) {
    absl::Format(
        &sink, "Team\n%s%s", absl::StrJoin(team.cards(), "\n", absl::DereferenceFormatter()),
        team.support_cards_.empty()
            ? ""
            : (absl::StrCat("\nSupport Cards\n", absl::StrJoin(team.support_cards_, "\n",
                                                               absl::DereferenceFormatter()))));
  }

 private:
  int CardPowerContrib(const Card* card) const;
  float CardBonusContrib(const Card* card) const;
  float CardSkillContrib(const Card* card, int card_index, UnitCount& unit_count) const;
  float EventBonus() const { return event_bonus_base_ + support_bonus_base_; }

  std::vector<const Card*> cards_;
  std::vector<const Card*> support_cards_;
  Attr attrs_;
  int attrs_count_;
  Unit primary_units_;
  Unit secondary_units_;
  float event_bonus_base_ = 0;
  float support_bonus_base_ = 0;
  bool attr_match_ = false;
  bool primary_units_match_ = false;
  bool secondary_units_match_ = false;
  Character chars_present_;
  CardRarityType rarities_present_;
};

}  // namespace sekai
