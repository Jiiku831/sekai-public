#pragma once

#include <filesystem>
#include <ios>
#include <string>
#include <string_view>

namespace sekai {

std::string GetRunfileContents(std::filesystem::path path);
std::string GetFileContents(std::filesystem::path path);
std::string GetFileContents(std::filesystem::path path, std::ios_base::openmode mode);

void WriteContentsToFile(std::filesystem::path path, std::string_view contents);

}  // namespace sekai
