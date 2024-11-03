#include "sekai/server.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "absl/log/log.h"
#include "sekai/db/proto/server.pb.h"

namespace sekai {
namespace {

TEST(ServerTest, ServerIsValid) {
  EXPECT_GT(GetServer(), 0);
  LOG(INFO) << "Server: " << db::Server_Name(GetServer());
}

}  // namespace
}  // namespace sekai
