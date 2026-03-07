#pragma once

#include "absl/status/statusor.h"
#include "sekai/run_analysis/handler.h"
#include "sekai/run_analysis/proto/service.pb.h"

namespace sekai::run_analysis {

class ComputeStatsHandler : public Handler<ComputeStatsRequest, ComputeStatsResponse> {
 public:
  absl::StatusOr<ComputeStatsResponse> Run(const ComputeStatsRequest& request) const override;
  absl::StatusOr<ComputeStatsResponse> RunOnResponse(
      const AnalyzePlayerResponse& player_response) const;
};

}  // namespace sekai::run_analysis
