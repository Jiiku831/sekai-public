#include <filesystem>
#include <string>

#include "absl/flags/flag.h"
#include "absl/log/absl_check.h"
#include "base/init.h"
#include "inja.hpp"

ABSL_FLAG(std::string, input, "", "input path");
ABSL_FLAG(std::string, base_dir, "", "base dir path");
ABSL_FLAG(std::string, output, "", "output path");

int main(int argc, char** argv) {
  Init(argc, argv);

  ABSL_QCHECK(!absl::GetFlag(FLAGS_input).empty()) << "No input specified!";
  ABSL_QCHECK(!absl::GetFlag(FLAGS_output).empty()) << "No output specified!";
  ABSL_QCHECK(!absl::GetFlag(FLAGS_base_dir).empty()) << "No base_dir specified!";

  inja::Environment env(absl::GetFlag(FLAGS_base_dir));
  env.write(env.parse_template(std::filesystem::relative(absl::GetFlag(FLAGS_input),
                                                         absl::GetFlag(FLAGS_base_dir))),
            /*data=*/{},
            std::filesystem::relative(absl::GetFlag(FLAGS_output), absl::GetFlag(FLAGS_base_dir)));
}
