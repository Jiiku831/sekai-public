
#include <string>

#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "base/init.h"
#include "sekai/run_analysis/analyze_player_handler.h"
#include "sekai/run_analysis/handlers.h"
#include "sekai/run_analysis/proto/service.pb.h"
#include "sekai/run_analysis/proto_util.h"

namespace sekai::run_analysis {
namespace {

nanobind::bytes WrapResponse(absl::string_view output) {
  if (output.empty()) {
    nanobind::raise("empty output");
  }
  if (output[0] != 0) {
    nanobind::raise("error: %s", output.substr(1).data());
  }
  return nanobind::bytes(output.substr(1).data(), output.size() - 1);
}

nanobind::bytes SendRequest(const std::string& path, nanobind::bytes input) {
  std::string_view request(static_cast<const char*>(input.data()), input.size());
  return WrapResponse(HandleRequest(path, request, /*binary=*/true));
}

void InitModule() { InitLogging(); }

NB_MODULE(bridge, m) {
  m.def("send_request", &SendRequest);
  m.def("init_module", &InitModule);
}

}  // namespace
}  // namespace sekai::run_analysis
