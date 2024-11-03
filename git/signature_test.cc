#include "git/signature.h"

#include <filesystem>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "absl/log/log.h"
#include "absl/status/status_matchers.h"

namespace git {
namespace {

using ::absl_testing::IsOk;

TEST(SignatureTest, CreatesNewSuccessfully) {
  absl::StatusOr<Signature> sig = Signature::New("lmao", "kek@lmao.com");
  EXPECT_THAT(sig, IsOk()) << sig.status();
}

}  // namespace
}  // namespace git
