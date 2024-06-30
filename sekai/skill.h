#pragma once

#include "sekai/bitset.h"
#include "sekai/config.h"
#include "sekai/db/proto/all.h"
#include "sekai/db/proto/enums.pb.h"
#include "sekai/unit_count_base.h"

namespace sekai {

class Skill {
 public:
  Skill() = default;
  Skill(const db::Skill& skill, int skill_level);

  float SkillValue(UnitCountBase& unit_count) const;
  float MaxSkillValue() const;
  float ReferenceBoostCappedMaxSkillValue() const {
    return std::min(kReferenceScoreBoostCap, MaxSkillValue());
  }
  float raw_skill_value() const { return skill_value_; }
  db::Unit db_skill_enhance_unit() const { return db_skill_enhance_unit_; }
  float skill_enhance_value() const { return skill_enhance_value_; }

  void ApplyCharacterRank(int rank);

  void SetCardMaxSkillValue(float value) { max_card_skill_value_ = value; }
  float ReferenceBoostCappedMaxCardSkillValue() const {
    return std::min(kReferenceScoreBoostCap, max_card_skill_value_);
  }

 private:
  SkillEffectType skill_effect_type_;
  float skill_value_ = 0;
  float max_skill_value_ = 0;

  // Subunit bonus related
  Unit skill_enhance_unit_;
  db::Unit db_skill_enhance_unit_ = db::UNIT_NONE;
  float skill_enhance_value_ = 0;

  // Unit count bonus related
  float unit_count_enhance_value_ = 0;

  // CR bonus related
  float character_rank_enhance_value_ = 0;
  float character_rank_profile_enhance_value_ = 0;

  // Reference score up related
  float reference_score_up_rate_ = 0;

  int skill_level_ = 0;

  float MaxSkillValueImpl() const;
  float max_card_skill_value_ = 0;
};

}  // namespace sekai
