#pragma once

#include <string_view>

#include <google/protobuf/message.h>

namespace sekai::run_analysis {

void PrintMessage(const char* title, const google::protobuf::Message& msg);

}  // namespace sekai::run_analysis
