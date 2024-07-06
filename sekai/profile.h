#pragma once

#include <array>
#include <cstddef>
#include <filesystem>
#include <span>
#include <vector>

#include "absl/base/nullability.h"
#include "absl/container/flat_hash_map.h"
#include "absl/status/status.h"
#include "absl/strings/str_format.h"
#include "sekai/card.h"
#include "sekai/db/proto/all.h"
#include "sekai/profile_bonus.h"
#include "sekai/proto/profile.pb.h"

namespace sekai {

ProfileProto EmptyProfileProto();

class Profile : public ProfileBonus {
 public:
  Profile();
  explicit Profile(const ProfileProto& profile);

  void LoadCardsFromCsv(std::filesystem::path path);
  absl::Status LoadCardsFromCsvString(std::string_view contents);
  absl::Status LoadCardsFromCsv(std::stringstream& ss);
  void ApplyEventBonus(const EventBonus& event_bonus);

  std::span<const BonusRate> attr_bonus() const override { return attr_bonus_; }
  std::span<const BonusRate> char_bonus() const override { return char_bonus_; }
  std::span<const BonusRate> cr_bonus() const override { return cr_bonus_; }
  std::span<const BonusRate> unit_bonus() const override { return unit_bonus_; }
  int bonus_power() const override { return bonus_power_; }
  int character_rank(int char_id) const override;
  std::vector<const Card*> CardPtrs() const {
    // TODO: just change team builder to use ref.
    std::vector<const Card*> out;
    out.reserve(cards_.size());
    for (const auto& [unused_id, card] : cards_) {
      out.push_back(&card);
    }
    return out;
  }
  absl::Nullable<const Card*> GetCard(int card_id) const;
  std::span<const Card* const> sorted_support() const { return sorted_support_; }

  ProfileProto CardsToProto() const;

 private:
  std::array<BonusRate, db::Attr_ARRAYSIZE> attr_bonus_;
  std::vector<BonusRate> char_bonus_;
  std::vector<BonusRate> cr_bonus_;
  std::vector<int> character_rank_;
  std::array<BonusRate, db::Unit_ARRAYSIZE> unit_bonus_;
  int bonus_power_ = 0;
  absl::flat_hash_map<int, Card> cards_;
  std::vector<const Card*> sorted_support_;
};

}  // namespace sekai
