#include "sekai/run_analysis/segment_analysis.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>

#include <google/protobuf/util/time_util.h>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_format.h"
#include "absl/time/time.h"
#include "base/util.h"
#include "sekai/run_analysis/clustering.h"
#include "sekai/run_analysis/proto/service.pb.h"
#include "sekai/run_analysis/proto_util.h"
#include "sekai/run_analysis/snapshot.h"
#include "sekai/run_analysis/stats_util.h"

namespace sekai::run_analysis {
namespace {

using ::google::protobuf::util::TimeUtil;

constexpr float kAutoStdevThreshold = 100;

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
  ValueDist dist;
  dist.set_mean(weighted_mean<float>(cont));
  dist.set_stdev(weighted_stdev<float>(cont, dist.mean()));
  return dist;
}

ConfidenceInterval ValueDistTo95pCI(const ValueDist& dist) {
  ConfidenceInterval res;
  res.set_value(dist.mean());
  res.set_lower_bound(dist.mean() - dist.stdev() * 2);
  res.set_upper_bound(dist.mean() + dist.stdev() * 2);
  res.set_confidence(0.95);
  return res;
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

  // Assuming perfectly consistent play, then the observed gph will be at
  // most 1 away from true gph. However, there will be variation over time. So let's just say
  // this is the 95% CI.
  ConfidenceInterval gph;
  float duration_hrs = (static_cast<float>(total_duration / absl::Seconds(1)) / 3600);
  gph.set_value(static_cast<float>(total_games) / duration_hrs);
  gph.set_lower_bound(static_cast<float>(total_games - 1) / duration_hrs);
  gph.set_upper_bound(static_cast<float>(total_games + 1) / duration_hrs);
  gph.set_confidence(0.95);

  return GameCountAnalysis{
      .ep_per_game = ToWeightedValueDist(ep_breakdown),
      .game_count = total_games,
      .estimated_gph = std::move(gph),
      .rejected_samples = rejected_samples,
  };
}

}  // namespace

absl::StatusOr<SegmentAnalysisResult> AnalyzeSegment(const Sequence& sequence) {
  SegmentAnalysisResult result;
  result.segment_length =
      sequence.empty() ? absl::ZeroDuration() : (sequence.back().time - sequence.front().time);
  constexpr absl::Duration kConfidentDuration = absl::Minutes(20);
  if (!sequence.empty()) {
    result.start = sequence.time_offset + sequence.front().time;
    result.end = sequence.time_offset + sequence.back().time;
  }
  result.clusters = FindClusters(sequence | std::views::drop(1));
  result.cluster_mean_ratio = std::numeric_limits<float>::quiet_NaN();
  float max_stdev = std::numeric_limits<float>::quiet_NaN();
  if (result.clusters.size() < 1) {
    return absl::InternalError("Expected at least 1 clusters.");
  }
  max_stdev = std::ranges::max(result.clusters | std::views::transform([](const Cluster& cluster) {
                                 return cluster.stdev;
                               }));
  result.is_auto = max_stdev < kAutoStdevThreshold;
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
  result.is_confident =
      result.game_count_analysis.ok() && result.segment_length >= kConfidentDuration;
  return result;
}

AnalyzeGraphResponse::Segment SegmentAnalysisResultToApiSegment(
    const absl::StatusOr<SegmentAnalysisResult>& res) {
  AnalyzeGraphResponse::Segment segment;
  if (!res.ok()) {
    ToStatusProto(res.status(), *segment.mutable_status());
    segment.set_is_confident(false);
    return segment;
  }

  segment.set_is_confident(res->is_confident);
  segment.set_is_auto(res->is_auto);
  if (res->start.has_value()) {
    *segment.mutable_start() = TimeUtil::NanosecondsToTimestamp(absl::ToUnixNanos(*res->start));
  }
  if (res->end.has_value()) {
    *segment.mutable_end() = TimeUtil::NanosecondsToTimestamp(absl::ToUnixNanos(*res->end));
  }
  if (!res->game_count_analysis.ok()) {
    ToStatusProto(res->game_count_analysis.status(), *segment.mutable_status());
    segment.set_is_confident(false);
    return segment;
  }
  *segment.mutable_games_per_hour() = res->game_count_analysis->estimated_gph;
  *segment.mutable_ep_per_game() = ValueDistTo95pCI(res->game_count_analysis->ep_per_game);
  segment.mutable_status()->set_code(0);
  return segment;
}

AnalyzeGraphResponse::Segment AnalyzeSegmentForApi(const Sequence& seq) {
  return SegmentAnalysisResultToApiSegment(AnalyzeSegment(seq));
}

}  // namespace sekai::run_analysis
