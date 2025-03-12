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
  std::vector<Snapshot> points = {
      {1, 1},
      {2, 1},
      {3, 1},
      {4, 1},
      {5, 1},
      // 15s gap
      {20, 1},
      {21, 1},
      {22, 1},
      {23, 1},
      {28, 1},
      {29, 1},
      // 31s gap
      {50, 1},
      {51, 1},
      {52, 1},
      {53, 1},
  };
  EXPECT_THAT(SplitIntoSegments(points, /*min_segment_length=*/1, /*max_segment_gap=*/10),
              ElementsAre(Field(&Segment::points, ElementsAreArray({
                                                      Snapshot{1, 1},
                                                      Snapshot{2, 1},
                                                      Snapshot{3, 1},
                                                      Snapshot{4, 1},
                                                      Snapshot{5, 1},
                                                  })),
                          Field(&Segment::points, ElementsAreArray({
                                                      Snapshot{20, 1},
                                                      Snapshot{21, 1},
                                                      Snapshot{22, 1},
                                                      Snapshot{23, 1},
                                                      Snapshot{28, 1},
                                                      Snapshot{29, 1},
                                                  })),
                          Field(&Segment::points, ElementsAreArray({
                                                      Snapshot{50, 1},
                                                      Snapshot{51, 1},
                                                      Snapshot{52, 1},
                                                      Snapshot{53, 1},
                                                  }))));
}

}  // namespace
}  // namespace sekai::run_analysis
