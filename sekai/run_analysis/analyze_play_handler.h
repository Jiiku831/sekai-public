#pragma once

#include "absl/status/statusor.h"
#include "sekai/run_analysis/handler.h"
#include "sekai/run_analysis/proto/service.pb.h"

namespace sekai::run_analysis {

class AnalyzePlayHandler : public Handler<AnalyzePlayRequest, AnalyzePlayResponse> {
 public:
  absl::StatusOr<AnalyzePlayResponse> Run(const AnalyzePlayRequest& request) const override;
};

}  // namespace sekai::run_analysis
