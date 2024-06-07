#include "sekai/html/report.h"

#include <string>
#include <string_view>

#include <ctml.hpp>

#include "sekai/file_util.h"

namespace sekai::html {

constexpr std::string_view kStylePath = "sekai/html/styles/report.css";

std::string CreateReport(const CTML::Node& node) {
  CTML::Document doc;
  doc.AppendNodeToHead(CTML::Node("style", GetRunfileContents(kStylePath)));
  doc.AppendNodeToBody(node);
  return doc.ToString();
}

}  // namespace sekai::html
