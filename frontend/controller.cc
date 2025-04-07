#include "frontend/controller.h"

#include <algorithm>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <system_error>
#include <variant>

#include <emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/val.h>
#include <google/protobuf/text_format.h>
#include <google/protobuf/util/json_util.h>

#include "absl/algorithm/container.h"
#include "absl/container/flat_hash_set.h"
#include "absl/flags/declare.h"
#include "absl/flags/flag.h"
#include "absl/log/log.h"
#include "absl/strings/str_split.h"
#include "absl/time/time.h"
#include "frontend/context.h"
#include "frontend/controller_base.h"
#include "frontend/element_id.h"
#include "frontend/log.h"
#include "frontend/proto/context.pb.h"
#include "sekai/array_size.h"
#include "sekai/bitset.h"
#include "sekai/card.h"
#include "sekai/challenge_live_estimator.h"
#include "sekai/character.h"
#include "sekai/config.h"
#include "sekai/db/master_db.h"
#include "sekai/db/proto/all.h"
#include "sekai/estimator.h"
#include "sekai/event_bonus.h"
#include "sekai/fixtures.h"
#include "sekai/max_level.h"
#include "sekai/parking.h"
#include "sekai/profile.h"
#include "sekai/proto/card_state.pb.h"
#include "sekai/proto/event_id.pb.h"
#include "sekai/proto/profile.pb.h"
#include "sekai/proto/team.pb.h"
#include "sekai/proto/world_bloom.pb.h"
#include "sekai/proto_util.h"
#include "sekai/team.h"
#include "sekai/team_builder/challenge_live_team_builder.h"
#include "sekai/team_builder/optimization_objective.h"
#include "sekai/team_builder/simulated_annealing_team_builder.h"

ABSL_DECLARE_FLAG(float, subunitless_offset);

namespace frontend {
namespace {

using ::emscripten::base;
using ::emscripten::class_;
using ::google::protobuf::TextFormat;
using ::google::protobuf::util::MessageToJsonString;
using ::sekai::CardState;
using ::sekai::ChallengeLiveTeamBuilder;
using ::sekai::CharacterArraySize;
using ::sekai::CreateMaxCardState;
using ::sekai::EventBonus;
using ::sekai::EventId;
using ::sekai::FixturesWithCharBonuses;
using ::sekai::OptimizeExactPoints;
using ::sekai::OptimizeFillTeam;
using ::sekai::PartitionedBuildTeam;
using ::sekai::ProfileProto;
using ::sekai::ReadBinaryProtoFile;
using ::sekai::SafeParseBinaryProto;
using ::sekai::SafeParseTextProto;
using ::sekai::SimpleEventBonus;
using ::sekai::SimulatedAnnealingTeamBuilder;
using ::sekai::WorldBloomVersion;
using ::sekai::db::AreaItem;
using ::sekai::db::Attr;
using ::sekai::db::Card;
using ::sekai::db::CardEpisode;
using ::sekai::db::CardRarityType;
using ::sekai::db::Event;
using ::sekai::db::GameCharacter;
using ::sekai::db::MasterDb;
using ::sekai::db::MySekaiFixture;
using ::sekai::db::MySekaiGate;
using ::sekai::db::Unit;

EM_JS(void, SetCardVisibility, (const char * element_id, bool state), {
    const elem = document.getElementById(UTF8ToString(element_id));
    if (state) {
      elem.classList.remove("hidden");
    } else {
      elem.classList.add("hidden");
    }
});

EM_JS(void, SetCardControls, (int card_id, bool state), {
  Array.from(
    document.querySelectorAll(
      `#card-list-card-${card_id} td:not(:first-child) input`))
    .forEach((e) => { e.disabled = !state; })
});

EM_JS(void, RefreshTeamInputs, (), {
  Array.from(
    document.querySelectorAll(".team-builder-id"))
  .forEach((e) => { e.dispatchEvent(new Event('blur')); });
});

EM_JS(void, RenderTeam, (int team_index, const char * context), {
  const parsed_context = JSON.parse(UTF8ToString(context));
  RenderTeamImpl(team_index, parsed_context);
});

// clang-format off
EM_JS(void, SetCardOwnership, (int card_id, bool state), {
  const elem = document.getElementById(`card-list-owned-${card_id}`);
  if (elem) {
    elem.checked = state;
  } else {
    console.warn(`Card ${card_id} not found in card list`);
  }
});

EM_JS(void, SetCardLevel, (int card_id, int level), {
  const elem = document.getElementById(`card-list-level-${card_id}`);
  if (elem) {
    elem.value = level;
  } else {
    console.warn(`Card ${card_id} not found in card list`);
  }
});

EM_JS(void, SetCardMasterRank, (int card_id, int level), {
  const elem = document.getElementById(`card-list-master-rank-${card_id}`);
  if (elem) {
    elem.value = level;
  } else {
    console.warn(`Card ${card_id} not found in card list`);
  }
});

EM_JS(void, SetCardSkillLevel, (int card_id, int level), {
  const elem = document.getElementById(`card-list-skill-level-${card_id}`);
  if (elem) {
    elem.value = level;
  } else {
    console.warn(`Card ${card_id} not found in card list`);
  }
});

EM_JS(void, SetCardTrained, (int card_id, bool state), {
  const elem = document.getElementById(`card-list-trained-${card_id}`);
  if (elem) {
    elem.checked = state;
  } else {
    console.warn(`Card ${card_id} not found in card list`);
  }
});

EM_JS(void, SetCardCanvasCrafted, (int card_id, bool state), {
  const elem = document.getElementById(`card-list-canvas-crafted-${card_id}`);
  if (elem) {
    elem.checked = state;
  } else {
    console.warn(`Card ${card_id} not found in card list`);
  }
});

EM_JS(void, SetCardEpisode1, (int card_id, bool state), {
  const elem = document.getElementById(`card-list-episode-1-${card_id}`);
  if (elem) {
    elem.checked = state;
  } else {
    console.warn(`Card ${card_id} not found in card list`);
  }
});

EM_JS(void, SetCardEpisode2, (int card_id, bool state), {
  const elem = document.getElementById(`card-list-episode-2-${card_id}`);
  if (elem) {
    elem.checked = state;
  } else {
    console.warn(`Card ${card_id} not found in card list`);
  }
});

EM_JS(void, SetAreaItemLevel, (int area_item_id, int level), {
  const elem = document.getElementById(`area-item-${area_item_id}`);
  if (elem) {
    elem.value = level;
  } else {
    console.warn(`Area item ${area_item_id} not found`);
  }
});

EM_JS(void, SetMySekaiFixtureCrafted, (int fixture_id, int state), {
  const elem = document.getElementById(`mysekai-fixture-${fixture_id}`);
  if (elem) {
    elem.checked = state;
  } else {
    console.warn(`Fixture ${fixture_id} not found in card list`);
  }
});

EM_JS(void, SetMySekaiGateLevel, (int gate_id, int level), {
  const elem = document.getElementById(`mysekai-gate-${gate_id}`);
  if (elem) {
    elem.value = level;
  } else {
    console.warn(`Gate ${gate_id} not found`);
  }
});

EM_JS(void, SetCharacterRankLevel, (int char_id, int level), {
  const elem = document.getElementById(`character-rank-${char_id}`);
  if (elem) {
    elem.value = level;
  } else {
    console.warn(`Character rank for character ${char_id} not found`);
  }
});

EM_JS(void, SetTitleBonus, (int value), {
  document.getElementById("title-bonus").value = value;
});

EM_JS(void, SetTeamCardId, (int team_index, int card_index, int card_id, bool use_untrained_skill), {
  let card = document.getElementById(`team-builder-${team_index}-${card_index}-id`);
  card.value = (card_id == 0 ? "" : card_id) + (use_untrained_skill ? "U" : "");
  card.dataset.cardId = card_id;
  card.dataset.useUntrained = use_untrained_skill;
});

EM_JS(void, SetTeamBuilderOutput, (const char * output), {
  document.getElementById("team-recommender-output").innerText = UTF8ToString(output);
});

EM_JS(void, SetUploadCardsOutput, (const char * output), {
  document.getElementById("upload-cards-output-area").innerText = UTF8ToString(output);
});

EM_JS(void, SetUseOldSubunitlessBonusCheckbox, (bool state), {
  document.getElementById("use-old-subunitless-bonus").checked = state;
});

EM_JS(void, SetLeadConstraintCheckbox, (int char_id, bool state), {
  document.getElementById(`lead-constraint-${char_id}`).checked = state;
});

EM_JS(void, SetKizunaConstraintCheckbox, (int char_1, int char_2, bool state), {
  document.getElementById(`kizuna-constraint-${char_1}-${char_2}`).checked = state;
  document.getElementById(`kizuna-constraint-${char_2}-${char_1}`).checked = state;
});

EM_JS(void, SetRarityConstraintCheckbox, (int rarity, bool state), {
  document.getElementById(`rarity-constraint-${rarity}`).checked = state;
});

EM_JS(void, SetMinLeadSkillInput, (int value), {
  document.getElementById("lead-skill-constraint").value = value;
});

EM_JS(void, SetCustomEventAttrRadio, (int value), {
  document.getElementById(`custom-event-attr-${value}`).checked = true;
});

EM_JS(void, SetCustomEventCharacterCheckbox, (int char_id, int unit_id, bool state), {
  document.getElementById(`custom-event-character-${char_id}-${unit_id}`).checked = state;
});

EM_JS(void, SetCustomEventChapterRadio, (int value), {
  document.getElementById(`custom-event-chapter-${value}`).checked = true;
});

EM_JS(void, SetCustomEventWorldBloomVersionRadio, (int value), {
  document.getElementById(`custom-event-wl-version-${value}`).checked = true;
});

EM_JS(void, SetSelectedEventDropdown, (int event_id, int chapter_id), {
    document.querySelector(
        `#event-bonus-event-select option[data-event-id='${event_id}'][data-chapter-id='${chapter_id}']`).selected = true;
});
// clang-format on

void UpdateCardState(int card_id, const CardState& state) {
  const Card* card = MasterDb::SafeFindFirst<Card>(card_id);
  if (card == nullptr) {
    LOG(WARNING) << "Card " << card_id << " not found, skipping update.";
    return;
  }
  SetCardLevel(card_id, state.level());
  SetCardMasterRank(card_id, state.master_rank());
  SetCardSkillLevel(card_id, state.skill_level());
  if (sekai::TrainableCard(card->card_rarity_type())) {
    SetCardTrained(card_id, state.special_training());
  }
  SetCardCanvasCrafted(card_id, state.canvas_crafted());
  for (const CardEpisode* episode : MasterDb::FindAll<CardEpisode>(card_id)) {
    bool read = absl::c_linear_search(state.card_episodes_read(), episode->id());
    switch (episode->card_episode_part_type()) {
      case CardEpisode::FIRST_PART:
        SetCardEpisode1(card_id, read);
        break;
      case CardEpisode::SECOND_PART:
        SetCardEpisode2(card_id, read);
        break;
      default:
        LOG(FATAL) << "unhandled case";
    }
  }
}

void ResetView() {
  for (const Card& card : MasterDb::GetAll<Card>()) {
    SetCardControls(card.id(), false);
    SetCardOwnership(card.id(), false);
  }
  for (const GameCharacter& game_char : MasterDb::GetAll<GameCharacter>()) {
    SetCharacterRankLevel(game_char.id(), 1);
  }
  for (const AreaItem& area_item : MasterDb::GetAll<AreaItem>()) {
    SetAreaItemLevel(area_item.id(), 0);
  }
  for (const MySekaiFixture* fixture : FixturesWithCharBonuses()) {
    SetMySekaiFixtureCrafted(fixture->id(), false);
  }
  for (const MySekaiGate& gate : MasterDb::GetAll<MySekaiGate>()) {
    SetMySekaiGateLevel(gate.id(), 0);
  }
  SetTitleBonus(0);

  SetSelectedEventDropdown(0, 0);
  SetUseOldSubunitlessBonusCheckbox(false);
  for (int char_id : sekai::UniqueCharacterIds()) {
    SetLeadConstraintCheckbox(char_id, true);
    for (int char_2 : sekai::UniqueCharacterIds()) {
      if (char_id == char_2) continue;
      SetKizunaConstraintCheckbox(char_id, char_2, true);
    }
  }
  for (CardRarityType rarity :
       sekai::EnumValues<CardRarityType, sekai::db::CardRarityType_descriptor>()) {
    if (rarity == sekai::db::RARITY_UNKNOWN) continue;
    SetRarityConstraintCheckbox(rarity, true);
  }
  SetMinLeadSkillInput(0);

  SetCustomEventAttrRadio(0);
  SetCustomEventChapterRadio(0);
  SetCustomEventWorldBloomVersionRadio(static_cast<int>(sekai::kDefaultWorldBloomVersion));
  for (const GameCharacter& game_char : MasterDb::GetAll<GameCharacter>()) {
    SetCustomEventCharacterCheckbox(game_char.id(), 0, false);
    if (sekai::LookupCharacterUnit(game_char.id()) == sekai::db::UNIT_VS) {
      for (Unit unit : sekai::EnumValues<Unit, sekai::db::Unit_descriptor>()) {
        if (unit == sekai::db::UNIT_VS || unit == sekai::db::UNIT_NONE) continue;
        SetCustomEventCharacterCheckbox(game_char.id(), static_cast<int>(unit), false);
      }
    }
  }
}

void ResetView(const ProfileProto& profile) {
  ResetView();

  for (const auto& [card_id, card_state] : profile.cards()) {
    SetCardOwnership(card_id, true);
    SetCardControls(card_id, true);
    UpdateCardState(card_id, card_state);
  }

  for (const GameCharacter& game_char : MasterDb::GetAll<GameCharacter>()) {
    SetCharacterRankLevel(game_char.id(), profile.character_ranks(game_char.id()));
  }
  for (const AreaItem& area_item : MasterDb::GetAll<AreaItem>()) {
    SetAreaItemLevel(area_item.id(), profile.area_item_levels(area_item.id()));
  }
  for (const MySekaiFixture* fixture : FixturesWithCharBonuses()) {
    auto crafted = profile.mysekai_fixture_crafted().find(fixture->id());
    if (crafted != profile.mysekai_fixture_crafted().end() && crafted->second) {
      SetMySekaiFixtureCrafted(fixture->id(), true);
    } else {
      SetMySekaiFixtureCrafted(fixture->id(), false);
    }
  }
  for (const MySekaiGate& gate : MasterDb::GetAll<MySekaiGate>()) {
    SetMySekaiGateLevel(gate.id(), profile.mysekai_gate_levels(gate.id()));
  }

  SetSelectedEventDropdown(profile.event_id().event_id(), profile.event_id().chapter_id());
  SetTitleBonus(profile.bonus_power());
  SetUseOldSubunitlessBonusCheckbox(profile.use_old_subunitless_bonus());
  for (int char_id : profile.exclude_lead_char_ids()) {
    SetLeadConstraintCheckbox(char_id, false);
  }
  for (const ProfileProto::KizunaPair& pair : profile.exclude_kizuna_pairs()) {
    SetKizunaConstraintCheckbox(pair.char_1(), pair.char_2(), false);
  }
  for (int rarity : profile.exclude_rarities()) {
    SetRarityConstraintCheckbox(rarity, false);
  }
  SetMinLeadSkillInput(profile.min_lead_skill());

  if (profile.custom_event().has_attr()) {
    SetCustomEventAttrRadio(profile.custom_event().attr());
  }
  if (profile.custom_event().has_chapter_char_id()) {
    SetCustomEventChapterRadio(profile.custom_event().chapter_char_id());
  }
  if (profile.has_world_bloom_version()) {
    SetCustomEventWorldBloomVersionRadio(profile.world_bloom_version());
  }
  for (const SimpleEventBonus::CharacterAndUnit& char_and_unit : profile.custom_event().chars()) {
    SetCustomEventCharacterCheckbox(char_and_unit.char_id(), static_cast<int>(char_and_unit.unit()),
                                    true);
  }
}

sekai::Constraints MakeConstraints(const ProfileProto& profile) {
  sekai::Constraints constraints;
  sekai::Character excluded_chars;
  for (int char_id : profile.exclude_lead_char_ids()) {
    excluded_chars.set(char_id);
  }
  for (int char_id : sekai::UniqueCharacterIds()) {
    if (!excluded_chars.test(char_id)) {
      constraints.AddLeadChar(char_id);
    }
  }

  absl::flat_hash_set<std::pair<int, int>> excluded_kizunas;
  for (const ProfileProto::KizunaPair& pair : profile.exclude_kizuna_pairs()) {
    excluded_kizunas.emplace(pair.char_1(), pair.char_2());
  }
  for (int char_1 : sekai::UniqueCharacterIds()) {
    for (int char_2 : sekai::UniqueCharacterIds()) {
      if (char_2 <= char_1) continue;
      if (!excluded_kizunas.contains({char_1, char_2}) &&
          !excluded_kizunas.contains({char_2, char_1})) {
        constraints.AddKizunaPair({char_1, char_2});
      }
    }
  }

  for (int rarity : profile.exclude_rarities()) {
    constraints.AddExcludedRarity(static_cast<CardRarityType>(rarity));
  }

  constraints.SetMinLeadSkill(profile.min_lead_skill());
  return constraints;
}

WorldBloomVersion GetWorldBloomVersion(const ProfileProto& profile) {
  if (profile.event_id().event_id() == 0) {
    if (profile.world_bloom_version() != sekai::WORLD_BLOOM_VERSION_UNKNOWN) {
      return profile.world_bloom_version();
    } else {
      return sekai::kDefaultWorldBloomVersion;
    }
  }
  return sekai::GetWorldBloomVersion(profile.event_id().event_id());
}

}  // namespace frontend

bool FilterState::Eval(const Card& card, bool owned) const {
  absl::Time release_time = absl::FromUnixMillis(card.release_at());
  if (release_time > absl::Now() && !unreleased_cards) {
    return false;
  }
  if (owned_cards && !owned) {
    return false;
  }
  return attr.test(card.attr()) && rarity.test(card.card_rarity_type()) &&
         character.test(card.character_id());
}

Controller::Controller() {
  UpdateCardListVisibilities(card_list_filter_state_, /*force_update=*/true);
}

void Controller::SetAttrFilterState(int attr, bool state) {
  if (!sekai::db::Attr_IsValid(attr)) {
    LOG(ERROR) << "Invalid attribute: " << attr;
    return;
  }
  FilterState new_state = card_list_filter_state_;
  new_state.attr.set(static_cast<Attr>(attr), state);
  UpdateCardListVisibilities(new_state);
}

void Controller::SetCharacterFilterState(int char_id, bool state) {
  if (char_id <= 0 || char_id >= CharacterArraySize()) {
    LOG(ERROR) << "Invalid char_id: " << char_id;
    return;
  }
  FilterState new_state = card_list_filter_state_;
  new_state.character.set(char_id, state);
  UpdateCardListVisibilities(new_state);
}

void Controller::SetRarityFilterState(int rarity, bool state) {
  if (!sekai::db::CardRarityType_IsValid(rarity)) {
    LOG(ERROR) << "Invalid rarity: " << rarity;
    return;
  }
  FilterState new_state = card_list_filter_state_;
  new_state.rarity.set(static_cast<CardRarityType>(rarity), state);
  UpdateCardListVisibilities(new_state);
}

void Controller::SetUnreleasedContentFilterState(bool state) {
  FilterState new_state = card_list_filter_state_;
  new_state.unreleased_cards = state;
  UpdateCardListVisibilities(new_state);
}

void Controller::SetOwnedCardsFilterState(bool state) {
  FilterState new_state = card_list_filter_state_;
  new_state.owned_cards = state;
  UpdateCardListVisibilities(new_state);
}

void Controller::SetAreaItemLevel(int area_item_id, int level) {
  if (level < 0 || level > sekai::kMaxAreaItemLevel || area_item_id < 0 ||
      area_item_id >= profile_proto_.area_item_levels_size()) {
    LOG(ERROR) << "Invalid area item: " << area_item_id << " level " << level;
    return;
  }
  if (profile_proto_.area_item_levels(area_item_id) == level) return;
  profile_proto_.set_area_item_levels(area_item_id, level);
  UpdateProfile();
}

void Controller::SetMySekaiFixtureCrafted(int fixture_id, bool state) {
  bool& current_state = (*profile_proto_.mutable_mysekai_fixture_crafted())[fixture_id];
  if (current_state == state) return;
  current_state = state;
  UpdateProfile();
}

void Controller::SetMySekaiGateLevel(int gate_id, int level) {
  if (level < 0 || level > sekai::kMaxMySekaiGateLevel || gate_id < 0 ||
      gate_id >= profile_proto_.mysekai_gate_levels_size()) {
    LOG(ERROR) << "Invalid gate: " << gate_id << " level " << level;
    return;
  }
  if (profile_proto_.mysekai_gate_levels(gate_id) == level) return;
  profile_proto_.set_mysekai_gate_levels(gate_id, level);
  UpdateProfile();
}

void Controller::SetCharacterRank(int char_id, int level) {
  if (level < 1 || level > sekai::kMaxCharacterRank ||
      char_id >= profile_proto_.character_ranks_size()) {
    LOG(ERROR) << "Invalid char_id: " << char_id;
    return;
  }
  if (profile_proto_.character_ranks(char_id) == level) return;
  profile_proto_.set_character_ranks(char_id, level);
  UpdateProfile();
}

void Controller::SetTitleBonus(int bonus) {
  if (bonus < 0 || bonus > 1000) {
    LOG(ERROR) << "Invalid bonus value: " << bonus;
    return;
  }
  if (profile_proto_.bonus_power() == bonus) return;
  profile_proto_.set_bonus_power(bonus);
  UpdateProfile();
}

void Controller::OnProfileUpdate() {
  event_bonus_ = std::visit(
      [this](auto&& arg) { return EventBonus(arg, GetWorldBloomVersion(profile_proto_)); },
      event_bonus_source());
  constraints_ = MakeConstraints(profile_proto_);
  profile_.ApplyEventBonus(event_bonus_);
  absl::SetFlag(&FLAGS_subunitless_offset, profile_proto_.use_old_subunitless_bonus() ? 10 : 0);

  if (profile_proto_.event_id().event_id() > 0) {
    const Event* event = MasterDb::SafeFindFirst<Event>(profile_proto_.event_id().event_id());
    if (event == nullptr) {
      LOG(WARNING) << "Event " << profile_proto_.event_id().event_id()
                   << " not found. Falling back to multi estimator.";
      estimator_mode_ = sekai::Estimator::Mode::kMulti;
    } else if (event->event_type() == Event::CHEERFUL_CARNIVAL) {
      estimator_mode_ = sekai::Estimator::Mode::kCheerful;
    } else {
      estimator_mode_ = sekai::Estimator::Mode::kMulti;
    }
  }

  RefreshTeams();
}

void Controller::UpdateCardListVisibilities(FilterState new_state, bool force_update) {
  FilterState& old_state = card_list_filter_state_;
  for (const Card& card : MasterDb::GetAll<Card>()) {
    bool owned = profile_.GetCard(card.id()) != nullptr;
    bool new_visibility = new_state.Eval(card, owned);
    if (old_state.Eval(card, owned) != new_visibility || force_update) {
      std::string element_id = CardListRowId(card.id());
      SetCardVisibility(element_id.c_str(), new_visibility);
    }
  }
  old_state = new_state;
}

void Controller::ImportCardsFromCsv(const std::string& cards_csv) {
  SetUploadCardsOutput(
      "Importing data... (if this doesn't change to Done then it means there was an error.)");
  sekai::Profile profile;
  if (absl::Status status = profile.LoadCardsFromCsvString(cards_csv); status.ok()) {
    profile.CardsToProto();
    ProfileProto new_profile = profile.CardsToProto();
    *profile_proto_.mutable_cards() = new_profile.cards();
    UpdateProfile();
    ResetView(profile_proto_);
    RefreshTeamInputs();
    SetUploadCardsOutput("Done.");
  } else {
    SetUploadCardsOutput(absl::StrCat("Failed to import CSV: ", status).c_str());
  }
}

void Controller::ImportDataFromTextProto(const std::string& text_proto) {
  SetUploadCardsOutput(
      "Importing data... (if this doesn't change to Done then it means there was an error.)");
  std::optional<ProfileProto> new_profile = SafeParseTextProto<ProfileProto>(text_proto);
  if (new_profile.has_value()) {
    profile_proto_ = *new_profile;
    UpdateProfile();
    ResetView(profile_proto_);
    RefreshTeamInputs();
    SetUploadCardsOutput("Done.");
  } else {
    SetUploadCardsOutput("Failed to import data from text proto.");
  }
}

void Controller::ImportDataFromProto(const std::string& binary_proto) {
  SetUploadCardsOutput(
      "Importing data... (if this doesn't change to Done then it means there was an error.)");
  std::optional<ProfileProto> new_profile = SafeParseBinaryProto<ProfileProto>(binary_proto);
  if (new_profile.has_value()) {
    profile_proto_ = *new_profile;
    UpdateProfile();
    ResetView(profile_proto_);
    RefreshTeamInputs();
    SetUploadCardsOutput("Done.");
  } else {
    SetUploadCardsOutput("Failed to import data from binary proto.");
  }
}

void Controller::SetCardOwned(int card_id, bool state) {
  SetCardControls(card_id, state);
  if (!state) {
    profile_proto_.mutable_cards()->erase(card_id);
  } else {
    CardState state = CreateMaxCardState(card_id);
    auto [new_state, success] = profile_proto_.mutable_cards()->emplace(card_id, state);
    UpdateCardState(card_id, new_state->second);
  }
  UpdateProfile();
  RefreshTeamInputs();
}

CardState* Controller::GetCardState(int card_id) {
  auto state = profile_proto_.mutable_cards()->find(card_id);
  if (state == profile_proto_.mutable_cards()->end()) {
    return nullptr;
  }
  return &state->second;
}

void Controller::SetCardLevel(int card_id, int level) {
  CardState* state = GetCardState(card_id);
  if (state == nullptr) {
    LOG(ERROR) << "Invalid card_id: " << card_id;
    return;
  }
  if (state->level() == level) return;
  state->set_level(level);
  UpdateProfile();
}

void Controller::SetCardMasterRank(int card_id, int level) {
  CardState* state = GetCardState(card_id);
  if (state == nullptr) {
    LOG(ERROR) << "Invalid card_id: " << card_id;
    return;
  }
  if (state->master_rank() == level) return;
  state->set_master_rank(level);
  UpdateProfile();
}

void Controller::SetCardSkillLevel(int card_id, int level) {
  CardState* state = GetCardState(card_id);
  if (state == nullptr) {
    LOG(ERROR) << "Invalid card_id: " << card_id;
    return;
  }
  if (state->skill_level() == level) return;
  state->set_skill_level(level);
  UpdateProfile();
}

void Controller::SetCardTrained(int card_id, bool state) {
  CardState* card_state = GetCardState(card_id);
  if (card_state == nullptr) {
    LOG(ERROR) << "Invalid card_id: " << card_id;
    return;
  }
  if (card_state->special_training() == state) return;
  card_state->set_special_training(state);
  UpdateProfile();
}

void Controller::SetCardCanvasCrafted(int card_id, bool state) {
  CardState* card_state = GetCardState(card_id);
  if (card_state == nullptr) {
    LOG(ERROR) << "Invalid card_id: " << card_id;
    return;
  }
  if (card_state->canvas_crafted() == state) return;
  card_state->set_canvas_crafted(state);
  UpdateProfile();
}

void Controller::SetCardEpisodeRead(int card_id, int episode_id, bool state) {
  CardState* card_state = GetCardState(card_id);
  if (card_state == nullptr) {
    LOG(ERROR) << "Invalid card_id: " << card_id;
    return;
  }
  auto it = absl::c_find(*card_state->mutable_card_episodes_read(), episode_id);
  if (!state) {
    if (it == card_state->mutable_card_episodes_read()->end()) {
      // Nothing to do.
      return;
    }
    card_state->mutable_card_episodes_read()->erase(it);
  } else {
    if (it != card_state->mutable_card_episodes_read()->end()) {
      // Nothing to do.
      return;
    }
    card_state->add_card_episodes_read(episode_id);
  }
  UpdateProfile();
}

void Controller::SetEventBonusByEvent(int event_id, int chapter_id) {
  if (event_id <= 0) {
    estimator_mode_ = sekai::Estimator::Mode::kMulti;
    profile_proto_.clear_event_id();
  } else {
    profile_proto_.mutable_event_id()->set_event_id(event_id);
    if (chapter_id > 0) {
      profile_proto_.mutable_event_id()->set_chapter_id(chapter_id);
    } else {
      profile_proto_.mutable_event_id()->clear_chapter_id();
    }
  }
  UpdateProfile();
}

void Controller::BuildChallengeLiveTeam(int char_id) {
  constexpr int kTeamBuilderOutputSlot = 1;
  if (char_id <= 0) {
    SetTeamBuilderOutput("ERROR: invalid character ID");
    return;
  }
  ChallengeLiveTeamBuilder builder{char_id};

  std::vector<sekai::Team> teams =
      builder.RecommendTeams(profile_.TeamBuilderCardPool(), profile_, event_bonus_, cl_estimator_);
  if (teams.empty()) {
    SetTeamBuilderOutput("WARNING: no teams built.");
  } else {
    teams[0].ReorderTeamForOptimalSkillValue();
    SetTeam(kTeamBuilderOutputSlot, teams[0]);
    SetTeamBuilderOutput("Done.");
  }
  RefreshTeam(kTeamBuilderOutputSlot);
}

void Controller::BuildEventTeam() {
  constexpr int kTeamBuilderOutputSlot = 0;
  SimulatedAnnealingTeamBuilder builder{{
      .early_exit_steps = 2'000'000,
      .enable_progress = false,
      .world_bloom_version = GetWorldBloomVersion(profile_proto_),
  }};
  builder.AddConstraints(constraints_);

  std::vector<sekai::Team> teams =
      builder.RecommendTeams(profile_.TeamBuilderCardPool(), profile_, event_bonus_, estimator());
  if (teams.empty()) {
    SetTeamBuilderOutput("WARNING: no teams built.");
  } else {
    teams[0].ReorderTeamForOptimalSkillValue(builder.constraints());
    SetTeam(kTeamBuilderOutputSlot, teams[0]);
    SetTeamBuilderOutput("Done.");
  }
  RefreshTeam(kTeamBuilderOutputSlot);
}

void Controller::OnSaveDataRead() {
  ResetView(profile_proto_);
  RefreshTeamInputs();
}

bool Controller::IsValidCard(int card_id) const { return profile_.GetCard(card_id) != nullptr; }

void Controller::SetTeamCard(int team_index, int card_index, int card_id,
                             bool use_untrained_skill) {
  if (teams_[team_index][card_index].card_id == card_id &&
      teams_[team_index][card_index].use_untrained_skill == use_untrained_skill)
    return;
  teams_[team_index][card_index] = {
      .card_id = card_id,
      .use_untrained_skill = use_untrained_skill,
  };
  RefreshTeam(team_index);
}

void Controller::SetTeamCardFromCard(int team_index, int card_index,
                                     absl::Nullable<const sekai::Card*> card) {
  if (card == nullptr) {
    SetTeamCardId(team_index, card_index, 0, false);
    SetTeamCard(team_index, card_index, 0, false);
  } else {
    bool use_untrained_skill = card->HasSecondarySkill() && !card->UseSecondarySkill();
    SetTeamCardId(team_index, card_index, card->card_id(), use_untrained_skill);
    SetTeamCard(team_index, card_index, card->card_id(), use_untrained_skill);
  }
}

void Controller::SetTeam(int team_index, const sekai::Team& team) {
  for (int i = 0; i < kTeamSize; ++i) {
    SetTeamCardFromCard(team_index, i, i < team.cards().size() ? team.cards()[i] : nullptr);
  }
}

void Controller::ClearTeamCard(int team_index, int card_index) {
  if (teams_[team_index][card_index].card_id == 0) return;
  teams_[team_index][card_index] = {};
  RefreshTeam(team_index);
}

void Controller::ClearTeam(int team_index) {
  teams_[team_index].fill({});
  for (int i = 0; i < kTeamSize; ++i) {
    SetTeamCardId(team_index, i, 0, false);
  }
  RefreshTeam(team_index);
}

void Controller::RefreshTeams() const {
  for (int i = 0; i < kNumTeams; ++i) {
    RefreshTeam(i);
  }
}

void Controller::RefreshTeam(int team_index) const {
  std::vector<const sekai::Card*> cards;
  for (int i = 0; i < kTeamSize; ++i) {
    const TeamCard& team_card = teams_[team_index][i];
    if (team_card.card_id == 0) continue;
    const sekai::Card* card = profile_.GetCard(team_card.card_id);
    if (card == nullptr) continue;
    if (card->HasSecondarySkill() && card->state().special_training() &&
        !team_card.use_untrained_skill) {
      card = profile_.GetSecondaryCard(team_card.card_id);
      // This should never be null here. Note that the secondary card is only
      // inserted if it is eligible.
      ABSL_CHECK_NE(card, nullptr);
    }
    cards.push_back(card);
  }
  sekai::Team team(cards);
  if (team_index != 2) {
    team.FillSupportCards(profile_.sorted_support(), GetWorldBloomVersion(profile_proto_));
  }
  sekai::TeamProto team_proto = team.ToProto(profile_, event_bonus_, estimator());
  if (target_points_.has_value() && team.cards().size() == kTeamSize) {
    OptimizeExactPoints objective = UnsafeGetParkingObjective();
    objective.AnnotateTeamProto(team, profile_, event_bonus_, team_proto);
    sekai::AnnotateTeamProtoWithMultiTurnParkingStrategy(
        team, profile_, event_bonus_, park_accuracy(), *target_points_, team_proto);
  }
  if (team.chars_present().count() == 1) {
    cl_estimator_.AnnotateTeamProto(profile_, event_bonus_, team, team_proto);
  }
  TeamContext context = CreateTeamContext(team_proto);
  std::string json;
  (void)MessageToJsonString(context, &json);
  RenderTeam(team_index, json.c_str());
}

void Controller::SetUseOldSubunitlessBonus(bool state) {
  profile_proto_.set_use_old_subunitless_bonus(state);
  UpdateProfile();
}

void Controller::SetLeadConstraint(int char_id, bool state) {
  if (state) {
    profile_proto_.mutable_exclude_lead_char_ids()->erase(
        std::remove_if(profile_proto_.mutable_exclude_lead_char_ids()->begin(),
                       profile_proto_.mutable_exclude_lead_char_ids()->end(),
                       [&](int c) { return c == char_id; }),
        profile_proto_.mutable_exclude_lead_char_ids()->end());
  } else {
    if (!absl::c_linear_search(*profile_proto_.mutable_exclude_lead_char_ids(), char_id)) {
      profile_proto_.add_exclude_lead_char_ids(char_id);
    }
  }
  UpdateProfile();
}

void Controller::SetKizunaConstraint(int char_1, int char_2, bool state) {
  if (char_1 > char_2) {
    std::swap(char_1, char_2);
  }
  if (state) {
    profile_proto_.mutable_exclude_kizuna_pairs()->erase(
        std::remove_if(profile_proto_.mutable_exclude_kizuna_pairs()->begin(),
                       profile_proto_.mutable_exclude_kizuna_pairs()->end(),
                       [&](const ProfileProto::KizunaPair& pair) {
                         return (pair.char_1() == char_1 && pair.char_2() == char_2) ||
                                (pair.char_1() == char_2 && pair.char_2() == char_1);
                       }),
        profile_proto_.mutable_exclude_kizuna_pairs()->end());
  } else {
    if (absl::c_none_of(*profile_proto_.mutable_exclude_kizuna_pairs(),
                        [&](const ProfileProto::KizunaPair& pair) {
                          return (pair.char_1() == char_1 && pair.char_2() == char_2) ||
                                 (pair.char_1() == char_2 && pair.char_2() == char_1);
                        })) {
      ProfileProto::KizunaPair& pair = *profile_proto_.add_exclude_kizuna_pairs();
      pair.set_char_1(char_1);
      pair.set_char_2(char_2);
    }
  }
  UpdateProfile();
}

void Controller::SetRarityConstraint(int rarity, bool state) {
  if (!sekai::db::CardRarityType_IsValid(rarity)) {
    LOG(ERROR) << "Invalid rarity: " << rarity;
    return;
  }
  if (state) {
    profile_proto_.mutable_exclude_rarities()->erase(
        std::remove_if(profile_proto_.mutable_exclude_rarities()->begin(),
                       profile_proto_.mutable_exclude_rarities()->end(),
                       [&](int c) { return c == rarity; }),
        profile_proto_.mutable_exclude_rarities()->end());
  } else {
    if (!absl::c_linear_search(*profile_proto_.mutable_exclude_rarities(), rarity)) {
      profile_proto_.add_exclude_rarities(static_cast<CardRarityType>(rarity));
    }
  }
  UpdateProfile();
}

void Controller::SetMinLeadSkill(int value) {
  profile_proto_.set_min_lead_skill(value);
  UpdateProfile();
}

void Controller::SetCustomEventAttr(int attr) {
  if (!sekai::db::Attr_IsValid(attr)) {
    LOG(ERROR) << "Invalid attr: " << attr;
    return;
  }
  if (attr == 0) {
    profile_proto_.mutable_custom_event()->clear_attr();
  } else {
    profile_proto_.mutable_custom_event()->set_attr(static_cast<Attr>(attr));
  }
  UpdateProfile();
}

void Controller::SetCustomEventCharacter(int char_id, int unit_id, bool state) {
  SimpleEventBonus& event_bonus = *profile_proto_.mutable_custom_event();
  if (!sekai::db::Unit_IsValid(unit_id)) {
    LOG(ERROR) << "Invalid unit: " << unit_id;
    return;
  }
  if (!state) {
    event_bonus.mutable_chars()->erase(
        std::remove_if(event_bonus.mutable_chars()->begin(), event_bonus.mutable_chars()->end(),
                       [&](const SimpleEventBonus::CharacterAndUnit& char_and_unit) {
                         if (unit_id == 0) {
                           return char_and_unit.char_id() == char_id &&
                                  (!char_and_unit.has_unit() ||
                                   static_cast<int>(char_and_unit.unit()) == unit_id);
                         }
                         return char_and_unit.char_id() == char_id &&
                                char_and_unit.unit() == static_cast<Unit>(unit_id);
                       }),
        event_bonus.mutable_chars()->end());
  } else {
    if (absl::c_none_of(*event_bonus.mutable_chars(),
                        [&](const SimpleEventBonus::CharacterAndUnit& char_and_unit) {
                          if (unit_id == 0) {
                            return char_and_unit.char_id() == char_id &&
                                   (!char_and_unit.has_unit() ||
                                    static_cast<int>(char_and_unit.unit()) == unit_id);
                          }
                          return char_and_unit.char_id() == char_id &&
                                 char_and_unit.unit() == static_cast<Unit>(unit_id);
                        })) {
      SimpleEventBonus::CharacterAndUnit& char_and_unit = *event_bonus.add_chars();
      char_and_unit.set_char_id(char_id);
      if (unit_id > 0) {
        char_and_unit.set_unit(static_cast<Unit>(unit_id));
      }
    }
  }
  UpdateProfile();
}

void Controller::SetCustomEventChapter(int char_id) {
  if (char_id == 0) {
    profile_proto_.mutable_custom_event()->clear_chapter_char_id();
  } else {
    profile_proto_.mutable_custom_event()->set_chapter_char_id(char_id);
  }
  UpdateProfile();
}

void Controller::SetCustomEventWorldBloomVersion(int version) {
  if (!sekai::WorldBloomVersion_IsValid(version) || version == 0) {
    LOG(ERROR) << "Invalid version: " << version;
    return;
  }
  profile_proto_.set_world_bloom_version(static_cast<WorldBloomVersion>(version));
  UpdateProfile();
}

void Controller::SetTargetPoints(int value) {
  if (value == 0) {
    target_points_ = std::nullopt;
  } else {
    target_points_ = value;
  }
  RefreshTeams();
}

void Controller::SetParkAccuracy(int value) {
  park_accuracy_ = value;
  RefreshTeams();
}

sekai::OptimizeExactPoints Controller::UnsafeGetParkingObjective() const {
  return {target_points_.value(), park_accuracy()};
}

void Controller::BuildParkingTeam(bool ignore_constraints) {
  constexpr int kTeamBuilderOutputSlot = 2;
  if (!target_points_.has_value()) {
    SetTeamBuilderOutput("ERROR: no target points set");
    return;
  }

  OptimizeExactPoints objective = UnsafeGetParkingObjective();
  SimulatedAnnealingTeamBuilder builder{{
                                            .early_exit_steps = 2'000'000,
                                            .enable_progress = false,
                                            .disable_support = true,
                                        },
                                        objective};
  if (!ignore_constraints) {
    builder.AddConstraints(constraints_);
  }

  std::vector<sekai::Team> teams =
      builder.RecommendTeams(profile_.TeamBuilderCardPool(), profile_, event_bonus_, estimator());
  sekai::Character all_chars;
  all_chars.set();
  if (teams.empty()) {
    ClearTeam(kTeamBuilderOutputSlot);
    SetTeamBuilderOutput("WARNING: no teams built.");
  } else if (objective.GetObjectiveFunction()(teams[0], profile_, event_bonus_, estimator(),
                                              all_chars) < 0) {
    LOG(INFO) << "Closest found:\n" << teams[0];
    ClearTeam(kTeamBuilderOutputSlot);
    SetTeamBuilderOutput("WARNING: no viable teams.");
  } else {
    teams[0].ReorderTeamForOptimalSkillValue(builder.constraints());
    SetTeam(kTeamBuilderOutputSlot, teams[0]);
    SetTeamBuilderOutput("Done.");
  }
  RefreshTeam(kTeamBuilderOutputSlot);
}

void Controller::BuildFillTeam(bool ignore_constraints, int min_power) {
  constexpr int kTeamBuilderOutputSlot = 1;

  OptimizeFillTeam objective{min_power};
  SimulatedAnnealingTeamBuilder builder{
      {
          .early_exit_steps = min_power > 0 ? 200'000 : 2'000'000,
          .enable_progress = false,
          .world_bloom_version = GetWorldBloomVersion(profile_proto_),
      },
      objective};
  if (!ignore_constraints) {
    builder.AddConstraints(constraints_);
  }

  std::vector<sekai::Team> teams =
      min_power > 0 ? PartitionedBuildTeam(builder, profile_.TeamBuilderCardPool(), profile_,
                                           event_bonus_, estimator())
                    : builder.RecommendTeams(profile_.TeamBuilderCardPool(), profile_, event_bonus_,
                                             estimator());
  if (teams.empty()) {
    ClearTeam(kTeamBuilderOutputSlot);
    SetTeamBuilderOutput("WARNING: no teams built.");
  } else if (teams[0].Power(profile_) < min_power) {
    LOG(INFO) << "Closest found:\n" << teams[0];
    ClearTeam(kTeamBuilderOutputSlot);
    SetTeamBuilderOutput("WARNING: no viable teams.");
  } else {
    teams[0].ReorderTeamForOptimalSkillValue(builder.constraints());
    SetTeam(kTeamBuilderOutputSlot, teams[0]);
    SetTeamBuilderOutput("Done.");
  }
  RefreshTeam(kTeamBuilderOutputSlot);
}

std::string Controller::SerializeStateToTextProto() const {
  std::string output;
  if (!TextFormat::PrintToString(profile_proto_, &output)) {
    LOG(ERROR) << "Failed to convert profile to textproto";
    return "";
  }
  return output;
}

emscripten::val Controller::SerializeStateToString() {
  profile_proto_bytes_ = profile_proto_.SerializeAsString();
  return emscripten::val(
      emscripten::typed_memory_view(profile_proto_bytes_.length(), profile_proto_bytes_.data()));
}

void Controller::ClearSerializedStringState() {
  profile_proto_bytes_.clear();
  profile_proto_bytes_.shrink_to_fit();
}

EMSCRIPTEN_BINDINGS(controller) {
  class_<Controller, base<ControllerBase>>("Controller")
      .constructor<>()
      .function("BuildChallengeLiveTeam", &Controller::BuildChallengeLiveTeam)
      .function("BuildEventTeam", &Controller::BuildEventTeam)
      .function("BuildFillTeam", &Controller::BuildFillTeam)
      .function("BuildParkingTeam", &Controller::BuildParkingTeam)
      .function("ClearSerializedStringState", &Controller::ClearSerializedStringState)
      .function("ClearTeamCard", &Controller::ClearTeamCard)
      .function("ImportCardsFromCsv", &Controller::ImportCardsFromCsv)
      .function("ImportDataFromProto", &Controller::ImportDataFromProto)
      .function("ImportDataFromTextProto", &Controller::ImportDataFromTextProto)
      .function("IsValidCard", &Controller::IsValidCard)
      .function("RefreshTeams", &Controller::RefreshTeams)
      .function("SerializeStateToString", &Controller::SerializeStateToString)
      .function("SerializeStateToTextProto", &Controller::SerializeStateToTextProto)
      .function("SetAreaItemLevel", &Controller::SetAreaItemLevel)
      .function("SetCardCanvasCrafted", &Controller::SetCardCanvasCrafted)
      .function("SetAttrFilterState", &Controller::SetAttrFilterState)
      .function("SetCardEpisodeRead", &Controller::SetCardEpisodeRead)
      .function("SetCardLevel", &Controller::SetCardLevel)
      .function("SetCardMasterRank", &Controller::SetCardMasterRank)
      .function("SetCardOwned", &Controller::SetCardOwned)
      .function("SetCardSkillLevel", &Controller::SetCardSkillLevel)
      .function("SetCardTrained", &Controller::SetCardTrained)
      .function("SetCharacterFilterState", &Controller::SetCharacterFilterState)
      .function("SetCharacterRank", &Controller::SetCharacterRank)
      .function("SetCustomEventAttr", &Controller::SetCustomEventAttr)
      .function("SetCustomEventChapter", &Controller::SetCustomEventChapter)
      .function("SetCustomEventCharacter", &Controller::SetCustomEventCharacter)
      .function("SetCustomEventWorldBloomVersion", &Controller::SetCustomEventWorldBloomVersion)
      .function("SetEventBonusByEvent", &Controller::SetEventBonusByEvent)
      .function("SetKizunaConstraint", &Controller::SetKizunaConstraint)
      .function("SetLeadConstraint", &Controller::SetLeadConstraint)
      .function("SetMinLeadSkill", &Controller::SetMinLeadSkill)
      .function("SetMySekaiFixtureCrafted", &Controller::SetMySekaiFixtureCrafted)
      .function("SetMySekaiGateLevel", &Controller::SetMySekaiGateLevel)
      .function("SetOwnedCardsFilterState", &Controller::SetOwnedCardsFilterState)
      .function("SetParkAccuracy", &Controller::SetParkAccuracy)
      .function("SetRarityFilterState", &Controller::SetRarityFilterState)
      .function("SetRarityConstraint", &Controller::SetRarityConstraint)
      .function("SetTargetPoints", &Controller::SetTargetPoints)
      .function("SetTeamCard", &Controller::SetTeamCard)
      .function("SetTitleBonus", &Controller::SetTitleBonus)
      .function("SetUnreleasedContentFilterState", &Controller::SetUnreleasedContentFilterState)
      .function("SetUseOldSubunitlessBonus", &Controller::SetUseOldSubunitlessBonus);
}

}  // namespace frontend
