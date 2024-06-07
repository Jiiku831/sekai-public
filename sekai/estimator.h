#pragma once

#include <cstddef>
#include <span>

#include <Eigen/Eigen>

#include "absl/base/nullability.h"
#include "sekai/config.h"
#include "sekai/db/proto/music_meta.pb.h"

namespace sekai {

struct LookupTableEntry {
  int lower_bound = 0;
  int upper_bound = 0;
};

class Estimator {
 public:
  enum class Mode { kSolo, kMulti, kCheerful, kAuto };

  Estimator(Mode mode, std::span<absl::Nonnull<const db::MusicMeta* const>> songs);
  Estimator(double a, double b, double c, double d) : a_(a), b_(b), c_(c), d_(d) {
    PopulateLookupTable();
  }

  double ExpectedEp(int power, double event_bonus, int average_skill, int encore_skill) const;
  int MinRequiredPower(double ep, double max_event_bonus = kMaxEventBonus,
                       int max_skill = kMaxSkillValue) const;
  int MinRequiredBonus(double ep, int max_power, int max_skill = kMaxSkillValue) const;

  Eigen::Vector4d GetEpEstimatorParams() const;

  void PopulateLookupTable(int min_skill = kMinSkillValue, int max_skill = kMaxSkillValue);
  LookupTableEntry LookupEstimatedEp(int power, int event_bonus) const;
  std::size_t LookupTableSize() const {
    return ep_lookup_table_.size() * ep_lookup_table_[0].size();
  }

 private:
  // EP estimator params
  double a_ = 0;
  double b_ = 0;
  double c_ = 0;
  double d_ = 0;

  static constexpr int kPowerBucketSize = 12;
  static constexpr int kPowerBuckets = (kMaxPower >> kPowerBucketSize) + 1;

  static constexpr int kEventBonusBucketSize = 4;
  static constexpr int kEventBonusBuckets = (kMaxEventBonus >> kEventBonusBucketSize) + 1;

  // Index: (Power, Event Bonus)
  std::array<std::array<LookupTableEntry, kEventBonusBuckets>, kPowerBuckets> ep_lookup_table_{};
};

Estimator RandomExEstimator(Estimator::Mode mode);

}  // namespace sekai
