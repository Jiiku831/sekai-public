#include "sekai/array_size.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace sekai {
namespace {

TEST(ArraySizeTest, TestCharacterArraySizeConstant) {
  EXPECT_LE(CharacterArraySize(), kCharacterArraySize);
}

TEST(ArraySizeTest, TestCardArraySizeConstant) { EXPECT_LE(CardArraySize(), kCardArraySize); }

TEST(ArraySizeTest, TestMySekaiGateArraySizeConstant) {
  EXPECT_LE(MySekaiGateArraySize(), kMySekaiGateArraySize);
}

}  // namespace
}  // namespace sekai
