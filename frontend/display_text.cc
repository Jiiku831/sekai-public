#include "frontend/display_text.h"

#include "absl/log/absl_check.h"
#include "sekai/db/proto/all.h"

namespace frontend {

using ::sekai::db::Attr;
using ::sekai::db::CardRarityType;

std::string GetRarityDisplayTextShort(CardRarityType attr) {
  switch (attr) {
    case sekai::db::RARITY_1:
      return "★1";
    case sekai::db::RARITY_2:
      return "★2";
    case sekai::db::RARITY_3:
      return "★3";
    case sekai::db::RARITY_4:
      return "★4";
    case sekai::db::RARITY_BD:
      return "BD";
    default:
      ABSL_CHECK(false) << "unhandled case";
  }
}

std::string GetRarityDisplayText(CardRarityType attr) {
  switch (attr) {
    case sekai::db::RARITY_1:
      return "★";
    case sekai::db::RARITY_2:
      return "★★";
    case sekai::db::RARITY_3:
      return "★★★";
    case sekai::db::RARITY_4:
      return "★★★★";
    case sekai::db::RARITY_BD:
      return "BD/AN";
    default:
      ABSL_CHECK(false) << "unhandled case";
  }
}

std::string GetAttrDisplayTextShort(Attr attr) {
  switch (attr) {
    case sekai::db::ATTR_COOL:
      return "クール Cool";
    case sekai::db::ATTR_MYST:
      return "ミステリアス Myst";
    case sekai::db::ATTR_CUTE:
      return "キュート Cute";
    case sekai::db::ATTR_PURE:
      return "ピュア Pure";
    case sekai::db::ATTR_HAPPY:
      return "ハッピー Happy";
    default:
      ABSL_CHECK(false) << "unhandled case";
  }
}

std::string GetAttrDisplayText(Attr attr) {
  switch (attr) {
    case sekai::db::ATTR_COOL:
      return "クール　　　 Cool";
    case sekai::db::ATTR_MYST:
      return "ミステリアス Myst";
    case sekai::db::ATTR_CUTE:
      return "キュート　　 Cute";
    case sekai::db::ATTR_PURE:
      return "ピュア　　　 Pure";
    case sekai::db::ATTR_HAPPY:
      return "ハッピー　　 Happy";
    default:
      ABSL_CHECK(false) << "unhandled case";
  }
}

std::string GetCharacterDisplayTextShort(int char_id) {
  static std::array names = {
      "",       "Ichika", "Saki",   "Honami", "Shiho",   "Minori", "Haruka", "Airi",  "Shizuku",
      "Kohane", "An",     "Akito",  "Toya",   "Tsukasa", "Emu",    "Nene",   "Rui",   "Kanade",
      "Mafuyu", "Ena",    "Mizuki", "Miku",   "Rin",     "Len",    "Luka",   "MEIKO", "KAITO",
  };
  ABSL_CHECK_NE(char_id, 0);
  ABSL_CHECK_LT(static_cast<std::size_t>(char_id), names.size());
  return names[char_id];
}

std::string GetCharacterDisplayText(int char_id) {
  static std::array names = {
      "",
      "一歌　 Ichika",
      "咲希　 Saki",
      "穂波　 Honami",
      "志歩　 Shiho",
      "みのり Minori",
      "遥　　 Haruka",
      "愛莉　 Airi",
      "雫　　 Shizuku",
      "こはね Kohane",
      "杏　　 An",
      "彰人　 Akito",
      "冬弥　 Toya",
      "司　　 Tsukasa",
      "えむ　 Emu",
      "寧々　 Nene",
      "類　　 Rui",
      "奏　　 Kanade",
      "まふゆ Mafuyu",
      "絵名　 Ena",
      "瑞希　 Mizuki",
      "ミク　 Miku",
      "リン　 Rin",
      "レン　 Len",
      "ルカ　 Luka",
      "MEIKO",
      "KAITO",
  };
  ABSL_CHECK_NE(char_id, 0);
  ABSL_CHECK_LT(static_cast<std::size_t>(char_id), names.size());
  return names[char_id];
}

std::string GetAreaDisplayTextShort(int area_id) {
  static std::array names = {"",         "",        "",          "",          "",
                             "LN Sekai", "",        "MMJ Sekai", "VBS Sekai", "WxS Sekai",
                             "25 Sekai", "Kamikou", "",          "Miyajyo"};
  ABSL_CHECK_NE(area_id, 0);
  ABSL_CHECK_LT(static_cast<std::size_t>(area_id), names.size());
  ABSL_CHECK(names[area_id][0] != '0');
  return names[area_id];
}

std::string GetAreaDisplayText(int area_id) {
  static std::array names = {"",
                             "",
                             "",
                             "",
                             "",
                             "教室のセカイ　　　　　 LN Sekai",
                             "",
                             "ステージのセカイ　　　 MMJ Sekai",
                             "ストリートのセカイ　　 VBS Sekai",
                             "ワンダーランドのセカイ WxS Sekai",
                             "誰もいないセカイ　　　 25 Sekai",
                             "神山高校　　　　　　　 Kamikou",
                             "",
                             "宮益坂女子学園　　　　 Miyajyo"};
  ABSL_CHECK_NE(area_id, 0);
  ABSL_CHECK_LT(static_cast<std::size_t>(area_id), names.size());
  ABSL_CHECK(names[area_id][0] != '0');
  return names[area_id];
}

std::string GetUnitDisplayTextShort(int unit_id) {
  static std::array names = {
      "", "LN", "MMJ", "VBS", "WxS", "25ji", "Miku",
  };
  ABSL_CHECK_NE(unit_id, 0);
  ABSL_CHECK_LT(static_cast<std::size_t>(unit_id), names.size());
  return names[unit_id];
}

std::string GetUnitDisplayText(int unit_id) {
  static std::array names = {
      "",
      "レオニ　　 LN",
      "モモジャン MMJ",
      "ビビバス　 VBS",
      "ワンダショ WxS",
      "ニーゴ　　 25ji",
      "ミク　　　 Miku",
  };
  ABSL_CHECK_NE(unit_id, 0);
  ABSL_CHECK_LT(static_cast<std::size_t>(unit_id), names.size());
  return names[unit_id];
}

}  // namespace frontend
