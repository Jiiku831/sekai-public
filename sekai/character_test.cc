#include "sekai/character.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "sekai/bitset.h"
#include "sekai/db/proto/enums.pb.h"

namespace sekai {
namespace {

TEST(LookupCharacterUnitTest, TestMiku) { EXPECT_EQ(LookupCharacterUnit(21), db::UNIT_VS); }

}  // namespace
}  // namespace sekai
