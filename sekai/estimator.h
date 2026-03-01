#pragma once

#include <cstddef>
#include <span>

#include <Eigen/Eigen>

#include "absl/base/nullability.h"
#include "sekai/config.h"
#include "sekai/db/proto/music_meta.pb.h"
#include "sekai/estimator_base.h"
#include "sekai/event_bonus.h"
#include "sekai/profile.h"

namespace sekai {

struct LookupTableEntry {
  int lower_bound = 0;
  int upper_bound = 0;
};

class Estimator : public EstimatorBase {
 public:
  enum class Mode { kSolo, kMulti, kCheerful, kAuto };

  Estimator(Mode mode, std::span<const db::MusicMeta* const absl_nonnull> songs);
  Estimator(double a, double b, double c, double d) : a_(a), b_(b), c_(c), d_(d) {
    PopulateLookupTable();
  }

  double ExpectedEp(int power, double event_bonus, float player_skill, float encore_skill,
                    float third_party_skill = 180) const;
  double ExpectedEpFixedEncore(int power, double event_bonus, float player_skill,
                               float encore_skill, float third_party_skill = 180) const;
  double ExpectedEpFixedEncoreInvSkill(int power, double event_bonus, float player_skill,
                                       double expected_ep) const;
  // Assuming skill values are drawn independently.
  double Variance(int fixed_power, double fixed_event_bonus, float fixed_player_skill,
                  float encore_skill_var, float third_party_skill_var) const;
  double VarianceInvSkill(int fixed_power, double fixed_event_bonus, float fixed_player_skill,
                          float encore_skill_var, float third_party_skill_var) const;
  int MinRequiredPower(double ep, double max_event_bonus = kMaxEventBonus,
                       float max_skill = kMaxSkillValue, float third_party_skill = 180) const;
  int MinRequiredBonus(double ep, int max_power, float max_skill = kMaxSkillValue,
                       float third_party_skill = 180) const;

  Eigen::Vector4d GetEpEstimatorParams() const;

  void PopulateLookupTable(int min_skill = kMinSkillValue, int max_skill = kMaxSkillValue);
  LookupTableEntry LookupEstimatedEp(int power, int event_bonus) const;
  std::size_t LookupTableSize() const {
    return ep_lookup_table_.size() * ep_lookup_table_[0].size();
  }

  double ExpectedValue(const Profile& profile, const EventBonus& event_bonus,
                       const Team& team) const override;
  double MaxExpectedValue(const Profile& profile, const EventBonus& event_bonus, const Team& team,
                          Character lead_chars) const override;
  double SmoothOptimizationObjective(const Profile& profile, const EventBonus& event_bonus,
                                     const Team& team, Character lead_chars) const override {
    return MaxExpectedValue(profile, event_bonus, team, lead_chars);
  }

  void AnnotateTeamProto(const Profile& profile, const EventBonus& event_bonus, const Team& team,
                         TeamProto& team_proto) const override;

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

class MySakiEstimator : public EstimatorBase {
 public:
  double ExpectedEp(int power, double event_bonus) const;
  double SmoothExpectedEp(int power, double event_bonus) const;
  double ExpectedValue(const Profile& profile, const EventBonus& event_bonus,
                       const Team& team) const override;
  double MaxExpectedValue(const Profile& profile, const EventBonus& event_bonus, const Team& team,
                          Character lead_chars) const override {
    return ExpectedValue(profile, event_bonus, team);
  }
  double SmoothOptimizationObjective(const Profile& profile, const EventBonus& event_bonus,
                                     const Team& team, Character lead_chars) const override;

  void AnnotateTeamProto(const Profile& profile, const EventBonus& event_bonus, const Team& team,
                         TeamProto& team_proto) const override;
};

Estimator RandomExEstimator(Estimator::Mode mode);

}  // namespace sekai
