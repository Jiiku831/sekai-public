#include "sekai/run_analysis/analyze_team_handler.h"

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "sekai/run_analysis/handler.h"
#include "sekai/run_analysis/proto/service.pb.h"

namespace sekai::run_analysis {

absl::StatusOr<AnalyzeTeamResponse> AnalyzeTeamHandler::Run(
    const AnalyzeTeamRequest& request) const {
  return absl::UnimplementedError("Not yet implemented.");
}

}  // namespace sekai::run_analysis
