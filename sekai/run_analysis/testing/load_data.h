#pragma once

#include <filesystem>
#include <vector>

#include "absl/status/statusor.h"
#include "sekai/run_analysis/proto/run_data.pb.h"
#include "sekai/run_analysis/segmentation.h"
#include "sekai/run_analysis/snapshot.h"

namespace sekai::run_analysis {

struct LoadedData {
  RunData raw_data;
  int64_t timestamp_offset;
  std::vector<Snapshot> points;
  std::vector<Segment> segments;
};

absl::StatusOr<LoadedData> LoadData(std::filesystem::path path);

}  // namespace sekai::run_analysis
