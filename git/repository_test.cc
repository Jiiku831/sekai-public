#include "git/repository.h"

#include <filesystem>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "absl/log/log.h"
#include "absl/status/status_matchers.h"
#include "testing/util.h"

namespace git {
namespace {

using ::absl_testing::IsOk;

class RepositoryTest : public ::testing::Test {
 protected:
  void SetUp() override {
    path_ = testing::GetTestTempDir() / "repo";
    std::filesystem::remove_all(path_);
    std::filesystem::create_directory(path_);
    LOG(INFO) << "Created temp repo directory: " << path_;
  }

  void TearDown() override { std::filesystem::remove_all(path_); }

  std::filesystem::path path_;
};

TEST_F(RepositoryTest, InitializeAndOpenRepo) {
  absl::StatusOr<Repository> init_repo = Repository::Init(path_);
  EXPECT_THAT(init_repo, IsOk()) << init_repo.status();
  absl::StatusOr<Repository> open_repo = Repository::Open(path_);
  EXPECT_THAT(open_repo, IsOk()) << open_repo.status();
}

}  // namespace
}  // namespace git
