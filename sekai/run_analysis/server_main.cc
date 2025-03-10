#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string_view>

#include "absl/base/log_severity.h"
#include "absl/base/no_destructor.h"
#include "absl/container/flat_hash_map.h"
#include "absl/log/globals.h"
#include "absl/log/log.h"
#include "absl/strings/str_cat.h"
#include "base/init.h"
#include "sekai/run_analysis/analyze_team_handler.h"
#include "sekai/run_analysis/handler.h"

using ::sekai::run_analysis::AnalyzeTeamHandler;
using ::sekai::run_analysis::HandlerBase;

const absl::NoDestructor<absl::flat_hash_map<std::string, const HandlerBase*>> kHandlers{{
    {"/analyze_team", new AnalyzeTeamHandler},
}};

std::string HandleRequestImpl(std::string_view path, std::string_view request) {
  auto handler = kHandlers->find(path);
  if (handler == kHandlers->end()) {
    std::string output = absl::StrCat("No handler found for path: ", path);
    output.insert(0, 1, static_cast<char>(absl::StatusCode::kNotFound));
    return output;
  }
  return handler->second->Run(request);
}

char* ToCStr(const std::string& str) {
  char* c_str = static_cast<char*>(std::malloc(str.size() + 1));
  std::strcpy(c_str, str.c_str());
  return c_str;
}

extern "C" {
char* Malloc(int size) { return static_cast<char*>(std::malloc(size)); }
void Free(char* ptr) { std::free(ptr); }
char* HandleRequest(const char* path, const char* request) {
  return ToCStr(HandleRequestImpl(path, request));
}
}

int main(int argc, char** argv) {
  Init(argc, argv);
  absl::SetMinLogLevel(absl::LogSeverityAtLeast::kInfo);
  absl::SetStderrThreshold(absl::LogSeverityAtLeast::kInfo);

  LOG(INFO) << "Initializing server...";

  LOG(INFO) << "Registered handlers:";
  for (const auto& [path, unused_handler] : *kHandlers) {
    LOG(INFO) << path;
  }

  LOG(INFO) << "Done!" << std::endl;

  return 0;
}
