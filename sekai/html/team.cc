#include "sekai/html/team.h"

#include <ctml.hpp>

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
  row.AppendChild(CTML::Node("td").AppendChild(CTML::Node(
      "pre.team-details",
      absl::StrFormat(
          "Power        %6d\nEvent Bonus  %5.1f%%\nSkill Value  %5.1f%%\nExpected EP  %6d",
          team.power(), team.event_bonus(), team.skill_value(), team.expected_ep()))));
  CTML::Node node{"table.team"};
  node.AppendChild(row);
  return node;
}

}  // namespace sekai::html
