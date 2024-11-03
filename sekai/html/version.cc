#include "sekai/html/version.h"

#include <ctml.hpp>

#include "absl/strings/str_format.h"
#include "sekai/version.h"

namespace sekai::html {

CTML::Node CurrentVersion() {
  return CTML::Node("div.current-versions")
      .AppendChild(CTML::Node("div.game-version")
                       .AppendText(absl::StrFormat("App Version: %v", GetCurrentAppVersion())))
      .AppendChild(CTML::Node("div.asset-version")
                       .AppendText(absl::StrFormat("Asset Version: %v", GetCurrentAssetVersion())))
      .AppendChild(CTML::Node("div.data-version")
                       .AppendText(absl::StrFormat("Data Version: %v", GetCurrentDataVersion())));
}

}  // namespace sekai::html
