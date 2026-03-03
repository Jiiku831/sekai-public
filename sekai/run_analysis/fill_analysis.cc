#include "sekai/run_analysis/fill_analysis.h"

#include <queue>

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

constexpr float kInclusionThreshold = 0.5;
constexpr float kAutoInclusionThreshold = 0.1;

struct InvSkillResult {
  double mu;
  double sigma;
  double max_likelihood;
};

float ClampSkillImpl(float skill, float min, float max) { return std::clamp(skill, min, max); }

float SelectSkill(bool is_auto, float card_skill, float skill_value) {
  return is_auto ? card_skill : skill_value;
}

float SelectSkillMax(bool is_auto, float card_skill) {
  return SelectSkill(is_auto, card_skill, kMaxSkillValue);
}

float SelectSkillMin(bool is_auto, float card_skill) {
  return SelectSkill(is_auto, card_skill, kMinSkillValue);
}

float ClampSkill(bool is_auto, float skill, float card_min, float card_max) {
  return ClampSkillImpl(skill, SelectSkillMin(is_auto, card_min),
                        SelectSkillMax(is_auto, card_max));
}

InvSkillResult InvSkill(const Estimator& estimator, double base_ppg, int power, float event_bonus,
                        float skill_value, float skill_min, float skill_max, float filler_skill_var,
                        bool is_auto) {
  InvSkillResult res;
  res.mu = estimator.ExpectedEpFixedEncoreInvSkill(power, event_bonus, skill_value, base_ppg);
  res.sigma = std::sqrt(estimator.VarianceInvSkill(power, event_bonus, skill_value,
                                                   filler_skill_var, filler_skill_var));

  double filler_skill = ClampSkill(is_auto, res.mu, skill_min, skill_max);
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
                 kPubOffset, kPubScale, /*is_pub=*/true),
    PlayStrategy(EBI_MAS_AUTO, "Ebi Mas (Auto)", Estimator::Mode::kAuto,
                 db::MasterDb::GetIf<db::MusicMeta>([](const db::MusicMeta& meta) {
                   return meta.music_id() == 74 && meta.difficulty() == db::DIFF_MASTER;
                 }),
                 kAutoOffset, kAutoScale, /*is_pub=*/false, /*is_auto=*/true),
    PlayStrategy(LNF_HARD_MULTI, "LnF Hard (Multi)", Estimator::Mode::kMulti,
                 db::MasterDb::GetIf<db::MusicMeta>([](const db::MusicMeta& meta) {
                   return meta.music_id() == 226 && meta.difficulty() == db::DIFF_HARD;
                 })),
    PlayStrategy(LNF_HARD_PUB, "LnF Hard (Pub)", Estimator::Mode::kMulti,
                 db::MasterDb::GetIf<db::MusicMeta>([](const db::MusicMeta& meta) {
                   return meta.music_id() == 226 && meta.difficulty() == db::DIFF_HARD;
                 }),
                 kPubOffset, kPubScale, /*is_pub=*/true),
    PlayStrategy(HCM_MAS_AUTO, "HCM Mas (Auto)", Estimator::Mode::kAuto,
                 db::MasterDb::GetIf<db::MusicMeta>([](const db::MusicMeta& meta) {
                   return meta.music_id() == 186 && meta.difficulty() == db::DIFF_MASTER;
                 }),
                 kAutoOffset, kAutoScale, /*is_pub=*/false, /*is_auto=*/true),
    PlayStrategy(SAGE_MAS_AUTO, "Sage Mas (Auto)", Estimator::Mode::kAuto,
                 db::MasterDb::GetIf<db::MusicMeta>([](const db::MusicMeta& meta) {
                   return meta.music_id() == 448 && meta.difficulty() == db::DIFF_MASTER;
                 }),
                 kAutoOffset, kAutoScale, /*is_pub=*/false, /*is_auto=*/true),
    PlayStrategy(RANDOM_EX_MULTI, "Random Ex (Multi)", Estimator::Mode::kMulti,
                 db::MasterDb::GetIf<db::MusicMeta>([](const db::MusicMeta& meta) {
                   return meta.difficulty() == db::DIFF_EXPERT;
                 })),
    PlayStrategy(RANDOM_EX_PUB, "Random Ex (Pub)", Estimator::Mode::kMulti,
                 db::MasterDb::GetIf<db::MusicMeta>([](const db::MusicMeta& meta) {
                   return meta.difficulty() == db::DIFF_EXPERT;
                 }),
                 kPubOffset, kPubScale, /*is_pub=*/true),
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
  const bool is_auto = strategy.is_auto();
  const double actual_skill_min = is_auto ? input_.card_skill_min : input_.skill_min;
  const double actual_skill_max = is_auto ? input_.card_skill_max : input_.skill_max;

  InvSkillResult max_res =
      InvSkill(strategy.estimator(), base_ppg, input_.power, input_.event_bonus, actual_skill_min,
               actual_skill_min, actual_skill_max, filler_skill_var_, is_auto);
  InvSkillResult min_res =
      InvSkill(strategy.estimator(), base_ppg, input_.power, input_.event_bonus, actual_skill_max,
               actual_skill_min, actual_skill_max, filler_skill_var_, is_auto);
  fill_power.set_value(ClampSkill(is_auto, (min_res.mu + max_res.mu) / 2, input_.card_skill_min,
                                  input_.card_skill_max));
  fill_power.set_upper_bound(ClampSkill(is_auto, max_res.mu + max_res.sigma * kAlpha,
                                        input_.card_skill_min, input_.card_skill_max));
  fill_power.set_lower_bound(ClampSkill(is_auto, min_res.mu - min_res.sigma * kAlpha,
                                        input_.card_skill_min, input_.card_skill_max));
  fill_power.set_confidence(0.95);

  double time = std::max(3600.0 / input_.observed_gph - strategy.estimator().t_mu(), 0.0);
  auto dist = MakeTimeDist(strategy);
  double tl = dist.Pdf(time) / dist.scale();
  double sl = std::max(max_res.max_likelihood, min_res.max_likelihood);
  double af = is_auto && boost_usage == 0 ? 0 : 1;
  double sf = MakeSkillDist(strategy, fill_power.value()).Pdf(fill_power.value());

  return FillAnalysis{
      .fill_power = fill_power,
      .likelihood = tl * sl * af * sf,
  };
}

absl::StatusOr<AnalyzePlayResponse> FillAnalyzer::RunAnalysis() const {
  AnalyzePlayResponse resp;
  std::priority_queue<std::tuple<double, std::size_t, std::size_t>> strategies;
  std::priority_queue<std::tuple<double, std::size_t, std::size_t>> auto_strategies;
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
      strategies.emplace(res.likelihood, i, j);
      if (boost_res.is_auto()) {
        auto_strategies.emplace(res.likelihood, i, j);
      }
      *boost_res.mutable_estimated_avg_filler_skill() = res.fill_power;
      boost_res.set_relative_likelihood(res.likelihood);
      boost_res.set_relative_likelihood_auto(res.likelihood);
    }
  }

  double max_likelihood = strategies.empty() ? 0 : std::get<0>(strategies.top());
  std::size_t most_likely_i = strategies.empty() ? 0 : std::get<1>(strategies.top());
  std::size_t most_likely_j = strategies.empty() ? 0 : std::get<2>(strategies.top());
  double max_likelihood_auto = auto_strategies.empty() ? 0 : std::get<0>(auto_strategies.top());
  std::size_t most_likely_i_auto = auto_strategies.empty() ? 0 : std::get<1>(auto_strategies.top());
  std::size_t most_likely_j_auto = auto_strategies.empty() ? 0 : std::get<2>(auto_strategies.top());

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

    while (!strategies.empty()) {
      std::size_t i = std::get<1>(strategies.top());
      std::size_t j = std::get<2>(strategies.top());
      const AnalyzePlayResponse::BoostUsageDetails& details =
          resp.play_strategies(i).boost_usage_details(j);
      if (details.relative_likelihood() < kInclusionThreshold) {
        break;
      }
      *resp.add_likely_play_strategies() = details;
      strategies.pop();
    }

    while (!auto_strategies.empty()) {
      std::size_t i = std::get<1>(auto_strategies.top());
      std::size_t j = std::get<2>(auto_strategies.top());
      const AnalyzePlayResponse::BoostUsageDetails& details =
          resp.play_strategies(i).boost_usage_details(j);
      if (details.relative_likelihood() < kAutoInclusionThreshold ||
          details.relative_likelihood_auto() < kInclusionThreshold) {
        break;
      }
      *resp.add_likely_auto_play_strategies() = details;
      auto_strategies.pop();
    }
  }

  if (!input_.include_full_details) {
    resp.clear_play_strategies();
  }
  return resp;
}

Distribution<boost::math::chi_squared> FillAnalyzer::MakeSkillDist(const PlayStrategy& strategy,
                                                                   float auto_val) const {
  constexpr float kPubOffset = 100;
  constexpr float kMultiOffset = 150;
  constexpr float kScale = 0.05;
  constexpr float kDof = 5;
  float offset = kMultiOffset;
  if (strategy.is_pub()) {
    offset = kPubOffset;
  } else if (strategy.is_auto()) {
    offset = auto_val - (kDof - 2) / kScale;
  }
  return Distribution(boost::math::chi_squared(kDof), offset, kScale);
}

}  // namespace sekai::run_analysis
