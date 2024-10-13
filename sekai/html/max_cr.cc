#include "sekai/html/max_cr.h"

#include <array>
#include <limits>
#include <variant>

#include <ctml.hpp>

#include "absl/log/absl_check.h"
#include "absl/log/log.h"
#include "absl/strings/str_format.h"
#include "frontend/display_text.h"
#include "sekai/array_size.h"
#include "sekai/max_character_rank.h"
#include "sekai/proto/max_character_rank.pb.h"

namespace sekai::html {
namespace {

constexpr std::array<std::variant<db::CharacterMissionType, CharacterRankSource::OtherSource>, 25>
    source_order = {
        db::CHARACTER_MISSION_TYPE_COLLECT_MEMBER,
        db::CHARACTER_MISSION_TYPE_COLLECT_STAMP,
        db::CHARACTER_MISSION_TYPE_COLLECT_COSTUME_3D,
        db::CHARACTER_MISSION_TYPE_COLLECT_CHARACTER_ARCHIVE_VOICE,
        db::CHARACTER_MISSION_TYPE_COLLECT_ANOTHER_VOCAL,
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
        CharacterRankSource::OTHER_SOURCE_CHALLENGE_LIVE,
        CharacterRankSource::OTHER_SOURCE_ANNI_2_STAMP,
        CharacterRankSource::OTHER_SOURCE_ANNI_3_STAMP,
        CharacterRankSource::OTHER_SOURCE_WORLD_LINK,
        CharacterRankSource::OTHER_SOURCE_ANNI_4_STAMP,
        CharacterRankSource::OTHER_SOURCE_ANNI_4_MEMORIAL_SELECT,
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

CTML::Node GenerateTableCell(const std::optional<CharacterRankSource> source) {
  if (!source.has_value()) {
    return CTML::Node("td").SetAttribute("class", "empty");
  }
  return CTML::Node("td")
      .SetAttribute("class", source->has_other_source()
                                 ? SourceToClass(source->other_source())
                                 : SourceToClass(source->character_mission_source()))
      .AppendChild(CTML::Node("div", FormatProgress(source->current_xp(), source->max_xp()))
                       .SetAttribute("class", "xp"))
      .AppendChild(CTML::Node("div", FormatProgress(source->progress(), source->max_progress()))
                       .SetAttribute("class", "progress"));
}

std::string FormatMaxRank(const MaxCharacterRank& max_cr) {
  return absl::StrFormat("%d.%d / %d.%d", max_cr.current_rank().rank(),
                         max_cr.current_rank().excess_xp(), max_cr.max_rank().rank(),
                         max_cr.max_rank().excess_xp());
}

CTML::Node GenerateRow(int char_id, const MaxCharacterRank& max_cr) {
  auto row = CTML ::Node("tr");
  row.AppendChild(CTML::Node("th", frontend::GetCharacterDisplayText(char_id)));
  row.AppendChild(
      CTML::Node("td")
          .SetAttribute("class", "rank")
          .AppendChild(CTML::Node("div", FormatMaxRank(max_cr)).SetAttribute("class", "xp"))
          .AppendChild(CTML::Node("div", FormatProgress(max_cr.current_xp(), max_cr.max_xp()))
                           .SetAttribute("class", "progress")));
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

}  // namespace

CTML::Node MaxCrTable(std::span<const MaxCharacterRank> max_cr) {
  ABSL_CHECK_GE(max_cr.size(), static_cast<std::size_t>(CharacterArraySize()));
  auto table = CTML::Node("table");
  table.AppendChild(GenerateHeader());
  for (int char_id : UniqueCharacterIds()) {
    table.AppendChild(GenerateRow(char_id, max_cr[char_id]));
  }
  return table;
}

}  // namespace sekai::html
