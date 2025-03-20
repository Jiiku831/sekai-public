#pragma once

#include <string_view>

#include "absl/status/statusor.h"
#include "sekai/run_analysis/proto/run_data.pb.h"

namespace sekai::run_analysis {

absl::StatusOr<RunData> ParseRunData(std::string_view json_str);

}  // namespace sekai::run_analysis
