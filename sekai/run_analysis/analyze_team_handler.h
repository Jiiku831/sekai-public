#pragma once

#include "absl/status/statusor.h"
#include "sekai/run_analysis/handler.h"
#include "sekai/run_analysis/proto/service.pb.h"

namespace sekai::run_analysis {

class AnalyzeTeamHandler : public Handler<AnalyzeTeamRequest, AnalyzeTeamResponse> {
 public:
  absl::StatusOr<AnalyzeTeamResponse> Run(const AnalyzeTeamRequest& request) const override;
};

}  // namespace sekai::run_analysis
