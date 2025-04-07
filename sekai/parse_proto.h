#pragma once

#include <optional>

#include <google/protobuf/descriptor.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/text_format.h>

#include "absl/log/absl_check.h"

namespace sekai {

template <typename T>
T ParseTextProto(const std::string& str) {
  T msg;
  ABSL_CHECK(google::protobuf::TextFormat::ParseFromString(str, &msg));
  return msg;
}

template <typename T>
std::optional<T> SafeParseTextProto(const std::string& str) {
  T msg;
  if (!google::protobuf::TextFormat::ParseFromString(str, &msg)) {
    return std::nullopt;
  }
  return msg;
}

template <typename T>
std::optional<T> SafeParseBinaryProto(const std::string& str) {
  T msg;
  if (!msg.ParseFromString(str)) {
    return std::nullopt;
  }
  return msg;
}

}  // namespace sekai
