#include "sekai/run_analysis/segmentation.h"

#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "sekai/run_analysis/snapshot.h"

namespace sekai::run_analysis {
namespace {

using ::testing::ElementsAre;
using ::testing::ElementsAreArray;
using ::testing::Field;

TEST(SegmentationTest, SplitAtGap) {
  Sequence seq = {
      .time_offset = absl::UnixEpoch(),
      .points =
          {
              {absl::Seconds(1), 1},
              {absl::Seconds(2), 1},
              {absl::Seconds(3), 1},
              {absl::Seconds(4), 1},
              {absl::Seconds(5), 1},
              // 15s gap
              {absl::Seconds(20), 1},
              {absl::Seconds(21), 1},
              {absl::Seconds(22), 1},
              {absl::Seconds(23), 1},
              {absl::Seconds(28), 1},
              {absl::Seconds(29), 1},
              // 31s gap
              {absl::Seconds(50), 1},
              {absl::Seconds(51), 1},
              {absl::Seconds(52), 1},
              {absl::Seconds(53), 1},
          },
  };
  EXPECT_THAT(
      SplitIntoSegments(seq, /*min_segment_length=*/1, /*max_segment_gap=*/absl::Seconds(10)),
      ElementsAre(Field(&Sequence::points, ElementsAreArray({
                                               Snapshot{absl::Seconds(1), 1},
                                               Snapshot{absl::Seconds(2), 1},
                                               Snapshot{absl::Seconds(3), 1},
                                               Snapshot{absl::Seconds(4), 1},
                                               Snapshot{absl::Seconds(5), 1},
                                           })),
                  Field(&Sequence::points, ElementsAreArray({
                                               Snapshot{absl::Seconds(20), 1},
                                               Snapshot{absl::Seconds(21), 1},
                                               Snapshot{absl::Seconds(22), 1},
                                               Snapshot{absl::Seconds(23), 1},
                                               Snapshot{absl::Seconds(28), 1},
                                               Snapshot{absl::Seconds(29), 1},
                                           })),
                  Field(&Sequence::points, ElementsAreArray({
                                               Snapshot{absl::Seconds(50), 1},
                                               Snapshot{absl::Seconds(51), 1},
                                               Snapshot{absl::Seconds(52), 1},
                                               Snapshot{absl::Seconds(53), 1},
                                           }))));
}

}  // namespace
}  // namespace sekai::run_analysis
