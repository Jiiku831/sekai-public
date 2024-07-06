#include "sekai/team_builder/event_team_builder.h"

#include <chrono>
#include <cstdint>
#include <map>
#include <mutex>
#include <thread>

#include <BS_thread_pool.hpp>
#include <indicators/progress_bar.hpp>

#include "absl/log/absl_check.h"
#include "absl/log/log.h"
#include "absl/strings/str_format.h"
#include "sekai/array_size.h"
#include "sekai/bitset.h"
#include "sekai/combinations.h"
#include "sekai/config.h"
#include "sekai/estimator.h"
#include "sekai/estimator_base.h"
#include "sekai/team_builder/card_pruning.h"
#include "sekai/team_builder/pool_utils.h"

namespace sekai {
namespace {

void UpdateBar(indicators::ProgressBar& bar, uint64_t progress, uint64_t max_progress) {
  bar.set_progress(progress);
  bar.set_option(
      indicators::option::PostfixText{absl::StrFormat("%llu/%llu", progress, max_progress)});
}

// Returns the amount of total teams.
uint64_t TotalTeams(std::span<const std::vector<const Card*>> char_pools) {
  uint64_t teams_total = 0;
  Combinations<int, 5>{
      UniqueCharacterIds(),
      [&](std::span<const int> candidate_chars) {
        std::size_t block_size = 1;
        for (std::size_t i = 0; i < 5; ++i) {
          block_size *= char_pools[candidate_chars[i]].size();
        }
        teams_total += block_size;
        return true;
      },
  }();
  return teams_total;
}

// Returns the amount of cards pruned.
int PrunePools(const EventTeamBuilder::Options& opts, const Estimator& estimator,
               int current_best_ep,
               std::array<std::vector<const Card*>, kCharacterArraySize>& char_pools) {
  CardPruningOptions pruning_opts{
      .estimator = estimator,
      .min_ep = current_best_ep,
      .max_power = opts.max_power,
      .min_power_in_max_power_team = opts.min_power_in_max_power_team,
      .max_bonus = opts.max_bonus,
      .min_bonus_in_max_bonus_team = opts.min_bonus_in_max_bonus_team,
      .max_skill = opts.max_skill,
  };
  int cards_pruned = 0;
  for (std::vector<const Card*>& cards : char_pools) {
    cards_pruned += PruneCards(pruning_opts, cards);
  }
  return cards_pruned;
}

int TotalCardsInPools(const std::array<std::vector<const Card*>, kCharacterArraySize>& char_pools) {
  int count = 0;
  for (const std::vector<const Card*>& pool : char_pools) {
    count += pool.size();
  }
  return count;
}

int RemoveLowValCards(std::array<std::vector<const Card*>, kCharacterArraySize>& char_pools) {
  CardRarityType rarities_to_remove;
  rarities_to_remove.set(db::RARITY_1);
  rarities_to_remove.set(db::RARITY_2);
  rarities_to_remove.set(db::RARITY_3);
  int cards_pruned = 0;
  for (std::vector<const Card*>& pool : char_pools) {
    std::size_t size_before = pool.size();
    std::erase_if(pool,
                  [&](const Card* card) { return rarities_to_remove.test(card->db_rarity()); });
    cards_pruned += size_before - pool.size();
  }
  return cards_pruned;
}

uint64_t PrunedProgress(std::span<const int> char_ids,
                        std::span<const std::vector<const Card*>> new_pools,
                        std::span<const std::vector<const Card*>> orig_pools,
                        int char_combs_evaluated, uint64_t total_teams_pruned) {
  int iteration_num = 0;
  uint64_t orig_teams_evaluated = 0;
  uint64_t new_teams_evaluated = 0;
  Combinations<int, 5>{
      char_ids,
      [&](std::span<const int> candidate_chars) {
        if (iteration_num >= char_combs_evaluated) return false;
        int orig_block_size = 1;
        int new_block_size = 1;
        for (int char_id : candidate_chars) {
          orig_block_size *= orig_pools[char_id].size();
          new_block_size *= new_pools[char_id].size();
        }
        orig_teams_evaluated += orig_block_size;
        new_teams_evaluated += new_block_size;
        ++iteration_num;
        return true;
      },
  }();
  uint64_t considered_teams_pruned = orig_teams_evaluated - new_teams_evaluated;
  return orig_teams_evaluated + (total_teams_pruned - considered_teams_pruned);
}

}  // namespace

std::vector<Team> EventTeamBuilder::RecommendTeamsImpl(std::span<const Card* const> pool,
                                                       const Profile& profile,
                                                       const EventBonus& event_bonus,
                                                       const EstimatorBase& estimator,
                                                       std::optional<absl::Time> deadline) {
  auto cast_estimator = dynamic_cast<const Estimator*>(&estimator);
  ABSL_CHECK_NE(cast_estimator, nullptr);
  return RecommendTeamsImpl(pool, profile, event_bonus, cast_estimator, deadline);
}

std::vector<Team> EventTeamBuilder::RecommendTeamsImpl(std::span<const Card* const> pool,
                                                       const Profile& profile,
                                                       const EventBonus& event_bonus,
                                                       const Estimator* estimator,
                                                       std::optional<absl::Time> deadline) {
  ABSL_CHECK_LE(CharacterArraySize(), kCharacterArraySize);
  BS::thread_pool thread_pool;

  Estimator local_estimator = *estimator;
  if (opts_.max_skill < kMaxSkillValue) {
    local_estimator.PopulateLookupTable(kMinSkillValue, opts_.max_skill);
  }
  if (opts_.min_ep > 0) {
    best_ep_ = opts_.min_ep;
  }

  // Sort cards per char first.
  std::array<std::vector<const Card*>, kCharacterArraySize> char_pools =
      PartitionCardPoolByCharacters(pool);
  std::array<std::vector<const Card*>, kCharacterArraySize> orig_char_pools = char_pools;

  std::unique_ptr<indicators::ProgressBar> bar;
  stats_.teams_total = TotalTeams(char_pools);
  uint64_t max_progress;
  if (opts_.enable_progress) {
    max_progress = stats_.teams_total;
    bar = std::make_unique<indicators::ProgressBar>(
        indicators::option::BarWidth{60}, indicators::option::ShowElapsedTime{true},
        indicators::option::ShowRemainingTime{true}, indicators::option::ShowPercentage{true},
        indicators::option::MaxProgress{max_progress},
        indicators::option::PostfixText{absl::StrFormat("0/%llu", max_progress)});
  }

  int char_combs_evaluated = 0;
  std::vector<int> sorted_char_ids = SortCharactersByMaxEventBonus(event_bonus);
  // TODO: add variant that search near the first
  Combinations<int, 5>{
      sorted_char_ids,
      [&](std::span<const int> candidate_chars) {
        Character chars_present;
        for (int char_id : candidate_chars) {
          chars_present.set(char_id);
        }
        if (!constraints_.CharacterSetSatisfiesConstraint(chars_present)) {
          return true;
        }
        Character lead_chars = constraints_.GetCharactersEligibleForLead(chars_present);

        // TODO: extract to fn
        bool should_prune = opts_.prune_every_n_steps > 0 && best_ep_ > 0 &&
                            char_combs_evaluated % opts_.prune_every_n_steps == 0;
        bool should_skip_low_val_cards =
            opts_.skip_low_val_cards_thresh > 0 && char_combs_evaluated == 0;
        if (should_prune || should_skip_low_val_cards) {
          Wait(thread_pool, max_progress, bar.get());
          if (should_prune) {
            stats_.cards_pruned += PrunePools(opts_, local_estimator, best_ep_, char_pools);
          }
          if (should_skip_low_val_cards &&
              TotalCardsInPools(char_pools) > opts_.skip_low_val_cards_thresh) {
            stats_.cards_pruned += RemoveLowValCards(char_pools);
          }
          uint64_t total_teams_pruned = stats_.teams_total - TotalTeams(char_pools);
          uint64_t new_progress = PrunedProgress(sorted_char_ids, char_pools, orig_char_pools,
                                                 char_combs_evaluated, total_teams_pruned);
          {
            std::lock_guard<std::mutex> guard{writer_mutex_};
            teams_considered_ = new_progress;
          }
        }
        ++char_combs_evaluated;
        std::array<int, 5> char_ids;
        std::array<std::size_t, 5> pool_sizes;
        for (std::size_t i = 0; i < 5; ++i) {
          char_ids[i] = candidate_chars[i];
          pool_sizes[i] = char_pools[candidate_chars[i]].size();
          if (pool_sizes[i] == 0) return true;
        }
        PartitionedProduct<5>(
            opts_.block_size, pool_sizes, [&](const std::array<Extent, 5>& extents) {
              if (thread_pool.get_tasks_queued() > 10000) {
                while (thread_pool.get_tasks_queued() > 100) {
                  if (opts_.enable_progress) {
                    uint64_t progress = 0;
                    {
                      std::lock_guard<std::mutex> guard{writer_mutex_};
                      progress = teams_considered_;
                    }
                    UpdateBar(*bar, progress, max_progress);
                  }
                  if (thread_pool.wait_for(std::chrono::milliseconds(100))) {
                    break;
                  }
                }
              }

              thread_pool.detach_task([this, &char_pools, &profile, &event_bonus, &local_estimator,
                                       char_ids, extents, lead_chars]() {
                double local_best_ep = 0;
                {
                  // Acquire the current best ep at task start time.
                  std::lock_guard<std::mutex> guard{writer_mutex_};
                  local_best_ep = best_ep_;
                }
                std::optional<Team> local_best_team;
                uint64_t local_teams_considered = 0;
                uint64_t local_teams_evaluated = 0;

                ForEachInBlock<5, const Card*>(
                    extents,
                    std::array<std::span<const Card* const>, 5>{
                        char_pools[char_ids[0]], char_pools[char_ids[1]], char_pools[char_ids[2]],
                        char_pools[char_ids[3]], char_pools[char_ids[4]]},
                    [this, &local_teams_considered, &local_teams_evaluated, &local_best_ep,
                     &local_best_team, &local_estimator, &profile, &event_bonus,
                     lead_chars](const std::array<const Card*, 5>& candidate_cards) {
                      ++local_teams_considered;
                      Team candidate_team{candidate_cards};
                      if (!support_pool_.empty()) {
                        candidate_team.FillSupportCards(support_pool_);
                      }

                      int power = candidate_team.Power(profile);
                      float eb = candidate_team.EventBonus(event_bonus);

                      LookupTableEntry entry =
                          local_estimator.LookupEstimatedEp(power, static_cast<int>(eb));

                      if (entry.upper_bound < local_best_ep) {
                        return true;
                      }

                      if (entry.lower_bound > local_best_ep &&
                          !constraints_.HasLeadSkillConstraint()) {
                        local_best_ep = entry.lower_bound;
                        local_best_team = candidate_team;
                        return true;
                      }

                      ++local_teams_evaluated;
                      int skill_value;
                      if (!constraints_.empty()) {
                        Team::SkillValueDetail skill_value_detail =
                            candidate_team.ConstrainedMaxSkillValue(lead_chars);
                        if (!constraints_.LeadSkillSatisfiesConstraint(
                                skill_value_detail.lead_skill)) {
                          return true;
                        }
                        skill_value = skill_value_detail.skill_value;
                      } else {
                        skill_value = candidate_team.MaxSkillValue();
                      }
                      double candidate_ep =
                          local_estimator.ExpectedEp(power, eb, skill_value, skill_value);
                      if (candidate_ep > local_best_ep) {
                        local_best_ep = candidate_ep;
                        local_best_team = candidate_team;
                      }
                      return true;
                    });

                std::lock_guard<std::mutex> guard{writer_mutex_};
                teams_evaluated_ += local_teams_evaluated;
                teams_considered_ += local_teams_considered;
                if (local_best_ep > best_ep_) {
                  best_ep_ = local_best_ep;
                  best_team_ = local_best_team;
                }
              });
              return true;
            });
        return true;
      },
  }();
  Wait(thread_pool, max_progress, bar.get());
  std::lock_guard<std::mutex> guard{writer_mutex_};
  if (bar != nullptr) {
    UpdateBar(*bar, teams_considered_, max_progress);
    bar->mark_as_completed();
  }
  stats_.teams_evaluated = teams_evaluated_;
  stats_.teams_considered = teams_considered_;
  stats_.teams_total = teams_considered_;
  if (best_team_.has_value()) {
    return {*best_team_};
  }
  return {};
}

void EventTeamBuilder::Wait(BS::thread_pool& thread_pool, uint64_t max_progress,
                            indicators::ProgressBar* bar) const {
  if (bar != nullptr) {
    while (!thread_pool.wait_for(std::chrono::milliseconds(100))) {
      uint64_t progress = 0;
      {
        std::lock_guard<std::mutex> guard{writer_mutex_};
        progress = teams_considered_;
      }
      UpdateBar(*bar, progress, max_progress);
    }
  } else {
    thread_pool.wait();
  }
}

}  // namespace sekai
