#include "sekai/run_analysis/parser.h"

#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "absl/status/status_matchers.h"
#include "sekai/file_util.h"

namespace sekai::run_analysis {
namespace {

using ::absl_testing::IsOk;

TEST(ParserTest, ParsesRunDataSuccessfully) {
  std::string json_str = GetRunfileContents("sekai/run_analysis/testdata/run_data_1.json");
  EXPECT_THAT(ParseRunData(json_str), IsOk());
}

}  // namespace
}  // namespace sekai::run_analysis
