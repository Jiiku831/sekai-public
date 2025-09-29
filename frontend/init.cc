#include "frontend/init.h"

#include <emscripten.h>
#include <google/protobuf/util/json_util.h>

#include "absl/log/absl_check.h"
#include "absl/strings/str_cat.h"
#include "frontend/context.h"
#include "frontend/display_text.h"
#include "frontend/element_id.h"
#include "frontend/proto/context.pb.h"
#include "sekai/card.h"
#include "sekai/character.h"
#include "sekai/db/master_db.h"
#include "sekai/db/proto/all.h"
#include "sekai/proto_util.h"

namespace frontend {
namespace {

using ::google::protobuf::util::MessageToJsonString;

EM_JS(void, CallInitialRender, (const char* context), {
    const parsed_context = JSON.parse(UTF8ToString(context));
    InitialRender(parsed_context);
});

}  // namespace

void InitializePage() {
  std::string json;
  (void)MessageToJsonString(CreateInitialRenderContext(), &json);
  CallInitialRender(json.c_str());
}

}  // namespace frontend
