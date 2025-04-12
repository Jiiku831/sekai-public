#include "sekai/run_analysis/config.h"

#include "absl/flags/flag.h"

ABSL_FLAG(bool, run_analysis_debug, false, "Enable debug mode.");

namespace sekai::run_analysis {

bool DebugEnabled() { return absl::GetFlag(FLAGS_run_analysis_debug); }

}  // namespace sekai::run_analysis
