#include <memory>
#include <span>
#include <string>
#include <vector>

#include <benchmark/benchmark.h>
#include <ctml.hpp>

#include "absl/container/flat_hash_set.h"
#include "absl/functional/any_invocable.h"
#include "sekai/card.h"
#include "sekai/db/master_db.h"
#include "sekai/db/proto/all.h"
#include "sekai/estimator.h"
#include "sekai/event_bonus.h"
#include "sekai/html/report.h"
#include "sekai/html/team.h"
#include "sekai/profile.h"
#include "sekai/proto/profile.pb.h"
#include "sekai/team.h"
#include "sekai/team_builder/event_team_builder.h"
#include "sekai/team_builder/max_bonus_team_builder.h"
#include "sekai/team_builder/naive_team_builder.h"
#include "sekai/team_builder/team_builder.h"
#include "testing/util.h"

namespace sekai {
namespace {

using ::testing::ParseTextProto;

constexpr int kMaxCards = 50;

ProfileProto TestProfile() {
  // clang-format off
  return ParseTextProto<ProfileProto>(R"pb(
    area_item_levels: [
      # Offset (ignored)
      0,
      # LN chars
      15, 15, 15, 15,
      # MMJ chars
      10, 10, 10, 10,
      # VBS chars
      10, 10, 10, 10,
      # WXS chars
      10, 10, 10, 10,
      # 25ji chars
      10, 10, 10, 10,
      # Miku items
      15, 15, 15, 15, 15,
      # Rin item
      15,
      # Len item
      15,
      # Luka item
      15,
      # KAITO item
      15,
      # MEIKO item
      15,
      # LN unit
      15, 15,
      # MMJ unit
      10, 10,
      # VBS unit
      10, 10,
      # WXS unit
      10, 10,
      # 25ji unit
      10, 10,
      # VS unit
      15, 15, 15, 15, 15,
      # Cool plant (miyajo first)
      15, 10,
      # Cute plant
      15, 15,
      # Pure plant
      15, 15,
      # Happy plant
      15, 15,
      # Mysterious plant
      10, 10
    ]
    character_ranks: [
      # Offset (ignored)
      0,
      # LN
      55, 73, 54, 53,
      # MMJ
      44, 47, 44, 41,
      # VBS
      47, 41, 43, 41,
      # WXS
      43, 43, 42, 41,
      # 25ji
      45, 42, 44, 46,
      # VS
      109, 52, 52, 54, 53, 50
    ]
    bonus_power: 270
  )pb");
  // clang-format on
}

EventBonus GetEventBonus() {
  auto event_id = ParseTextProto<EventId>(R"pb(event_id: 117)pb");
  EventBonus bonus(event_id);
  return bonus;
}

std::vector<Card> MakeCards(const Profile& profile, const EventBonus& event_bonus) {
  std::vector<Card> cards;
  for (const db::Card& card : db::MasterDb::GetAll<db::Card>()) {
    CardState state;
    state.set_level(MaxLevelForRarity(card.card_rarity_type()));
    if (card.card_rarity_type() == db::RARITY_3 || card.card_rarity_type() == db::RARITY_4) {
      state.set_special_training(true);
    }
    cards.emplace_back(card, state);
    cards.back().ApplyProfilePowerBonus(profile);
    cards.back().ApplyEventBonus(event_bonus);
    if (cards.size() >= kMaxCards) break;
  }
  return cards;
}

std::vector<const Card*> MakePool(std::span<const Card> cards) {
  std::vector<const Card*> pool;
  for (const Card& card : cards) {
    pool.push_back(&card);
  }
  return pool;
}

Estimator MakeEstimator() {
  std::vector<const db::MusicMeta*> metas =
      db::MasterDb::GetIf<db::MusicMeta>([](const db::MusicMeta& meta) {
        return meta.difficulty() == "expert" && meta.music_id() <= 100;
      });
  return Estimator{Estimator::Mode::kMulti, metas};
}

bool ValidateTeam(const Team& team) {
  absl::flat_hash_set<int> char_ids;
  for (const Card* card : team.cards()) {
    auto [unused, not_present] = char_ids.insert(card->character_id());
    if (!not_present) return false;
  }
  return true;
}

std::pair<Team*, int> ExtractBestEp(std::span<Team> teams, const Profile& profile,
                                    const EventBonus& event_bonus, const Estimator& estimator) {
  Team* best_team = nullptr;
  double best_ep = 0;
  for (Team& candidate_team : teams) {
    if (!ValidateTeam(candidate_team)) {
      continue;
    }
    int power = candidate_team.Power(profile);
    float eb = candidate_team.EventBonus(event_bonus);
    candidate_team.ReorderTeamForOptimalSkillValue();
    int skill_value = candidate_team.SkillValue();
    double candidate_ep = estimator.ExpectedEp(power, eb, skill_value, skill_value);
    best_ep = std::max(best_ep, candidate_ep);
    best_team = &candidate_team;
  }
  return std::make_pair(best_team, static_cast<int>(best_ep));
}

class TeamReporter {
 public:
  void Report(std::string_view name, const TeamProto& team) { teams_.emplace_back(name, team); }

  ~TeamReporter() {
    CTML::Node root("div");
    absl::flat_hash_set<std::string> seen;
    for (const auto& [name, team] : teams_) {
      auto [unused_it, new_team] = seen.insert(name);
      if (!new_team) continue;
      root.AppendChild(
          CTML::Node("div").AppendChild(CTML::Node("h3", name)).AppendChild(html::Team(team)));
    }
    std::ofstream fout("/tmp/benchmark.html");
    fout << html::CreateReport(root);
  }

 private:
  std::vector<std::pair<std::string, TeamProto>> teams_;
};
TeamReporter kTeamReporter;

void RunBenchmark(absl::AnyInvocable<std::unique_ptr<TeamBuilder>() const> make_builder,
                  benchmark::State& state) {
  Profile profile{TestProfile()};
  EventBonus event_bonus = GetEventBonus();
  Estimator estimator = MakeEstimator();
  std::vector<Card> cards = MakeCards(profile, event_bonus);
  std::vector<const Card*> pool = MakePool(cards);
  TeamProto best_team;
  for (auto _ : state) {
    std::unique_ptr<TeamBuilder> builder = make_builder();
    std::vector<Team> teams = builder->RecommendTeams(pool, profile, event_bonus, estimator);
    state.PauseTiming();
    state.counters["Considered"] = builder->stats().teams_considered;
    state.counters["Evaluated"] = builder->stats().teams_evaluated;
    state.counters["Pruned"] = builder->stats().cards_pruned;
    Team* best_team_ptr = nullptr;
    std::tie(best_team_ptr, state.counters["Best EP"]) =
        ExtractBestEp(teams, profile, event_bonus, estimator);
    if (best_team_ptr != nullptr) {
      best_team = best_team_ptr->ToProto(profile, event_bonus, estimator);
    }
    state.ResumeTiming();
  }
  kTeamReporter.Report(state.name(), best_team);
}

template <typename T>
void BM_RecommendTeam(benchmark::State& state) {
  RunBenchmark([]() { return std::make_unique<T>(); }, state);
}

template <auto obj>
void BM_RecommendTeamNaive(benchmark::State& state) {
  RunBenchmark([]() { return std::make_unique<NaiveTeamBuilder>(obj); }, state);
}

void BM_RecommendTeamPruneByPower(benchmark::State& state) {
  // These pruning params are for when kMaxCards = 200;
  RunBenchmark(
      []() {
        return std::make_unique<EventTeamBuilder>(EventTeamBuilder::Options{
            .prune_every_n_steps = 1000,
            .max_power = 303983,
            .min_power_in_max_power_team = 54937,
            .max_skill = 210,
        });
      },
      state);
}

void BM_RecommendTeamPruneByBonus(benchmark::State& state) {
  // These pruning params are for when kMaxCards = 200;
  RunBenchmark(
      []() {
        return std::make_unique<EventTeamBuilder>(EventTeamBuilder::Options{
            .prune_every_n_steps = 1000,
            .max_bonus = 250,
            .min_bonus_in_max_bonus_team = 35,
            .max_skill = 210,
        });
      },
      state);
}

void BM_RecommendTeamPruneByAll(benchmark::State& state) {
  // These pruning params are for when kMaxCards = 200;
  RunBenchmark(
      []() {
        return std::make_unique<EventTeamBuilder>(EventTeamBuilder::Options{
            .prune_every_n_steps = 1000,
            .max_power = 303983,
            .min_power_in_max_power_team = 54937,
            .max_bonus = 250,
            .min_bonus_in_max_bonus_team = 35,
            .max_skill = 210,
        });
      },
      state);
}

BENCHMARK(BM_RecommendTeam<NaiveTeamBuilder>)->Name("Naive")->Unit(benchmark::kMillisecond);
BENCHMARK(BM_RecommendTeamNaive<NaiveTeamBuilder::Objective::kOptimizeBonus>)
    ->Name("Naive (Bonus)")
    ->Unit(benchmark::kMillisecond);
BENCHMARK(BM_RecommendTeamNaive<NaiveTeamBuilder::Objective::kOptimizePower>)
    ->Name("Naive (Power)")
    ->Unit(benchmark::kMillisecond);
BENCHMARK(BM_RecommendTeamNaive<NaiveTeamBuilder::Objective::kOptimizeSkill>)
    ->Name("Naive (Skill)")
    ->Unit(benchmark::kMillisecond);
BENCHMARK(BM_RecommendTeam<MaxBonusTeamBuilder>)
    ->Name("Partitioned (Max Bonus)")
    ->Unit(benchmark::kMillisecond);
BENCHMARK(BM_RecommendTeam<EventTeamBuilder>)
    ->Name("Partitioned (Event)")
    ->Unit(benchmark::kMillisecond);
BENCHMARK(BM_RecommendTeamPruneByPower)
    ->Name("Partitioned (Event, Power)")
    ->Unit(benchmark::kMillisecond);
BENCHMARK(BM_RecommendTeamPruneByBonus)
    ->Name("Partitioned (Event, Bonus)")
    ->Unit(benchmark::kMillisecond);
BENCHMARK(BM_RecommendTeamPruneByAll)
    ->Name("Partitioned (Event, All)")
    ->Unit(benchmark::kMillisecond);

}  // namespace
}  // namespace sekai

BENCHMARK_MAIN();
