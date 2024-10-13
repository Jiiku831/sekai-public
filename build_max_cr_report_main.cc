#include <fstream>
#include <optional>
#include <string>
#include <vector>

#include <ctml.hpp>

#include "absl/flags/flag.h"
#include "absl/log/absl_check.h"
#include "absl/time/time.h"
#include "base/init.h"
#include "sekai/html/max_cr.h"
#include "sekai/html/report.h"
#include "sekai/max_character_rank.h"

ABSL_FLAG(std::string, output, "", "output path");
ABSL_FLAG(bool, raw, "", "output raw table html");

using namespace sekai;

int main(int argc, char** argv) {
  Init(argc, argv);

  ABSL_CHECK(!absl::GetFlag(FLAGS_output).empty());
  std::ofstream fout(absl::GetFlag(FLAGS_output));
  CTML::Node table = html::MaxCrTable(GetMaxCharacterRanks(absl::Now()));
  fout << (absl::GetFlag(FLAGS_raw) ? table.ToString() : html::CreateReport(table));
}
