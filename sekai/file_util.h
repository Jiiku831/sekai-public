#pragma once

#include <filesystem>
#include <ios>
#include <string>
#include <string_view>

#include "absl/status/statusor.h"

namespace sekai {

std::string GetRunfileContents(std::filesystem::path path);
std::string GetFileContents(std::filesystem::path path);
std::string GetFileContents(std::filesystem::path path, std::ios_base::openmode mode);
absl::StatusOr<std::string> SafeGetFileContents(std::filesystem::path path,
                                                std::ios_base::openmode mode = std::ios_base::in);

void WriteContentsToFile(std::filesystem::path path, std::string_view contents);

}  // namespace sekai
