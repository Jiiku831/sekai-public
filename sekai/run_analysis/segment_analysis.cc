#include "sekai/run_analysis/segment_analysis.h"

#include <limits>

#include "clustering.h"
#include "sekai/run_analysis/clustering.h"
#include "sekai/run_analysis/snapshot.h"

namespace sekai::run_analysis {

// struct SegmentAnalysisResult {
//   std::vector<Cluster> clusters;
// };
absl::StatusOr<SegmentAnalysisResult> AnalyzeSegment(const Sequence& sequence) {
  SegmentAnalysisResult result;
  result.clusters = FindClusters(sequence | std::views::drop(1));
  result.cluster_mean_ratio = std::numeric_limits<float>::quiet_NaN();
  if (result.clusters.size() < 1) {
    return absl::InternalError("Expected at least 1 clusters.");
  }
  if (result.clusters.size() < 2) {
    return result;
  }
  // Look at the two largest clusters
  if (result.clusters.size() > 2) {
    std::stable_sort(
        result.clusters.begin(), result.clusters.end(),
        [](const Cluster& lhs, const Cluster& rhs) { return lhs.vals.size() > rhs.vals.size(); });
  }
  float mean0 = result.clusters[0].mean;
  float mean1 = result.clusters[1].mean;
  result.cluster_mean_ratio = std::max(mean0, mean1) / std::min(mean0, mean1);
  return result;
}

}  // namespace sekai::run_analysis
