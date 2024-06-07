#pragma once

#include <span>

#include <Eigen/Eigen>

#include "sekai/bitset.h"
#include "sekai/db/proto/all.h"
#include "sekai/event_bonus.h"
#include "sekai/profile_bonus.h"
#include "sekai/proto/card.pb.h"
#include "sekai/proto/card_state.pb.h"

namespace sekai {

int MaxLevelForRarity(db::CardRarityType rarity);
bool TrainableCard(db::CardRarityType rarity);
CardState CreateMaxCardState(int card_id);

class Card;

struct UnitCount {
  bool& unit_count_populated;
  std::array<int, db::Unit_ARRAYSIZE>& unit_count;
  std::span<const Card* const> cards;

  int operator()(db::Unit unit);
};

class Card {
 public:
  Card(const db::Card& card, const CardState& state);

  int card_id() const { return card_id_; }
  int character_id() const { return character_id_; }

  int SkillValue(UnitCount unit_count) const;
  int raw_skill_value() const { return skill_value_; }

  db::Attr db_attr() const { return db_attr_; }
  db::Unit db_primary_unit() const { return db_primary_unit_; }
  db::Unit db_secondary_unit() const { return db_secondary_unit_; }

  const Attr& attr() const { return attr_; }
  const Unit& primary_unit() const { return primary_unit_; }
  const Unit& secondary_unit() const { return secondary_unit_; }
  bool has_subunit() const { return has_subunit_; }

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

  CardProto ToProto() const;

 private:
  // Card attributes
  const db::Card& db_card_;
  CardState state_;
  int card_id_ = 0;
  int character_id_ = 0;
  bool has_subunit_ = false;
  Attr attr_;
  db::Attr db_attr_ = db::ATTR_UNKNOWN;
  Unit primary_unit_;
  Unit secondary_unit_;
  db::Unit char_unit_ = db::UNIT_NONE;
  db::Unit db_primary_unit_ = db::UNIT_NONE;
  db::Unit support_unit_ = db::UNIT_NONE;
  db::Unit db_secondary_unit_ = db::UNIT_NONE;
  CardRarityType rarity_;
  db::CardRarityType db_rarity_ = db::RARITY_UNKNOWN;

  // Card state
  Eigen::Vector3i power_vec_{0, 0, 0};
  int power_ = 0;
  int skill_value_ = 0;
  Unit skill_enhance_unit_;
  db::Unit db_skill_enhance_unit_ = db::UNIT_NONE;
  int skill_enhance_value_;
  int master_rank_ = 0;
  int skill_level_ = 1;

  float GetBonus(const EventBonus& event_bonus) const;
  float event_bonus_ = 0;
  float support_bonus_ = 0;

  int cr_power_bonus_ = 0;
  // Index: (Attr, Primary Unit, Secondary Unit)
  std::array<std::array<std::array<int, 2>, 2>, 2> area_item_bonus_{};
  std::array<std::array<std::array<int, 2>, 2>, 2> precomputed_power_{};
};

}  // namespace sekai
