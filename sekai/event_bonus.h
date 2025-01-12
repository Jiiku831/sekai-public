#pragma once

#include "absl/base/nullability.h"
#include "sekai/bitset.h"
#include "sekai/character.h"
#include "sekai/config.h"
#include "sekai/db/proto/all.h"
#include "sekai/proto/event_bonus.pb.h"
#include "sekai/proto/event_id.pb.h"

namespace sekai {

class EventBonus {
 public:
  EventBonus();
  virtual ~EventBonus() = default;
  explicit EventBonus(const db::Event& event);
  explicit EventBonus(absl::Nullable<const db::Event*> event);
  explicit EventBonus(const EventId& event_id);
  explicit EventBonus(const SimpleEventBonus& event_bonus);

  EventBonusProto ToProto() const;

  float card_bonus(int card_id) const { return card_bonus_[card_id]; }
  float deck_bonus(int character_id, db::Attr attr, db::Unit unit) const {
    return deck_bonus_[character_id][attr][unit];
  }
  float master_rank_bonus(db::CardRarityType rarity, int master_rank) const {
    return master_rank_bonus_[rarity][master_rank];
  }
  float skill_level_bonus(db::CardRarityType rarity, int skill_level) const {
    return skill_level_bonus_[rarity][skill_level];
  }
  float MaxDeckBonusForChar(int character_id) const;

  float diff_attr_bonus(int count) const { return diff_attr_bonus_[count]; }
  bool has_diff_attr_bonus() const { return has_diff_attr_bonus_; }

  // Index: (Id, Attr, Unit)
  using DeckBonusType =
      std::vector<std::array<std::array<float, db::Unit_ARRAYSIZE>, db::Attr_ARRAYSIZE>>;

  absl::Nullable<const EventBonus*> SupportUnitBonus() const { return support_bonus_.get(); }

  virtual float GetBonus(int card_id, int character_id, db::Attr attr, db::Unit unit,
                         db::CardRarityType rarity, int master_rank, int skill_level) const {
    return card_bonus(card_id) + deck_bonus(character_id, attr, unit) +
           master_rank_bonus(rarity, master_rank) + skill_level_bonus(rarity, skill_level);
  }

 protected:
  std::vector<float> card_bonus_;
  DeckBonusType deck_bonus_;
  std::array<std::array<float, kMasterRankArraySize>, db::CardRarityType_ARRAYSIZE>
      master_rank_bonus_;
  std::array<std::array<float, kSkillLevelArraySize>, db::CardRarityType_ARRAYSIZE>
      skill_level_bonus_;
  std::array<float, db::Attr_ARRAYSIZE> diff_attr_bonus_;
  bool has_diff_attr_bonus_ = false;

  std::shared_ptr<EventBonus> support_bonus_ = nullptr;
};

class SupportUnitEventBonus : public EventBonus {
 public:
  SupportUnitEventBonus();
  explicit SupportUnitEventBonus(const EventId& event_id);
  explicit SupportUnitEventBonus(const SimpleEventBonus& event_bonus);

  float GetBonus(int card_id, int character_id, db::Attr attr, db::Unit unit,
                 db::CardRarityType rarity, int master_rank, int skill_level) const override {
    if (chapter_unit_.test(db::UNIT_VS) && LookupCharacterUnit(character_id) != db::UNIT_VS) {
      return 0;
    }
    if (!chapter_unit_.test(db::UNIT_VS) && !chapter_unit_.test(unit)) {
      return 0;
    }
    return card_bonus(card_id) + deck_bonus(character_id, attr, unit) +
           master_rank_bonus(rarity, master_rank) + skill_level_bonus(rarity, skill_level);
  }

 private:
  int chapter_char_ = 0;
  Unit chapter_unit_;
};

}  // namespace sekai
