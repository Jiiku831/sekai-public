#include "sekai/profile.h"

#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "absl/base/no_destructor.h"
#include "absl/base/nullability.h"
#include "absl/log/absl_check.h"
#include "absl/log/log.h"
#include "absl/strings/ascii.h"
#include "absl/strings/numbers.h"
#include "absl/strings/str_split.h"
#include "base/util.h"
#include "sekai/array_size.h"
#include "sekai/bonus_limit.h"
#include "sekai/card.h"
#include "sekai/db/master_db.h"
#include "sekai/db/proto/all.h"
#include "sekai/fixtures.h"
#include "sekai/max_level.h"
#include "sekai/team_builder/pool_utils.h"

namespace sekai {

namespace {

using ::sekai::db::MasterDb;

void LoadAreaItemBonus(const ProfileProto& profile,
                       std::array<BonusRate, db::Attr_ARRAYSIZE>& attr_bonus,
                       std::vector<BonusRate>& char_bonus,
                       std::array<BonusRate, db::Unit_ARRAYSIZE>& unit_bonus) {
  for (const db::AreaItemLevel& area_item_level : MasterDb::GetAll<db::AreaItemLevel>()) {
    ABSL_CHECK_LT(area_item_level.area_item_id(),
                  static_cast<int64_t>(profile.area_item_levels_size()));
    if (area_item_level.level() != profile.area_item_levels(area_item_level.area_item_id()))
      continue;
    if (area_item_level.has_target_unit()) {
      BonusRate& rate = unit_bonus[area_item_level.target_unit()];
      rate.rate += area_item_level.power1_bonus_rate();
      rate.matching_rate += area_item_level.power1_all_match_bonus_rate();
    }
    if (area_item_level.has_target_card_attr()) {
      BonusRate& rate = attr_bonus[area_item_level.target_card_attr()];
      rate.rate += area_item_level.power1_bonus_rate();
      rate.matching_rate += area_item_level.power1_all_match_bonus_rate();
    }
    if (area_item_level.has_target_game_character_id()) {
      ABSL_CHECK_LT(area_item_level.target_game_character_id(),
                    static_cast<int64_t>(char_bonus.size()));
      BonusRate& rate = char_bonus[area_item_level.target_game_character_id()];
      rate.rate += area_item_level.power1_bonus_rate();
    }
  }
}

void LoadMySekaiFixtureBonus(const ProfileProto& profile,
                             std::array<int, kCharacterArraySize>& fixture_bonus) {
  fixture_bonus.fill(0);
  for (const auto& [fixture_id, crafted] : profile.mysekai_fixture_crafted()) {
    if (!crafted) continue;
    const db::MySekaiFixture* fixture = MasterDb::SafeFindFirst<db::MySekaiFixture>(fixture_id);
    if (fixture == nullptr) {
      LOG(WARNING) << "Ignoring unknown fixture: " << fixture_id;
      continue;
    }
    if (!fixture->has_mysekai_fixture_game_character_group_performance_bonus_id()) {
      LOG(WARNING) << "Ignoring fixture with no bonus: " << fixture_id;
      continue;
    }
    const db::MySekaiFixtureGameCharacterGroupPerformanceBonus& bonus =
        MasterDb::FindFirst<db::MySekaiFixtureGameCharacterGroupPerformanceBonus>(
            fixture->mysekai_fixture_game_character_group_performance_bonus_id());
    const db::MySekaiFixtureGameCharacterGroup& group =
        MasterDb::FindFirst<db::MySekaiFixtureGameCharacterGroup>(
            bonus.mysekai_fixture_game_character_group_id());
    fixture_bonus[group.game_character_id()] =
        std::min(fixture_bonus[group.game_character_id()] + bonus.bonus_rate(),
                 kMaxMySekaiFixtureBonusLimit);
  }
}

void LoadMySekaiGateBonus(const ProfileProto& profile,
                          std::array<float, db::Unit_ARRAYSIZE>& gate_bonus) {
  gate_bonus.fill(0);
  for (int gate_id = 0; gate_id < profile.mysekai_gate_levels_size(); ++gate_id) {
    const int gate_level = profile.mysekai_gate_levels(gate_id);
    if (gate_level <= 0) continue;
    const db::MySekaiGate* gate = MasterDb::SafeFindFirst<db::MySekaiGate>(gate_id);
    if (gate == nullptr) {
      LOG(WARNING) << "Ignoring unknown gate: " << gate_id;
      continue;
    }
    std::vector<const db::MySekaiGateLevel*> levels =
        MasterDb::FindAll<db::MySekaiGateLevel>(gate_id);
    bool level_found = false;
    for (const db::MySekaiGateLevel* level : levels) {
      if (level->level() == gate_level) {
        level_found = true;
        gate_bonus[gate->unit()] = level->power_bonus_rate();
        gate_bonus[db::UNIT_VS] = std::max(gate_bonus[db::UNIT_VS], level->power_bonus_rate());
        break;
      }
    }
    ABSL_CHECK(level_found) << "Invalid level " << gate_level << " for gate " << gate_id;
  }
}

void LoadCharacterRankBonus(const ProfileProto& profile, std::vector<BonusRate>& cr_bonus) {
  for (const db::CharacterRank& rank : MasterDb::GetAll<db::CharacterRank>()) {
    ABSL_CHECK_LT(rank.character_id(), static_cast<int64_t>(profile.character_ranks_size()));
    if (rank.character_rank() != profile.character_ranks(rank.character_id())) continue;
    ABSL_CHECK_LT(rank.character_id(), static_cast<int64_t>(cr_bonus.size()));
    cr_bonus[rank.character_id()].rate = rank.power1_bonus_rate();
  }
}

// Returns true if the card was successfully inserted.
bool AddCard(const db::Card& card, const CardState& state, absl::flat_hash_map<int, Card>& cards,
             absl::flat_hash_map<int, Card>& secondary_cards) {
  auto [new_card, inserted] = cards.emplace(card.id(), Card{card, state});
  if (!inserted) {
    return inserted;
  }
  if (new_card->second.HasSecondarySkill() && state.special_training()) {
    auto [secondary_card, unused] = secondary_cards.emplace(card.id(), Card{card, state});
    secondary_card->second.UseSecondarySkill(true);
  }
  return inserted;
}

void LoadCards(const ProfileProto& profile, absl::flat_hash_map<int, Card>& cards,
               absl::flat_hash_map<int, Card>& secondary_cards) {
  for (const auto& [card_id, state] : profile.cards()) {
    const db::Card* card = MasterDb::SafeFindFirst<db::Card>(card_id);
    if (card == nullptr) {
      LOG(WARNING) << "Card not found, skipping: " << card_id;
      continue;
    }
    AddCard(*card, state, cards, secondary_cards);
  }
}

bool TryParseInt(std::string_view str, int& out) {
  if (str.empty()) return false;
  int tmp;
  if (absl::SimpleAtoi(str, &tmp)) {
    std::swap(tmp, out);
    return true;
  }
  return false;
}

}  // namespace

Profile::Profile()
    : attr_bonus_(),
      char_bonus_(CharacterArraySize()),
      cr_bonus_(CharacterArraySize()),
      character_rank_(CharacterArraySize()),
      unit_bonus_() {}

Profile::Profile(const ProfileProto& profile) : Profile() {
  bonus_power_ = profile.bonus_power();
  LoadAreaItemBonus(profile, attr_bonus_, char_bonus_, unit_bonus_);
  LoadCharacterRankBonus(profile, cr_bonus_);
  LoadMySekaiFixtureBonus(profile, mysekai_fixture_bonus_);
  LoadMySekaiGateBonus(profile, mysekai_gate_bonus_);
  for (int char_id : UniqueCharacterIds()) {
    ABSL_CHECK_LT(char_id, static_cast<int>(profile.character_ranks_size()));
    character_rank_[char_id] = profile.character_ranks(char_id);
  }

  LoadCards(profile, cards_, secondary_cards_);
  ApplyProfileBonus();
}

void Profile::ApplyProfileBonus() {
  for (auto& [unused_id, card] : cards_) {
    card.ApplyProfilePowerBonus(*this);
  }
  for (auto& [unused_id, card] : secondary_cards_) {
    card.ApplyProfilePowerBonus(*this);
  }
}

int Profile::character_rank(int char_id) const {
  ABSL_CHECK_LT(char_id, static_cast<int>(character_rank_.size()));
  return character_rank_[char_id];
}

void Profile::LoadCardsFromCsv(std::filesystem::path path) {
  std::ifstream fin(path);
  std::stringstream ss;
  ss << fin.rdbuf();
  ABSL_CHECK_OK(LoadCardsFromCsv(ss));
}

absl::Status Profile::LoadCardsFromCsvString(std::string_view contents) {
  std::stringstream ss;
  ss << contents;
  return LoadCardsFromCsv(ss);
}

absl::StatusOr<int> TryGetOffset(std::span<const std::string> header) {
  if (header.size() <= 4) {
    return absl::InvalidArgumentError(absl::StrCat(
        "Header row has invalid length, expected at least 4 columns got: ", header.size()));
  }
  if (header[3] == "canvas") {
    return 1;
  }
  return 0;
}

absl::Status Profile::LoadCardsFromCsv(std::stringstream& ss) {
  int row = 0;
  int offset = 0;
  for (std::string line; std::getline(ss, line);) {
    ++row;
    std::vector<std::string> parts = absl::StrSplit(line, ',');
    if (row == 1) {
      ASSIGN_OR_RETURN(offset, TryGetOffset(parts));
    }
    if (parts[0] != "TRUE") continue;
    if (parts.empty()) continue;
    if (parts.size() <= static_cast<std::size_t>(7 + offset)) {
      return absl::InvalidArgumentError(absl::StrCat("Invalid input row: ", row));
    }
    int card_id = 0;
    if (!TryParseInt(parts[7 + offset], card_id)) {
      return absl::InvalidArgumentError(
          absl::StrCat("Failed to parse card id as number: '", parts[1], "' on row ", row));
    }
    int master_rank = 0;
    int skill_level = 1;
    if (std::string_view mr_str = absl::StripAsciiWhitespace(parts[1]);
        !mr_str.empty() && !TryParseInt(mr_str, master_rank)) {
      return absl::InvalidArgumentError(absl::StrCat("Failed to parse master rank as number: '",
                                                     parts[1], "' for card ", card_id, " on row ",
                                                     row));
    }
    if (master_rank < 0) {
      LOG(WARNING) << "Clamping master rank for card " << card_id << " to 0";
      master_rank = 0;
    }
    if (master_rank > 5) {
      LOG(WARNING) << "Clamping master rank for card " << card_id << " to 5";
      master_rank = 5;
    }
    if (std::string_view sl_str = absl::StripAsciiWhitespace(parts[2]);
        !sl_str.empty() && !TryParseInt(sl_str, skill_level)) {
      return absl::InvalidArgumentError(absl::StrCat("Failed to parse skill level as number: '",
                                                     parts[1], "' for card ", card_id, " on row ",
                                                     row));
    }
    if (skill_level < 1) {
      LOG(WARNING) << "Clamping skill level for card " << card_id << " to 1";
      skill_level = 1;
    }
    if (skill_level > 4) {
      LOG(WARNING) << "Clamping skill level for card " << card_id << " to 4";
      skill_level = 4;
    }
    bool canvas_crafted = false;
    if (offset > 0 && parts[3] == "TRUE") {
      canvas_crafted = true;
    }
    const db::Card* card = MasterDb::SafeFindFirst<db::Card>(card_id);
    if (card == nullptr) {
      LOG(WARNING) << "Card " << card_id << " not found in master db. skipping";
      continue;
    }
    CardState state = CreateMaxCardState(card_id);
    state.set_master_rank(master_rank);
    state.set_skill_level(skill_level);
    state.set_canvas_crafted(canvas_crafted);
    bool success = AddCard(*card, state, cards_, secondary_cards_);
    if (!success) {
      LOG(WARNING) << "Found duplicate card ID " << card_id << ". skipping";
      continue;
    }
  }
  ApplyProfileBonus();
  return absl::OkStatus();
}

void Profile::ApplyEventBonus(const EventBonus& event_bonus) {
  for (auto& [unused_id, card] : cards_) {
    card.ApplyEventBonus(event_bonus);
  }
  for (auto& [unused_id, card] : secondary_cards_) {
    card.ApplyEventBonus(event_bonus);
  }
  sorted_support_ = GetSortedSupportPool(PrimaryCardPool());
}

absl::Nullable<const Card*> Profile::GetCard(int card_id) const {
  auto card = cards_.find(card_id);
  if (card == cards_.end()) return nullptr;
  return &card->second;
}

absl::Nullable<const Card*> Profile::GetSecondaryCard(int card_id) const {
  auto card = secondary_cards_.find(card_id);
  if (card == secondary_cards_.end()) return nullptr;
  return &card->second;
}

ProfileProto Profile::CardsToProto() const {
  ProfileProto profile;
  for (const auto& [card_id, card] : cards_) {
    (*profile.mutable_cards())[card_id] = card.state();
  }
  return profile;
}

std::vector<const Card*> Profile::TeamBuilderCardPool() const {
  std::vector<const Card*> out;
  out.reserve(cards_.size());
  for (const auto& [unused_id, card] : cards_) {
    out.push_back(&card);
  }
  for (const auto& [unused_id, card] : secondary_cards_) {
    out.push_back(&card);
  }
  return out;
}

std::vector<const Card*> Profile::PrimaryCardPool() const {
  std::vector<const Card*> out;
  out.reserve(cards_.size());
  for (const auto& [unused_id, card] : cards_) {
    out.push_back(&card);
  }
  return out;
}

ProfileProto EmptyProfileProto() {
  ProfileProto profile;
  profile.mutable_area_item_levels()->Resize(AreaItemArraySize(), 0);
  profile.mutable_character_ranks()->Resize(CharacterArraySize(), 1);
  return profile;
}

const Profile& Profile::Min() {
  static const absl::NoDestructor<Profile> kMinProfile{EmptyProfileProto()};
  return *kMinProfile;
}
const Profile& Profile::Max() {
  static const absl::NoDestructor<Profile> kMaxProfile{[] {
    ProfileProto profile_proto;
    profile_proto.mutable_area_item_levels()->Resize(AreaItemArraySize(), kMaxAreaItemLevel);
    profile_proto.mutable_character_ranks()->Resize(CharacterArraySize(), kMaxCharacterRank);
    profile_proto.set_bonus_power(kMaxTitleBonus);
    profile_proto.mutable_mysekai_gate_levels()->Resize(MySekaiGateArraySize(),
                                                        kMaxMySekaiGateLevel);
    profile_proto.set_mysekai_gate_levels(0, 0);
    for (const db::MySekaiFixture* fixture : FixturesWithCharBonuses()) {
      (*profile_proto.mutable_mysekai_fixture_crafted())[fixture->id()] = true;
    }
    return profile_proto;
  }()};
  return *kMaxProfile;
}

}  // namespace sekai
