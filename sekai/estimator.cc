#include "sekai/estimator.h"

#include <span>
#include <string_view>

#include <Eigen/Eigen>

#include "absl/base/nullability.h"
#include "absl/log/absl_check.h"
#include "sekai/db/master_db.h"
#include "sekai/db/proto/music_meta.pb.h"
#include "sekai/event_bonus.h"
#include "sekai/profile.h"
#include "sekai/team.h"

namespace sekai {
namespace {

using ::sekai::db::MasterDb;

constexpr int kNumSkills = 6;

int GetEpFactor(Estimator::Mode mode) {
  switch (mode) {
    case Estimator::Mode::kSolo:
      return 2000000;
    case Estimator::Mode::kMulti:
      return 1700000;
    case Estimator::Mode::kCheerful:
      return 1700000;
    case Estimator::Mode::kAuto:
      return 2000000;
  }
  ABSL_CHECK(false) << "unhandled case";
  return 0;
}

double GetScoreFactor(Estimator::Mode mode) {
  switch (mode) {
    case Estimator::Mode::kSolo:
      return 1.0;
    case Estimator::Mode::kMulti:
      return 1.23;
    case Estimator::Mode::kCheerful:
      return 1.23;
    case Estimator::Mode::kAuto:
      return 1.0;
    default:
      ABSL_CHECK(false) << "unhandled case";
      return 0;
  }
}

double GetLifeFactor(Estimator::Mode mode) {
  switch (mode) {
    case Estimator::Mode::kSolo:
      return 1.0;
    case Estimator::Mode::kMulti:
      return 1.0;
    case Estimator::Mode::kCheerful:
      return 1.35;
    case Estimator::Mode::kAuto:
      return 1.0;
    default:
      ABSL_CHECK(false) << "unhandled case";
      return 0;
  }
}

double GetBaseScore(Estimator::Mode mode, const db::MusicMeta& meta) {
  switch (mode) {
    case Estimator::Mode::kSolo:
      return meta.base_score();
    case Estimator::Mode::kMulti:
    case Estimator::Mode::kCheerful:
      return meta.base_score() + meta.fever_score();
    case Estimator::Mode::kAuto:
      return meta.base_score_auto();
    default:
      ABSL_CHECK(false) << "unhandled case";
      return 0;
  }
}

struct SkillScores {
  double skill_score = 0;
  double encore_score = 0;
};

SkillScores GetSkillScores(Estimator::Mode mode, const db::MusicMeta& meta) {
  SkillScores out;
  ABSL_CHECK_EQ(meta.skill_score_solo_size(), kNumSkills);
  ABSL_CHECK_EQ(meta.skill_score_multi_size(), kNumSkills);
  ABSL_CHECK_EQ(meta.skill_score_auto_size(), kNumSkills);
  for (int i = 0; i < kNumSkills; ++i) {
    double& dest = i == (kNumSkills - 1) ? out.encore_score : out.skill_score;
    switch (mode) {
      case Estimator::Mode::kSolo:
        dest += meta.skill_score_solo(i);
        break;
      case Estimator::Mode::kMulti:
      case Estimator::Mode::kCheerful:
        dest += meta.skill_score_multi(i);
        break;
      case Estimator::Mode::kAuto:
        dest += meta.skill_score_auto(i);
        break;
      default:
        ABSL_CHECK(false) << "unhandled case";
    }
  }
  return out;
}

double Mean(std::span<const double> data) {
  Eigen::Map<const Eigen::VectorXd> vector{data.data(), static_cast<int32_t>(data.size())};
  return vector.mean();
}

Estimator MakeEstimator(Estimator::Mode mode,
                        std::span<absl::Nonnull<const db::MusicMeta* const>> metas) {
  std::vector<double> event_rate;
  std::vector<double> event_rate_base;
  std::vector<double> event_rate_skill;
  std::vector<double> event_rate_encore;
  event_rate.reserve(metas.size());
  event_rate_base.reserve(metas.size());
  event_rate_skill.reserve(metas.size());
  event_rate_encore.reserve(metas.size());

  for (absl::Nonnull<const db::MusicMeta*> meta : metas) {
    SkillScores skill_scores = GetSkillScores(mode, *meta);
    double base_score = GetBaseScore(mode, *meta);
    event_rate.push_back(meta->event_rate());
    event_rate_base.push_back(meta->event_rate() * base_score);
    event_rate_skill.push_back(meta->event_rate() * skill_scores.skill_score);
    event_rate_encore.push_back(meta->event_rate() * skill_scores.encore_score);
  }

  double mean_event_rate = Mean(event_rate);
  double mean_event_rate_base = Mean(event_rate_base);
  double mean_event_rate_skill = Mean(event_rate_skill);
  double mean_event_rate_encore = Mean(event_rate_encore);

  int ep_factor = GetEpFactor(mode);
  double score_factor = GetScoreFactor(mode);
  double life_factor = GetLifeFactor(mode);

  double a = score_factor * life_factor * mean_event_rate;
  double b = 4 * life_factor * mean_event_rate_base / ep_factor;
  double c = 4 * life_factor * mean_event_rate_skill / ep_factor;
  double d = 4 * life_factor * mean_event_rate_encore / ep_factor;

  return Estimator(a, b, c, d);
}

}  // namespace

Estimator::Estimator(Mode mode, std::span<absl::Nonnull<const db::MusicMeta* const>> metas)
    : Estimator(MakeEstimator(mode, metas)) {}

double Estimator::ExpectedEp(int power, double event_bonus, int average_skill,
                             int encore_skill) const {
  // TODO: make option
  average_skill = 180 + static_cast<float>(average_skill - 180) / 5;
  encore_skill = 180 + static_cast<float>(encore_skill - 180) / 5;
  const double& a = a_;
  const double& b = b_;
  const double& c = c_;
  const double& d = d_;
  const double p = static_cast<double>(power);
  const double q = 1 + static_cast<double>(event_bonus) / 100;
  const double f = static_cast<double>(average_skill) / 100;
  const double e = static_cast<double>(encore_skill) / 100;
  return a * q + p * q * (b + f * c + e * d);
}

int Estimator::MinRequiredPower(double ep, double max_event_bonus, int max_skill) const {
  max_skill = 180 + static_cast<float>(max_skill - 180) / 5;
  const double& a = a_;
  const double& b = b_;
  const double& c = c_;
  const double& d = d_;
  const double q = 1 + static_cast<double>(max_event_bonus) / 100;
  const double f = static_cast<double>(max_skill) / 100;
  const double e = static_cast<double>(max_skill) / 100;
  const double x = ep;
  // x = a * q + p * q * (b + f * c + e * d);
  // x - a * q = p * q * (b + f * c + e * d);
  // (x - a * q) / (q * (b + f * c + e * d)) = p;
  return static_cast<int>((x - a * q) / (q * (b + f * c + e * d)));
}

int Estimator::MinRequiredBonus(double ep, int max_power, int max_skill) const {
  max_skill = 180 + static_cast<float>(max_skill - 180) / 5;
  const double& a = a_;
  const double& b = b_;
  const double& c = c_;
  const double& d = d_;
  const double p = static_cast<double>(max_power);
  const double f = static_cast<double>(max_skill) / 100;
  const double e = static_cast<double>(max_skill) / 100;
  const double x = ep;
  // x = a * q + p * q * (b + f * c + e * d);
  // x = q * (a + p * (b + f * c + e * d));
  // x / (a + p * (b + f * c + e * d)) = q;
  return static_cast<int>(((x / (a + p * (b + f * c + e * d))) - 1) * 100);
}

Eigen::Vector4d Estimator::GetEpEstimatorParams() const { return Eigen::Vector4d{a_, b_, c_, d_}; }

void Estimator::PopulateLookupTable(int min_skill, int max_skill) {
  ABSL_CHECK_LE(min_skill, max_skill);
  for (int i = 0; i < kPowerBuckets; ++i) {
    for (int j = 0; j < kEventBonusBuckets; ++j) {
      int power = i << kPowerBucketSize;
      int event_bonus = j << kEventBonusBucketSize;
      ep_lookup_table_[i][j].lower_bound =
          static_cast<int>(ExpectedEp(power, event_bonus, min_skill, min_skill));

      power = (i + 1) << kPowerBucketSize;
      event_bonus = (j + 1) << kEventBonusBucketSize;
      ep_lookup_table_[i][j].upper_bound =
          static_cast<int>(ExpectedEp(power, event_bonus, max_skill, max_skill)) + 1;
    }
  }
}

LookupTableEntry Estimator::LookupEstimatedEp(int power, int event_bonus) const {
  return ep_lookup_table_[power >> kPowerBucketSize][event_bonus >> kEventBonusBucketSize];
}

double Estimator::ExpectedValue(const Profile& profile, const EventBonus& event_bonus,
                                const Team& team) const {
  int power = team.Power(profile);
  float eb = team.EventBonus(event_bonus);
  int skill_value = team.SkillValue();
  return ExpectedEp(power, eb, skill_value, skill_value);
}

double Estimator::MaxExpectedValue(const Profile& profile, const EventBonus& event_bonus,
                                   const Team& team, Character lead_chars) const {
  int power = team.Power(profile);
  float eb = team.EventBonus(event_bonus);
  int skill_value = team.ConstrainedMaxSkillValue(lead_chars).skill_value;
  return ExpectedEp(power, eb, skill_value, skill_value);
}

void Estimator::AnnotateTeamProto(const Profile& profile, const EventBonus& event_bonus,
                                  const Team& team, TeamProto& team_proto) const {
  team_proto.set_expected_ep(ExpectedValue(profile, event_bonus, team));
}

Estimator RandomExEstimator(Estimator::Mode mode) {
  std::vector<absl::Nonnull<const db::MusicMeta*>> metas = MasterDb::GetIf<db::MusicMeta>(
      [](const db::MusicMeta& meta) { return meta.difficulty() == db::DIFF_EXPERT; });
  return Estimator{mode, metas};
}

}  // namespace sekai
