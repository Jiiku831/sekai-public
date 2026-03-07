#include <cstdlib>
#include <cstring>
#include <iostream>

#include "absl/base/log_severity.h"
#include "absl/log/globals.h"
#include "absl/log/log.h"
#include "absl/log/log_entry.h"
#include "absl/log/log_sink.h"
#include "absl/log/log_sink_registry.h"
#include "base/init.h"
#include "emscripten/console.h"
#include "sekai/db/master_db.h"
#include "sekai/run_analysis/handlers.h"
#include "sekai/run_analysis/proto/service.pb.h"

using namespace ::sekai::run_analysis;

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

char* ToCStr(const std::string& str) {
  char* c_str = static_cast<char*>(std::malloc(str.size() + 1));
  std::memcpy(c_str, str.c_str(), str.size() + 1);
  return c_str;
}

extern "C" {
char* Malloc(int size) { return static_cast<char*>(std::malloc(size)); }
void Free(char* ptr) { std::free(ptr); }
char* HandleRequest(const char* path, const char* request) {
  return ToCStr(sekai::run_analysis::HandleRequest(path, request));
}
}

int main(int argc, char** argv) {
  Init(argc, argv);
  absl::SetMinLogLevel(absl::LogSeverityAtLeast::kInfo);
  absl::SetStderrThreshold(absl::LogSeverityAtLeast::kInfinity);
  absl::AddLogSink(WorkerLogSink::Get());

  LOG(INFO) << "Initializing server...";
  LOG(INFO) << sekai::db::MasterDb::Get().DebugString();

  LogRegisteredHandlers();

  LOG(INFO) << "Done!" << std::endl;

  return 0;
}
