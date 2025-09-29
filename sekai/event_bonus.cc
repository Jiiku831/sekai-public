#include "sekai/event_bonus.h"

#include "absl/base/nullability.h"
#include "absl/flags/flag.h"
#include "absl/log/absl_check.h"
#include "sekai/array_size.h"
#include "sekai/character.h"
#include "sekai/config.h"
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

EventBonus::EventBonus(const EventId& event_id, std::optional<WorldBloomVersion> wl_version)
    : EventBonus(MasterDb::SafeFindFirst<db::Event>(event_id.event_id())) {
  const db::Event* event = MasterDb::SafeFindFirst<db::Event>(event_id.event_id());
  if (event == nullptr) {
    LOG(WARNING) << "Event not found: " << event_id.event_id();
    return;
  }
  if (event_id.chapter_id() > 0) {
    support_bonus_ = std::shared_ptr<EventBonus>(new SupportUnitEventBonus{event_id, wl_version});
  }
}

EventBonus::EventBonus(const SimpleEventBonus& event_bonus,
                       std::optional<WorldBloomVersion> wl_version)
    : EventBonus() {
  if (event_bonus.chapter_char_id() > 0) {
    support_bonus_ =
        std::shared_ptr<EventBonus>(new SupportUnitEventBonus{event_bonus, wl_version});
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

EventBonus::EventBonus(const db::Event* event, std::optional<WorldBloomVersion> wl_version)
    : EventBonus(event != nullptr ? *event : db::Event::default_instance(), wl_version) {}

EventBonus::EventBonus(const db::Event& event, std::optional<WorldBloomVersion> wl_version)
    : EventBonus() {
  if (event.id() == 0) {
    return;
  }
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
    EventBonusProto::DeckBonus* absl_nullable deck_bonus = nullptr;
    for (auto attr : EnumValues<db::Attr, db::Attr_descriptor>()) {
      EventBonusProto::AttrBonus* absl_nullable attr_bonus = nullptr;
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
          EventBonusProto::UnitBonus* absl_nonnull unit_bonus = attr_bonus->add_unit_bonus();
          unit_bonus->set_unit(unit);
          unit_bonus->set_rate(rate);
        }
      }
    }
  }

  for (auto rarity : EnumValues<db::CardRarityType, db::CardRarityType_descriptor>()) {
    EventBonusProto::LeveledBonus* absl_nonnull mr_bonus = proto.add_master_rank_bonus();
    EventBonusProto::LeveledBonus* absl_nonnull sl_bonus = proto.add_skill_level_bonus();
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

SupportUnitEventBonus::SupportUnitEventBonus(std::optional<WorldBloomVersion> version)
    : EventBonus() {
  // Reset deck, skill level and master rank bonuses
  deck_bonus_.clear();
  deck_bonus_.resize(CharacterArraySize());
  for (auto& bonus : skill_level_bonus_) {
    bonus.fill(0);
  }
  for (auto& bonus : master_rank_bonus_) {
    bonus.fill(0);
  }

  // Fill support deck bonus
  if (version.has_value()) {
    WorldBloomConfig config = GetWorldBloomConfig(*version);
    baseline_char_bonus_ = config.support_char_bonus();
    baseline_card_bonus_ = config.support_wl_card_bonus();
    for (const int card_id : config.support_wl_cards()) {
      support_bonus_cards_.insert(card_id);
    }
    for (int i = 0; i < config.support_team_bonus().master_rank_bonus_size(); ++i) {
      for (int j = 0; j < config.support_team_bonus().master_rank_bonus(i).level_bonus_size();
           ++j) {
        master_rank_bonus_[i][j] = config.support_team_bonus().master_rank_bonus(i).level_bonus(j);
      }
    }
    for (int i = 0; i < config.support_team_bonus().skill_level_bonus_size(); ++i) {
      for (int j = 0; j < config.support_team_bonus().skill_level_bonus(i).level_bonus_size();
           ++j) {
        skill_level_bonus_[i][j] = config.support_team_bonus().skill_level_bonus(i).level_bonus(j);
      }
    }
  } else {
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
      if (baseline_char_bonus_ == 0) {
        baseline_char_bonus_ = specific_char_bonus - baseline_rarity_bonus;
      } else {
        ABSL_CHECK_EQ(baseline_char_bonus_, specific_char_bonus - baseline_rarity_bonus);
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
        skill_level_bonus_[rarity][skill_level_bonus.skill_level()] =
            skill_level_bonus.bonus_rate();
      }
    }
  }
}

SupportUnitEventBonus::SupportUnitEventBonus(const EventId& event_id,
                                             std::optional<WorldBloomVersion> version)
    : SupportUnitEventBonus(version.value_or(GetWorldBloomVersion(event_id.event_id()))) {
  const db::WorldBloom* absl_nullable world_bloom = nullptr;
  for (const auto* cand : MasterDb::FindAll<db::WorldBloom>(event_id.event_id())) {
    if (cand->chapter_no() == event_id.chapter_id()) {
      world_bloom = cand;
      break;
    }
  }
  ABSL_CHECK_NE(world_bloom, nullptr);

  chapter_char_ = world_bloom->game_character_id();
  ABSL_CHECK_LT(chapter_char_, static_cast<int64_t>(deck_bonus_.size()));
  PopulateChapterSpecificBonus();
}

SupportUnitEventBonus::SupportUnitEventBonus(const SimpleEventBonus& event_bonus,
                                             std::optional<WorldBloomVersion> version)
    : SupportUnitEventBonus(version.value_or(kDefaultWorldBloomVersion)) {
  ABSL_CHECK_GT(event_bonus.chapter_char_id(), 0);
  ABSL_CHECK_LT(event_bonus.chapter_char_id(), static_cast<int64_t>(deck_bonus_.size()));

  chapter_char_ = event_bonus.chapter_char_id();
  ABSL_CHECK_LT(chapter_char_, static_cast<int64_t>(deck_bonus_.size()));
  PopulateChapterSpecificBonus();
}

void SupportUnitEventBonus::PopulateChapterSpecificBonus() {
  db_chapter_unit_ = chapter_char_ == 0 ? db::UNIT_NONE : LookupCharacterUnit(chapter_char_);
  chapter_unit_.set(db_chapter_unit_);

  // Index: (Id, Attr, Unit)
  for (auto attr : EnumValues<db::Attr, db::Attr_descriptor>()) {
    if (attr == db::ATTR_UNKNOWN) continue;
    deck_bonus_[chapter_char_][attr][db_chapter_unit_] = baseline_char_bonus_;
    if (db_chapter_unit_ == db::UNIT_VS) {
      for (auto subunit : EnumValues<db::Unit, db::Unit_descriptor>()) {
        if (subunit != db::UNIT_NONE && subunit != db::UNIT_VS) {
          deck_bonus_[chapter_char_][attr][subunit] = baseline_char_bonus_;
        }
      }
    }
  }

  for (const int card_id : support_bonus_cards_) {
    if (MasterDb::FindFirst<db::Card>(card_id).character_id() == chapter_char_) {
      card_bonus_[card_id] = baseline_card_bonus_;
    }
  }
}

}  // namespace sekai
