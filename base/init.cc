#include "base/init.h"

#include "absl/flags/parse.h"
#include "absl/log/initialize.h"

void Init(int argc, char** argv) {
  absl::ParseCommandLine(argc, argv);
  absl::InitializeLog();
}
