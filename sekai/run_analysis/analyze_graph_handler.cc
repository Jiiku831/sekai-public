#include "sekai/run_analysis/analyze_graph_handler.h"

#include <cstdint>
#include <iterator>

#include <google/protobuf/util/time_util.h>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/time/time.h"
#include "base/util.h"
#include "sekai/run_analysis/config.h"
#include "sekai/run_analysis/segment_analysis.h"
#include "sekai/run_analysis/segmentation.h"
#include "sekai/run_analysis/sequence_util.h"

namespace sekai::run_analysis {
namespace {

using ::google::protobuf::util::TimeUtil;

absl::StatusOr<Sequence> CreateSequence(const AnalyzeGraphRequest& req) {
  if (req.timestamps_size() != req.points_size()) {
    return absl::InvalidArgumentError(absl::StrFormat(
        "Invalid graph: timestamps (length=%d) and points (length=%d) should have the same length.",
        req.timestamps_size(), req.points_size()));
  }
  if (req.timestamps().empty()) {
    return absl::InvalidArgumentError("Invalid graph: input graph is empty!");
  }
  Sequence seq = {.time_offset = absl::FromUnixMillis(req.timestamps(0))};
  std::transform(req.timestamps().begin(), req.timestamps().end(), req.points().begin(),
                 std::back_inserter(seq.points), [&](const int64_t& ts, const int pt) {
                   return Snapshot{absl::FromUnixMillis(ts) - seq.time_offset, pt};
                 });
  return seq;
}

}  // namespace

absl::StatusOr<AnalyzeGraphResponse> AnalyzeGraphHandler::Run(
    const AnalyzeGraphRequest& request) const {
  ASSIGN_OR_RETURN(Sequence raw_sequence, CreateSequence(request));
  Sequence input_sequence = ProcessSequence(raw_sequence);
  ASSIGN_OR_RETURN(RunSegments segments, SegmentRuns(input_sequence));
  AnalyzeGraphResponse response;
  *response.mutable_observed_duration() =
      TimeUtil::NanosecondsToDuration(segments.total_duration() / absl::Nanoseconds(1));
  *response.mutable_total_uptime() =
      TimeUtil::NanosecondsToDuration(segments.total_uptime() / absl::Nanoseconds(1));
  *response.mutable_total_downtime() =
      TimeUtil::NanosecondsToDuration(segments.total_downtime() / absl::Nanoseconds(1));
  *response.mutable_total_auto_time() =
      TimeUtil::NanosecondsToDuration(segments.total_auto_time() / absl::Nanoseconds(1));
  for (const absl::StatusOr<SegmentAnalysisResult>& res : segments.analyzed_segments()) {
    *response.add_active_segments() = SegmentAnalysisResultToApiSegment(res);
  }
  return response;
}

}  // namespace sekai::run_analysis
