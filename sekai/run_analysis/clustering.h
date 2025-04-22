#pragma once

#include <limits>
#include <span>
#include <vector>

#include "absl/base/nullability.h"
#include "absl/strings/str_format.h"
#include "sekai/run_analysis/config.h"
#include "sekai/run_analysis/snapshot.h"

namespace sekai::run_analysis {

struct Cluster {
  static constexpr float kOutlierSentinel = -1;
  float mean = 0;
  float stdev = 0;
  float rss = 0;
  std::vector<Snapshot> vals;

  template <typename Sink>
  friend void AbslStringify(Sink& sink, const Cluster& cluster) {
    absl::Format(&sink, "Cluster{%f, %f}:\n%s", cluster.mean, cluster.stdev,
                 absl::StrJoin(cluster.vals, "\n"));
  }
};

struct ClusterDebug {
  struct ClusterSplitDebug {
    float rss = std::numeric_limits<float>::quiet_NaN();
    float rss_ratio = std::numeric_limits<float>::quiet_NaN();
    float min_size = std::numeric_limits<float>::quiet_NaN();
    std::vector<float> initial_means;
    std::vector<Cluster> clusters;
  };
  std::vector<ClusterSplitDebug> split_debug;
};

// Run steps generally only have 2 or 3 clusters, so this only looks for one of those.
std::vector<Cluster> FindClusters(
    std::span<const Snapshot> pts, absl::Nullable<ClusterDebug*> debug = nullptr,
    float min_size_ratio = kMinClusterSizeRatio,
    int outlier_iterations = kClusteringOutlierIterations,
    float outlier_rejection_threshold = kClusteringOutlierRejectionThresh);

}  // namespace sekai::run_analysis
