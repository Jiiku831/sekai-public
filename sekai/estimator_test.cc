#include "sekai/estimator.h"

#include <array>

#include <Eigen/Eigen>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "absl/base/nullability.h"
#include "absl/log/log.h"
#include "gmock/gmock.h"
#include "sekai/db/master_db.h"
#include "sekai/db/proto/all.h"

namespace sekai {
namespace {

using ::sekai::db::MasterDb;
using ::testing::Each;
using ::testing::FloatEq;
using ::testing::Gt;
using ::testing::Pointwise;

constexpr int kMaxPower = 400'000;
constexpr int kMaxSkill = 270;
constexpr int kMaxBonus = 750;

std::vector<absl::Nonnull<const db::MusicMeta*>> GetTestSongs() {
  return MasterDb::GetIf<db::MusicMeta>([](const db::MusicMeta& meta) {
    return meta.difficulty() == db::DIFF_MASTER && meta.music_id() <= 3;
  });
}

TEST(EstimatorTest, AutoEpEstimatorParams) {
  std::vector<double> expected = {
      111.66666666666667,
      0.00017507933333333332,
      4.1180107149980073e-05,
      8.377384042506114e-06,
  };
  Eigen::Vector4d actual = Estimator(Estimator::Mode::kAuto, GetTestSongs()).GetEpEstimatorParams();
  EXPECT_THAT(actual, Pointwise(FloatEq(), expected));
}

TEST(EstimatorTest, SoloEpEstimatorParams) {
  std::vector<double> expected = {
      111.66666666666667,
      0.00026255376057510599,
      6.2561463276098066e-05,
      1.3474270916673305e-05,
  };
  Eigen::Vector4d actual = Estimator(Estimator::Mode::kSolo, GetTestSongs()).GetEpEstimatorParams();
  EXPECT_THAT(actual, Pointwise(FloatEq(), expected));
}

TEST(EstimatorTest, MultiEpEstimatorParams) {
  std::vector<double> expected = {
      137.35,
      0.00034683671855194142,
      8.3153773191065584e-05,
      1.5852083431380358e-05,
  };
  Eigen::Vector4d actual =
      Estimator(Estimator::Mode::kMulti, GetTestSongs()).GetEpEstimatorParams();
  EXPECT_THAT(actual, Pointwise(FloatEq(), expected));
}

TEST(EstimatorTest, CheerfulEpEstimatorParams) {
  std::vector<double> expected = {
      185.42250000000001,
      0.00046822957004512094,
      0.00011225759380793855,
      2.1400312632363484e-05,
  };
  Eigen::Vector4d actual =
      Estimator(Estimator::Mode::kCheerful, GetTestSongs()).GetEpEstimatorParams();
  EXPECT_THAT(actual, Pointwise(FloatEq(), expected));
}

TEST(EstimatorTest, EpEstimatorParamsHcmMasMulti) {
  Estimator estimator{
      159.9,
      0.00042125460343927563,
      6.0073288583554635e-05,
      1.585454068017476e-05,
  };
  EXPECT_EQ(1253, static_cast<int>(estimator.ExpectedEp(275000, 300, 180, 180)));
}

TEST(EstimatorTest, LookupTable) {
  Estimator estimator{
      159.9,
      0.00042125460343927563,
      6.0073288583554635e-05,
      1.585454068017476e-05,
  };
  int max_spread = 0;
  for (int power = 0; power <= kMaxPower; power += 500) {
    for (int event_bonus = 0; event_bonus <= kMaxBonus; event_bonus += 10) {
      for (int skill_value = 36; skill_value <= kMaxSkill; skill_value += 10) {
        double actual_ep = estimator.ExpectedEp(power, event_bonus, skill_value, skill_value);
        auto lookup_ep = estimator.LookupEstimatedEp(power, event_bonus);
        max_spread = std::max(max_spread, lookup_ep.upper_bound - lookup_ep.lower_bound);
        EXPECT_LE(lookup_ep.lower_bound, actual_ep);
        EXPECT_LE(actual_ep, lookup_ep.upper_bound);
      }
    }
  }
  EXPECT_LT(max_spread, 1000);
  LOG(INFO) << "Lookup table size: " << estimator.LookupTableSize();
}

TEST(EstimatorTest, LookupTableWithMinMaxSkills) {
  Estimator estimator{
      159.9,
      0.00042125460343927563,
      6.0073288583554635e-05,
      1.585454068017476e-05,
  };
  Estimator estimator2{
      159.9,
      0.00042125460343927563,
      6.0073288583554635e-05,
      1.585454068017476e-05,
  };
  int min_skill = 100;
  int max_skill = 200;
  estimator2.PopulateLookupTable(min_skill, max_skill);
  for (int power = 0; power <= kMaxPower; power += 500) {
    for (int event_bonus = 0; event_bonus <= kMaxBonus; event_bonus += 10) {
      for (int skill_value = min_skill; skill_value <= max_skill; skill_value += 10) {
        double actual_ep = estimator.ExpectedEp(power, event_bonus, skill_value, skill_value);
        auto no_minmax_ep = estimator.LookupEstimatedEp(power, event_bonus);
        auto minmax_ep = estimator2.LookupEstimatedEp(power, event_bonus);
        EXPECT_LE(no_minmax_ep.lower_bound, minmax_ep.lower_bound);
        EXPECT_LE(minmax_ep.lower_bound, actual_ep);
        EXPECT_LE(actual_ep, minmax_ep.upper_bound);
        EXPECT_LE(minmax_ep.upper_bound, no_minmax_ep.upper_bound);
      }
    }
  }
}

TEST(EstimatorTest, MinRequiredPower) {
  double tgt_ep = 1000;
  double max_bonus = 300;
  int max_skill_value = 200;
  Estimator estimator{
      159.9,
      0.00042125460343927563,
      6.0073288583554635e-05,
      1.585454068017476e-05,
  };
  int min_power = estimator.MinRequiredPower(tgt_ep, max_bonus, max_skill_value);
  for (int power = 0; power < min_power; power += 500) {
    for (int event_bonus = 0; event_bonus <= max_bonus; event_bonus += 10) {
      for (int skill_value = 36; skill_value <= max_skill_value; skill_value += 10) {
        ASSERT_LT(estimator.ExpectedEp(power, event_bonus, skill_value, skill_value), tgt_ep);
      }
    }
  }
}

TEST(EstimatorTest, MinRequiredBonus) {
  double tgt_ep = 1000;
  double max_power = 300'000;
  int max_skill_value = 200;
  Estimator estimator{
      159.9,
      0.00042125460343927563,
      6.0073288583554635e-05,
      1.585454068017476e-05,
  };
  int min_bonus = estimator.MinRequiredBonus(tgt_ep, max_power, max_skill_value);
  for (int power = 0; power < max_power; power += 500) {
    for (int event_bonus = 0; event_bonus <= min_bonus; event_bonus += 10) {
      for (int skill_value = 36; skill_value <= max_skill_value; skill_value += 10) {
        ASSERT_LT(estimator.ExpectedEp(power, event_bonus, skill_value, skill_value), tgt_ep);
      }
    }
  }
}

TEST(RandomExEstimatorTest, LoadsSuccessfully) {
  Eigen::Vector4d params = RandomExEstimator(Estimator::Mode::kMulti).GetEpEstimatorParams();
  EXPECT_THAT(params, Each(Gt(0)));
}

}  // namespace
}  // namespace sekai
