#include "sekai/unit_count_base.h"

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

float UnitCountBase::ReferenceBoostCappedSkillValueTotal() {
  if (!unit_count_populated_) {
    PopulateUnitCount();
  }
  return reference_boost_capped_skill_value_total_;
}

}  // namespace sekai
