#pragma once

#include "absl/status/statusor.h"
#include "sekai/run_analysis/clustering.h"
#include "sekai/run_analysis/snapshot.h"

namespace sekai::run_analysis {

struct SegmentAnalysisResult {
  std::vector<Cluster> clusters;
  float cluster_mean_ratio;
};
absl::StatusOr<SegmentAnalysisResult> AnalyzeSegment(const Sequence& sequence);

}  // namespace sekai::run_analysis
