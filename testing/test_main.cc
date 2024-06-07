// Custom googletest main

#include <iostream>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "absl/log/globals.h"
#include "absl/log/initialize.h"

int main(int argc, char** argv) {
  testing::InitGoogleMock(&argc, argv);
  absl::InitializeLog();
  absl::SetStderrThreshold(absl::MinLogLevel());
  GTEST_FLAG_SET(color, "yes");
  return RUN_ALL_TESTS();
}
