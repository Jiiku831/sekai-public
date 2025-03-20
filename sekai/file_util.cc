#include "sekai/file_util.h"

#include <filesystem>
#include <fstream>
#include <ios>
#include <sstream>
#include <string>
#include <string_view>

#include "absl/log/log.h"
#include "absl/strings/str_format.h"
#include "sekai/config.h"

namespace sekai {

std::string GetRunfileContents(std::filesystem::path path) {
  return GetFileContents(SekaiRunfilesRoot() / path);
}

std::string GetFileContents(std::filesystem::path path) {
  return GetFileContents(path, std::ios_base::in);
}

std::string GetFileContents(std::filesystem::path path, std::ios_base::openmode mode) {
  absl::StatusOr<std::string> data = SafeGetFileContents(path, mode);
  if (!data.ok()) {
    LOG(ERROR) << "Failed to open file:\n" << data.status();
  }
  return data.value_or("");
}

absl::StatusOr<std::string> SafeGetFileContents(std::filesystem::path path,
                                                std::ios_base::openmode mode) {
  if (!std::filesystem::exists(path)) {
    return absl::NotFoundError(absl::StrFormat("File %s does not exist. (cwd: %s)", path,
                                               std::filesystem::current_path()));
  }
  std::ifstream fin(path, mode);
  std::string data;
  fin.seekg(0, std::ios::end);
  data.resize(fin.tellg());
  fin.seekg(0, std::ios::beg);
  fin.read(&data[0], data.size());
  fin.close();
  return data;
}

void WriteContentsToFile(std::filesystem::path path, std::string_view contents) {
  std::ofstream fout(path);
  fout << contents;
}

}  // namespace sekai
