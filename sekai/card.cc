#include "sekai/card.h"

#include <array>
#include <span>

#include "absl/base/nullability.h"
#include "absl/log/absl_check.h"
#include "sekai/array_size.h"
#include "sekai/character.h"
#include "sekai/db/master_db.h"
#include "sekai/db/proto/all.h"
#include "sekai/event_bonus.h"
#include "sekai/profile_bonus.h"
#include "sekai/proto/card.pb.h"
#include "sekai/proto/card_state.pb.h"
#include "sekai/skill.h"
#include "sekai/unit_count_base.h"

namespace sekai {

namespace {

using ::sekai::db::MasterDb;

template <typename T>
Eigen::Vector3i GetBonusPower(const T& msg) {
  return Eigen::Vector3i(msg.power1_bonus_fixed(), msg.power2_bonus_fixed(),
                         msg.power3_bonus_fixed());
}

const Eigen::Vector3i& GetCanvasBonusPower(db::CardRarityType rarity) {
  static const std::array<Eigen::Vector3i, db::CardRarityType_ARRAYSIZE>* const kBonus = [] {
    auto bonuses = new std::array<Eigen::Vector3i, db::CardRarityType_ARRAYSIZE>;
    bonuses->fill(Eigen::Vector3i(0, 0, 0));
    for (const auto& bonus : MasterDb::GetAll<db::CardMySekaiCanvasBonus>()) {
      (*bonuses)[bonus.card_rarity_type()] = Eigen::Vector3i(
          bonus.power1_bonus_fixed(), bonus.power2_bonus_fixed(), bonus.power3_bonus_fixed());
    }
    return bonuses;
  }();
  ABSL_CHECK(db::CardRarityType_IsValid(rarity));
  return (*kBonus)[rarity];
}

Eigen::Vector3i GetBasePower(const db::Card& card, int level) {
  ABSL_CHECK_GT(level, 0);
  ABSL_CHECK_LT(level - 1, static_cast<int>(card.card_parameters().param1_size()));
  ABSL_CHECK_LT(level - 1, static_cast<int>(card.card_parameters().param2_size()));
  ABSL_CHECK_LT(level - 1, static_cast<int>(card.card_parameters().param3_size()));
  Eigen::Vector3i power(card.card_parameters().param1(level - 1),
                        card.card_parameters().param2(level - 1),
                        card.card_parameters().param3(level - 1));
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
  if (state.special_training() || AlwaysTrainedState(card)) {
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

bool TrainableCard(const db::Card& card) {
  return (card.card_rarity_type() == db::RARITY_3 || card.card_rarity_type() == db::RARITY_4) &&
         !AlwaysTrainedState(card);
}

bool AlwaysTrainedState(const db::Card& card) {
  return card.initial_special_training_status() == db::Card::INITIAL_SPECIAL_TRAINING_TRUE;
}

Card::Card(const db::Card& card, const CardState& state) : CardBase(card), state_(state) {
  // Card state
  if (state.has_master_rank()) {
    master_rank_ = state.master_rank();
  }
  ABSL_CHECK_GE(master_rank_, 0);
  ABSL_CHECK_LE(master_rank_, 5);
  if (state.has_skill_level()) {
    skill_level_ = state.skill_level();
  }
  ABSL_CHECK_GE(skill_level_, 1);
  ABSL_CHECK_LE(skill_level_, 4);

  power_vec_ = GetCardPower(card, state);
  if (state.canvas_crafted()) {
    power_vec_ += GetCanvasBonusPower(card.card_rarity_type());
  }
  power_ = power_vec_.sum();
  const db::Skill& skill = MasterDb::Get().Get<db::Skill>().FindFirst(card.skill_id());
  skill_ = Skill(skill, skill_level_);
  if (card.has_special_training_skill_id() && state.special_training()) {
    has_secondary_skill_ = true;
    const db::Skill& secondary_skill =
        MasterDb::Get().Get<db::Skill>().FindFirst(card.special_training_skill_id());
    secondary_skill_ = Skill(secondary_skill, skill_level_);
  }
  skill_.SetCardMaxSkillValue(MaxSkillValue());
  if (has_secondary_skill_) {
    secondary_skill_.SetCardMaxSkillValue(MaxSkillValue());
  }
}

float Card::GetBonus(const EventBonus& event_bonus) const {
  return event_bonus.GetBonus(card_id_, character_id_, db_attr_, db_primary_unit_, db_rarity_,
                              master_rank_, skill_level_);
}

void Card::ApplyEventBonus(const EventBonus& event_bonus, const EventBonus& support_bonus) {
  event_bonus_ = GetBonus(event_bonus);
  support_bonus_ = GetBonus(support_bonus);
}

void Card::ApplyProfilePowerBonus(const ProfileBonus& profile) {
  skill_.ApplyCharacterRank(profile.character_rank(character_id_));
  secondary_skill_.ApplyCharacterRank(profile.character_rank(character_id_));

  float char_power_factor = profile.char_bonus()[character_id_].rate / 100;
  float cr_power_factor = profile.cr_bonus()[character_id_].rate / 100;
  cr_power_bonus_ = (power_vec_.cast<float>() * cr_power_factor).cast<int>().sum();
  float fixture_power_factor =
      static_cast<float>(profile.mysekai_fixture_bonus()[character_id_]) / 1000;
  fixture_power_bonus_ = static_cast<int>(power_vec_.sum() * fixture_power_factor);
  float gate_power_factor = profile.mysekai_gate_bonus()[db_primary_unit_] / 100;
  gate_power_bonus_ = static_cast<int>(power_vec_.sum() * gate_power_factor);

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
            power_ + cr_power_bonus_ + area_item_bonus + fixture_power_bonus_ + gate_power_bonus_;
      }
    }
  }
}

CardProto Card::ToProto(UnitCountBase& unit_count) const {
  CardProto proto;
  proto.set_card_id(card_id_);
  *proto.mutable_db_card() = db_card_;
  *proto.mutable_state() = state_;
  if (has_secondary_skill_) {
    if (use_secondary_skill_) {
      proto.set_skill_state(CardProto::USE_SECONDARY_SKILL);
    } else {
      proto.set_skill_state(CardProto::USE_PRIMARY_SKILL);
    }
  } else {
    proto.set_skill_state(CardProto::PRIMARY_SKILL_ONLY);
  }
  return proto;
}

CardState CreateMaxCardState(int card_id) {
  CardState state;
  const db::Card& card = MasterDb::FindFirst<db::Card>(card_id);
  state.set_level(MaxLevelForRarity(card.card_rarity_type()));
  state.set_master_rank(kMasterRankMax);
  state.set_skill_level(kSkillLevelMax);
  if (TrainableCard(card)) {
    state.set_special_training(true);
  }
  for (const db::CardEpisode* episode : MasterDb::FindAll<db::CardEpisode>(card_id)) {
    state.add_card_episodes_read(episode->id());
  }
  return state;
}

float Card::SkillValue(int card_index, UnitCountBase& unit_count) const {
  if (use_secondary_skill_) {
    return secondary_skill_.SkillValue(card_index, unit_count);
  }
  return skill_.SkillValue(card_index, unit_count);
}

float Card::MaxSkillValue() const {
  if (use_secondary_skill_) {
    return secondary_skill_.MaxSkillValue();
  }
  return skill_.MaxSkillValue();
}

}  // namespace sekai
