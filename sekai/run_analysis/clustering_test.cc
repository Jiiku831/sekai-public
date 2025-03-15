#include "sekai/run_analysis/clustering.h"

#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "sekai/run_analysis/snapshot.h"

namespace sekai::run_analysis {
namespace {

using ::testing::Field;
using ::testing::FloatEq;
using ::testing::UnorderedElementsAre;

TEST(FindClustersTest, FindsTheClusters) {
  std::vector<Snapshot> vals = {
      Snapshot{.diff = 1}, Snapshot{.diff = 9}, Snapshot{.diff = 1}, Snapshot{.diff = 9},
      Snapshot{.diff = 1}, Snapshot{.diff = 9}, Snapshot{.diff = 1}, Snapshot{.diff = 9},
      Snapshot{.diff = 1}, Snapshot{.diff = 9}, Snapshot{.diff = 1}, Snapshot{.diff = 9},
      Snapshot{.diff = 1}, Snapshot{.diff = 9}, Snapshot{.diff = 1}, Snapshot{.diff = 9},
      Snapshot{.diff = 1}, Snapshot{.diff = 9}, Snapshot{.diff = 1}, Snapshot{.diff = 9},
  };

  std::vector<Cluster> clusters = FindClusters(vals);
  EXPECT_THAT(clusters, UnorderedElementsAre(
                            Field(&Cluster::mean, FloatEq(1)), Field(&Cluster::mean, FloatEq(9)),
                            Field(&Cluster::mean, FloatEq(Cluster::kOutlierSentinel))));
}

}  // namespace
}  // namespace sekai::run_analysis
