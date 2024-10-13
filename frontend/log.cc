#include "frontend/log.h"

#include <source_location>
#include <string>
#include <string_view>

#include <emscripten.h>
#include <google/protobuf/message.h>
#include <google/protobuf/util/json_util.h>

#include "absl/log/log.h"

namespace frontend {
namespace {

using ::google::protobuf::util::MessageToJsonString;

// clang-format off
EM_JS(void, LogProto, (const char * proto), {
    console.log(JSON.parse(UTF8ToString(proto)));
});
// clang-format on

}  // namespace

void LogDebugProto(std::string_view title, const google::protobuf::Message& msg,
                   std::source_location loc) {
  std::string json;
  (void)MessageToJsonString(msg, &json);
  LOG(INFO).AtLocation(loc.file_name(), loc.line()) << title;
  LogProto(json.c_str());
}
}  // namespace frontend
