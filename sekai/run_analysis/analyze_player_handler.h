#pragma once

#include "absl/status/statusor.h"
#include "sekai/run_analysis/analyze_graph_handler.h"
#include "sekai/run_analysis/analyze_play_handler.h"
#include "sekai/run_analysis/analyze_team_handler.h"
#include "sekai/run_analysis/compute_stats_handler.h"
#include "sekai/run_analysis/handler.h"
#include "sekai/run_analysis/proto/service.pb.h"

namespace sekai::run_analysis {

class AnalyzePlayerHandler : public Handler<AnalyzePlayerRequest, AnalyzePlayerResponse> {
 public:
  absl::StatusOr<AnalyzePlayerResponse> Run(const AnalyzePlayerRequest& request) const override;

 private:
  AnalyzeGraphHandler graph_handler_;
  AnalyzePlayHandler play_handler_;
  AnalyzeTeamHandler team_handler_;
  ComputeStatsHandler stats_handler_;

  absl::StatusOr<AnalyzePlayerResponse::PlayerResponse> RunForPlayer(
      const AnalyzePlayerRequest::PlayerRequest& request) const;
};

}  // namespace sekai::run_analysis
