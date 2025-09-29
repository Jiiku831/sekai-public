#include "base/init.h"

#include <vector>

#include "absl/flags/parse.h"
#include "absl/log/initialize.h"

std::vector<char*> Init(int argc, char** argv) {
  std::vector<char*> remaining = absl::ParseCommandLine(argc, argv);
  absl::InitializeLog();
  return remaining;
}
