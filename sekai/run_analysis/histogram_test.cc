#include "sekai/run_analysis/histogram.h"

#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "histogram.h"
#include "sekai/run_analysis/snapshot.h"

namespace sekai::run_analysis {
namespace {

using ::testing::ElementsAre;
using ::testing::ElementsAreArray;
using ::testing::Field;

TEST(HistogramsTest, ComputeSingleSegment) {
  std::vector<Sequence> seqs = {{
      .time_offset = absl::UnixEpoch(),
      .points =
          {
              Snapshot{absl::Minutes(0), 0},
              Snapshot{absl::Minutes(5), 0},
              Snapshot{absl::Minutes(10), 0},
              Snapshot{absl::Minutes(15), 0},
              Snapshot{absl::Minutes(20), 0},
              Snapshot{absl::Minutes(25), 0},
              Snapshot{absl::Minutes(30), 10},
              Snapshot{absl::Minutes(35), 20},
              Snapshot{absl::Minutes(40), 30},
              Snapshot{absl::Minutes(45), 40},
              Snapshot{absl::Minutes(55), 50},
          },
  }};
  Histograms histograms = ComputeHistograms(seqs, /*smoothing_window=*/3,
                                            /*interval=*/absl::Minutes(5));
  EXPECT_EQ(histograms.smoothing_window, 3);
  EXPECT_THAT(histograms.steps, ElementsAreArray({0, 0, 0, 0, 0, 10, 10, 10, 10, 10}));
  EXPECT_THAT(histograms.speeds, ElementsAreArray({0, 0, 0, 0, 0, 120, 120, 120, 120, 120}));
  EXPECT_THAT(histograms.smoothed_speeds, ElementsAreArray({0, 0, 0, 40, 80, 120, 120, 120}));
}

TEST(HistogramsTest, ComputeTwoSegments) {
  std::vector<Sequence> seqs = {
      {
          .time_offset = absl::UnixEpoch(),
          .points =
              {
                  Snapshot{absl::Minutes(0), 0},
                  Snapshot{absl::Minutes(5), 0},
                  Snapshot{absl::Minutes(10), 0},
                  Snapshot{absl::Minutes(15), 0},
                  Snapshot{absl::Minutes(20), 0},
                  Snapshot{absl::Minutes(25), 0},
              },
      },
      {
          .time_offset = absl::UnixEpoch(),
          .points =
              {
                  Snapshot{absl::Minutes(30), 10},
                  Snapshot{absl::Minutes(35), 20},
                  Snapshot{absl::Minutes(40), 30},
                  Snapshot{absl::Minutes(45), 40},
                  Snapshot{absl::Minutes(50), 50},
                  Snapshot{absl::Minutes(55), 60},
              },
      },
  };
  Histograms histograms = ComputeHistograms(seqs, /*smoothing_window=*/3,
                                            /*interval=*/absl::Minutes(5));
  EXPECT_EQ(histograms.smoothing_window, 3);
  EXPECT_THAT(histograms.steps, ElementsAreArray({0, 0, 0, 0, 0, 10, 10, 10, 10, 10}));
  EXPECT_THAT(histograms.speeds, ElementsAreArray({0, 0, 0, 0, 0, 120, 120, 120, 120, 120}));
  EXPECT_THAT(histograms.smoothed_speeds, ElementsAreArray({0, 0, 0, 120, 120, 120}));
}

}  // namespace
}  // namespace sekai::run_analysis
