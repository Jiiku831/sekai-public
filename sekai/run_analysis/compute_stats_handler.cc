#include "sekai/run_analysis/compute_stats_handler.h"

#include "absl/algorithm/container.h"
#include "absl/base/nullability.h"
#include "base/util.h"
#include "sekai/run_analysis/proto/service.pb.h"
#include "sekai/run_analysis/stats_util.h"

namespace sekai::run_analysis {
namespace {

absl::StatusOr<ComputeStatsResponse::PopulationStats> ComputeStats(
    const AnalyzePlayerResponse& response, int64_t max_duration) {
  const int64_t dur_threshold = max_duration * 0.95;
  std::vector<std::pair<float, float>> power, bonus, uptime, downtime, autotime, gph, epg, rate,
      auto_gph, auto_epg, auto_rate, fill;
  power.reserve(response.players_size());
  bonus.reserve(response.players_size());
  uptime.reserve(response.players_size());
  downtime.reserve(response.players_size());
  autotime.reserve(response.players_size());
  constexpr int kAverageSegments = 20;
  gph.reserve(response.players_size() * kAverageSegments);
  epg.reserve(response.players_size() * kAverageSegments);
  rate.reserve(response.players_size() * kAverageSegments);
  auto_gph.reserve(response.players_size() * kAverageSegments);
  auto_epg.reserve(response.players_size() * kAverageSegments);
  auto_rate.reserve(response.players_size() * kAverageSegments);
  fill.reserve(response.players_size() * kAverageSegments);
  for (const AnalyzePlayerResponse::PlayerResponse& player : response.players()) {
    power.emplace_back(player.team().team_power(), 1);
    bonus.emplace_back(player.team().event_bonus(), 1);
    if (player.graph().observed_duration().seconds() >= dur_threshold) {
      uptime.emplace_back(player.graph().total_uptime().seconds(), 1);
      downtime.emplace_back(player.graph().total_downtime().seconds(), 1);
      autotime.emplace_back(player.graph().total_auto_time().seconds(), 1);
    }
    for (const AnalyzeGraphResponse::Segment& segment : player.graph().active_segments()) {
      if (!segment.is_confident()) continue;
      float segment_weight =
          static_cast<float>(segment.end().seconds() - segment.start().seconds()) / 600;
      if (segment.is_auto()) {
        auto_gph.emplace_back(segment.games_per_hour().value(), segment_weight);
        auto_epg.emplace_back(segment.ep_per_game().value(), segment_weight);
        auto_rate.emplace_back(segment.games_per_hour().value() * segment.ep_per_game().value(),
                               segment_weight);
      } else {
        gph.emplace_back(segment.games_per_hour().value(), segment_weight);
        epg.emplace_back(segment.ep_per_game().value(), segment_weight);
        rate.emplace_back(segment.games_per_hour().value() * segment.ep_per_game().value(),
                          segment_weight);
      }
      const AnalyzePlayResponse::BoostUsageDetails* absl_nullable strategy_to_use = nullptr;
      if (!segment.play_analysis().likely_play_strategies().empty()) {
        strategy_to_use = &segment.play_analysis().likely_play_strategies(0);
      }
      if (segment.is_auto() && !segment.play_analysis().likely_auto_play_strategies().empty()) {
        strategy_to_use = &segment.play_analysis().likely_auto_play_strategies(0);
      }
      if (strategy_to_use != nullptr && !strategy_to_use->is_auto()) {
        fill.emplace_back(strategy_to_use->estimated_avg_filler_skill().value(), segment_weight);
      }
    }
  }
  absl::c_sort(power);
  absl::c_sort(bonus);
  absl::c_sort(uptime);
  absl::c_sort(downtime);
  absl::c_sort(autotime);
  absl::c_sort(gph);
  absl::c_sort(epg);
  absl::c_sort(rate);
  absl::c_sort(auto_gph);
  absl::c_sort(auto_epg);
  absl::c_sort(auto_rate);
  absl::c_sort(fill);
  ComputeStatsResponse::PopulationStats stats;
  constexpr int kSamples = 15;
  ASSIGN_OR_RETURN(*stats.mutable_power_dist(),
                   ComputeDist(EpanechnikovKernel(10'000), power, kSamples));
  ASSIGN_OR_RETURN(*stats.mutable_bonus_dist(),
                   ComputeDist(EpanechnikovKernel(25), bonus, kSamples));
  ASSIGN_OR_RETURN(*stats.mutable_uptime_dist(),
                   ComputeDist(EpanechnikovKernel(3600), uptime, kSamples));
  ASSIGN_OR_RETURN(*stats.mutable_downtime_dist(),
                   ComputeDist(EpanechnikovKernel(3600), downtime, kSamples));
  ASSIGN_OR_RETURN(*stats.mutable_auto_time_dist(),
                   ComputeDist(EpanechnikovKernel(3600), autotime, kSamples));
  ASSIGN_OR_RETURN(*stats.mutable_gph_dist(), ComputeDist(EpanechnikovKernel(5), gph, kSamples));
  ASSIGN_OR_RETURN(*stats.mutable_epg_dist(),
                   ComputeDist(EpanechnikovKernel(15'000), epg, kSamples));
  ASSIGN_OR_RETURN(*stats.mutable_rate_dist(),
                   ComputeDist(EpanechnikovKernel(25'000), rate, kSamples));
  ASSIGN_OR_RETURN(*stats.mutable_auto_gph_dist(),
                   ComputeDist(EpanechnikovKernel(5), auto_gph, kSamples));
  ASSIGN_OR_RETURN(*stats.mutable_auto_epg_dist(),
                   ComputeDist(EpanechnikovKernel(15'000), auto_epg, kSamples));
  ASSIGN_OR_RETURN(*stats.mutable_auto_rate_dist(),
                   ComputeDist(EpanechnikovKernel(25'000), auto_rate, kSamples));
  ASSIGN_OR_RETURN(*stats.mutable_fill_dist(), ComputeDist(EpanechnikovKernel(25), fill, kSamples));
  return stats;
}

}  // namespace

absl::StatusOr<ComputeStatsResponse> ComputeStatsHandler::Run(
    const ComputeStatsRequest& request) const {
  return RunOnResponse(request.analyze_player_response());
}

absl::StatusOr<ComputeStatsResponse> ComputeStatsHandler::RunOnResponse(
    const AnalyzePlayerResponse& player_response) const {
  ComputeStatsResponse response;
  int64_t max_duration = 0;
  for (const AnalyzePlayerResponse::PlayerResponse& player : player_response.players()) {
    max_duration = std::max(max_duration, player.graph().observed_duration().seconds());
  }
  ASSIGN_OR_RETURN(*response.mutable_population_stats(),
                   ComputeStats(player_response, max_duration));
  return response;
}

}  // namespace sekai::run_analysis
