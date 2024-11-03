#include <fstream>
#include <string>

#include <ctml.hpp>

#include "absl/flags/flag.h"
#include "absl/log/absl_check.h"
#include "base/init.h"
#include "sekai/html/version.h"

ABSL_FLAG(std::string, output, "", "output path");

int main(int argc, char** argv) {
  Init(argc, argv);

  ABSL_CHECK(!absl::GetFlag(FLAGS_output).empty());
  std::ofstream fout(absl::GetFlag(FLAGS_output));
  fout << sekai::html::CurrentVersion().ToString();
}
