#include <iostream>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "absl/log/absl_check.h"
#include "absl/log/globals.h"
#include "absl/log/initialize.h"
#include "git/init.h"

int main(int argc, char** argv) {
  testing::InitGoogleMock(&argc, argv);
  absl::InitializeLog();
  absl::SetStderrThreshold(absl::MinLogLevel());
  GTEST_FLAG_SET(color, "yes");
  ABSL_CHECK_OK(git::Init());
  return RUN_ALL_TESTS();
}
