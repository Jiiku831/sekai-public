#pragma once

#include <optional>
#include <span>
#include <vector>

#include "absl/base/nullability.h"
#include "sekai/bitset.h"
#include "sekai/card.h"
#include "sekai/db/proto/enums.pb.h"
#include "sekai/proto/constraints.pb.h"

namespace sekai {

class Constraints {
 public:
  Constraints() {}
  explicit Constraints(const TeamConstraints& constraints);

  bool empty() const { return empty_; }

  void AddLeadChar(int char_id);
  void AddKizunaPair(std::pair<int, int> chars);
  void SetMinLeadSkill(int min_skill);
  void AddExcludedRarity(db::CardRarityType rarity);

  // Returns if the character set satisfies the constraint.
  bool CharacterSetSatisfiesConstraint(Character chars) const;

  // Returns if the character is eligible to be lead.
  Character GetCharactersEligibleForLead(Character chars_present) const;

  // Returns a new pool with unwanted rarities removed.
  std::vector<absl::Nonnull<const Card*>> FilterCardPool(
      std::span<absl::Nonnull<const Card*> const> pool) const;

  bool LeadSkillSatisfiesConstraint(int lead_skill) const { return lead_skill >= min_lead_skill_; }

  bool HasLeadSkillConstraint() const { return min_lead_skill_ > 0; }

 private:
  bool empty_ = true;
  Character lead_chars_;
  std::vector<Character> kizuna_pairs_;
  int min_lead_skill_ = 0;
  CardRarityType excluded_rarities_;

  void UpdateEmpty();
};

}  // namespace sekai
