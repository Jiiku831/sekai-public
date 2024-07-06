#include "sekai/profile.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "absl/base/nullability.h"
#include "absl/log/absl_check.h"
#include "absl/log/log.h"
#include "absl/strings/numbers.h"
#include "absl/strings/str_split.h"
#include "sekai/array_size.h"
#include "sekai/card.h"
#include "sekai/db/master_db.h"
#include "sekai/db/proto/all.h"
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

void LoadCharacterRankBonus(const ProfileProto& profile, std::vector<BonusRate>& cr_bonus) {
  for (const db::CharacterRank& rank : MasterDb::GetAll<db::CharacterRank>()) {
    ABSL_CHECK_LT(rank.character_id(), static_cast<int64_t>(profile.character_ranks_size()));
    if (rank.character_rank() != profile.character_ranks(rank.character_id())) continue;
    ABSL_CHECK_LT(rank.character_id(), static_cast<int64_t>(cr_bonus.size()));
    cr_bonus[rank.character_id()].rate = rank.power1_bonus_rate();
  }
}

void LoadCards(const ProfileProto& profile, absl::flat_hash_map<int, Card>& cards) {
  for (const auto& [card_id, state] : profile.cards()) {
    cards.emplace(card_id, Card{MasterDb::FindFirst<db::Card>(card_id), state});
  }
}

bool TryParseInt(std::string_view str, int& out) {
  if (str.empty()) return false;
  int tmp;
  if (absl::SimpleAtoi(str, &tmp)) {
    std::swap(tmp, out);
    return true;
  } else {
    return false;
  }
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
  for (int char_id : UniqueCharacterIds()) {
    ABSL_CHECK_LT(char_id, static_cast<int>(profile.character_ranks_size()));
    character_rank_[char_id] = profile.character_ranks(char_id);
  }

  LoadCards(profile, cards_);
  for (auto& [unused_id, card] : cards_) {
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

absl::Status Profile::LoadCardsFromCsv(std::stringstream& ss) {
  for (std::string line; std::getline(ss, line);) {
    std::vector<std::string> parts = absl::StrSplit(line, ',');
    if (parts[0] != "TRUE") continue;
    if (parts.empty()) continue;
    if (parts.size() <= 7) {
      return absl::InvalidArgumentError(absl::StrCat("Invalid input line: ", line));
    }
    int card_id = 0;
    if (!TryParseInt(parts[7], card_id)) {
      return absl::InvalidArgumentError(absl::StrCat("Failed to parse as int: ", parts[7]));
    }
    int master_rank = 0;
    int skill_level = 1;
    if (!parts[1].empty() && !TryParseInt(parts[1], master_rank)) {
      return absl::InvalidArgumentError(absl::StrCat("Failed to parse as int: ", parts[1]));
    }
    if (!parts[2].empty() && !TryParseInt(parts[2], skill_level)) {
      return absl::InvalidArgumentError(absl::StrCat("Failed to parse as int: ", parts[2]));
    }
    const db::Card& card = MasterDb::FindFirst<db::Card>(card_id);
    CardState state = CreateMaxCardState(card_id);
    state.set_master_rank(master_rank);
    state.set_skill_level(skill_level);
    auto [new_card, success] = cards_.emplace(card_id, Card{card, state});
    if (!success) {
      return absl::InvalidArgumentError(absl::StrCat("Found duplicate id: ", card_id));
    }
    new_card->second.ApplyProfilePowerBonus(*this);
  }
  return absl::OkStatus();
}

void Profile::ApplyEventBonus(const EventBonus& event_bonus) {
  for (auto& [unused_id, card] : cards_) {
    card.ApplyEventBonus(event_bonus);
  }
  sorted_support_ = GetSortedSupportPool(CardPtrs());
}

absl::Nullable<const Card*> Profile::GetCard(int card_id) const {
  auto card = cards_.find(card_id);
  if (card == cards_.end()) return nullptr;
  return &card->second;
}

ProfileProto Profile::CardsToProto() const {
  ProfileProto profile;
  for (const auto& [card_id, card] : cards_) {
    (*profile.mutable_cards())[card_id] = card.state();
  }
  return profile;
}

ProfileProto EmptyProfileProto() {
  ProfileProto profile;
  profile.mutable_area_item_levels()->Resize(AreaItemArraySize(), 0);
  profile.mutable_character_ranks()->Resize(CharacterArraySize(), 1);
  return profile;
}

}  // namespace sekai
