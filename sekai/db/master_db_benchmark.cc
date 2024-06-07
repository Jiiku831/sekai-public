#include <memory>

#include <benchmark/benchmark.h>

#include "absl/log/absl_check.h"
#include "sekai/db/item_db.h"
#include "sekai/db/master_db.h"
#include "sekai/db/proto/all.h"

void BM_MasterDbLoad(benchmark::State& state) {
  for (auto _ : state) {
    auto db = sekai::db::MasterDb::Create();
    ABSL_CHECK_GT(db->Get<sekai::db::Card>().GetAll().size(), 0u);
    ABSL_CHECK_NE(db, nullptr);
  }
}

BENCHMARK(BM_MasterDbLoad)->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();
