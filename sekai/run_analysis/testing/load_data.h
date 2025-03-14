#pragma once

#include <filesystem>
#include <vector>

#include "absl/status/statusor.h"
#include "absl/time/time.h"
#include "sekai/run_analysis/histogram.h"
#include "sekai/run_analysis/proto/run_data.pb.h"
#include "sekai/run_analysis/segmentation.h"
#include "sekai/run_analysis/snapshot.h"

namespace sekai::run_analysis {

struct LoadedData {
  RunData raw_data;
  absl::Time timestamp_offset;
  Sequence raw_sequence;
  Sequence processed_sequence;
  std::vector<Sequence> segments;
  std::vector<Runs> runs;
  Histograms histograms;
  std::vector<Histograms> run_histograms;
  Histograms combined_run_histograms;
};

absl::StatusOr<LoadedData> LoadData(std::filesystem::path path);

}  // namespace sekai::run_analysis
