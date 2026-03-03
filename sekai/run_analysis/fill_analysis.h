#pragma once

#include <boost/math/distributions/chi_squared.hpp>
#include <boost/math/distributions/normal.hpp>

#include "absl/base/nullability.h"
#include "absl/status/statusor.h"
#include "sekai/db/proto/music_meta.pb.h"
#include "sekai/estimator.h"
#include "sekai/run_analysis/proto/service.pb.h"
#include "sekai/run_analysis/stats_util.h"

namespace sekai::run_analysis {

constexpr float kDefaultSkillSigma = 10;
constexpr float kDefaultTimeOffset = 37.7;
constexpr float kDefaultTimeScale = 0.2;
constexpr float kDefaultTimeDof = 5;

// Time adjustments
constexpr float kPubOffset = 2;
constexpr float kPubScale = 0.125 / kDefaultTimeScale;
constexpr float kAutoOffset = 20 - kDefaultTimeOffset;
constexpr float kAutoScale = 0.135 / kDefaultTimeScale;

class PlayStrategy;
constexpr std::size_t kPlayStrategyCount = 13;
extern const std::array<PlayStrategy, kPlayStrategyCount> kStrategies;

struct FillAnalysisInput {
  int power;
  float event_bonus;
  float skill_min;
  float skill_max;
  float card_skill_min;
  float card_skill_max;
  float observed_gph;
  float observed_ppg;

  bool include_full_details = true;
};

struct FillAnalysisParameters {
  float filler_skill_sigma = kDefaultSkillSigma;
  // Fit from past event data.
  float time_offset = kDefaultTimeOffset;
  float time_scale = kDefaultTimeScale;
  float time_dof = kDefaultTimeDof;
};

struct FillAnalysis {
  ConfidenceInterval fill_power;
  double likelihood;
};

class PlayStrategy {
 public:
  PlayStrategy(PlayStrategyName api_name, absl::string_view name, Estimator::Mode mode,
               std::vector<const db::MusicMeta * absl_nonnull> metas, double added_time_offset = 0,
               double added_time_scale = 1, bool is_pub = false, bool is_auto = false)
      : api_name_(api_name),
        name_(name),
        estimator_(mode, metas),
        added_time_offset_(added_time_offset),
        added_time_scale_(added_time_scale),
        is_pub_(is_pub),
        is_auto_(is_auto) {}

  PlayStrategyName api_name() const { return api_name_; }
  const char* absl_nonnull name() const { return name_.c_str(); }
  const Estimator& estimator() const { return estimator_; }
  double added_time_offset() const { return added_time_offset_; }
  double added_time_scale() const { return added_time_scale_; }
  double is_pub() const { return is_pub_; }
  double is_auto() const { return is_auto_; }

 private:
  PlayStrategyName api_name_;
  std::string name_;
  Estimator estimator_;
  double added_time_offset_, added_time_scale_;
  bool is_pub_;
  bool is_auto_;
};

class FillAnalyzer {
 public:
  FillAnalyzer() : FillAnalyzer({}, {}) {}
  FillAnalyzer(FillAnalysisInput input, FillAnalysisParameters params)
      : input_(std::move(input)),
        params_(std::move(params)),
        filler_skill_var_(params.filler_skill_sigma * params.filler_skill_sigma) {}
  FillAnalyzer(const AnalyzePlayRequest& request)
      : FillAnalyzer(
            {
                .power = request.power(),
                .event_bonus = request.event_bonus(),
                .skill_min = request.skill_min(),
                .skill_max = request.skill_max(),
                .card_skill_min = request.card_skill_min(),
                .card_skill_max = request.card_skill_max(),
                .observed_gph = request.observed_games_per_hour(),
                .observed_ppg = request.observed_ep_per_game(),
                .include_full_details = request.include_full_details(),
            },
            {}) {}

  absl::StatusOr<AnalyzePlayResponse> RunAnalysis() const;
  absl::StatusOr<FillAnalysis> RunFillAnalysisForStrategy(const PlayStrategy& strategy,
                                                          int boost_usage) const;
  Distribution<boost::math::chi_squared> MakeTimeDist(double added_time_offset = 0,
                                                      double added_time_scale = 1) const {
    return Distribution(boost::math::chi_squared(params_.time_dof),
                        params_.time_offset + added_time_offset,
                        params_.time_scale * added_time_scale);
  }
  Distribution<boost::math::chi_squared> MakeTimeDist(const PlayStrategy& strategy) const {
    return MakeTimeDist(strategy.added_time_offset(), strategy.added_time_scale());
  }
  Distribution<boost::math::chi_squared> MakeSkillDist(const PlayStrategy& strategy,
                                                       float auto_val) const;

  static Distribution<boost::math::normal> MakePlayDist(const Estimator& estimator,
                                                        float multiplier, int power,
                                                        float event_bonus, float player_skill,
                                                        float filler_skill, float filler_skill_var);

 private:
  FillAnalysisInput input_;
  FillAnalysisParameters params_;
  double filler_skill_var_;
};

}  // namespace sekai::run_analysis
