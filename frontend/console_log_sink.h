#pragma once

#include "absl/log/log_entry.h"
#include "absl/log/log_sink.h"

class ConsoleLogSink : public absl::LogSink {
 public:
  ConsoleLogSink() = default;

  static ConsoleLogSink* Get() {
    static auto* const kLogSink = new ConsoleLogSink;
    return kLogSink;
  }

  void Send(const absl::LogEntry& entry) override;
};
