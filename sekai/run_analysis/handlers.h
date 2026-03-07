#pragma once

#include <string>
#include <string_view>

namespace sekai::run_analysis {

// For JSON: the first byte will represent the status of the whole response.
// For binary: the first byte will represent the status of serializing the response.
std::string HandleRequest(std::string_view path, std::string_view request, bool binary = false);

void LogRegisteredHandlers();

}  // namespace sekai::run_analysis
