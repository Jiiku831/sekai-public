#include "sekai/run_analysis/testing/load_data.h"

#include <algorithm>
#include <functional>
#include <iterator>
#include <ranges>
#include <string>
#include <vector>

#include "absl/log/log.h"
#include "absl/status/statusor.h"
#include "base/util.h"
#include "sekai/file_util.h"
#include "sekai/ranges_util.h"
#include "sekai/run_analysis/config.h"
#include "sekai/run_analysis/parser.h"
#include "sekai/run_analysis/proto/run_data.pb.h"
#include "sekai/run_analysis/segmentation.h"
#include "sekai/run_analysis/sequence_util.h"
#include "sekai/run_analysis/snapshot.h"

namespace sekai::run_analysis {

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
  data.segments = SplitIntoSegments(data.processed_sequence, kMinSegmentLength, kMaxSegmentGap);
#ifdef __cpp_lib_bind_back
#warning std::bind_back is available to replace this crap
#endif
  data.runs = RangesTo<std::vector<Runs>>(data.segments | std::views::transform([](const auto& x) {
                                            return SegmentRuns(x, kWindow, kBreakpointShift,
                                                               kBreakpointThresholdLow,
                                                               kBreakpointThresholdHigh);
                                          }));
  data.histograms = ComputeHistograms(data.segments, kWindow, kInterval);
  data.run_histograms =
      RangesTo<std::vector<Histograms>>(data.runs | std::views::transform([](const auto& x) {
                                          return ComputeHistograms(x.runs, kWindow, kInterval);
                                        }));
  data.combined_run_histograms = Histograms::Join(data.run_histograms);
  return data;
}

}  // namespace sekai::run_analysis
