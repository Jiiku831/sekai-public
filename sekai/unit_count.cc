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
    float other_skill_value_total = 0;
    for (const Card* other_card : cards_) {
      if (card == other_card) continue;
      other_skill_value_total +=
          other_card->ReferenceBoostCappedMaxSkillValue(card->skill().level());
    }
    if (cards_.size() > 1) {
      reference_boost_average_capped_skill_value_.push_back(other_skill_value_total /
                                                            (cards_.size() - 1));
    } else {
      reference_boost_average_capped_skill_value_.push_back(0);
    }
  }
}

}  // namespace sekai
