#include "sekai/html/team.h"

#include <ctml.hpp>

#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "sekai/db/proto/all.h"
#include "sekai/html/card.h"
#include "sekai/proto/card.pb.h"

namespace sekai::html {

CTML::Node Team(const TeamProto& team) {
  CTML::Node row{"tr"};
  for (const CardProto& card : team.cards()) {
    row.AppendChild(CTML::Node("td").AppendChild(Card(card)));
  }
  std::string team_details =
      absl::StrFormat("Power        %6d\nEvent Bonus  %5.1f%%\nSkill Value  %5.1f%%", team.power(),
                      team.event_bonus(), team.skill_value());
  if (team.has_expected_ep()) {
    absl::StrAppend(&team_details, absl::StrFormat("\nExpected EP  %d", team.expected_ep()));
  }
  if (team.has_expected_score()) {
    absl::StrAppend(&team_details, absl::StrFormat("\nEst. Score  %7d\nBest Song   %s",
                                                   team.expected_score(), team.best_song_name()));
  }
  if (team.has_support_bonus()) {
    absl::StrAppend(&team_details,
                    absl::StrFormat("\n\n\nEvent Bonus Details\n-------------------\nMain         "
                                    "%5.1f%%\nSupport      %5.1f%%\nTotal        %5.1f%%",
                                    team.main_bonus(), team.support_bonus(), team.event_bonus()));
  }
  row.AppendChild(CTML::Node("td").AppendChild(CTML::Node("pre.team-details", team_details)));
  CTML::Node node{"table.team"};
  node.AppendChild(row);

  if (!team.support_cards().empty()) {
    CTML::Node support_row{"tr"};
    for (int i = 0; i < team.support_cards_size(); ++i) {
      support_row.AppendChild(CTML::Node("td").AppendChild(SupportCard(team.support_cards(i))));
    }

    CTML::Node support_table{"table.support-unit"};
    support_table.AppendChild(support_row);

    node.AppendChild(CTML::Node("tr").AppendChild(
        CTML::Node("td").SetAttribute("colspan", "6").AppendChild(support_table)));
  }

  return node;
}

}  // namespace sekai::html
