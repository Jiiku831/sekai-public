#pragma once

#include <algorithm>
#include <array>
#include <span>

#include "absl/base/attributes.h"
#include "sekai/bitset.h"
#include "sekai/db/proto/enums.pb.h"
#include "sekai/unit_count_base.h"

namespace sekai {

class Card;

class UnitCount : public UnitCountBase {
 public:
  explicit UnitCount(std::span<const Card* const> cards ABSL_ATTRIBUTE_LIFETIME_BOUND)
      : cards_(cards) {}

 private:
  void PopulateUnitCount() override;

  std::span<const Card* const> cards_;
};

}  // namespace sekai
