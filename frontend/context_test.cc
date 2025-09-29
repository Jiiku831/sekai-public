#include "frontend/context.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace frontend {
namespace {

TEST(InitialRenderContextTest, BuildSuccessfully) {
  InitialRenderContext context = CreateInitialRenderContext();
  EXPECT_TRUE(context.has_card_list());
  EXPECT_TRUE(context.has_power_bonus());
  EXPECT_TRUE(context.has_team_builder());
}

}  // namespace
}  // namespace frontend
