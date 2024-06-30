#pragma once

#include <span>

#include <Eigen/Eigen>

#include "sekai/bitset.h"
#include "sekai/card_base.h"
#include "sekai/config.h"
#include "sekai/db/proto/all.h"
#include "sekai/event_bonus.h"
#include "sekai/profile_bonus.h"
#include "sekai/proto/card.pb.h"
#include "sekai/proto/card_state.pb.h"
#include "sekai/skill.h"
#include "sekai/unit_count_base.h"

namespace sekai {

int MaxLevelForRarity(db::CardRarityType rarity);
bool TrainableCard(db::CardRarityType rarity);
CardState CreateMaxCardState(int card_id);

class Card : public CardBase {
 public:
  Card(const db::Card& card, const CardState& state);

  int card_id() const { return card_id_; }
  int character_id() const { return character_id_; }

  float SkillValue(UnitCountBase& unit_count) const;
  float MaxSkillValue() const;
  float ReferenceBoostCappedMaxSkillValue() const {
    return std::min(kReferenceScoreBoostCap, MaxSkillValue());
  }

  void ApplyEventBonus(const EventBonus& event_bonus, const SupportUnitEventBonus& support_bonus);
  void ApplyEventBonus(const EventBonus& event_bonus) {
    ApplyEventBonus(event_bonus, SupportUnitEventBonus{});
  }

  void ApplyProfilePowerBonus(const ProfileBonus& profile);

  float event_bonus() const { return event_bonus_; }
  float support_bonus() const { return support_bonus_; }

  const Eigen::Vector3i& power_vec() const { return power_vec_; }
  int power() const { return power_; }
  int cr_power_bonus() const { return cr_power_bonus_; }
  int area_item_power_bonus(bool attr_match, bool primary_unit_match,
                            bool secondary_unit_match) const {
    return area_item_bonus_[attr_match][primary_unit_match][secondary_unit_match];
  }
  int precomputed_power(bool attr_match, bool primary_unit_match, bool secondary_unit_match) const {
    return precomputed_power_[attr_match][primary_unit_match][secondary_unit_match];
  }
  int max_power() const { return precomputed_power(true, true, true); }

  db::CardRarityType db_rarity() const { return db_rarity_; }

  CardProto ToProto(UnitCountBase& unit_count) const;

 private:
  // Card state
  CardState state_;
  int master_rank_ = 0;
  int skill_level_ = 1;

  // Derived stats (power, skill, bonus) from card state
  Eigen::Vector3i power_vec_{0, 0, 0};
  int power_ = 0;

  Skill skill_;
  Skill secondary_skill_;
  bool has_secondary_skill_ = false;

  float GetBonus(const EventBonus& event_bonus) const;
  float event_bonus_ = 0;
  float support_bonus_ = 0;

  // Power bonus
  int cr_power_bonus_ = 0;
  // Index: (Attr, Primary Unit, Secondary Unit)
  std::array<std::array<std::array<int, 2>, 2>, 2> area_item_bonus_{};
  std::array<std::array<std::array<int, 2>, 2>, 2> precomputed_power_{};
};

}  // namespace sekai
