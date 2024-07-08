#include "frontend/display_text.h"

#include "absl/log/absl_check.h"
#include "sekai/db/proto/all.h"

namespace frontend {

using ::sekai::db::Attr;
using ::sekai::db::CardRarityType;

std::string GetRarityDisplayText(CardRarityType attr) {
  switch (attr) {
    case sekai::db::RARITY_1:
      return "1*";
    case sekai::db::RARITY_2:
      return "2*";
    case sekai::db::RARITY_3:
      return "3*";
    case sekai::db::RARITY_4:
      return "4*";
    case sekai::db::RARITY_BD:
      return "BD";
    default:
      ABSL_CHECK(false) << "unhandled case";
  }
}

std::string GetAttrDisplayText(Attr attr) {
  switch (attr) {
    case sekai::db::ATTR_COOL:
      return "Cool";
    case sekai::db::ATTR_MYST:
      return "Myst";
    case sekai::db::ATTR_CUTE:
      return "Cute";
    case sekai::db::ATTR_PURE:
      return "Pure";
    case sekai::db::ATTR_HAPPY:
      return "Happy";
    default:
      ABSL_CHECK(false) << "unhandled case";
  }
}

std::string GetCharacterDisplayText(int char_id) {
  static std::array names = {
      "",       "Ichika", "Saki",   "Honami", "Shiho",   "Minori", "Haruka", "Airi",  "Shizuku",
      "Kohane", "An",     "Akito",  "Toya",   "Tsukasa", "Emu",    "Nene",   "Rui",   "Kanade",
      "Mafuyu", "Ena",    "Mizuki", "Miku",   "Rin",     "Len",    "Luka",   "MEIKO", "KAITO",
  };
  ABSL_CHECK_NE(char_id, 0);
  ABSL_CHECK_LT(char_id, names.size());
  return names[char_id];
}

std::string GetAreaDisplayText(int area_id) {
  static std::array names = {"",         "",        "",          "",          "",
                             "LN Sekai", "",        "MMJ Sekai", "VBS Sekai", "WxS Sekai",
                             "25 Sekai", "Kamikou", "",          "Miyajyo"};
  ABSL_CHECK_NE(area_id, 0);
  ABSL_CHECK_LT(area_id, names.size());
  ABSL_CHECK(names[area_id][0] != '0');
  return names[area_id];
}

std::string GetUnitDisplayText(int unit_id) {
  static std::array names = {
      "", "LN", "MMJ", "VBS", "WxS", "25ji", "Miku",
  };
  ABSL_CHECK_NE(unit_id, 0);
  ABSL_CHECK_LT(unit_id, names.size());
  return names[unit_id];
}

}  // namespace frontend
