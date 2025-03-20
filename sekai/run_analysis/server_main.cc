#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string_view>

#include "absl/base/log_severity.h"
#include "absl/base/no_destructor.h"
#include "absl/container/flat_hash_map.h"
#include "absl/log/globals.h"
#include "absl/log/log.h"
#include "absl/log/log_entry.h"
#include "absl/log/log_sink.h"
#include "absl/log/log_sink_registry.h"
#include "absl/strings/str_cat.h"
#include "base/init.h"
#include "emscripten/console.h"
#include "sekai/db/master_db.h"
#include "sekai/run_analysis/analyze_graph_handler.h"
#include "sekai/run_analysis/analyze_team_handler.h"
#include "sekai/run_analysis/batch_handler.h"
#include "sekai/run_analysis/handler.h"
#include "sekai/run_analysis/proto/service.pb.h"

using namespace ::sekai::run_analysis;

const absl::NoDestructor<absl::flat_hash_map<std::string, const HandlerBase*>> kHandlers{{
    {"/analyze_graph", new AnalyzeGraphHandler},
    {"/analyze_team", new AnalyzeTeamHandler},
    {"/batch_analyze_graph",
     new BatchHandler<AnalyzeGraphHandler, BatchAnalyzeGraphRequest, BatchAnalyzeGraphResponse>},
}};

class WorkerLogSink : public absl::LogSink {
 public:
  WorkerLogSink() = default;

  static WorkerLogSink* Get() {
    static auto* const kLogSink = new WorkerLogSink;
    return kLogSink;
  }

  void Send(const absl::LogEntry& entry) override {
    switch (entry.log_severity()) {
      case absl::LogSeverity::kInfo:
        // emscripten_console_log(entry.text_message_with_prefix_and_newline_c_str());
        break;
      case absl::LogSeverity::kWarning:
        emscripten_console_warn(entry.text_message_with_prefix_and_newline_c_str());
        break;
      case absl::LogSeverity::kError:
      case absl::LogSeverity::kFatal:
        emscripten_console_error(entry.text_message_with_prefix_and_newline_c_str());
        break;
    }
  }
};

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
  std::memcpy(c_str, str.c_str(), str.size() + 1);
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
  absl::SetStderrThreshold(absl::LogSeverityAtLeast::kInfinity);
  absl::AddLogSink(WorkerLogSink::Get());

  LOG(INFO) << "Initializing server...";
  LOG(INFO) << sekai::db::MasterDb::Get().DebugString();

  LOG(INFO) << "Registered handlers:\n"
            << absl::StrJoin(*kHandlers, "\n", [](std::string* out, const auto& handler) {
                 absl::StrAppend(out, handler.first);
               });

  LOG(INFO) << "Done!" << std::endl;

  return 0;
}
