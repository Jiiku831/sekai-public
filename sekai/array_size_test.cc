#include "sekai/array_size.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace sekai {
namespace {

TEST(ArraySizeTest, TestCharacterArraySizeConstant) {
  EXPECT_LE(CharacterArraySize(), kCharacterArraySize);
}

}  // namespace
}  // namespace sekai
