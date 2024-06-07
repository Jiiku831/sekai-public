#pragma once

#include <string>

#include <ctml.hpp>

namespace sekai::html {

// Creates HTML for a self contained report.
std::string CreateReport(const CTML::Node& node);

}  // namespace sekai::html
