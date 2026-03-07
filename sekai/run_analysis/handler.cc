#include "sekai/run_analysis/handler.h"

#include "absl/base/no_destructor.h"
#include "absl/container/flat_hash_map.h"
#include "absl/strings/str_cat.h"
#include "sekai/run_analysis/analyze_graph_handler.h"
#include "sekai/run_analysis/analyze_play_handler.h"
#include "sekai/run_analysis/analyze_player_handler.h"
#include "sekai/run_analysis/analyze_team_handler.h"
#include "sekai/run_analysis/batch_handler.h"
#include "sekai/run_analysis/compute_stats_handler.h"
#include "sekai/run_analysis/handlers.h"
#include "sekai/run_analysis/proto/service.pb.h"

namespace sekai::run_analysis {

const absl::NoDestructor<absl::flat_hash_map<std::string, const HandlerBase*>> kHandlers{{
    {"/analyze_graph", new AnalyzeGraphHandler},
    {"/analyze_play", new AnalyzePlayHandler},
    {"/analyze_player", new AnalyzePlayerHandler},
    {"/analyze_team", new AnalyzeTeamHandler},
    {"/batch_analyze_graph",
     new BatchHandler<AnalyzeGraphHandler, BatchAnalyzeGraphRequest, BatchAnalyzeGraphResponse>},
    {"/compute_stats", new ComputeStatsHandler},
}};

std::string HandleRequest(std::string_view path, std::string_view request, bool binary) {
  auto handler = kHandlers->find(path);
  if (handler == kHandlers->end()) {
    std::string output = absl::StrCat("No handler found for path: ", path);
    output.insert(0, 1, static_cast<char>(absl::StatusCode::kNotFound));
    return output;
  }
  return binary ? handler->second->RunBinary(request) : handler->second->Run(request);
}

void LogRegisteredHandlers() {
  LOG(INFO) << "Registered handlers:\n"
            << absl::StrJoin(*kHandlers, "\n", [](std::string* out, const auto& handler) {
                 absl::StrAppend(out, handler.first);
               });
}

}  // namespace sekai::run_analysis
