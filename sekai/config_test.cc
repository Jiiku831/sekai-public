#include "sekai/config.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "absl/log/log.h"

namespace sekai {
namespace {

using ::testing::IsEmpty;
using ::testing::Not;

TEST(ConfigTest, CppVersion) {
  LOG(INFO) << CppVersion();
  EXPECT_THAT(CppVersion(), Not(IsEmpty()));
}

TEST(ConfigTest, SekaiBestRoot) {
  LOG(INFO) << SekaiBestRoot();
  EXPECT_THAT(static_cast<std::string>(SekaiBestRoot()), Not(IsEmpty()));
}

TEST(ConfigTest, MasterDbRoot) {
  LOG(INFO) << MasterDbRoot();
  EXPECT_THAT(static_cast<std::string>(MasterDbRoot()), Not(IsEmpty()));
}

TEST(ConfigTest, SekaiRunfilesRoot) {
  LOG(INFO) << SekaiRunfilesRoot();
  EXPECT_THAT(static_cast<std::string>(SekaiRunfilesRoot()), Not(IsEmpty()));
}

TEST(ConfigTest, MainRunfilesRoot) {
  LOG(INFO) << MainRunfilesRoot();
  EXPECT_THAT(static_cast<std::string>(MainRunfilesRoot()), Not(IsEmpty()));
}

}  // namespace
}  // namespace sekai
