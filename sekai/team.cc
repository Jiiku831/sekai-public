#include "sekai/team.h"

#include <cstddef>
#include <limits>
#include <span>
#include <tuple>

#include <Eigen/Eigen>

#include "sekai/array_size.h"
#include "sekai/bitset.h"
#include "sekai/card.h"
#include "sekai/config.h"
#include "sekai/db/master_db.h"
#include "sekai/db/proto/all.h"
#include "sekai/event_bonus.h"
#include "sekai/profile.h"
#include "sekai/proto/card.pb.h"
#include "sekai/proto/team.pb.h"
#include "sekai/team_builder/constraints.h"
#include "sekai/unit_count.h"

namespace sekai {

Team::Team(std::span<const Card* const> cards) {
  cards_.reserve(cards.size());
  for (const Card* card : cards) {
    cards_.push_back(card);
    primary_units_ |= card->primary_unit();
    secondary_units_ |= card->secondary_unit();
    attrs_ |= card->attr();
    event_bonus_base_ += CardBonusContrib(card);
    chars_present_.set(card->character_id());
    rarities_present_.set(card->db_rarity());
  }
  attrs_count_ = attrs_.count();
  attr_match_ = attrs_count_ == 1;
  primary_units_match_ = (primary_units_.count() == 1);
  secondary_units_match_ = (secondary_units_.count() == 1);
}

int Team::CardPowerContrib(const Card* card) const {
  return card->precomputed_power(attr_match_, primary_units_match_, secondary_units_match_);
}

float Team::CardBonusContrib(const Card* card) const { return card->event_bonus(); }

float Team::CardSkillContrib(const Card* card, UnitCount& unit_count) const {
  return card->SkillValue(unit_count);
}

int Team::Power(const Profile& profile) const {
  int power = profile.bonus_power();

  for (const Card* card : cards_) {
    power += CardPowerContrib(card);
  }

  return power;
}

Eigen::Vector4i Team::PowerDetailed(const Profile& profile) const {
  int base_power = 0;
  int area_item_bonus = 0;
  int character_rank_bonus = 0;

  bool attr_match = (attrs_.count() == 1);
  bool primary_units_match = (primary_units_.count() == 1);
  bool secondary_units_match = (secondary_units_.count() == 1);

  for (const Card* card : cards_) {
    base_power += card->power();
    area_item_bonus +=
        card->area_item_power_bonus(attr_match, primary_units_match, secondary_units_match);
    character_rank_bonus += card->cr_power_bonus();
  }

  return Eigen::Vector4i{
      base_power,
      area_item_bonus,
      character_rank_bonus,
      profile.bonus_power(),
  };
}

int Team::MinPowerContrib(const Profile& profile) const {
  int min_power = std::numeric_limits<int>::max();
  for (const Card* card : cards_) {
    int power = CardPowerContrib(card);
    if (power < min_power) {
      min_power = power;
    }
  }
  return min_power;
}

float Team::EventBonus(const class EventBonus& event_bonus) const {
  if (!event_bonus.has_diff_attr_bonus()) {
    return EventBonus();
  }
  return EventBonus() + event_bonus.diff_attr_bonus(attrs_count_);
}

float Team::MinBonusContrib() const {
  float min_bonus = std::numeric_limits<float>::infinity();
  for (const Card* card : cards_) {
    min_bonus = std::min(min_bonus, CardBonusContrib(card));
  }
  return min_bonus;
  // return std::ranges::min(
  //     cards_ | std::views::transform(CardBonusContrib));
}

float Team::SkillValue() const {
  float skill_value = 0;
  int skill_factor = 1;
  UnitCount unit_count(cards_);
  for (const Card* card : cards_) {
    skill_value += CardSkillContrib(card, unit_count) / skill_factor;
    skill_factor = 5;
  }
  return skill_value;
}

float Team::MaxSkillValue() const {
  float total_skill = 0;
  float max_skill = 0;
  UnitCount unit_count(cards_);
  for (const Card* card : cards_) {
    float skill_value = CardSkillContrib(card, unit_count);
    max_skill = std::max(skill_value, max_skill);
    total_skill += skill_value;
  }
  return max_skill + (total_skill - max_skill) / 5;
}

Team::SkillValueDetail Team::ConstrainedMaxSkillValue(Character eligible_leads) const {
  float total_skill = 0;
  float max_skill = 0;
  UnitCount unit_count(cards_);
  for (const Card* card : cards_) {
    float skill_value = CardSkillContrib(card, unit_count);
    total_skill += skill_value;
    if (eligible_leads.test(card->character_id())) {
      max_skill = std::max(skill_value, max_skill);
    }
  }
  return {
      .lead_skill = max_skill,
      .skill_value = max_skill + (total_skill - max_skill) / 5,
  };
}

Team::SkillValueDetail Team::ConstrainedMaxSkillValue(const Constraints& constraints) const {
  Character eligible_leads = constraints.GetCharactersEligibleForLead(chars_present_);
  return ConstrainedMaxSkillValue(eligible_leads);
}

void Team::ReorderTeamForOptimalSkillValue() {
  Character eligible_leads;
  eligible_leads.set();
  ReorderTeamForOptimalSkillValue(eligible_leads);
}

void Team::ReorderTeamForOptimalSkillValue(const Constraints& constraints) {
  Character eligible_leads = constraints.GetCharactersEligibleForLead(chars_present_);
  ReorderTeamForOptimalSkillValue(eligible_leads);
  ReorderTeamForKizuna(constraints.kizuna_pairs());
}

void Team::ReorderTeamForOptimalSkillValue(Character eligible_leads) {
  int max_skill_index = 0;
  float max_skill = 0;
  UnitCount unit_count(cards_);
  for (std::size_t i = 0; i < cards_.size(); ++i) {
    float skill_value = CardSkillContrib(cards_[i], unit_count);
    if (skill_value > max_skill && eligible_leads.test(cards_[i]->character_id())) {
      max_skill_index = i;
      max_skill = skill_value;
    }
  }
  if (max_skill_index > 0) {
    std::swap(cards_[0], cards_[max_skill_index]);
  }
}

void Team::ReorderTeamForKizuna(std::span<const Character> kizuna_pairs) {
  int lead = cards_[0]->character_id();
  for (std::size_t i = 2; i < cards_.size(); ++i) {
    Character candidate_pair;
    candidate_pair.set(lead);
    candidate_pair.set(cards_[i]->character_id());
    for (const Character kizuna_pair : kizuna_pairs) {
      if ((kizuna_pair & candidate_pair).count() == 2) {
        std::swap(cards_[1], cards_[i]);
      }
    }
  }
}

std::vector<int> Team::GetSkillValues() const {
  std::vector<int> skills;
  UnitCount unit_count(cards_);
  for (const Card* card : cards_) {
    skills.push_back(CardSkillContrib(card, unit_count));
  }
  return skills;
}

bool Team::SatisfiesConstraints(const Constraints& constraints) const {
  return constraints.CharacterSetSatisfiesConstraint(chars_present_) &&
         ConstrainedMaxSkillValue(constraints).lead_skill >= constraints.min_lead_skill() &&
         constraints.RaritiesSatisfiesConstraint(rarities_present_);
}

TeamProto Team::ToProto(const Profile& profile, const class EventBonus& event_bonus,
                        const EstimatorBase& estimator) const {
  TeamProto team;
  UnitCount unit_count(cards_);
  for (const Card* card : cards_) {
    CardProto* card_proto = team.add_cards();
    *card_proto = card->ToProto(unit_count);
    card_proto->set_team_power_contrib(CardPowerContrib(card));
    card_proto->set_team_bonus_contrib(CardBonusContrib(card));
    card_proto->set_team_skill_contrib(CardSkillContrib(card, unit_count));
  }
  float support_bonus = 0;
  for (const Card* card : support_cards_) {
    CardProto* card_proto = team.add_support_cards();
    *card_proto = card->ToProto(unit_count);
    card_proto->set_team_bonus_contrib(card->support_bonus());
    support_bonus += card->support_bonus();
  }
  team.set_power(Power(profile));
  team.set_event_bonus(EventBonus(event_bonus));
  team.set_skill_value(SkillValue());
  estimator.AnnotateTeamProto(profile, event_bonus, *this, team);
  Eigen::Vector4i power_detailed = PowerDetailed(profile);
  *team.mutable_power_detailed() = {power_detailed.begin(), power_detailed.end()};
  if (!support_cards_.empty()) {
    team.set_support_bonus(support_bonus);
    team.set_main_bonus(team.event_bonus() - support_bonus);
  }
  return team;
}

void Team::FillSupportCards(std::span<const Card* const> sorted_pool, WorldBloomVersion version) {
  const WorldBloomConfig& wl_config = GetWorldBloomConfig(version);
  std::bitset<kCardArraySize> cards_present;
  support_cards_.clear();
  support_bonus_base_ = 0;
  for (const Card* card : cards_) {
    cards_present.set(card->card_id());
  }
  for (std::size_t i = 0;
       i < sorted_pool.size() && support_cards_.size() < wl_config.support_team_size(); ++i) {
    const Card* candidate_card = sorted_pool[i];
    if (cards_present.test(candidate_card->card_id())) {
      continue;
    }
    support_cards_.push_back(candidate_card);
    support_bonus_base_ += candidate_card->support_bonus();
  }
}

}  // namespace sekai
