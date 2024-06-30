#include "sekai/unit_count.h"

#include <array>
#include <span>

#include "absl/base/attributes.h"
#include "sekai/bitset.h"
#include "sekai/card.h"
#include "sekai/db/proto/enums.pb.h"

class Card;

namespace sekai {

void UnitCount::PopulateUnitCount() {
  unit_count_populated_ = true;
  for (const Card* card : cards_) {
    ++unit_count_[card->db_primary_unit()];
    units_present_.set(card->db_primary_unit());
    skill_value_total_ += card->MaxSkillValue();
    reference_boost_capped_skill_value_total_ += card->ReferenceBoostCappedMaxSkillValue();
  }
}

}  // namespace sekai
