#include "sekai/run_analysis/analyze_team_handler.h"

#include <memory>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_format.h"
#include "base/util.h"
#include "sekai/card.h"
#include "sekai/config.h"
#include "sekai/db/master_db.h"
#include "sekai/event_bonus.h"
#include "sekai/proto/card_state.pb.h"
#include "sekai/run_analysis/handler.h"
#include "sekai/run_analysis/proto/service.pb.h"
#include "sekai/team.h"

namespace sekai::run_analysis {

using ::sekai::db::MasterDb;

absl::StatusOr<AnalyzeTeamResponse> AnalyzeTeamHandler::Run(
    const AnalyzeTeamRequest& request) const {
  if (request.cards().size() != kTeamSize) {
    return absl::InvalidArgumentError(
        absl::StrCat("Expected 5 cards, got ", request.cards().size()));
  }

  ASSIGN_OR_RETURN(const auto* event, MasterDb::FindFirstOrError<db::Event>(request.event_id()));
  EventBonus event_bonus(event);

  std::vector<std::unique_ptr<Card>> cards;
  std::vector<const Card*> card_ptrs;
  for (const DeckCard& card : request.cards()) {
    CardState card_state;
    // Level does not matter for the purpose of this analysis.
    card_state.set_level(1);
    card_state.set_master_rank(card.master_rank());
    if (card_state.master_rank() < 0 || card_state.master_rank() > kMasterRankMax) {
      return absl::InvalidArgumentError(
          absl::StrFormat("Expected master rank to be within [0, 5], got %d for card %d",
                          card_state.master_rank(), card.card_id()));
    }
    ASSIGN_OR_RETURN(const auto* db_card, MasterDb::FindFirstOrError<db::Card>(card.card_id()));
    cards.push_back(std::make_unique<Card>(*db_card, card_state));
    card_ptrs.push_back(cards.back().get());
    cards.back()->ApplyEventBonus(event_bonus);
  }
  Team team(card_ptrs);

  AnalyzeTeamResponse response;
  response.set_event_bonus(team.EventBonus(event_bonus));
  return response;
}

}  // namespace sekai::run_analysis
