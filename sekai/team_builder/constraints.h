#pragma once

#include <span>
#include <vector>

#include "absl/base/nullability.h"
#include "absl/strings/str_format.h"
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
  // If using this function, you must add all the lead character first or else
  // no kizuna will be added.
  void AddKizunaPair(std::pair<int, int> chars);
  void SetMinLeadSkill(int min_skill);
  void AddExcludedRarity(db::CardRarityType rarity);

  // Returns if the character set satisfies the constraint.
  bool CharacterSetSatisfiesConstraint(Character chars) const;

  // Returns if the character is eligible to be lead.
  Character GetCharactersEligibleForLead(Character chars_present) const;

  bool RaritiesSatisfiesConstraint(CardRarityType rarities) const {
    return (excluded_rarities_ & rarities).none();
  }

  // Returns a new pool with unwanted rarities removed.
  std::vector<absl::Nonnull<const Card*>> FilterCardPool(
      std::span<absl::Nonnull<const Card*> const> pool) const;

  bool LeadSkillSatisfiesConstraint(int lead_skill) const { return lead_skill >= min_lead_skill_; }

  bool HasLeadSkillConstraint() const { return min_lead_skill_ > 0; }

  int min_lead_skill() const { return min_lead_skill_; }
  std::span<const int> lead_char_ids() const { return lead_chars_vector_; }
  std::span<const Character> kizuna_pairs() const { return kizuna_pairs_; }
  std::span<const std::pair<int, int>> kizuna_std_pairs() const { return kizuna_std_pairs_; }

  template <typename Sink>
  friend void AbslStringify(Sink& sink, const Constraints& constraints) {
    absl::Format(
        &sink,
        "Constraints\nRequired leads: %s\nRequired kizuna: %s\nExcluded rarity: %v\nMin Skill: %d",
        absl::StrJoin(constraints.lead_chars_vector_, ", "),
        absl::StrJoin(constraints.kizuna_std_pairs_, ", ", absl::PairFormatter(":")),
        constraints.excluded_rarities_, constraints.min_lead_skill_);
  }

 private:
  bool empty_ = true;
  Character lead_chars_;
  std::vector<int> lead_chars_vector_;
  std::vector<Character> kizuna_pairs_;
  std::vector<std::pair<int, int>> kizuna_std_pairs_;
  int min_lead_skill_ = 0;
  CardRarityType excluded_rarities_;

  void UpdateEmpty();
};

}  // namespace sekai
