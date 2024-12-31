#pragma once

#include <algorithm>
#include <array>
#include <span>

#include "absl/base/attributes.h"
#include "sekai/bitset.h"
#include "sekai/config.h"
#include "sekai/db/proto/enums.pb.h"

namespace sekai {

class Card;

class UnitCountBase {
 public:
  virtual ~UnitCountBase() = default;

  int CharacterCount(db::Unit unit);
  bool UnitPresent(db::Unit unit);
  int UnitCount();
  float SkillValueTotal();
  float ReferenceBoostAverageCappedSkillValue(int card_index);

 protected:
  virtual void PopulateUnitCount() = 0;

  bool unit_count_populated_ = false;
  std::array<int, db::Unit_ARRAYSIZE> unit_count_{};
  Unit units_present_;
  float skill_value_total_ = 0;
  std::vector<float> reference_boost_average_capped_skill_value_;
};

}  // namespace sekai
