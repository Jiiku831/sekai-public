#include "sekai/card.h"

#include <span>

#include "absl/base/nullability.h"
#include "sekai/array_size.h"
#include "sekai/character.h"
#include "sekai/db/master_db.h"
#include "sekai/db/proto/all.h"
#include "sekai/event_bonus.h"
#include "sekai/profile_bonus.h"
#include "sekai/proto/card.pb.h"
#include "sekai/proto/card_state.pb.h"

namespace sekai {

namespace {

using ::sekai::db::MasterDb;

template <typename T>
Eigen::Vector3i GetBonusPower(const T& msg) {
  return Eigen::Vector3i(msg.power1_bonus_fixed(), msg.power2_bonus_fixed(),
                         msg.power3_bonus_fixed());
}

int GetMaxSkill(const db::Skill& skill, int skill_level) {
  int skill_value = 0;
  for (const db::SkillEffect& effect : skill.skill_effects()) {
    auto type = effect.skill_effect_type();
    if (type != db::SkillEffect::SCORE_UP && type != db::SkillEffect::SCORE_UP_CONDITION_LIFE &&
        type != db::SkillEffect::SCORE_UP_KEEP)
      continue;
    for (const db::SkillEffect::Details& details : effect.skill_effect_details()) {
      if (details.level() != skill_level) continue;
      skill_value = std::max(details.activate_effect_value(), skill_value);
    }
  }
  return skill_value;
}

Eigen::Vector3i GetBasePower(const db::Card& card, int level) {
  Eigen::Vector3i power(0, 0, 0);
  for (const db::Card::CardParameter& param : card.card_parameters()) {
    if (param.card_level() != level) continue;
    switch (param.card_parameter_type()) {
      case db::CARD_PARAM_1:
        power(0) = param.power();
        break;
      case db::CARD_PARAM_2:
        power(1) = param.power();
        break;
      case db::CARD_PARAM_3:
        power(2) = param.power();
        break;
      default:
        break;
    }
  }
  return power;
}

Eigen::Vector3i GetSpecialTrainingBonus(const db::Card& card) {
  return Eigen::Vector3i(card.special_training_power1_bonus_fixed(),
                         card.special_training_power2_bonus_fixed(),
                         card.special_training_power3_bonus_fixed());
}

Eigen::Vector3i GetMasterRankBonus(const db::CardRarityType& rarity, int master_rank) {
  const std::vector<absl::Nonnull<const db::MasterLesson*>> lessons =
      db::MasterDb::Get().Get<db::MasterLesson>().FindAll(rarity);
  Eigen::Vector3i bonus(0, 0, 0);
  for (absl::Nonnull<const db::MasterLesson*> lesson : lessons) {
    if (lesson->master_rank() > master_rank) continue;
    bonus += GetBonusPower(*lesson);
  }
  return bonus;
}

Eigen::Vector3i GetCardEpisodeBonus(int card_id, const CardState& state) {
  const std::vector<absl::Nonnull<const db::CardEpisode*>> episodes =
      db::MasterDb::Get().Get<db::CardEpisode>().FindAll(card_id);
  Eigen::Vector3i bonus(0, 0, 0);
  for (int episode_id : state.card_episodes_read()) {
    auto it = std::find_if(episodes.begin(), episodes.end(),
                           [&episode_id](absl::Nonnull<const db::CardEpisode*> episode) {
                             return episode->id() == episode_id;
                           });
    if (it != episodes.end()) {
      bonus += GetBonusPower(**it);
    }
  }
  return bonus;
}

Eigen::Vector3i GetCardPower(const db::Card& card, const CardState& state) {
  Eigen::Vector3i power = GetBasePower(card, state.has_level() ? state.level() : 1);
  if (state.special_training()) {
    power += GetSpecialTrainingBonus(card);
  }
  power += GetMasterRankBonus(card.card_rarity_type(), state.master_rank());
  power += GetCardEpisodeBonus(card.id(), state);
  return power;
}

}  // namespace

int MaxLevelForRarity(db::CardRarityType rarity) {
  switch (rarity) {
    case db::RARITY_1:
      return 20;

    case db::RARITY_2:
      return 30;
    case db::RARITY_3:
      return 50;
    case db::RARITY_4:
    case db::RARITY_BD:
      return 60;
    default:
      ABSL_CHECK(false) << "unhandled case";
      return 0;
  }
}

bool TrainableCard(db::CardRarityType rarity) {
  return rarity == db::RARITY_3 || rarity == db::RARITY_4;
}

int UnitCount::operator()(db::Unit unit) {
  if (!unit_count_populated) {
    unit_count_populated = true;
    for (const Card* card : cards) {
      ++unit_count[card->db_primary_unit()];
    }
  }
  return unit_count[unit];
}

Card::Card(const db::Card& card, const CardState& state) : db_card_(card), state_(state) {
  card_id_ = card.id();
  attr_.set(card.attr());
  db_attr_ = card.attr();
  character_id_ = card.character_id();
  support_unit_ = card.support_unit();
  char_unit_ = LookupCharacterUnit(card.character_id());
  if (support_unit_ != db::UNIT_NONE) {
    primary_unit_.set(support_unit_);
    db_primary_unit_ = support_unit_;
    secondary_unit_.set(char_unit_);
    db_secondary_unit_ = char_unit_;
    has_subunit_ = true;
  } else {
    primary_unit_.set(char_unit_);
    db_primary_unit_ = char_unit_;
    secondary_unit_.set(char_unit_);
    db_secondary_unit_ = char_unit_;
  }
  db_rarity_ = card.card_rarity_type();
  rarity_.set(card.card_rarity_type());

  // Card state
  power_vec_ = GetCardPower(card, state);
  power_ = power_vec_.sum();
  const db::Skill& skill = MasterDb::Get().Get<db::Skill>().FindFirst(card.skill_id());
  skill_value_ = GetMaxSkill(skill, state.has_skill_level() ? state.skill_level() : 1);
  if (!skill.skill_effects().empty() &&
      skill.skill_effects(0).skill_enhance().skill_enhance_type() ==
          db::SkillEnhance::SUB_UNIT_SCORE_UP) {
    db_skill_enhance_unit_ =
        skill.skill_effects(0).skill_enhance().skill_enhance_condition().unit();
    skill_enhance_unit_ = Unit(db_skill_enhance_unit_);
    skill_enhance_value_ = skill.skill_effects(0).skill_enhance().activate_effect_value();
  }
  if (state.has_master_rank()) {
    master_rank_ = state.master_rank();
  }
  if (state.has_skill_level()) {
    skill_level_ = state.skill_level();
  }
}

int Card::SkillValue(UnitCount unit_count) const {
  if (db_skill_enhance_unit_ == db::UNIT_NONE) return skill_value_;
  int count = unit_count(db_skill_enhance_unit_);
  int factor = count == 5 ? 5 : count - 1;
  return skill_value_ + factor * skill_enhance_value_;
}

float Card::GetBonus(const EventBonus& event_bonus) const {
  return event_bonus.card_bonus(card_id_) +
         event_bonus.deck_bonus(character_id_, db_attr_, db_primary_unit_) +
         event_bonus.master_rank_bonus(db_rarity_, master_rank_) +
         event_bonus.skill_level_bonus(db_rarity_, skill_level_);
}

void Card::ApplyEventBonus(const EventBonus& event_bonus,
                           const SupportUnitEventBonus& support_bonus) {
  event_bonus_ = GetBonus(event_bonus);
  support_bonus_ = GetBonus(support_bonus);
}

void Card::ApplyProfilePowerBonus(const ProfileBonus& profile) {
  float char_power_factor = profile.char_bonus()[character_id_].rate / 100;
  float cr_power_factor = profile.cr_bonus()[character_id_].rate / 100;
  cr_power_bonus_ = (power_vec_.cast<float>() * cr_power_factor).cast<int>().sum();

  // Index: (Attr, Primary Unit, Secondary Unit)
  for (int attr_matching : {0, 1}) {
    for (int primary_unit_matching : {0, 1}) {
      for (int secondary_unit_matching : {0, 1}) {
        float power_factor =
            char_power_factor +
            (profile.attr_bonus()[db_attr_][attr_matching] +
             std::max(profile.unit_bonus()[db_primary_unit_][primary_unit_matching],
                      profile.unit_bonus()[db_secondary_unit_][secondary_unit_matching])) /
                100;
        int area_item_bonus = (power_vec_.cast<float>() * power_factor).cast<int>().sum();
        area_item_bonus_[attr_matching][primary_unit_matching][secondary_unit_matching] =
            area_item_bonus;
        precomputed_power_[attr_matching][primary_unit_matching][secondary_unit_matching] =
            power_ + cr_power_bonus_ + area_item_bonus;
      }
    }
  }
}

CardProto Card::ToProto() const {
  CardProto proto;
  proto.set_card_id(card_id_);
  *proto.mutable_db_card() = db_card_;
  *proto.mutable_state() = state_;
  return proto;
}

CardState CreateMaxCardState(int card_id) {
  CardState state;
  const db::Card& card = MasterDb::FindFirst<db::Card>(card_id);
  state.set_level(MaxLevelForRarity(card.card_rarity_type()));
  state.set_master_rank(kMasterRankMax);
  state.set_skill_level(kSkillLevelMax);
  if (TrainableCard(card.card_rarity_type())) {
    state.set_special_training(true);
  }
  for (const db::CardEpisode* episode : MasterDb::FindAll<db::CardEpisode>(card_id)) {
    state.add_card_episodes_read(episode->id());
  }
  return state;
}

bool Card::IsUnit(db::Unit unit) const {
  return db_primary_unit_ == unit || db_secondary_unit_ == unit;
}

}  // namespace sekai
