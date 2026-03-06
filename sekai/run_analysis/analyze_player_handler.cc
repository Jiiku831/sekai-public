#include "sekai/run_analysis/analyze_player_handler.h"

#include "absl/algorithm/container.h"
#include "absl/status/statusor.h"
#include "base/util.h"
#include "sekai/config.h"
#include "sekai/run_analysis/proto/service.pb.h"

namespace sekai::run_analysis {

absl::StatusOr<AnalyzePlayerResponse> AnalyzePlayerHandler::Run(
    const AnalyzePlayerRequest& request) const {
  AnalyzePlayerResponse response;
  for (const AnalyzePlayerRequest::PlayerRequest& player : request.players()) {
    ASSIGN_OR_RETURN(*response.add_players(), RunForPlayer(player));
  }
  if (request.compute_stats()) {
    ASSIGN_OR_RETURN(*response.mutable_stats(), stats_handler_.RunOnResponse(response));
  }
  return response;
}

absl::StatusOr<AnalyzePlayerResponse::PlayerResponse> AnalyzePlayerHandler::RunForPlayer(
    const AnalyzePlayerRequest::PlayerRequest& request) const {
  AnalyzePlayerResponse::PlayerResponse response;
  ASSIGN_OR_RETURN(*response.mutable_team(), team_handler_.Run(request.team()));
  ASSIGN_OR_RETURN(*response.mutable_graph(), graph_handler_.Run(request.graph()));

  AnalyzePlayRequest base_play_request;
  base_play_request.set_power(request.team().team_power());
  base_play_request.set_event_bonus(response.team().event_bonus());
  base_play_request.set_skill_max(response.team().skill_details().skill_value_upper_bound());
  base_play_request.set_skill_min(response.team().skill_details().skill_value_lower_bound());
  base_play_request.set_card_skill_min(
      response.team().skill_details().skill_lower_bounds().empty()
          ? kMinCardSkillValue
          : *absl::c_min_element(response.team().skill_details().skill_lower_bounds()));
  base_play_request.set_card_skill_max(
      response.team().skill_details().skill_upper_bounds().empty()
          ? kMinCardSkillValue
          : *absl::c_min_element(response.team().skill_details().skill_upper_bounds()));
  for (AnalyzeGraphResponse::Segment& segment :
       *response.mutable_graph()->mutable_active_segments()) {
    if (!segment.is_confident()) continue;
    AnalyzePlayRequest play_request = base_play_request;
    play_request.set_observed_games_per_hour(segment.games_per_hour().value());
    play_request.set_observed_ep_per_game(segment.ep_per_game().value());
    ASSIGN_OR_RETURN(*segment.mutable_play_analysis(), play_handler_.Run(play_request));
  }
  return response;
}

}  // namespace sekai::run_analysis
