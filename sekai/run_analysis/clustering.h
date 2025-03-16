#pragma once

#include <span>
#include <vector>

#include "absl/strings/str_format.h"
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

// Run steps generally only have 2 or 3 clusters, so this only looks for one of those.
std::vector<Cluster> FindClusters(std::span<const Snapshot> pts);

}  // namespace sekai::run_analysis
