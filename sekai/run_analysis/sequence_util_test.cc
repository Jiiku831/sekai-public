#include "sekai/run_analysis/sequence_util.h"

#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "sekai/run_analysis/segmentation.h"
#include "sekai/run_analysis/snapshot.h"

namespace sekai::run_analysis {
namespace {

using ::testing::ElementsAre;
using ::testing::ElementsAreArray;
using ::testing::Field;

TEST(AlignSequenceTest, AlignAtInterval) {
  Sequence seq = {
      .time_offset = absl::UnixEpoch(),
      .points =
          {
              {absl::Seconds(1), 1},
              {absl::Seconds(7), 1},
              {absl::Seconds(15), 1},
              {absl::Seconds(34), 1},
              {absl::Seconds(35), 1},
          },
  };
  EXPECT_THAT(AlignSequence(seq, absl::Seconds(10)),
              Field(&Sequence::points, ElementsAreArray({
                                           Snapshot{absl::Seconds(0), 1},
                                           Snapshot{absl::Seconds(10), 1},
                                           Snapshot{absl::Seconds(20), 1},
                                           Snapshot{absl::Seconds(30), 1},
                                           Snapshot{absl::Seconds(40), 1},
                                       })));
}

TEST(InterpolateSequenceTest, InterpolateAtIntervals) {
  Sequence seq = {
      .time_offset = absl::UnixEpoch(),
      .points =
          {
              {absl::Seconds(0), 0},
              {absl::Seconds(20), 20},
              {absl::Seconds(30), 30},
              {absl::Seconds(35), 35},
              {absl::Seconds(45), 45},
          },
  };
  EXPECT_THAT(InterpolateSequence(seq, absl::Seconds(5), /*max_gap=*/absl::Minutes(1)),
              Field(&Sequence::points, ElementsAreArray({
                                           Snapshot{absl::Seconds(0), 0},
                                           Snapshot{absl::Seconds(5), 5},
                                           Snapshot{absl::Seconds(10), 10},
                                           Snapshot{absl::Seconds(15), 15},
                                           Snapshot{absl::Seconds(20), 20},
                                           Snapshot{absl::Seconds(25), 25},
                                           Snapshot{absl::Seconds(30), 30},
                                           Snapshot{absl::Seconds(35), 35},
                                           Snapshot{absl::Seconds(40), 40},
                                           Snapshot{absl::Seconds(45), 45},
                                       })));
}

TEST(InterpolateSequenceTest, SkipLargeGap) {
  Sequence seq = {
      .time_offset = absl::UnixEpoch(),
      .points =
          {
              {absl::Seconds(0), 0},
              {absl::Seconds(30), 30},
              {absl::Seconds(40), 40},
              {absl::Seconds(100), 100},
              {absl::Seconds(120), 120},
          },
  };
  EXPECT_THAT(InterpolateSequence(seq, absl::Seconds(5), /*max_gap=*/absl::Seconds(30)),
              Field(&Sequence::points, ElementsAreArray({
                                           Snapshot{absl::Seconds(0), 0},
                                           Snapshot{absl::Seconds(5), 5},
                                           Snapshot{absl::Seconds(10), 10},
                                           Snapshot{absl::Seconds(15), 15},
                                           Snapshot{absl::Seconds(20), 20},
                                           Snapshot{absl::Seconds(25), 25},
                                           Snapshot{absl::Seconds(30), 30},
                                           Snapshot{absl::Seconds(35), 35},
                                           Snapshot{absl::Seconds(40), 40},
                                           Snapshot{absl::Seconds(100), 100},
                                           Snapshot{absl::Seconds(105), 105},
                                           Snapshot{absl::Seconds(110), 110},
                                           Snapshot{absl::Seconds(115), 115},
                                           Snapshot{absl::Seconds(120), 120},
                                       })));
}

}  // namespace
}  // namespace sekai::run_analysis
