#include "sekai/run_analysis/parser.h"

#include <string_view>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_format.h"
#include "base/util.h"
#include "sekai/db/json/parser.h"
#include "sekai/run_analysis/proto/run_data.pb.h"

namespace sekai::run_analysis {

absl::StatusOr<RunData> ParseRunData(std::string_view json_str) {
  ASSIGN_OR_RETURN(std::vector<RunData> data, sekai::db::ParseJsonFromString<RunData>(json_str));
  if (data.size() != 1) {
    return absl::InvalidArgumentError(absl::StrFormat(
        "Expected input to contain exactly one RunData. Got %d instead.", data.size()));
  }
  return data[0];
}

}  // namespace sekai::run_analysis
