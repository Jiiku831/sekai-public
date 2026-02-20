#include <array>
#include <filesystem>
#include <tuple>

#include "absl/flags/flag.h"
#include "absl/log/log.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_format.h"
#include "base/init.h"
#include "load_data.h"
#include "sekai/run_analysis/clustering.h"
#include "sekai/run_analysis/segment_analysis.h"

ABSL_FLAG(std::string, run_data, "", "Run data to load");

namespace sekai::run_analysis {
namespace {

absl::Status RunOnFile(std::filesystem::path data_path) {
  absl::StatusOr<LoadedData> data = LoadDataV2(data_path);
  if (!data.ok()) {
    LOG(ERROR) << "Failed to load data:\n" << data.status();
    return absl::FailedPreconditionError("Failed to load data");
  }
  LOG(INFO) << "Data loaded!";
  for (std::size_t i = 0; i < data->segments.analyzed_segments().size(); ++i) {
    const absl::StatusOr<SegmentAnalysisResult>& analysis = data->segments.analyzed_segments()[i];
    if (!analysis.ok()) {
      LOG(INFO) << absl::StrFormat("Segment %d analysis failed: %s", i, analysis.status());
      continue;
    }

    if (!analysis->is_confident) {
      LOG(INFO) << absl::StrFormat("Segment %d analysis not confident", i);
      continue;
    }

    if (!analysis->game_count_analysis.ok()) {
      LOG(INFO) << absl::StrFormat("Segment %d game count analysis not present", i);
      continue;
    }

    bool def_real = false;
    if (analysis->segment_length >= absl::Hours(8)) def_real = true;
    float mean = analysis->game_count_analysis->ep_per_game.mean();
    float stdev = analysis->game_count_analysis->ep_per_game.stdev();
    std::cout << mean << "," << stdev << "," << stdev / mean << "," << def_real << "\r\n";
  }
  return absl::OkStatus();
}

int Run() {
  std::filesystem::path data_dir = absl::GetFlag(FLAGS_run_data);
  if (data_dir.empty()) {
    LOG(ERROR) << "No data specified to read.";
    return 1;
  }
  for (const std::filesystem::directory_entry& entry :
       std::filesystem::directory_iterator(data_dir)) {
    absl::Status status = RunOnFile(entry.path());
    if (!status.ok()) {
      return 1;
    }
  }
  return 0;
}

}  // namespace
}  // namespace sekai::run_analysis

int main(int argc, char** argv) {
  Init(argc, argv);
  return sekai::run_analysis::Run();
}
