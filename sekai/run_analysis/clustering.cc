#include "sekai/run_analysis/clustering.h"

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <vector>

#include "absl/log/log.h"
#include "sekai/ranges_util.h"
#include "sekai/run_analysis/snapshot.h"
#include "sekai/run_analysis/stats_util.h"

namespace sekai::run_analysis {

namespace {

inline auto GetDiffs() {
  return std::views::transform([](const Snapshot& pt) { return pt.diff; });
}

std::vector<float> InitialMeans(std::span<const int> vals, int num_clusters) {
  // Given the nature of the data, we assign the initial means by equally dividing the range of the
  // values into (num_clusters + 1) parts;
  auto [min, max] = std::minmax_element(vals.begin(), vals.end());
  // Add a little bias to the top and bottom elements to ensure that they are different and the
  // result is stable.
  constexpr float kBias = 0.05;
  float step = static_cast<float>(*max - *min) / (num_clusters + 1);
  std::vector<float> initial;
  initial.reserve(num_clusters);
  for (int i = 1; i <= num_clusters; ++i) {
    float val = *min + i * step;
    if (i == 1) {
      val *= 1 - kBias;
    } else if (i == num_clusters) {
      val *= 1 + kBias;
    }
    initial.push_back(val);
  }
  return initial;
}

int GetClusterAssignment(const float pt, std::span<const float> means, int num_clusters) {
  int cluster = 0;
  float dist = std::abs(means[0] - pt);
  for (int i = 1; i < num_clusters; ++i) {
    float new_dist = std::abs(means[i] - pt);
    if (new_dist < dist) {
      dist = new_dist;
      cluster = i;
    }
  }
  return cluster;
}

std::vector<Cluster> AssignClusters(std::span<const Snapshot> pts, int num_clusters) {
  std::vector<float> means =
      InitialMeans(RangesTo<std::vector<int>>(pts | GetDiffs()), num_clusters);

  // EM step. Clusters should be obvious, so no need to do too many iterations.
  constexpr int kMaxIter = 1000;
  std::vector<int> cluster(pts.size(), 0);
  bool clusters_changed = true;
  for (int i = 0; i < kMaxIter && clusters_changed; ++i) {
    clusters_changed = false;
    std::vector<float> totals(num_clusters, 0);
    std::vector<int> counts(num_clusters, 0);
    for (std::size_t j = 0; j < pts.size(); ++j) {
      auto val = pts[j].diff;
      int assignment = GetClusterAssignment(val, means, num_clusters);
      totals[assignment] += val;
      ++counts[assignment];
      if (cluster[j] != assignment) {
        clusters_changed = true;
        cluster[j] = assignment;
      }
    }
    for (int j = 0; j < num_clusters; ++j) {
      means[j] = totals[j] / counts[j];
    }
  }

  std::vector<Cluster> results;
  results.reserve(num_clusters);
  for (int i = 0; i < num_clusters; ++i) {
    results.push_back({.mean = means[i]});
  }
  for (std::size_t i = 0; i < pts.size(); ++i) {
    results[cluster[i]].vals.push_back(pts[i]);
  }
  return results;
}

float RssMetric(std::span<const Cluster> clusters, std::size_t total_size) {
  float rss = 0;
  for (std::size_t i = 0; i < clusters.size(); ++i) {
    auto dev = stdev<float>(clusters[i].vals | GetDiffs(), clusters[i].mean);
    rss += dev / total_size * clusters[i].vals.size() * dev;
  }
  return rss;
}

std::vector<Cluster> FindBestClusters(std::span<const Snapshot> pts, int max_num_clusters) {
  if (pts.empty()) {
    return std::vector<Cluster>(1);
  }
  // Single cluster model
  auto diffs = pts | GetDiffs();
  float sample_mean = mean<float>(diffs);
  float rss = variance<float>(diffs, sample_mean) * pts.size();
  constexpr float kRssThreshold = 0.8;

  std::vector<Cluster> candidate;
  constexpr float kMinRss = 100;
  for (int i = 2; i <= max_num_clusters && rss > kMinRss; ++i) {
    std::vector<Cluster> new_candidate = AssignClusters(pts, i);
    float new_rss = RssMetric(new_candidate, pts.size());
    if (new_rss / rss >= kRssThreshold) {
      break;
    }
    for (const Cluster& cluster : new_candidate) {
      constexpr int kMinClusterSize = 3;
      if (cluster.vals.size() < kMinClusterSize) {
        break;
      }
    }
    rss = new_rss;
    candidate = std::move(new_candidate);
  }

  if (candidate.size() == 0) {
    candidate.push_back(Cluster{
        .mean = sample_mean,
        .vals = {pts.begin(), pts.end()},
    });
  }
  return candidate;
}

Cluster RemoveOutliers(std::span<Cluster> clusters) {
  Cluster outliers = {
      .mean = Cluster::kOutlierSentinel,
  };
  for (std::size_t i = 0; i < clusters.size(); ++i) {
    constexpr int kOutlierIterations = 3;
    bool outliers_removed = true;
    for (int j = 0; j < kOutlierIterations && outliers_removed; ++j) {
      std::size_t outlier_init_size = outliers.vals.size();
      constexpr float kMinStdev = 2000;
      float cluster_mean = clusters[i].mean;
      float cluster_stdev =
          std::max(kMinStdev, stdev<float>(clusters[i].vals | GetDiffs(), cluster_mean));
      clusters[i].vals.erase(std::remove_if(clusters[i].vals.begin(), clusters[i].vals.end(),
                                            [&](const Snapshot& pt) {
                                              constexpr float kRejectionThreshold = 2;
                                              bool is_outlier = std::abs(pt.diff - cluster_mean) >
                                                                kRejectionThreshold * cluster_stdev;
                                              if (is_outlier) {
                                                // Standard guarantees exactly N applications of p.
                                                outliers.vals.push_back(pt);
                                              }
                                              return is_outlier;
                                            }),
                             clusters[i].vals.end());
      // Recompute mean/stdev
      clusters[i].mean = mean<float>(clusters[i].vals | GetDiffs());
      clusters[i].stdev = stdev<float>(clusters[i].vals | GetDiffs(), clusters[i].mean);
      outliers_removed = outliers.vals.size() != outlier_init_size;
    }
  }
  return outliers;
}

}  // namespace

std::vector<Cluster> FindClusters(std::span<const Snapshot> pts) {
  constexpr std::size_t kMaxClusters = 3;
  std::vector<Cluster> results = FindBestClusters(pts, kMaxClusters);

  results.push_back(RemoveOutliers(std::span(results)));
  return results;
}

}  // namespace sekai::run_analysis
