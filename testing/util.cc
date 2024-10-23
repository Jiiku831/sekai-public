#include "testing/util.h"

#include <cstdlib>
#include <filesystem>

#include "absl/log/absl_check.h"

namespace testing {

std::filesystem::path GetTestTempDir() {
  char* temp_dir = std::getenv("TEST_TMPDIR");
  ABSL_CHECK_NE(temp_dir, nullptr) << "TEST_TMPDIR was requested but not set in env.";
  std::filesystem::path path = temp_dir;
  ABSL_CHECK(!path.empty()) << "TEST_TMPDIR was requested but was empty.";
  return path;
}

std::filesystem::path GetDataPath(std::filesystem::path path) {
  std::filesystem::path full_path = std::getenv("RUNFILES_DIR");
  full_path /= path;
  return full_path;
}

}  // namespace testing
