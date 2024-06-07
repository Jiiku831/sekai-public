#include "sekai/html/team.h"

#include <ctml.hpp>

#include "absl/strings/str_format.h"
#include "sekai/db/proto/all.h"
#include "sekai/html/card.h"
#include "sekai/proto/card.pb.h"

namespace sekai::html {

CTML::Node Team(const TeamProto& team) {
  CTML::Node node{"div.team"};
  for (const CardProto& card : team.cards()) {
    node.AppendChild(Card(card));
  }
  node.AppendChild(CTML::Node(
      "pre.team-details",
      absl::StrFormat("power:       %d\nevent bonus: %.2f%%\nskill_value: +%d%%\nexpected ep: %d",
                      team.power(), team.event_bonus(), team.skill_value(), team.expected_ep())));
  return node;
}

}  // namespace sekai::html
