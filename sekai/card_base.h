#pragma once

#include "absl/base/attributes.h"
#include "sekai/bitset.h"
#include "sekai/db/proto/all.h"
#include "sekai/proto/card_state.pb.h"

namespace sekai {

class CardBase {
 public:
  explicit CardBase(const db::Card& card ABSL_ATTRIBUTE_LIFETIME_BOUND);

  db::Attr db_attr() const { return db_attr_; }
  db::Unit db_primary_unit() const { return db_primary_unit_; }
  db::Unit db_secondary_unit() const { return db_secondary_unit_; }
  const Attr& attr() const { return attr_; }
  const Unit& primary_unit() const { return primary_unit_; }
  const Unit& secondary_unit() const { return secondary_unit_; }
  bool has_subunit() const { return has_subunit_; }
  bool IsUnit(db::Unit unit) const;

 protected:
  // Card attributes
  const db::Card& db_card_;
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
  float skill_cap_ = -1;
};

}  // namespace sekai
