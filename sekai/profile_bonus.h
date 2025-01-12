#pragma once

#include <cstddef>
#include <span>

#include "absl/strings/str_format.h"

namespace sekai {

struct BonusRate {
  float rate = 0;
  float matching_rate = 0;

  float operator[](std::size_t index) const {
    if (index == 0) {
      return rate;
    }
    return matching_rate;
  }

  template <typename Sink>
  friend void AbslStringify(Sink& sink, const BonusRate& rate) {
    absl::Format(&sink, "BonusRate(%f, %f)", rate.rate, rate.matching_rate);
  }

  friend void PrintTo(const BonusRate& rate, std::ostream* os) {
    *os << "BonusRate(" << rate.rate << "," << rate.matching_rate << ")";
  }
};

class ProfileBonus {
 public:
  virtual std::span<const BonusRate> attr_bonus() const = 0;
  virtual std::span<const BonusRate> char_bonus() const = 0;
  virtual std::span<const BonusRate> cr_bonus() const = 0;
  virtual std::span<const BonusRate> unit_bonus() const = 0;
  virtual std::span<const int> mysekai_fixture_bonus() const = 0;
  virtual std::span<const float> mysekai_gate_bonus() const = 0;
  virtual int bonus_power() const = 0;
  virtual int character_rank(int char_id) const = 0;
};

}  // namespace sekai
