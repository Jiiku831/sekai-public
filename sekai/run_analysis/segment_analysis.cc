#include "sekai/run_analysis/segment_analysis.h"

#include "sekai/run_analysis/clustering.h"
#include "sekai/run_analysis/snapshot.h"

namespace sekai::run_analysis {

// struct SegmentAnalysisResult {
//   std::vector<Cluster> clusters;
// };
absl::StatusOr<SegmentAnalysisResult> AnalyzeSegment(const Sequence& sequence) {
  SegmentAnalysisResult result;
  result.clusters = FindClusters(sequence | std::views::drop(1));
  if (result.clusters.size() < 2) {
    return absl::InternalError("Expected at least 2 clusters.");
  }
  float mean0 = result.clusters[0].mean;
  float mean1 = result.clusters[1].mean;
  result.cluster_mean_ratio = std::max(mean0, mean1) / std::min(mean0, mean1);
  return result;
}

}  // namespace sekai::run_analysis
