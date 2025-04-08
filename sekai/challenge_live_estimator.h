#pragma once

#include <vector>

#include "sekai/estimator_base.h"

namespace sekai {

class ChallengeLiveSongEstimator {
 public:
  ChallengeLiveSongEstimator(const db::MusicMeta* meta);

  double ExpectedValue(int power, std::span<const int> sorted_skills) const;

  const db::MusicMeta* meta() const { return meta_; }

 private:
  const db::MusicMeta* meta_;
  std::vector<float> sorted_skill_factor_;
  float base_factor_;
};

class ChallengeLiveEstimator : public EstimatorBase {
 public:
  ChallengeLiveEstimator();
  ChallengeLiveEstimator(std::span<absl::Nonnull<const db::MusicMeta* const>> songs);
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
  std::vector<ChallengeLiveSongEstimator> metas_;

  struct ValueDetail {
    double value = 0;
    const db::MusicMeta* meta = nullptr;
  };
  ValueDetail ExpectedValueImpl(const Profile& profile, const EventBonus& event_bonus,
                                const Team& team) const;
};

const EstimatorBase& SoloEbiMasEstimator();

}  // namespace sekai
