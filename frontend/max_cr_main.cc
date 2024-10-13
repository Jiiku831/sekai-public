#include <iostream>
#include <ostream>
#include <string>

#include <emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/console.h>

#include "absl/base/log_severity.h"
#include "absl/log/globals.h"
#include "absl/log/log.h"
#include "absl/log/log_sink_registry.h"
#include "absl/time/time.h"
#include "base/init.h"
#include "frontend/console_log_sink.h"
#include "frontend/controller_base.h"
#include "frontend/init.h"
#include "sekai/db/master_db.h"
#include "sekai/html/max_cr.h"
#include "sekai/max_character_rank.h"

using namespace frontend;
using ::emscripten::base;
using ::emscripten::class_;

EM_JS(void, InitHost, (), { InitPage(); });
EM_JS(void, UpdateTable, (const char* contents), { document.getElementById("output").innerHTML = UTF8ToString(contents);
    });

class OhNyaJiiController : public ControllerBase {
 public:
  OhNyaJiiController() = default;

  void RefreshMaxCr(const std::string& time_spec = "") {
    absl::Time time = absl::Now();
    if (!time_spec.empty()) {
      std::string err;
      if (!absl::ParseTime(absl::RFC3339_sec, absl::StrCat(time_spec, ":00+09:00"), &time, &err)) {
        LOG(ERROR) << "Failed to parse time spec: " << time_spec << ":00+09:00 with error: " << err;
      }
    }

    CTML::Node table = sekai::html::MaxCrTable(sekai::GetMaxCharacterRanks(time));
    UpdateTable(table.ToString().c_str());
  }

 private:
  void OnProfileUpdate() override{};
  void OnSaveDataRead() override { RefreshMaxCr(); };
};

EMSCRIPTEN_BINDINGS(controller) {
  class_<OhNyaJiiController, base<ControllerBase>>("OhNyaJiiController")
      .constructor<>()
      .function("RefreshMaxCr", &OhNyaJiiController::RefreshMaxCr);
}

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
