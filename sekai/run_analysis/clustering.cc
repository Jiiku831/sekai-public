#include "sekai/run_analysis/clustering.h"

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <limits>
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

std::vector<float> InitialMeans(std::span<const int> sorted_vals, int num_clusters) {
  std::vector<float> initial;
  initial.reserve(num_clusters);
  if (sorted_vals.empty()) {
    return std::vector<float>(num_clusters);
  }
  if (num_clusters >= 1) {
    initial.push_back(*sorted_vals.begin());
  }
  if (num_clusters >= 2) {
    initial.push_back(*sorted_vals.rbegin());
  }
  if (num_clusters >= 3) {
    initial.push_back(sorted_vals[sorted_vals.size() / 2]);
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

std::vector<Cluster> AssignClusters(std::span<const Snapshot> pts,
                                    std::span<const int> sorted_diffs, int num_clusters,
                                    ClusterDebug::ClusterSplitDebug* absl_nullable debug) {
  constexpr float kOutlierPercentile = 0.05;
  std::span<const int> truncated_vals = sorted_diffs.subspan(
      int(static_cast<float>(sorted_diffs.size()) * kOutlierPercentile),
      int(static_cast<float>(sorted_diffs.size()) * (1 - 2 * kOutlierPercentile)));
  float min_val = 0;
  float max_val = 0;
  if (!truncated_vals.empty()) {
    min_val = *truncated_vals.begin();
    max_val = *truncated_vals.rbegin();
  }
  std::vector<float> means = InitialMeans(truncated_vals, num_clusters);
  if (debug != nullptr) {
    debug->initial_means = means;
  }

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
      int assignment =
          val < min_val || val > max_val ? -1 : GetClusterAssignment(val, means, num_clusters);
      if (assignment >= 0) {
        totals[assignment] += val;
        ++counts[assignment];
      }
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
    if (cluster[i] >= 0) {
      results[cluster[i]].vals.push_back(pts[i]);
    }
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

std::vector<Cluster> FindBestClusters(std::span<const Snapshot> pts, int max_num_clusters,
                                      float min_size_ratio, ClusterDebug* absl_nullable debug) {
  if (pts.empty()) {
    return std::vector<Cluster>(1);
  }
  // Single cluster model
  auto diffs = RangesTo<std::vector>(pts | GetDiffs());
  std::sort(diffs.begin(), diffs.end());
  float sample_mean = mean<float>(diffs);
  float rss = variance<float>(diffs, sample_mean) * pts.size();
  if (debug != nullptr) {
    debug->split_debug.push_back({
        .rss = rss,
        .min_size = 1,
        .clusters = {{
            .mean = sample_mean,
            .vals = {pts.begin(), pts.end()},
        }},
    });
  }
  constexpr float kRssThreshold = 0.8;

  std::vector<Cluster> candidate;
  constexpr float kMinRss = 100;
  for (int i = 2; i <= max_num_clusters && rss > kMinRss; ++i) {
    if (debug != nullptr) {
      debug->split_debug.push_back({});
    }
    std::vector<Cluster> new_candidate =
        AssignClusters(pts, diffs, i, debug != nullptr ? &debug->split_debug.back() : nullptr);
    float new_rss = RssMetric(new_candidate, pts.size());
    if (debug != nullptr) {
      debug->split_debug.back().rss = new_rss;
      debug->split_debug.back().rss = new_rss;
      debug->split_debug.back().rss_ratio = new_rss / rss;
      debug->split_debug.back().min_size = std::numeric_limits<float>::infinity();
      debug->split_debug.back().clusters = new_candidate;
    }
    if (new_rss / rss >= kRssThreshold) {
      break;
    }
    bool reject = false;
    for (const Cluster& cluster : new_candidate) {
      if (debug != nullptr) {
        debug->split_debug.back().min_size =
            std::min(debug->split_debug.back().min_size,
                     static_cast<float>(cluster.vals.size()) / pts.size());
      }
      if (cluster.vals.size() < min_size_ratio * pts.size()) {
        reject = true;
      }
    }
    if (reject) break;
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

void RecomputeMeanStdev(std::span<Cluster> clusters) {
  for (std::size_t i = 0; i < clusters.size(); ++i) {
    clusters[i].mean = mean<float>(clusters[i].vals | GetDiffs());
    clusters[i].stdev = stdev<float>(clusters[i].vals | GetDiffs(), clusters[i].mean);
  }
}

Cluster RemoveOutliers(std::span<Cluster> clusters, int iterations, float rejection_threshold) {
  Cluster outliers = {
      .mean = Cluster::kOutlierSentinel,
  };
  for (std::size_t i = 0; i < clusters.size(); ++i) {
    bool outliers_removed = true;
    for (int j = 0; j < iterations && outliers_removed; ++j) {
      std::size_t outlier_init_size = outliers.vals.size();
      constexpr float kMinStdev = 2000;
      float cluster_mean = clusters[i].mean;
      float cluster_stdev =
          std::max(kMinStdev, stdev<float>(clusters[i].vals | GetDiffs(), cluster_mean));
      clusters[i].vals.erase(std::remove_if(clusters[i].vals.begin(), clusters[i].vals.end(),
                                            [&](const Snapshot& pt) {
                                              bool is_outlier = std::abs(pt.diff - cluster_mean) >
                                                                rejection_threshold * cluster_stdev;
                                              if (is_outlier) {
                                                // Standard guarantees exactly N applications of
                                                // p.
                                                outliers.vals.push_back(pt);
                                              }
                                              return is_outlier;
                                            }),
                             clusters[i].vals.end());
      RecomputeMeanStdev(clusters);
      outliers_removed = outliers.vals.size() != outlier_init_size;
    }
  }
  return outliers;
}

}  // namespace

std::vector<Cluster> FindClusters(std::span<const Snapshot> pts, ClusterDebug* absl_nullable debug,
                                  float min_size_ratio, int outlier_iterations,
                                  float outlier_rejection_threshold) {
  constexpr std::size_t kMaxClusters = 3;
  std::vector<Cluster> results = FindBestClusters(pts, kMaxClusters, min_size_ratio, debug);
  RecomputeMeanStdev(results);

  results.push_back(
      RemoveOutliers(std::span(results), outlier_iterations, outlier_rejection_threshold));
  return results;
}

}  // namespace sekai::run_analysis
