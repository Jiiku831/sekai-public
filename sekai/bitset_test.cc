#include "sekai/bitset.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "sekai/array_size.h"
#include "sekai/db/proto/enums.pb.h"

namespace sekai {
namespace {

TEST(BitsetTest, Attr) {
  Attr attr;
  EXPECT_FALSE(attr[db::ATTR_COOL]);
  attr[db::ATTR_COOL] = true;
  EXPECT_TRUE(attr[db::ATTR_COOL]);
}

TEST(BitsetTest, AttrInitValue) {
  Attr attr(db::ATTR_COOL);
  EXPECT_TRUE(attr[db::ATTR_COOL]);
}

TEST(BitsetTest, Flip) {
  Attr attr(db::ATTR_COOL);
  Attr new_attr = ~attr;
  EXPECT_TRUE(attr[db::ATTR_COOL]);
  EXPECT_FALSE(new_attr[db::ATTR_COOL]);
  EXPECT_EQ(~new_attr, attr);
}

TEST(BitsetTest, OrOp) {
  Attr attr(db::ATTR_COOL);
  Attr other_attr(db::ATTR_MYST);
  Attr new_attr = attr | other_attr;
  EXPECT_TRUE(new_attr[db::ATTR_COOL]);
  EXPECT_TRUE(new_attr[db::ATTR_MYST]);
}

TEST(CharacterTest, TestSize) {
  Character character;
  EXPECT_GE(character.size(), CharacterArraySize());
}

}  // namespace
}  // namespace sekai
