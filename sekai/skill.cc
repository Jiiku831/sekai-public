#include "sekai/skill.h"

#include "absl/log/absl_check.h"
#include "sekai/bitset.h"
#include "sekai/config.h"
#include "sekai/db/proto/all.h"
#include "sekai/db/proto/enums.pb.h"
#include "sekai/unit_count_base.h"

namespace sekai {
namespace {

constexpr int kMaxUnitCount = 2;
constexpr int kBonusCharacterRankStep = 2;
constexpr int kMaxBonusCharacterRank = 100;

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

}  // namespace

Skill::Skill(const db::Skill& skill, int skill_level) {
  skill_level_ = skill_level;
  skill_value_ = GetMaxSkill(skill, skill_level);
  bool found = false;
  for (const db::SkillEffect& skill_effect : skill.skill_effects()) {
    skill_effect_type_.set(skill_effect.skill_effect_type());
    if (skill_effect.skill_enhance().skill_enhance_type() == db::SkillEnhance::SUB_UNIT_SCORE_UP) {
      db_skill_enhance_unit_ = skill_effect.skill_enhance().skill_enhance_condition().unit();
      skill_enhance_unit_ = Unit(db_skill_enhance_unit_);
      skill_enhance_value_ = skill_effect.skill_enhance().activate_effect_value();
    } else if (skill_effect.skill_effect_type() == db::SkillEffect::SCORE_UP_UNIT_COUNT) {
      ABSL_CHECK_LE(skill_effect.activate_unit_count(), kMaxUnitCount);
      if (skill_effect.activate_unit_count() == 1) {
        for (const db::SkillEffect::Details& details : skill_effect.skill_effect_details()) {
          if (details.level() == skill_level) {
            unit_count_enhance_value_ = details.activate_effect_value();
            found = true;
            break;
          }
        }
      }
    } else if (skill_effect.skill_effect_type() == db::SkillEffect::SCORE_UP_CHARACTER_RANK) {
      ABSL_CHECK_LE(skill_effect.activate_character_rank(), kMaxBonusCharacterRank);
      if (skill_effect.activate_character_rank() == kBonusCharacterRankStep) {
        for (const db::SkillEffect::Details& details : skill_effect.skill_effect_details()) {
          if (details.level() == skill_level) {
            character_rank_enhance_value_ = details.activate_effect_value();
            found = true;
            break;
          }
        }
      }
    } else if (skill_effect.skill_effect_type() ==
               db::SkillEffect::OTHER_MEMBER_SCORE_UP_REFERENCE_RATE) {
      for (const db::SkillEffect::Details& details : skill_effect.skill_effect_details()) {
        if (details.level() == skill_level) {
          reference_score_up_rate_ = static_cast<float>(details.activate_effect_value()) / 100;
          reference_score_up_cap_ = details.activate_effect_value2();
          found = true;
          break;
        }
      }
    }
  }
  ABSL_CHECK(!(skill_effect_type_.test(db::SkillEffect::SCORE_UP_UNIT_COUNT) ||
               skill_effect_type_.test(db::SkillEffect::SCORE_UP_CHARACTER_RANK) ||
               skill_effect_type_.test(db::SkillEffect::OTHER_MEMBER_SCORE_UP_REFERENCE_RATE)) ||
             found);
  max_card_skill_value_ = max_skill_value_ = MaxSkillValueImpl();
}

float Skill::SkillValue(int card_index, UnitCountBase& unit_count) const {
  if (db_skill_enhance_unit_) {
    int count = unit_count.CharacterCount(db_skill_enhance_unit_);
    int factor = count == 5 ? 5 : count - 1;
    return skill_value_ + factor * skill_enhance_value_;
  }
  if (skill_effect_type_.test(db::SkillEffect::SCORE_UP_UNIT_COUNT)) {
    int count = unit_count.UnitCount();
    return skill_value_ + unit_count_enhance_value_ * std::min(count - 1, kMaxUnitCount);
  }
  if (skill_effect_type_.test(db::SkillEffect::SCORE_UP_CHARACTER_RANK)) {
    return skill_value_ + character_rank_profile_enhance_value_;
  }
  if (skill_effect_type_.test(db::SkillEffect::OTHER_MEMBER_SCORE_UP_REFERENCE_RATE)) {
    return skill_value_ +
           unit_count.ReferenceBoostAverageCappedSkillValue(card_index) * reference_score_up_rate_;
  }
  return skill_value_;
}

void Skill::ApplyCharacterRank(int rank) {
  character_rank_profile_enhance_value_ =
      static_cast<int>(std::min(rank, kMaxBonusCharacterRank) / kBonusCharacterRankStep) *
      character_rank_enhance_value_;
}

float Skill::MaxSkillValue() const { return max_skill_value_; }

float Skill::MaxSkillValueImpl() const {
  if (db_skill_enhance_unit_) {
    return skill_value_ + 5 * skill_enhance_value_;
  }
  if (skill_effect_type_.test(db::SkillEffect::SCORE_UP_UNIT_COUNT)) {
    return skill_value_ + unit_count_enhance_value_ * kMaxUnitCount;
  }
  if (skill_effect_type_.test(db::SkillEffect::SCORE_UP_CHARACTER_RANK)) {
    return skill_value_ + character_rank_enhance_value_ *
                              static_cast<int>(kMaxBonusCharacterRank / kBonusCharacterRankStep);
  }
  if (skill_effect_type_.test(db::SkillEffect::OTHER_MEMBER_SCORE_UP_REFERENCE_RATE)) {
    return skill_value_ + reference_score_up_cap_;
  }
  return skill_value_;
}

}  // namespace sekai
