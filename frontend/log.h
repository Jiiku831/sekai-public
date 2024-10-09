#pragma once

#include <source_location>
#include <string_view>

#include <google/protobuf/message.h>

namespace frontend {

void LogDebugProto(std::string_view title, const google::protobuf::Message& msg,
                   std::source_location loc = std::source_location::current());

}  // namespace frontend
