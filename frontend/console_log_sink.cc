#include "frontend/console_log_sink.h"

#include "absl/base/log_severity.h"
#include "absl/log/log_entry.h"
#include "emscripten/console.h"

void ConsoleLogSink::Send(const absl::LogEntry& entry) {
  switch (entry.log_severity()) {
    case absl::LogSeverity::kInfo:
      emscripten_console_log(entry.text_message_with_prefix_and_newline_c_str());
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
