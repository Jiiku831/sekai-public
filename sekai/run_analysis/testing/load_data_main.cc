#include <array>
#include <filesystem>
#include <tuple>

#include "absl/flags/flag.h"
#include "absl/log/log.h"
#include "base/init.h"
#include "load_data.h"

ABSL_FLAG(std::string, run_data, "", "Run data to load");

using ::sekai::run_analysis::LoadData;
using ::sekai::run_analysis::LoadedData;

int main(int argc, char** argv) {
  Init(argc, argv);

  std::filesystem::path data_path = absl::GetFlag(FLAGS_run_data);
  if (data_path.empty()) {
    LOG(ERROR) << "No data specified to read.";
    return 1;
  }
  absl::StatusOr<LoadedData> data = LoadData(data_path);
  if (!data.ok()) {
    LOG(ERROR) << "Failed to load data:\n" << data.status();
    return 1;
  }
  LOG(INFO) << "Data loaded:\n" << data->raw_data.DebugString();
}
