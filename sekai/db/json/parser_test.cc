#include "sekai/db/json/parser.h"

#include <fstream>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "absl/log/log.h"
#include "sekai/config.h"
#include "sekai/db/proto/card.pb.h"

namespace sekai::db {
namespace {

TEST(ParserTest, TryParseCards) {
  LOG(INFO) << std::filesystem::current_path();
  LOG(INFO) << MasterDbRoot() / "cards.json";
  std::ifstream ifs;
  internal::JsonParser<Card> parser;
  ifs.open(MasterDbRoot() / "cards.json", std::ifstream::in);
  bool status = nlohmann::json::sax_parse(ifs, &parser);
  EXPECT_TRUE(status) << parser.status();
  EXPECT_GT(parser.objs().size(), 700);
}

}  // namespace
}  // namespace sekai::db
