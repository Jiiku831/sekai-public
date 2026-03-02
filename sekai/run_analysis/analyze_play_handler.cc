#include "sekai/run_analysis/analyze_play_handler.h"

#include "absl/status/statusor.h"
#include "sekai/run_analysis/fill_analysis.h"
#include "sekai/run_analysis/handler.h"
#include "sekai/run_analysis/proto/service.pb.h"

namespace sekai::run_analysis {

absl::StatusOr<AnalyzePlayResponse> AnalyzePlayHandler::Run(
    const AnalyzePlayRequest& request) const {
  FillAnalyzer analyzer(request);
  return analyzer.RunAnalysis();
}

}  // namespace sekai::run_analysis
