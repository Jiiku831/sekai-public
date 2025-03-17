#pragma once

#include "absl/status/statusor.h"
#include "absl/time/time.h"
#include "sekai/run_analysis/clustering.h"
#include "sekai/run_analysis/snapshot.h"

namespace sekai::run_analysis {

struct ValueDist {
  float mu = 0;
  float sigma = 0;
};

struct GameCountAnalysis {
  ValueDist ep_per_game;
  int game_count = 0;
  float estimated_gph = 0;
  Sequence rejected_samples;
};

struct SegmentAnalysisResult {
  absl::Duration segment_length;
  std::vector<Cluster> clusters;
  float cluster_mean_ratio;
  absl::StatusOr<GameCountAnalysis> game_count_analysis;
};

absl::StatusOr<SegmentAnalysisResult> AnalyzeSegment(const Sequence& sequence);

}  // namespace sekai::run_analysis
