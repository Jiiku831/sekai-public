#include "sekai/run_analysis/segment_analysis.h"

#include <cmath>
#include <cstdlib>
#include <limits>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_format.h"
#include "absl/time/time.h"
#include "base/util.h"
#include "sekai/run_analysis/clustering.h"
#include "sekai/run_analysis/snapshot.h"
#include "sekai/run_analysis/stats_util.h"

namespace sekai::run_analysis {

namespace {

absl::StatusOr<int> InferMinClusterGameCount(float cluster_mean_ratio) {
  constexpr float kMeanRatioTolerance = 0.2;
  // If mean_ratio is ~1.5, then the min_cluster_mean is the mean of 2 games.
  // If mean_ratio is ~2.0, then the min_cluster_mean is the mean of 1 game.
  int game_count = 0;
  if (std::abs(cluster_mean_ratio - 1.5) < kMeanRatioTolerance) {
    game_count = 2;
  } else if (std::abs(cluster_mean_ratio - 2.0) < kMeanRatioTolerance) {
    game_count = 1;
  }
  if (game_count == 0) {
    return absl::FailedPreconditionError(
        absl::StrFormat("Cluster mean ratio (%.2f) exceeds tolerance.", cluster_mean_ratio));
  }
  return game_count;
}

template <typename Cont>
ValueDist ToWeightedValueDist(const Cont& cont) {
  float mu = weighted_mean<float>(cont);
  float sigma = weighted_stdev<float>(cont, mu);
  return ValueDist{mu, sigma};
}

absl::StatusOr<GameCountAnalysis> RunGameCountAnalysis(float cluster_mean_ratio,
                                                       float min_cluster_mean,
                                                       const Sequence& seq) {
  ASSIGN_OR_RETURN(int min_cluster_game_count, InferMinClusterGameCount(cluster_mean_ratio));
  float initial_avg_ep_est = min_cluster_mean / min_cluster_game_count;

  constexpr float kEpTol = 0.2;
  // (Avg EP, Game Count)
  std::vector<std::pair<float, int>> ep_breakdown;
  Sequence rejected_samples = seq.CopyEmpty();
  absl::Duration total_duration;
  int total_games = 0;
  for (const Snapshot& pt : seq | std::views::drop(1)) {
    const float diff = pt.diff;
    float game_count_f = diff / initial_avg_ep_est;
    int game_count = std::round(game_count_f);
    if (game_count < 1 || game_count > 3 || std::abs(game_count_f - game_count) > kEpTol) {
      rejected_samples.push_back(pt);
      continue;
    }

    float avg_ep = diff / game_count;
    ep_breakdown.emplace_back(avg_ep, game_count);
    total_games += game_count;
    total_duration += kInterval;
  }

  return GameCountAnalysis{
      .ep_per_game = ToWeightedValueDist(ep_breakdown),
      .game_count = total_games,
      .estimated_gph = static_cast<float>(total_games) /
                       (static_cast<float>(total_duration / absl::Seconds(1)) / 3600),
      .rejected_samples = rejected_samples,
  };
}

}  // namespace

absl::StatusOr<SegmentAnalysisResult> AnalyzeSegment(const Sequence& sequence) {
  SegmentAnalysisResult result;
  result.segment_length =
      sequence.empty() ? absl::ZeroDuration() : (sequence.back().time - sequence.front().time);
  result.clusters = FindClusters(sequence | std::views::drop(1));
  result.cluster_mean_ratio = std::numeric_limits<float>::quiet_NaN();
  if (result.clusters.size() < 1) {
    return absl::InternalError("Expected at least 1 clusters.");
  }
  if (result.clusters.size() < 2) {
    return result;
  }
  // Look at the two largest clusters
  if (result.clusters.size() > 2) {
    std::stable_sort(
        result.clusters.begin(), result.clusters.end(),
        [](const Cluster& lhs, const Cluster& rhs) { return lhs.vals.size() > rhs.vals.size(); });
  }
  float mean0 = result.clusters[0].mean;
  float mean1 = result.clusters[1].mean;
  float min_mean = std::min(mean0, mean1);
  result.cluster_mean_ratio = std::max(mean0, mean1) / min_mean;
  result.game_count_analysis = RunGameCountAnalysis(result.cluster_mean_ratio, min_mean, sequence);
  return result;
}

}  // namespace sekai::run_analysis
