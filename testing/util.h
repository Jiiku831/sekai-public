#include <filesystem>
#include <string_view>

#include <gmock/gmock.h>
#include <google/protobuf/util/message_differencer.h>
#include <gtest/gtest.h>

#include "sekai/proto_util.h"

namespace testing {

MATCHER_P(ProtoEquals, other, "") {
  google::protobuf::util::MessageDifferencer differ;
  std::string diffs;
  differ.ReportDifferencesToString(&diffs);
  bool result = differ.Compare(other, arg);
  *result_listener << "with differences:\n" << diffs;
  return result;
}

std::filesystem::path GetDataPath(std::filesystem::path path);

template <typename T>
T ParseTextProto(const std::string& str) {
  return sekai::ParseTextProto<T>(str);
}

template <typename T>
T ParseTextProtoFile(std::filesystem::path path) {
  return sekai::ParseTextProtoFile<T>(path);
}

}  // namespace testing
