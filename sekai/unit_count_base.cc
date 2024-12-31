#include "sekai/unit_count_base.h"

#include "absl/log/absl_check.h"
#include "sekai/config.h"
#include "sekai/db/proto/enums.pb.h"

namespace sekai {

int UnitCountBase::CharacterCount(db::Unit unit) {
  if (!unit_count_populated_) {
    PopulateUnitCount();
  }
  return unit_count_[unit];
}

bool UnitCountBase::UnitPresent(db::Unit unit) {
  if (!unit_count_populated_) {
    PopulateUnitCount();
  }
  return units_present_.test(unit);
}

int UnitCountBase::UnitCount() {
  if (!unit_count_populated_) {
    PopulateUnitCount();
  }
  return units_present_.count();
}

float UnitCountBase::SkillValueTotal() {
  if (!unit_count_populated_) {
    PopulateUnitCount();
  }
  return skill_value_total_;
}

float UnitCountBase::ReferenceBoostAverageCappedSkillValue(int card_index) {
  if (!unit_count_populated_) {
    PopulateUnitCount();
  }
  ABSL_CHECK_GE(card_index, 0);
  ABSL_CHECK_LT(card_index, reference_boost_average_capped_skill_value_.size());
  return reference_boost_average_capped_skill_value_[card_index];
}

}  // namespace sekai
