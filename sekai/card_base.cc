#include "sekai/card_base.h"

#include "sekai/character.h"

namespace sekai {

CardBase::CardBase(const db::Card& card ABSL_ATTRIBUTE_LIFETIME_BOUND) : db_card_(card) {
  card_id_ = card.id();
  attr_.set(card.attr());
  db_attr_ = card.attr();
  character_id_ = card.character_id();
  support_unit_ = card.support_unit();
  char_unit_ = LookupCharacterUnit(card.character_id());
  if (support_unit_ != db::UNIT_NONE) {
    primary_unit_.set(support_unit_);
    db_primary_unit_ = support_unit_;
    secondary_unit_.set(char_unit_);
    db_secondary_unit_ = char_unit_;
    has_subunit_ = true;
  } else {
    primary_unit_.set(char_unit_);
    db_primary_unit_ = char_unit_;
    secondary_unit_.set(char_unit_);
    db_secondary_unit_ = char_unit_;
  }
  db_rarity_ = card.card_rarity_type();
  rarity_.set(card.card_rarity_type());
}

bool CardBase::IsUnit(db::Unit unit) const {
  return db_primary_unit_ == unit || db_secondary_unit_ == unit;
}

}  // namespace sekai
