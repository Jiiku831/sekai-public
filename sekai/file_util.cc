#include "sekai/file_util.h"

#include <filesystem>
#include <fstream>
#include <ios>
#include <sstream>
#include <string>
#include <string_view>

#include "absl/log/log.h"
#include "sekai/config.h"

namespace sekai {

std::string GetRunfileContents(std::filesystem::path path) {
  return GetFileContents(SekaiRunfilesRoot() / path);
}

std::string GetFileContents(std::filesystem::path path) {
  return GetFileContents(path, std::ios_base::in);
}

std::string GetFileContents(std::filesystem::path path, std::ios_base::openmode mode) {
  if (!std::filesystem::exists(path)) {
    LOG(ERROR) << "file does not exist: " << path << " (cwd: " << std::filesystem::current_path()
               << ")";
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
