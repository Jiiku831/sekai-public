#include <iostream>
#include <ostream>

#include <emscripten.h>
#include <emscripten/console.h>

#include "absl/base/log_severity.h"
#include "absl/log/globals.h"
#include "absl/log/log.h"
#include "absl/log/log_sink_registry.h"
#include "base/init.h"
#include "frontend/console_log_sink.h"
#include "frontend/init.h"
#include "sekai/db/master_db.h"

EM_JS(void, InitHost, (), { InitPage(); });

void InitLogs() {
  // TODO: add macro
  absl::SetMinLogLevel(absl::LogSeverityAtLeast::kInfo);
  absl::SetStderrThreshold(absl::LogSeverityAtLeast::kInfinity);
  absl::AddLogSink(ConsoleLogSink::Get());
}

int main(int argc, char** argv) {
  Init(argc, argv);
  InitLogs();

  LOG(INFO) << sekai::db::MasterDb::Get().DebugString();

  frontend::InitializePage();
  InitHost();

  // Load saved data
  EM_ASM(
      FS.mkdir("/idbfs");
      FS.mount(IDBFS, {}, "/idbfs");
      FS.syncfs(true, function (err) {
          assert(!err);
          controller.ReadSaveData();
        });
  );

  return 0;
}
