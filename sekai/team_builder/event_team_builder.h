#pragma once

#include <optional>
#include <span>
#include <vector>

#include <BS_thread_pool.hpp>
#include <indicators/progress_bar.hpp>

#include "absl/base/thread_annotations.h"
#include "absl/time/time.h"
#include "sekai/card.h"
#include "sekai/estimator.h"
#include "sekai/event_bonus.h"
#include "sekai/profile.h"
#include "sekai/team.h"
#include "sekai/team_builder/constraints.h"
#include "sekai/team_builder/team_builder_base.h"

namespace sekai {

class EventTeamBuilder : public TeamBuilderBase {
 public:
  struct Options {
    // If true, a progress bar will be displayed.
    bool enable_progress = false;

    // Multithreading options
    int block_size = 16;

    // If enabled, will prune the card pool every n character combinations evaluated.
    // Note: this requires waiting for existing tasks which may become a performance penalty if not
    // a lot of cards are removed.
    int prune_every_n_steps = 0;
    // Pruning options
    int max_power = kMaxPower;
    int min_power_in_max_power_team = 0;
    int max_bonus = kMaxEventBonus;
    int min_bonus_in_max_bonus_team = 0;
    int max_skill = kMaxSkillValue;
    int min_ep = 0;

    // If the number of cards in the pool is above this amount, drop all low value cards (1-3
    // stars).
    // TODO: progressively remove cards starting from low value low bonus
    int skip_low_val_cards_thresh = 200;
    // TODO: Add option to refresh lookup table
  };
  EventTeamBuilder() = default;
  explicit EventTeamBuilder(const Options& options) : opts_(options) {}

 protected:
  Options opts_{};
  mutable std::mutex writer_mutex_;
  uint64_t teams_evaluated_ ABSL_GUARDED_BY(writer_mutex_) = 0;
  uint64_t teams_considered_ ABSL_GUARDED_BY(writer_mutex_) = 0;
  std::optional<Team> best_team_ ABSL_GUARDED_BY(writer_mutex_);
  double best_ep_ ABSL_GUARDED_BY(writer_mutex_) = 0;

  void Wait(BS::thread_pool& thread_pool, uint64_t max_progress,
            indicators::ProgressBar* bar = nullptr) const;

  std::vector<Team> RecommendTeamsImpl(std::span<const Card* const> pool, const Profile& profile,
                                       const EventBonus& event_bonus, const Estimator* estimator,
                                       std::optional<absl::Time> deadline = std::nullopt);
  std::vector<Team> RecommendTeamsImpl(std::span<const Card* const> pool, const Profile& profile,
                                       const EventBonus& event_bonus,
                                       const EstimatorBase& estimator,
                                       std::optional<absl::Time> deadline = std::nullopt) override;
};

}  // namespace sekai
