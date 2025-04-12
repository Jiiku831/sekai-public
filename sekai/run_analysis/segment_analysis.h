#pragma once

#include "absl/status/statusor.h"
#include "absl/time/time.h"
#include "sekai/run_analysis/clustering.h"
#include "sekai/run_analysis/proto/service.pb.h"
#include "sekai/run_analysis/snapshot.h"

namespace sekai::run_analysis {

struct GameCountAnalysis {
  ValueDist ep_per_game;
  int game_count = 0;
  ConfidenceInterval estimated_gph;
  Sequence rejected_samples;
};

struct SegmentAnalysisResult {
  bool is_confident;
  bool is_auto;
  std::optional<absl::Time> start;
  std::optional<absl::Time> end;
  absl::Duration segment_length;
  std::vector<Cluster> clusters;
  float cluster_mean_ratio;
  absl::StatusOr<GameCountAnalysis> game_count_analysis;
};

absl::StatusOr<SegmentAnalysisResult> AnalyzeSegment(const Sequence& sequence);
AnalyzeGraphResponse::Segment SegmentAnalysisResultToApiSegment(
    const absl::StatusOr<SegmentAnalysisResult>& res);
AnalyzeGraphResponse::Segment AnalyzeSegmentForApi(const Sequence& sequence);

}  // namespace sekai::run_analysis
