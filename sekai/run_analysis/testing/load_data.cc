#include "sekai/run_analysis/testing/load_data.h"

#include <algorithm>
#include <iterator>
#include <string>

#include "absl/status/statusor.h"
#include "base/util.h"
#include "sekai/file_util.h"
#include "sekai/run_analysis/config.h"
#include "sekai/run_analysis/parser.h"
#include "sekai/run_analysis/proto/run_data.pb.h"
#include "sekai/run_analysis/segmentation.h"
#include "sekai/run_analysis/snapshot.h"

namespace sekai::run_analysis {

absl::StatusOr<std::vector<Snapshot>> ConvertPointsGraph(int64_t timestamp_offset,
                                                         const PointsGraph& graph) {
  if (graph.timestamps_size() != graph.points_size()) {
    return absl::InvalidArgumentError("Invalid graph: timestamps and points size differ!");
  }
  std::vector<Snapshot> points;
  std::transform(graph.timestamps().begin(), graph.timestamps().end(), graph.points().begin(),
                 std::back_inserter(points), [&](const auto ts, const auto pt) {
                   return Snapshot{static_cast<int>(ts - timestamp_offset) / 1000, pt};
                 });
  return points;
}

absl::StatusOr<LoadedData> LoadData(std::filesystem::path path) {
  LoadedData data;
  ASSIGN_OR_RETURN(std::string contents, SafeGetFileContents(path));
  ASSIGN_OR_RETURN(data.raw_data, ParseRunData(contents));
  const PointsGraph& graph = data.raw_data.user_graph().overall();
  if (graph.timestamps().empty()) {
    return absl::InvalidArgumentError("Empty graph");
  }
  data.timestamp_offset = graph.timestamps(0);
  ASSIGN_OR_RETURN(data.points, ConvertPointsGraph(data.timestamp_offset, graph));
  data.segments = SplitIntoSegments(data.points, kMinSegmentLength, kMaxSegmentGap);
  return data;
}

}  // namespace sekai::run_analysis
