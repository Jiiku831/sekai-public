#include "sekai/run_analysis/clustering.h"

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <vector>

#include "absl/log/log.h"
#include "sekai/run_analysis/snapshot.h"
#include "sekai/run_analysis/stats_util.h"

namespace sekai::run_analysis {

namespace {

inline auto GetDiffs() {
  return std::views::transform([](const Snapshot& pt) { return pt.diff; });
}

}  // namespace

std::vector<Cluster> FindClusters(std::span<const Snapshot> pts) {
  constexpr std::size_t kClusters = 2;
  constexpr float kInitialDelta = 0.1;
  // Given the nature of the data, we assign the initial means to the mean plus some delta;
  std::vector<float> means(kClusters, mean<float>(pts | GetDiffs()));
  means[0] *= 1 - kInitialDelta;
  means[1] *= 1 + kInitialDelta;

  // The clusters are obvious, so there is no need to do too many iterations.
  constexpr int kMaxIter = 1000;
  std::vector<int> cluster(pts.size(), 0);
  bool clusters_changed = true;
  for (int i = 0; i < kMaxIter && clusters_changed; ++i) {
    clusters_changed = false;
    std::vector<float> totals(kClusters, 0);
    std::vector<int> counts(kClusters, 0);
    for (std::size_t j = 0; j < pts.size(); ++j) {
      int val = pts[j].diff;
      int assignment = (std::abs(val - means[0]) < std::abs(val - means[1])) ? 0 : 1;
      totals[assignment] += val;
      ++counts[assignment];
      if (cluster[j] != assignment) {
        clusters_changed = true;
        cluster[j] = assignment;
      }
    }
    for (std::size_t j = 0; j < means.size(); ++j) {
      means[j] = totals[j] / counts[j];
    }
  }

  // Make the clusters.
  std::vector<Cluster> results;
  results.reserve(kClusters);
  for (std::size_t i = 0; i < kClusters; ++i) {
    results.push_back({.mean = means[i]});
  }
  for (std::size_t i = 0; i < pts.size(); ++i) {
    results[cluster[i]].vals.push_back(pts[i]);
  }

  Cluster outliers = {
      .mean = Cluster::kOutlierSentinel,
  };
  for (std::size_t i = 0; i < kClusters; ++i) {
    constexpr int kOutlierIterations = 3;
    bool outliers_removed = true;
    for (int j = 0; j < kOutlierIterations && outliers_removed; ++j) {
      std::size_t outlier_init_size = outliers.vals.size();
      constexpr float kMinStdev = 200;
      float cluster_mean = results[i].mean;
      float cluster_stdev =
          std::max(kMinStdev, stdev<float>(results[i].vals | GetDiffs(), cluster_mean));
      std::remove_copy_if(results[i].vals.begin(), results[i].vals.end(),
                          std::back_inserter(outliers.vals), [&](const Snapshot& pt) {
                            constexpr int kRejectionThreshold = 2;
                            return std::abs(pt.diff - cluster_mean) <
                                   kRejectionThreshold * cluster_stdev;
                          });
      // Recompute mean/stdev
      results[i].mean = mean<float>(results[i].vals | GetDiffs());
      results[i].stdev = stdev<float>(results[i].vals | GetDiffs(), results[i].mean);
      outliers_removed = outliers.vals.size() != outlier_init_size;
    }
  }
  results.push_back(std::move(outliers));
  return results;
}

}  // namespace sekai::run_analysis
