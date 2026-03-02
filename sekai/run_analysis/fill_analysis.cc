#include "sekai/run_analysis/fill_analysis.h"

#include <boost/math/distributions/normal.hpp>

#include "absl/log/log.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_format.h"
#include "base/util.h"
#include "sekai/config.h"
#include "sekai/db/master_db.h"
#include "sekai/db/proto/music_meta.pb.h"
#include "stats_util.h"

namespace sekai::run_analysis {
namespace {

// Variation in play. Not sure how we can model this.
// Probably can use player point variance?
constexpr float kPlayVariance = 0.3;

struct InvSkillResult {
  double mu;
  double sigma;
  double max_likelihood;
};

float ClampSkillImpl(float skill, float min, float max) { return std::clamp(skill, min, max); }

float ClampSkill(float skill) { return ClampSkillImpl(skill, kMinSkillValue, kMaxSkillValue); }

InvSkillResult InvSkill(const Estimator& estimator, double base_ppg, int power, float event_bonus,
                        float skill_value, float skill_min, float skill_max, float filler_skill_var,
                        bool is_auto) {
  InvSkillResult res;
  res.mu = estimator.ExpectedEpFixedEncoreInvSkill(power, event_bonus, skill_value, base_ppg);
  res.sigma = std::sqrt(estimator.VarianceInvSkill(power, event_bonus, skill_value,
                                                   filler_skill_var, filler_skill_var));

  double filler_skill = is_auto ? ClampSkillImpl(res.mu, skill_min, skill_max) : ClampSkill(res.mu);
  res.max_likelihood = FillAnalyzer::MakePlayDist(estimator, 1, power, event_bonus, skill_value,
                                                  filler_skill, filler_skill_var)
                           .Pdf(base_ppg);
  return res;
}

}  // namespace

const std::array<PlayStrategy, kPlayStrategyCount> kStrategies = {
    PlayStrategy(EBI_EX_MULTI, "Ebi Ex (Multi)", Estimator::Mode::kMulti,
                 db::MasterDb::GetIf<db::MusicMeta>([](const db::MusicMeta& meta) {
                   return meta.music_id() == 74 && meta.difficulty() == db::DIFF_EXPERT;
                 })),
    PlayStrategy(EBI_EX_PUB, "Ebi Ex (Pub)", Estimator::Mode::kMulti,
                 db::MasterDb::GetIf<db::MusicMeta>([](const db::MusicMeta& meta) {
                   return meta.music_id() == 74 && meta.difficulty() == db::DIFF_EXPERT;
                 }),
                 kPubOffset, kPubScale),
    PlayStrategy(EBI_MAS_AUTO, "Ebi Mas (Auto)", Estimator::Mode::kAuto,
                 db::MasterDb::GetIf<db::MusicMeta>([](const db::MusicMeta& meta) {
                   return meta.music_id() == 74 && meta.difficulty() == db::DIFF_MASTER;
                 }),
                 kAutoOffset, kAutoScale, /*is_auto=*/true),
    PlayStrategy(LNF_HARD_MULTI, "LnF Hard (Multi)", Estimator::Mode::kMulti,
                 db::MasterDb::GetIf<db::MusicMeta>([](const db::MusicMeta& meta) {
                   return meta.music_id() == 226 && meta.difficulty() == db::DIFF_HARD;
                 })),
    PlayStrategy(LNF_HARD_PUB, "LnF Hard (Pub)", Estimator::Mode::kMulti,
                 db::MasterDb::GetIf<db::MusicMeta>([](const db::MusicMeta& meta) {
                   return meta.music_id() == 226 && meta.difficulty() == db::DIFF_HARD;
                 }),
                 kPubOffset, kPubScale),
    PlayStrategy(HCM_MAS_AUTO, "HCM Mas (Auto)", Estimator::Mode::kAuto,
                 db::MasterDb::GetIf<db::MusicMeta>([](const db::MusicMeta& meta) {
                   return meta.music_id() == 186 && meta.difficulty() == db::DIFF_MASTER;
                 }),
                 kAutoOffset, kAutoScale, /*is_auto=*/true),
    PlayStrategy(SAGE_MAS_AUTO, "Sage Mas (Auto)", Estimator::Mode::kAuto,
                 db::MasterDb::GetIf<db::MusicMeta>([](const db::MusicMeta& meta) {
                   return meta.music_id() == 448 && meta.difficulty() == db::DIFF_MASTER;
                 }),
                 kAutoOffset, kAutoScale, /*is_auto=*/true),
    PlayStrategy(RANDOM_EX_MULTI, "Random Ex (Multi)", Estimator::Mode::kMulti,
                 db::MasterDb::GetIf<db::MusicMeta>([](const db::MusicMeta& meta) {
                   return meta.difficulty() == db::DIFF_EXPERT;
                 })),
    PlayStrategy(RANDOM_EX_PUB, "Random Ex (Pub)", Estimator::Mode::kMulti,
                 db::MasterDb::GetIf<db::MusicMeta>([](const db::MusicMeta& meta) {
                   return meta.difficulty() == db::DIFF_EXPERT;
                 }),
                 kPubOffset, kPubScale),
};

Distribution<boost::math::normal> FillAnalyzer::MakePlayDist(const Estimator& estimator,
                                                             float multiplier, int power,
                                                             float event_bonus, float player_skill,
                                                             float filler_skill,
                                                             float filler_skill_var) {
  double mu = multiplier * estimator.ExpectedEpFixedEncore(power, event_bonus, player_skill,
                                                           filler_skill, filler_skill);
  double sigma = multiplier *
                 std::sqrt(estimator.Variance(power, event_bonus, player_skill, filler_skill_var,
                                              filler_skill_var)) *
                 (1 + kPlayVariance);
  return Distribution(boost::math::normal(mu, sigma));
}

absl::StatusOr<FillAnalysis> FillAnalyzer::RunFillAnalysisForStrategy(const PlayStrategy& strategy,
                                                                      int boost_usage) const {
  if (boost_usage < 0 || static_cast<std::size_t>(boost_usage) >= kBoostMultipliers.size()) {
    return absl::InvalidArgumentError(
        absl::StrFormat("Boost usage (%d) is out of range.", boost_usage));
  }

  ConfidenceInterval fill_power;
  constexpr float kAlpha = 2;
  const int multiplier = kBoostMultipliers[boost_usage];
  const double base_ppg = input_.observed_ppg / multiplier;

  InvSkillResult max_res =
      InvSkill(strategy.estimator(), base_ppg, input_.power, input_.event_bonus, input_.skill_min,
               input_.skill_min, input_.skill_max, filler_skill_var_, strategy.is_auto());
  InvSkillResult min_res =
      InvSkill(strategy.estimator(), base_ppg, input_.power, input_.event_bonus, input_.skill_max,
               input_.skill_min, input_.skill_max, filler_skill_var_, strategy.is_auto());
  fill_power.set_value(ClampSkill((min_res.mu + max_res.mu) / 2));
  fill_power.set_upper_bound(ClampSkill(max_res.mu + max_res.sigma * kAlpha));
  fill_power.set_lower_bound(ClampSkill(min_res.mu - min_res.sigma * kAlpha));
  fill_power.set_confidence(0.95);

  double time = std::max(3600.0 / input_.observed_gph - strategy.estimator().t_mu(), 0.0);
  auto dist = MakeTimeDist(strategy);
  double tl = dist.Pdf(time) / dist.scale();
  double sl = std::max(max_res.max_likelihood, min_res.max_likelihood);
  double af = strategy.is_auto() && boost_usage == 0 ? 0 : 1;

  return FillAnalysis{
      .fill_power = fill_power,
      .likelihood = tl * sl * af,
  };
}

absl::StatusOr<AnalyzePlayResponse> FillAnalyzer::RunAnalysis() const {
  AnalyzePlayResponse resp;
  std::size_t most_likely_i = 0;
  std::size_t most_likely_j = 0;
  double max_likelihood = -std::numeric_limits<double>::infinity();
  std::size_t most_likely_i_auto = 0;
  std::size_t most_likely_j_auto = 0;
  double max_likelihood_auto = -std::numeric_limits<double>::infinity();
  for (std::size_t i = 0; i < kStrategies.size(); ++i) {
    AnalyzePlayResponse::StrategyDetails& strat_res = *resp.add_play_strategies();
    strat_res.set_strategy(kStrategies[i].api_name());
    for (std::size_t j = 0; j < kBoostMultipliers.size(); ++j) {
      AnalyzePlayResponse::BoostUsageDetails& boost_res = *strat_res.add_boost_usage_details();
      boost_res.set_strategy(kStrategies[i].api_name());
      boost_res.set_boost_usage(j);
      boost_res.set_multiplier(kBoostMultipliers[j]);
      boost_res.set_is_auto(kStrategies[i].is_auto());
      ASSIGN_OR_RETURN(FillAnalysis res, RunFillAnalysisForStrategy(kStrategies[i], j));
      if (res.likelihood > max_likelihood) {
        max_likelihood = res.likelihood;
        most_likely_i = i;
        most_likely_j = j;
      }
      if (boost_res.is_auto() && res.likelihood > max_likelihood_auto) {
        max_likelihood_auto = res.likelihood;
        most_likely_i_auto = i;
        most_likely_j_auto = j;
      }
      *boost_res.mutable_estimated_avg_filler_skill() = res.fill_power;
      boost_res.set_relative_likelihood(res.likelihood);
    }
  }

  if (max_likelihood > 0) {
    for (std::size_t i = 0; i < kStrategies.size(); ++i) {
      for (std::size_t j = 0; j < kBoostMultipliers.size(); ++j) {
        resp.mutable_play_strategies(i)->mutable_boost_usage_details(j)->set_relative_likelihood(
            resp.play_strategies(i).boost_usage_details(j).relative_likelihood() / max_likelihood);
        if (i == most_likely_i && j == most_likely_j) {
          resp.mutable_play_strategies(i)->mutable_boost_usage_details(j)->set_most_likely(true);
        }

        if (kStrategies[i].is_auto()) {
          resp.mutable_play_strategies(i)
              ->mutable_boost_usage_details(j)
              ->set_relative_likelihood_auto(
                  resp.play_strategies(i).boost_usage_details(j).relative_likelihood_auto() /
                  max_likelihood_auto);
          if (i == most_likely_i_auto && j == most_likely_j_auto) {
            resp.mutable_play_strategies(i)->mutable_boost_usage_details(j)->set_most_likely_auto(
                true);
          }
        }
      }
    }
  }
  return resp;
}

}  // namespace sekai::run_analysis
