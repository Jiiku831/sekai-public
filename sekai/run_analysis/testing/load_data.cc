#include "sekai/run_analysis/testing/load_data.h"

#include <algorithm>
#include <functional>
#include <iterator>
#include <ranges>
#include <string>
#include <vector>

#include "absl/flags/flag.h"
#include "absl/log/log.h"
#include "absl/status/statusor.h"
#include "base/util.h"
#include "sekai/file_util.h"
#include "sekai/ranges_util.h"
#include "sekai/run_analysis/analyze_team_handler.h"
#include "sekai/run_analysis/config.h"
#include "sekai/run_analysis/parser.h"
#include "sekai/run_analysis/proto/run_data.pb.h"
#include "sekai/run_analysis/proto/service.pb.h"
#include "sekai/run_analysis/segmentation.h"
#include "sekai/run_analysis/sequence_util.h"
#include "sekai/run_analysis/snapshot.h"

ABSL_FLAG(int, event_id, 159, "Event ID");

namespace sekai::run_analysis {
namespace {

absl::StatusOr<Sequence> ConvertPointsGraph(absl::Time time_offset, const PointsGraph& graph) {
  if (graph.timestamps_size() != graph.points_size()) {
    return absl::InvalidArgumentError("Invalid graph: timestamps and points size differ!");
  }
  Sequence sequence = {
      .time_offset = time_offset,
  };
  std::transform(graph.timestamps().begin(), graph.timestamps().end(), graph.points().begin(),
                 std::back_inserter(sequence.points), [&](const auto ts, const auto pt) {
                   return Snapshot{absl::FromUnixMillis(ts) - time_offset, pt};
                 });
  return sequence;
}

AnalyzeTeamRequest MakeAnalyzeTeamRequest(const RunData& data) {
  AnalyzeTeamRequest req;
  req.set_event_id(absl::GetFlag(FLAGS_event_id));
  for (const auto& deck_card : data.team()) {
    auto* req_card = req.add_cards();
    req_card->set_card_id(deck_card.card_id());
    req_card->set_master_rank(deck_card.master_rank());
    req_card->set_trained_state(deck_card.display_state() == DISPLAY_STATE_TRAINED);
  }
  req.set_team_power(data.team_power().total_power());
  return req;
}

}  // namespace

absl::StatusOr<LoadedData> LoadData(std::filesystem::path path) {
  LoadedData data;
  ASSIGN_OR_RETURN(std::string contents, SafeGetFileContents(path));
  ASSIGN_OR_RETURN(data.raw_data, ParseRunData(contents));
  const PointsGraph& graph = data.raw_data.user_graph().overall();
  if (graph.timestamps().empty()) {
    return absl::InvalidArgumentError("Empty graph");
  }
  data.timestamp_offset = absl::FromUnixMillis(graph.timestamps(0));
  ASSIGN_OR_RETURN(data.raw_sequence, ConvertPointsGraph(data.timestamp_offset, graph));
  data.processed_sequence = ProcessSequence(data.raw_sequence, kInterval, kMaxSegmentGap);
  ASSIGN_OR_RETURN(data.segments, SegmentRuns(data.processed_sequence));
  data.run_histograms = RangesTo<std::vector<Histograms>>(
      data.segments.active_segments() | std::views::transform([](const Sequence& seq) {
        return ComputeHistograms(seq, kWindow, kInterval);
      }));
  data.histograms = Histograms::Join(data.run_histograms);
  AnalyzeTeamHandler handler;
  data.team_analysis_request = MakeAnalyzeTeamRequest(data.raw_data);
  data.team_analysis = handler.Run(data.team_analysis_request);
  return data;
}

}  // namespace sekai::run_analysis
