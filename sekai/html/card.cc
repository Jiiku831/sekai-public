#include "sekai/html/card.h"

#include <string_view>

#include <ctml.hpp>

#include "absl/log/absl_check.h"
#include "absl/strings/ascii.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "sekai/db/proto/all.h"
#include "sekai/proto/card.pb.h"

namespace sekai::html {
namespace {

constexpr std::string_view kUntrainedStarUrl =
    "https://sekai.best/assets/rarity_star_normal-BYSplh9m.png";
constexpr std::string_view kTrainedStarUrl =
    "https://sekai.best/assets/rarity_star_afterTraining-CUlLhfpl.png";
constexpr std::string_view kBirthdayIconUrl =
    "https://sekai.best/assets/rarity_birthday-Ct3DrYez.png";

std::string GetAttrUrl(db::Attr attr) {
  switch (attr) {
    case db::ATTR_CUTE:
      return "https://sekai.best/assets/icon_attribute_cute-BqKuT21a.png";
    case db::ATTR_COOL:
      return "https://sekai.best/assets/icon_attribute_cool-Cm_EFAKA.png";
    case db::ATTR_PURE:
      return "https://sekai.best/assets/icon_attribute_pure-DMCNUXNX.png";
    case db::ATTR_HAPPY:
      return "https://sekai.best/assets/icon_attribute_happy-POeZUq3N.png";
    case db::ATTR_MYST:
      return "https://sekai.best/assets/icon_attribute_mysterious-DRt6JUuH.png";
    default:
      ABSL_CHECK(false) << "unhandled case";
      return "";
  }
}

std::string GetFrameUrl(db::CardRarityType rarity) {
  switch (rarity) {
    // clang-format off
    case db::RARITY_1: return "data:image/png;base64,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";
    case db::RARITY_2: return "data:image/png;base64,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";
    case db::RARITY_3: return "data:image/png;base64,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";
    case db::RARITY_BD: return "https://sekai.best/assets/cardFrame_S_bd-CrsQsaNc.png";
    case db::RARITY_4: return "https://sekai.best/assets/cardFrame_S_4-DkwuVAqt.png";
    // clang-format on
    default:
      ABSL_CHECK(false) << "unhandled case";
      return "";
  }
}

int RarityIconCount(db::CardRarityType rarity) {
  switch (rarity) {
    case db::RARITY_1:
      return 1;
    case db::RARITY_2:
      return 2;
    case db::RARITY_3:
      return 3;
    case db::RARITY_BD:
      return 1;
    case db::RARITY_4:
      return 4;
    default:
      ABSL_CHECK(false) << "unhandled case";
      return 0;
  }
}

std::string RarityIconUrl(db::CardRarityType rarity, bool trained) {
  if (rarity == db::RARITY_BD) {
    return std::string(kBirthdayIconUrl);
  }
  return std::string(trained ? kTrainedStarUrl : kUntrainedStarUrl);
}

constexpr std::string_view kThumbnailPrefix =
    "https://storage.sekai.best/sekai-jp-assets/thumbnail/chara_rip/";
constexpr std::string_view kThumbnailUntrainedSuffix = "_normal.webp";
constexpr std::string_view kThumbnailTrainedSuffix = "_after_training.webp";

std::string GetThumbnailUrl(const db::Card& card, bool trained) {
  return absl::StrCat(kThumbnailPrefix, card.assetbundle_name(),
                      trained ? kThumbnailTrainedSuffix : kThumbnailUntrainedSuffix);
}

}  // namespace

CTML::Node Card(const db::Card& card, bool trained) {
  // TODO: master rank
  CTML::Node svg{"svg.card-svg"};
  svg.SetAttribute("xmlns", "http://www.w3.org/2000/svg").SetAttribute("viewBox", "0 0 156 156");
  svg.AppendChild(CTML::Node("image")
                      .SetAttribute("href", GetThumbnailUrl(card, trained))
                      .SetAttribute("x", "8")
                      .SetAttribute("y", "8")
                      .SetAttribute("height", "140")
                      .SetAttribute("width", "140"));
  svg.AppendChild(CTML::Node("image")
                      .SetAttribute("href", GetFrameUrl(card.card_rarity_type()))
                      .SetAttribute("x", "0")
                      .SetAttribute("y", "0")
                      .SetAttribute("height", "156")
                      .SetAttribute("width", "156"));
  svg.AppendChild(CTML::Node("image")
                      .SetAttribute("href", GetAttrUrl(card.attr()))
                      .SetAttribute("x", "1")
                      .SetAttribute("y", "1")
                      .SetAttribute("height", "35")
                      .SetAttribute("width", "35"));
  int rarity_icon_count = RarityIconCount(card.card_rarity_type());
  std::string rarity_icon = RarityIconUrl(card.card_rarity_type(), trained);
  for (int i = 0; i < rarity_icon_count; ++i) {
    svg.AppendChild(CTML::Node("image")
                        .SetAttribute("href", rarity_icon)
                        .SetAttribute("x", std::to_string(10 + 26 * i))
                        .SetAttribute("y", "118")
                        .SetAttribute("height", "28")
                        .SetAttribute("width", "28"));
  }
  return svg;
}

CTML::Node Card(const CardProto& card) {
  bool use_trained_thumbnail = card.state().special_training();
  if (card.skill_state() == CardProto::USE_PRIMARY_SKILL) {
    use_trained_thumbnail = false;
  }
  CTML::Node table{"table.card"};
  table.AppendChild(CTML::Node("tr").AppendChild(
      CTML::Node("td.card-svg-td").AppendChild(Card(card.db_card(), use_trained_thumbnail))));
  std::string w_skill_string = "n/a";
  switch (card.skill_state()) {
    case CardProto::PRIMARY_SKILL_ONLY:
      w_skill_string = "n/a";
      break;
    case CardProto::USE_PRIMARY_SKILL:
      w_skill_string = "before";
      break;
    case CardProto::USE_SECONDARY_SKILL:
      w_skill_string = "after";
      break;
    default:
      w_skill_string = "??";
      break;
  }
  std::string detail_string =
      absl::StrFormat(R"(
Lv %-2d/MR %d/SL %d
---------------
Power    %6d
Bonus    %5.1f%%
Skill    %5.0f%%
W Skill  %6s)",
                      // Trained  %6s
                      // Stories  %6d)",
                      card.state().level(), card.state().master_rank(), card.state().skill_level(),
                      card.team_power_contrib(), card.team_bonus_contrib(),
                      card.team_skill_contrib(), w_skill_string);
  // card.state().special_training() ? "yes" : "no",
  // card.state().card_episodes_read_size());
  absl::StripAsciiWhitespace(&detail_string);
  table.AppendChild(
      CTML::Node("tr").AppendChild(CTML::Node("td").AppendChild(CTML::Node("pre", detail_string))));
  return table;
}

CTML::Node SupportCard(const CardProto& card) {
  bool use_trained_thumbnail = card.state().special_training();
  CTML::Node table{"table.card.support-card"};
  table.AppendChild(CTML::Node("tr").AppendChild(
      CTML::Node("td.card-svg-td").AppendChild(Card(card.db_card(), use_trained_thumbnail))));
  std::string detail_string =
      absl::StrFormat("MR%d/SL%d\n%.1f%%", card.state().master_rank(), card.state().skill_level(),
                      card.team_bonus_contrib());
  table.AppendChild(
      CTML::Node("tr").AppendChild(CTML::Node("td").AppendChild(CTML::Node("pre", detail_string))));
  return table;
}

}  // namespace sekai::html
