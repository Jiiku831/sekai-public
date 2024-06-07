#include "testing/util.h"

#include <cstdlib>
#include <filesystem>

namespace testing {

std::filesystem::path GetDataPath(std::filesystem::path path) {
  std::filesystem::path full_path = std::getenv("RUNFILES_DIR");
  full_path /= path;
  return full_path;
}

}  // namespace testing
