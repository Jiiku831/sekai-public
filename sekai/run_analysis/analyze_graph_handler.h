#pragma once

#include "absl/status/statusor.h"
#include "sekai/run_analysis/handler.h"
#include "sekai/run_analysis/proto/service.pb.h"

namespace sekai::run_analysis {

class AnalyzeGraphHandler : public Handler<AnalyzeGraphRequest, AnalyzeGraphResponse> {
 public:
  absl::StatusOr<AnalyzeGraphResponse> Run(const AnalyzeGraphRequest& request) const override;
};

}  // namespace sekai::run_analysis
