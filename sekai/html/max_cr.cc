#include "sekai/html/max_cr.h"

#include <algorithm>
#include <array>
#include <limits>
#include <string_view>
#include <variant>

#include <ctml.hpp>

#include "absl/log/absl_check.h"
#include "absl/log/log.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "frontend/display_text.h"
#include "sekai/array_size.h"
#include "sekai/character.h"
#include "sekai/max_character_rank.h"
#include "sekai/proto/max_character_rank.pb.h"
#include "sekai/proto_util.h"

namespace sekai::html {
namespace {

constexpr std::array<std::variant<db::CharacterMissionType, CharacterRankSource::OtherSource>, 35>
    source_order = {
        db::CHARACTER_MISSION_TYPE_COLLECT_MEMBER,
        db::CHARACTER_MISSION_TYPE_COLLECT_STAMP,
        db::CHARACTER_MISSION_TYPE_COLLECT_COSTUME_3D,
        db::CHARACTER_MISSION_TYPE_COLLECT_CHARACTER_ARCHIVE_VOICE,
        db::CHARACTER_MISSION_TYPE_COLLECT_ANOTHER_VOCAL,
        db::CHARACTER_MISSION_TYPE_READ_MYSEKAI_FIXTURE_TALK,
        db::CHARACTER_MISSION_TYPE_READ_AREA_TALK,
        db::CHARACTER_MISSION_TYPE_PLAY_LIVE,
        db::CHARACTER_MISSION_TYPE_PLAY_LIVE_EX,
        db::CHARACTER_MISSION_TYPE_WAITING_ROOM,
        db::CHARACTER_MISSION_TYPE_WAITING_ROOM_EX,
        db::CHARACTER_MISSION_TYPE_READ_CARD_EPISODE_FIRST,
        db::CHARACTER_MISSION_TYPE_READ_CARD_EPISODE_SECOND,
        db::CHARACTER_MISSION_TYPE_AREA_ITEM_LEVEL_UP_CHARACTER,
        db::CHARACTER_MISSION_TYPE_AREA_ITEM_LEVEL_UP_UNIT,
        db::CHARACTER_MISSION_TYPE_AREA_ITEM_LEVEL_UP_REALITY_WORLD,
        db::CHARACTER_MISSION_TYPE_SKILL_LEVEL_UP_RARE,
        db::CHARACTER_MISSION_TYPE_SKILL_LEVEL_UP_STANDARD,
        db::CHARACTER_MISSION_TYPE_MASTER_RANK_UP_RARE,
        db::CHARACTER_MISSION_TYPE_MASTER_RANK_UP_STANDARD,
        db::CHARACTER_MISSION_TYPE_COLLECT_MYSEKAI_FIXTURE,
        db::CHARACTER_MISSION_TYPE_COLLECT_MYSEKAI_CANVAS,
        CharacterRankSource::OTHER_SOURCE_CHALLENGE_LIVE,
        CharacterRankSource::OTHER_SOURCE_ANNI_2_STAMP,
        CharacterRankSource::OTHER_SOURCE_ANNI_3_STAMP,
        CharacterRankSource::OTHER_SOURCE_WORLD_LINK,
        CharacterRankSource::OTHER_SOURCE_ANNI_4_STAMP,
        CharacterRankSource::OTHER_SOURCE_ANNI_4_MEMORIAL_SELECT,
        CharacterRankSource::OTHER_SOURCE_MOVIE_STAMP,
        CharacterRankSource::OTHER_SOURCE_ANNI_4_5_STAMP,
        CharacterRankSource::OTHER_SOURCE_WORLD_LINK_2,
        CharacterRankSource::OTHER_SOURCE_ANNI_5_STAMP,
        CharacterRankSource::OTHER_SOURCE_ANNI_5_MEMORIAL_SELECT,
        CharacterRankSource::OTHER_SOURCE_BDAY_LIVE,
        CharacterRankSource::OTHER_SOURCE_NEW_YEAR_5_GACHA,
};

std::string SourceToClass(db::CharacterMissionType source) { return "char_mission"; }

std::string SourceToClass(CharacterRankSource::OtherSource source) { return "other_source"; }

template <typename T>
CTML::Node GenerateHeaderCell(T source) {
  return CTML::Node("th", SourceDescription(source)).SetAttribute("class", SourceToClass(source));
}

std::string FormatProgress(int current, int max) {
  if (current == std::numeric_limits<int>::max()) {
    return absl::StrFormat("\u221e / %d", max);
  }
  return absl::StrFormat("%d / %d", current, max);
}

CTML::Node CreateProgressNode(int current, int max, std::string_view cls) {
  return CTML::Node("div", FormatProgress(current, max))
      .SetAttribute("class", absl::StrCat(cls, current >= max ? " capped" : ""));
}

CTML::Node GenerateTableCell(const std::optional<CharacterRankSource> source) {
  if (!source.has_value()) {
    return CTML::Node("td").SetAttribute("class", "empty");
  }
  return CTML::Node("td")
      .SetAttribute("class", source->has_other_source()
                                 ? SourceToClass(source->other_source())
                                 : SourceToClass(source->character_mission_source()))
      .AppendChild(CreateProgressNode(source->current_xp(), source->max_xp(), "xp"))
      .AppendChild(CreateProgressNode(source->progress(), source->max_progress(), "progress"));
}

std::string FormatMaxRank(const MaxCharacterRank& max_cr) {
  return absl::StrFormat("%d.%d / %d.%d", max_cr.current_rank().rank(),
                         max_cr.current_rank().excess_xp(), max_cr.max_rank().rank(),
                         max_cr.max_rank().excess_xp());
}

void AppendCharNameAndRank(int char_id, const MaxCharacterRank& max_cr, CTML::Node& row,
                           bool short_name = false) {
  row.AppendChild(CTML::Node("th", short_name ? frontend::GetCharacterDisplayTextShort(char_id)
                                              : frontend::GetCharacterDisplayText(char_id))
                      .SetAttribute("class", "char-name"));
  row.AppendChild(
      CTML::Node("td")
          .SetAttribute("class", "rank")
          .AppendChild(CTML::Node("div", FormatMaxRank(max_cr)).SetAttribute("class", "xp"))
          .AppendChild(CreateProgressNode(max_cr.current_xp(), max_cr.max_xp(), "progress")));
}

CTML::Node GenerateRow(int char_id, const MaxCharacterRank& max_cr) {
  auto row = CTML ::Node("tr");
  AppendCharNameAndRank(char_id, max_cr, row);
  for (const auto& source : source_order) {
    row.AppendChild(std::visit(
        [&max_cr](auto&& source) {
          std::optional<CharacterRankSource> cr_source = GetCharacterRankSource(max_cr, source);
          return GenerateTableCell(cr_source);
        },
        source));
  }
  return row;
}

CTML::Node GenerateHeader() {
  auto row = CTML::Node("tr");
  row.AppendChild(CTML::Node("th", "Char"));
  row.AppendChild(CTML::Node("th", "Rank").SetAttribute("class", "rank"));
  for (const auto& source : source_order) {
    row.AppendChild(std::visit([](auto&& source) { return GenerateHeaderCell(source); }, source));
  }
  return row;
}

CTML::Node GenerateSummaryTableHeaderRow() {
  auto row = CTML::Node("tr");
  for ([[maybe_unused]] auto unit : EnumValuesExcludingDefault<db::Unit, db::Unit_descriptor>()) {
    row.AppendChild(CTML::Node("th", "Char"));
    row.AppendChild(CTML::Node("th", "Rank").SetAttribute("class", "rank"));
  }
  return row;
}

int GetMaxUnitOffset() {
  int max_unit_offset = 0;
  int cur_unit_offset = 0;
  db::Unit unit = db::UNIT_NONE;
  for (int char_id : UniqueCharacterIds()) {
    if (auto char_unit = LookupCharacterUnit(char_id); char_unit != unit) {
      unit = char_unit;
      max_unit_offset = std::max(max_unit_offset, cur_unit_offset);
      cur_unit_offset = 0;
    }
    ++cur_unit_offset;
  }
  max_unit_offset = std::max(max_unit_offset, cur_unit_offset);
  return max_unit_offset;
}

CTML::Node GenerateSummaryTableRow(std::span<const MaxCharacterRank> max_cr, int unit_offset) {
  auto row = CTML::Node("tr");
  db::Unit unit = db::UNIT_NONE;
  int cur_unit_offset = 0;
  bool unit_written = true;
  for (int char_id : UniqueCharacterIds()) {
    if (auto char_unit = LookupCharacterUnit(char_id); char_unit != unit) {
      if (!unit_written) {
        row.AppendChild(CTML::Node("td"))
            .AppendChild(CTML::Node("td").SetAttribute("class", "rank"));
      }
      unit = char_unit;
      cur_unit_offset = 0;
      unit_written = false;
    } else {
      ++cur_unit_offset;
    }
    if (unit_offset == cur_unit_offset) {
      AppendCharNameAndRank(char_id, max_cr[char_id], row, /*short_name=*/true);
      unit_written = true;
    }
  }
  return row;
}

CTML::Node GenerateSummaryTable(std::span<const MaxCharacterRank> max_cr) {
  auto table = CTML::Node("table");
  table.AppendChild(GenerateSummaryTableHeaderRow());
  int max_unit_offset = GetMaxUnitOffset();
  for (int offset = 0; offset < max_unit_offset; ++offset) {
    table.AppendChild(GenerateSummaryTableRow(max_cr, offset));
  }
  return table;
}

CTML::Node GenerateDetailsTable(std::span<const MaxCharacterRank> max_cr) {
  auto table = CTML::Node("table");
  table.AppendChild(GenerateHeader());
  for (int char_id : UniqueCharacterIds()) {
    table.AppendChild(GenerateRow(char_id, max_cr[char_id]));
  }
  return table;
}

}  // namespace

CTML::Node MaxCrTable(std::span<const MaxCharacterRank> max_cr) {
  ABSL_CHECK_GE(max_cr.size(), static_cast<std::size_t>(CharacterArraySize()));
  auto container = CTML::Node("div");
  container.AppendChild(CTML::Node("h2", "Max Character Ranks"))
      .AppendChild(GenerateSummaryTable(max_cr))
      .AppendChild(CTML::Node("h2", "Details"))
      .AppendChild(GenerateDetailsTable(max_cr));
  return container;
}

}  // namespace sekai::html
