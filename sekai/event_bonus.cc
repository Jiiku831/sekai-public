#include "sekai/event_bonus.h"

#include "absl/base/nullability.h"
#include "absl/flags/flag.h"
#include "absl/log/absl_check.h"
#include "sekai/array_size.h"
#include "sekai/character.h"
#include "sekai/db/master_db.h"
#include "sekai/db/proto/all.h"
#include "sekai/proto_util.h"

ABSL_FLAG(float, subunitless_offset, 0, "The EB offset to apply to subunitless vs");

namespace sekai {
namespace {

using ::sekai::db::MasterDb;

constexpr float kDefaultBonusRate = 25;
constexpr float kDefaultCardBonusRate = 20;

void PopulateAttrBonus(db::Attr attr, float rate, EventBonus::DeckBonusType& deck_bonus) {
  for (int char_id = 1; char_id < static_cast<int64_t>(deck_bonus.size()); ++char_id) {
    db::Unit char_unit = LookupCharacterUnit(char_id);
    deck_bonus[char_id][attr][char_unit] = std::max(deck_bonus[char_id][attr][char_unit], rate);
    if (char_unit == db::UNIT_VS) {
      for (db::Unit unit : EnumValues<db::Unit, db::Unit_descriptor>()) {
        if (unit == db::UNIT_NONE) continue;
        deck_bonus[char_id][attr][unit] = std::max(deck_bonus[char_id][attr][unit], rate);
      }
    }
  }
}

void PopulateCharBonus(int char_id, db::Unit unit, float rate,
                       EventBonus::DeckBonusType& deck_bonus) {
  db::Unit char_unit = LookupCharacterUnit(char_id);
  for (auto attr : EnumValues<db::Attr, db::Attr_descriptor>()) {
    if (attr == db::ATTR_UNKNOWN) continue;
    deck_bonus[char_id][attr][unit] = std::max(deck_bonus[char_id][attr][unit], rate);
    if (char_unit == db::UNIT_VS) {
      // Subunitless bonus
      deck_bonus[char_id][attr][char_unit] = std::max(
          deck_bonus[char_id][attr][char_unit], rate - absl::GetFlag(FLAGS_subunitless_offset));
    }
  }
}

void PopulateAttrCharBonus(int char_id, db::Unit unit, db::Attr attr, float rate,
                           EventBonus::DeckBonusType& deck_bonus) {
  deck_bonus[char_id][attr][unit] = std::max(deck_bonus[char_id][attr][unit], rate);
  db::Unit char_unit = LookupCharacterUnit(char_id);
  if (char_unit == db::UNIT_VS) {
    // Subunitless bonus
    deck_bonus[char_id][attr][char_unit] = std::max(deck_bonus[char_id][attr][char_unit],
                                                    rate - absl::GetFlag(FLAGS_subunitless_offset));
  }
}

void PopulateDiffAttrBonus(std::array<float, db::Attr_ARRAYSIZE>& diff_attr_bonus) {
  for (const auto& bonus : MasterDb::GetAll<db::WorldBloomDifferentAttributeBonus>()) {
    diff_attr_bonus[bonus.attribute_count()] = bonus.bonus_rate();
  }
}

void PopulateMasterRankBonus(std::array<std::array<float, kMasterRankArraySize>,
                                        db::CardRarityType_ARRAYSIZE>& master_rank_bonus) {
  for (const auto& bonus : MasterDb::GetAll<db::EventRarityBonusRate>()) {
    ABSL_CHECK_LT(bonus.card_rarity_type(), static_cast<int64_t>(master_rank_bonus.size()));
    ABSL_CHECK_LT(bonus.master_rank(), static_cast<int64_t>(master_rank_bonus[0].size()));
    master_rank_bonus[bonus.card_rarity_type()][bonus.master_rank()] = bonus.bonus_rate();
  }
}

}  // namespace

EventBonus::EventBonus()
    : card_bonus_(CardArraySize()),
      deck_bonus_(CharacterArraySize()),
      master_rank_bonus_(),
      skill_level_bonus_(),
      diff_attr_bonus_() {}

EventBonus::EventBonus(const EventId& event_id)
    : EventBonus(MasterDb::FindFirst<db::Event>(event_id.event_id())) {
  if (event_id.chapter_id() > 0) {
    support_bonus_ = std::shared_ptr<EventBonus>(new SupportUnitEventBonus{event_id});
  }
}

EventBonus::EventBonus(const SimpleEventBonus& event_bonus) : EventBonus() {
  if (event_bonus.chapter_char_id() > 0) {
    support_bonus_ = std::shared_ptr<EventBonus>(new SupportUnitEventBonus{event_bonus});
  }
  if (event_bonus.attr() != db::ATTR_UNKNOWN) {
    PopulateAttrBonus(event_bonus.attr(), kDefaultBonusRate, deck_bonus_);
  }
  for (const SimpleEventBonus::CharacterAndUnit& char_and_unit : event_bonus.chars()) {
    db::Unit unit = LookupCharacterUnit(char_and_unit.char_id());
    if (char_and_unit.has_unit()) {
      unit = char_and_unit.unit();
    }
    PopulateCharBonus(char_and_unit.char_id(), unit, kDefaultBonusRate, deck_bonus_);
    if (event_bonus.attr() != db::ATTR_UNKNOWN) {
      PopulateAttrCharBonus(char_and_unit.char_id(), unit, event_bonus.attr(),
                            kDefaultBonusRate * 2, deck_bonus_);
    }
  }
  for (int card : event_bonus.cards()) {
    ABSL_CHECK_LT(card, static_cast<int64_t>(card_bonus_.size()));
    card_bonus_[card] = kDefaultCardBonusRate;
  }

  PopulateMasterRankBonus(master_rank_bonus_);

  if (event_bonus.attr() == db::ATTR_UNKNOWN && event_bonus.has_chapter_char_id()) {
    PopulateDiffAttrBonus(diff_attr_bonus_);
    has_diff_attr_bonus_ = true;
  }
}

EventBonus::EventBonus(const db::Event& event) : EventBonus() {
  for (const auto& bonus : MasterDb::FindAll<db::EventDeckBonus>(event.id())) {
    // Index: (Id, Attr, Unit)
    db::Attr attr = bonus->card_attr();
    db::Unit unit = db::UNIT_NONE;
    int char_id = 0;
    if (bonus->has_game_character_unit_id()) {
      const auto& character_unit =
          MasterDb::FindFirst<db::GameCharacterUnit>(bonus->game_character_unit_id());
      unit = character_unit.unit();
      char_id = character_unit.game_character_id();
      ABSL_CHECK_LT(char_id, static_cast<int64_t>(deck_bonus_.size()));
    }
    if (bonus->has_game_character_unit_id() && bonus->has_card_attr()) {
      PopulateAttrCharBonus(char_id, unit, attr, bonus->bonus_rate(), deck_bonus_);
    } else if (bonus->has_game_character_unit_id()) {
      PopulateCharBonus(char_id, unit, bonus->bonus_rate(), deck_bonus_);
    } else if (bonus->has_card_attr()) {
      PopulateAttrBonus(attr, bonus->bonus_rate(), deck_bonus_);
    }
  }

  for (const auto* card : MasterDb::FindAll<db::EventCard>(event.id())) {
    ABSL_CHECK_LT(card->card_id(), static_cast<int64_t>(card_bonus_.size()));
    card_bonus_[card->card_id()] = card->bonus_rate();
  }

  PopulateMasterRankBonus(master_rank_bonus_);

  if (event.event_type() == db::Event::WORLD_BLOOM) {
    PopulateDiffAttrBonus(diff_attr_bonus_);
    has_diff_attr_bonus_ = true;
  }
}

EventBonusProto EventBonus::ToProto() const {
  EventBonusProto proto;
  for (int card_id = 0; card_id < static_cast<int64_t>(card_bonus_.size()); ++card_id) {
    if (card_bonus_[card_id] > 0) {
      (*proto.mutable_card_bonus())[card_id] = card_bonus_[card_id];
    }
  }

  // Index: (Id, Attr, Unit)
  for (int char_id = 0; char_id < static_cast<int64_t>(deck_bonus_.size()); ++char_id) {
    absl::Nullable<EventBonusProto::DeckBonus*> deck_bonus = nullptr;
    for (auto attr : EnumValues<db::Attr, db::Attr_descriptor>()) {
      absl::Nullable<EventBonusProto::AttrBonus*> attr_bonus = nullptr;
      for (auto unit : EnumValues<db::Unit, db::Unit_descriptor>()) {
        float rate = deck_bonus_[char_id][attr][unit];
        if (rate > 0) {
          if (deck_bonus == nullptr) {
            deck_bonus = proto.add_deck_bonus();
            deck_bonus->set_char_id(char_id);
          }
          if (attr_bonus == nullptr) {
            attr_bonus = deck_bonus->add_attr_bonus();
            attr_bonus->set_attr(attr);
          }
          absl::Nonnull<EventBonusProto::UnitBonus*> unit_bonus = attr_bonus->add_unit_bonus();
          unit_bonus->set_unit(unit);
          unit_bonus->set_rate(rate);
        }
      }
    }
  }

  for (auto rarity : EnumValues<db::CardRarityType, db::CardRarityType_descriptor>()) {
    absl::Nonnull<EventBonusProto::LeveledBonus*> mr_bonus = proto.add_master_rank_bonus();
    absl::Nonnull<EventBonusProto::LeveledBonus*> sl_bonus = proto.add_skill_level_bonus();
    for (float bonus : master_rank_bonus_[rarity]) {
      mr_bonus->add_level_bonus(bonus);
    }
    for (float bonus : skill_level_bonus_[rarity]) {
      sl_bonus->add_level_bonus(bonus);
    }
  }

  for (auto bonus : diff_attr_bonus_) {
    proto.add_diff_attr_bonus(bonus);
  }

  return proto;
}

float EventBonus::MaxDeckBonusForChar(int character_id) const {
  float max_bonus = 0;
  for (int i = 0; i < db::Attr_ARRAYSIZE; ++i) {
    for (int j = 0; j < db::Unit_ARRAYSIZE; ++j) {
      max_bonus = std::max(max_bonus, deck_bonus_[character_id][i][j]);
    }
  }
  return max_bonus;
}

SupportUnitEventBonus::SupportUnitEventBonus() : EventBonus() {
  // Reset deck, skill level and master rank bonuses
  deck_bonus_.clear();
  deck_bonus_.resize(CharacterArraySize());
  for (auto& bonus : skill_level_bonus_) {
    bonus.fill(0);
  }
  for (auto& bonus : master_rank_bonus_) {
    bonus.fill(0);
  }
}

SupportUnitEventBonus::SupportUnitEventBonus(const EventId& event_id) : SupportUnitEventBonus() {
  absl::Nullable<const db::WorldBloom*> world_bloom = nullptr;
  for (const auto* cand : MasterDb::FindAll<db::WorldBloom>(event_id.event_id())) {
    if (cand->chapter_no() == event_id.chapter_id()) {
      world_bloom = cand;
      break;
    }
  }
  ABSL_CHECK_NE(world_bloom, nullptr);

  float baseline_char_bonus = 0;
  for (const auto& bonus : MasterDb::GetAll<db::WorldBloomSupportDeckBonus>()) {
    // Index: (Id, Attr, Unit)
    db::CardRarityType rarity = bonus.card_rarity_type();
    float specific_char_bonus = 0;
    float other_char_bonus = 0;
    for (const auto& char_bonus : bonus.character_bonuses()) {
      switch (char_bonus.type()) {
        case db::WorldBloomSupportDeckBonus::CharacterBonus::OTHERS:
          other_char_bonus = char_bonus.bonus_rate();
          break;
        case db::WorldBloomSupportDeckBonus::CharacterBonus::SPECIFIC:
          specific_char_bonus = char_bonus.bonus_rate();
          break;
        default:
          ABSL_CHECK(false) << "unhandled case: "
                            << db::WorldBloomSupportDeckBonus::CharacterBonus::Type_Name(
                                   char_bonus.type());
      }
    }
    float baseline_rarity_bonus = other_char_bonus;
    if (baseline_char_bonus == 0) {
      baseline_char_bonus = specific_char_bonus - baseline_rarity_bonus;
    } else {
      ABSL_CHECK_EQ(baseline_char_bonus, specific_char_bonus - baseline_rarity_bonus);
    }

    for (const auto& master_rank_bonus : bonus.master_rank_bonuses()) {
      ABSL_CHECK_LT(rarity, static_cast<int64_t>(master_rank_bonus_.size()));
      ABSL_CHECK_LT(master_rank_bonus.master_rank(),
                    static_cast<int64_t>(master_rank_bonus_[rarity].size()));
      master_rank_bonus_[rarity][master_rank_bonus.master_rank()] =
          baseline_rarity_bonus + master_rank_bonus.bonus_rate();
    }

    for (const auto& skill_level_bonus : bonus.skill_level_bonuses()) {
      ABSL_CHECK_LT(rarity, static_cast<int64_t>(skill_level_bonus_.size()));
      ABSL_CHECK_LT(skill_level_bonus.skill_level(),
                    static_cast<int64_t>(skill_level_bonus_[rarity].size()));
      skill_level_bonus_[rarity][skill_level_bonus.skill_level()] = skill_level_bonus.bonus_rate();
    }
  }

  chapter_char_ = world_bloom->game_character_id();
  ABSL_CHECK_LT(chapter_char_, static_cast<int64_t>(deck_bonus_.size()));
  db::Unit unit = LookupCharacterUnit(chapter_char_);
  chapter_unit_.set(unit);
  // Index: (Id, Attr, Unit)
  for (auto attr : EnumValues<db::Attr, db::Attr_descriptor>()) {
    if (attr == db::ATTR_UNKNOWN) continue;
    deck_bonus_[chapter_char_][attr][unit] = baseline_char_bonus;
  }
}

SupportUnitEventBonus::SupportUnitEventBonus(const SimpleEventBonus& event_bonus)
    : SupportUnitEventBonus() {
  ABSL_CHECK_GT(event_bonus.chapter_char_id(), 0);
  ABSL_CHECK_LT(event_bonus.chapter_char_id(), static_cast<int64_t>(deck_bonus_.size()));
  float baseline_char_bonus = 0;
  for (const auto& bonus : MasterDb::GetAll<db::WorldBloomSupportDeckBonus>()) {
    // Index: (Id, Attr, Unit)
    db::CardRarityType rarity = bonus.card_rarity_type();
    float specific_char_bonus = 0;
    float other_char_bonus = 0;
    for (const auto& char_bonus : bonus.character_bonuses()) {
      switch (char_bonus.type()) {
        case db::WorldBloomSupportDeckBonus::CharacterBonus::OTHERS:
          other_char_bonus = char_bonus.bonus_rate();
          break;
        case db::WorldBloomSupportDeckBonus::CharacterBonus::SPECIFIC:
          specific_char_bonus = char_bonus.bonus_rate();
          break;
        default:
          ABSL_CHECK(false) << "unhandled case: "
                            << db::WorldBloomSupportDeckBonus::CharacterBonus::Type_Name(
                                   char_bonus.type());
      }
    }
    float baseline_rarity_bonus = other_char_bonus;
    if (baseline_char_bonus == 0) {
      baseline_char_bonus = specific_char_bonus - baseline_rarity_bonus;
    } else {
      ABSL_CHECK_EQ(baseline_char_bonus, specific_char_bonus - baseline_rarity_bonus);
    }

    for (const auto& master_rank_bonus : bonus.master_rank_bonuses()) {
      ABSL_CHECK_LT(rarity, static_cast<int64_t>(master_rank_bonus_.size()));
      ABSL_CHECK_LT(master_rank_bonus.master_rank(),
                    static_cast<int64_t>(master_rank_bonus_[rarity].size()));
      master_rank_bonus_[rarity][master_rank_bonus.master_rank()] =
          baseline_rarity_bonus + master_rank_bonus.bonus_rate();
    }

    for (const auto& skill_level_bonus : bonus.skill_level_bonuses()) {
      ABSL_CHECK_LT(rarity, static_cast<int64_t>(skill_level_bonus_.size()));
      ABSL_CHECK_LT(skill_level_bonus.skill_level(),
                    static_cast<int64_t>(skill_level_bonus_[rarity].size()));
      skill_level_bonus_[rarity][skill_level_bonus.skill_level()] = skill_level_bonus.bonus_rate();
    }
  }

  chapter_char_ = event_bonus.chapter_char_id();
  ABSL_CHECK_LT(chapter_char_, static_cast<int64_t>(deck_bonus_.size()));
  db::Unit unit = LookupCharacterUnit(chapter_char_);
  chapter_unit_.set(unit);
  // Index: (Id, Attr, Unit)
  for (auto attr : EnumValues<db::Attr, db::Attr_descriptor>()) {
    if (attr == db::ATTR_UNKNOWN) continue;
    deck_bonus_[chapter_char_][attr][unit] = baseline_char_bonus;
    if (unit == db::UNIT_VS) {
      for (auto subunit : EnumValues<db::Unit, db::Unit_descriptor>()) {
        if (subunit != db::UNIT_NONE && subunit != db::UNIT_VS) {
          deck_bonus_[chapter_char_][attr][subunit] = baseline_char_bonus;
        }
      }
    }
  }
}

}  // namespace sekai
