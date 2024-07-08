#include "sekai/config.h"

#include <cstdlib>
#include <filesystem>
#include <string_view>

namespace sekai {
namespace {

#ifdef __cplusplus
#define xstr(exp) str(exp)
#define str(exp) #exp
constexpr std::string_view kCppVersion = xstr(__cplusplus);
#undef xstr
#undef str
#else
constexpr std::string_view kCppVersion = "<unknown>";
#endif

constexpr std::string_view kMainRepoRoot = "_main";

std::filesystem::path RunfilesDir(std::filesystem::path relpath, std::string_view repo_root) {
  char* run_files = std::getenv("RUNFILES_DIR");
  std::filesystem::path path;
  if (run_files == nullptr) {
    path = "./";
    path /= relpath;
  } else {
    path = std::getenv("RUNFILES_DIR");
    path /= repo_root;
    path /= relpath;
  }
  return path;
}

std::string_view SekaiRepoRoot() {
#ifdef SEKAI_REPO_ROOT
  return SEKAI_REPO_ROOT;
#else
  return kMainRepoRoot;
#endif
}

}  // namespace

std::string_view CppVersion() { return kCppVersion; }

const std::filesystem::path& SekaiBestRoot() {
  static const std::filesystem::path* const kPath = [] {
    auto* path = new std::filesystem::path;
    *path = RunfilesDir("data/storage-sekai-best", SekaiRepoRoot());
    return path;
  }();
  return *kPath;
}

const std::filesystem::path& MasterDbRoot() {
  static const std::filesystem::path* const kPath = [] {
    auto* path = new std::filesystem::path;
    *path = RunfilesDir("external/sekai-master-db~", SekaiRepoRoot());
    return path;
  }();
  return *kPath;
}

const std::filesystem::path& SekaiRunfilesRoot() {
  static const std::filesystem::path* const kPath = [] {
    auto* path = new std::filesystem::path;
    *path = RunfilesDir("./", SekaiRepoRoot());
    return path;
  }();
  return *kPath;
}

const std::filesystem::path& MainRunfilesRoot() {
  static const std::filesystem::path* const kPath = [] {
    auto* path = new std::filesystem::path;
    *path = RunfilesDir("./", kMainRepoRoot);
    return path;
  }();
  return *kPath;
}

}  // namespace sekai
