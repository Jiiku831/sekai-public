#include "sekai/team_builder/constraints.h"

#include <span>

#include "absl/base/nullability.h"
#include "absl/log/absl_check.h"
#include "sekai/bitset.h"
#include "sekai/card.h"

namespace sekai {

Constraints::Constraints(const TeamConstraints& constraints) {
  for (int lead_char_id : constraints.lead_char_ids()) {
    AddLeadChar(lead_char_id);
  }

  for (const TeamConstraints::KizunaPair& kizuna_pair : constraints.kizuna_pairs()) {
    AddKizunaPair(std::make_pair(kizuna_pair.char_1(), kizuna_pair.char_2()));
  }

  SetMinLeadSkill(constraints.min_lead_skill());

  for (int rarity : constraints.exclude_rarities()) {
    AddExcludedRarity(static_cast<db::CardRarityType>(rarity));
  }
}

void Constraints::AddLeadChar(int char_id) {
  ABSL_CHECK_GT(char_id, 0) << "invalid char_id";
  ABSL_CHECK_LT(char_id, static_cast<int>(lead_chars_.size())) << "invalid char_id";
  lead_chars_.set(char_id);
  lead_chars_vector_.push_back(char_id);
  UpdateEmpty();
}

void Constraints::AddKizunaPair(std::pair<int, int> chars) {
  ABSL_CHECK_GT(chars.first, 0) << "invalid char1";
  ABSL_CHECK_LT(chars.first, static_cast<int>(lead_chars_.size())) << "invalid char1";
  ABSL_CHECK_GT(chars.second, 0) << "invalid char2";
  ABSL_CHECK_LT(chars.second, static_cast<int>(lead_chars_.size())) << "invalid char2";
  ABSL_CHECK_NE(chars.first, chars.second) << "chars in kizuna pair must be distinct";
  if (!lead_chars_.test(chars.first) && !lead_chars_.test(chars.second)) {
    // Never able to satisfy this kizuna pairing due to lead constraint.
    return;
  }
  Character kizuna_pair;
  kizuna_pair.set(chars.first);
  kizuna_pair.set(chars.second);
  kizuna_pairs_.push_back(kizuna_pair);
  kizuna_std_pairs_.push_back(chars);
  UpdateEmpty();
}

void Constraints::SetMinLeadSkill(int min_skill) {
  min_lead_skill_ = min_skill;
  UpdateEmpty();
}

void Constraints::AddExcludedRarity(db::CardRarityType rarity) {
  excluded_rarities_.set(rarity);
  UpdateEmpty();
}

void Constraints::UpdateEmpty() {
  empty_ = (lead_chars_.none() || lead_chars_.count() == lead_chars_.size() - 1) &&
           kizuna_pairs_.empty() && min_lead_skill_ == 0 && excluded_rarities_.none();
}

bool Constraints::CharacterSetSatisfiesConstraint(Character chars) const {
  if (empty_) return true;
  if (lead_chars_.any() && (lead_chars_ & chars).none()) return false;
  if (kizuna_pairs_.empty()) return true;
  for (const Character& kizuna_pair : kizuna_pairs_) {
    if ((kizuna_pair & chars).count() == 2) {
      return true;
    }
  }
  return false;
}

Character Constraints::GetCharactersEligibleForLead(Character chars_present) const {
  Character leads;
  if (empty_ || lead_chars_.none()) {
    leads.set();
  }
  leads |= lead_chars_;
  leads.reset(0);

  Character leads_from_kizuna;
  for (const Character& kizuna_pair : kizuna_pairs_) {
    if ((kizuna_pair & chars_present).count() == 2) {
      leads_from_kizuna |= kizuna_pair;
    }
  }
  if (!kizuna_pairs_.empty()) {
    leads &= leads_from_kizuna;
  }

  return leads;
}

std::vector<absl::Nonnull<const Card*>> Constraints::FilterCardPool(
    std::span<absl::Nonnull<const Card*> const> pool) const {
  std::vector<absl::Nonnull<const Card*>> new_pool;
  for (const Card* card : pool) {
    if (excluded_rarities_.test(card->db_rarity())) continue;
    new_pool.push_back(card);
  }
  return new_pool;
}

}  // namespace sekai
