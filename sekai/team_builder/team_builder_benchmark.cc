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
#include "sekai/team_builder/constraints.h"
#include "sekai/team_builder/event_team_builder.h"
#include "sekai/team_builder/max_bonus_team_builder.h"
#include "sekai/team_builder/max_power_team_builder.h"
#include "sekai/team_builder/naive_team_builder.h"
#include "sekai/team_builder/simulated_annealing_team_builder.h"
#include "sekai/team_builder/team_builder_base.h"
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
        return meta.difficulty() == db::DIFF_EXPERT && meta.music_id() <= 100;
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

void RunBenchmark(absl::AnyInvocable<std::unique_ptr<TeamBuilderBase>() const> make_builder,
                  benchmark::State& state) {
  Profile profile{TestProfile()};
  EventBonus event_bonus{ParseTextProto<EventId>(R"pb(event_id: 117)pb")};
  Estimator estimator = MakeEstimator();
  std::vector<Card> cards = MakeCards(profile, event_bonus);
  std::vector<const Card*> pool = MakePool(cards);
  TeamProto best_team;
  for (auto _ : state) {
    std::unique_ptr<TeamBuilderBase> builder = make_builder();
    std::vector<Team> teams = builder->RecommendTeams(pool, profile, event_bonus, estimator);
    state.PauseTiming();
    // state.counters["Considered"] = builder->stats().teams_considered;
    // state.counters["Evaluated"] = builder->stats().teams_evaluated;
    // state.counters["Pruned"] = builder->stats().cards_pruned;

    if (!teams.empty()) {
      Team& candidate_team = teams[0];
      if (!ValidateTeam(candidate_team)) {
        LOG(WARNING) << "Team builder generated invalid team.";
      }
      int power = candidate_team.Power(profile);
      state.counters["Max Power"] =
          std::max(state.counters["Max Power"].value, static_cast<double>(power));
      float eb = candidate_team.EventBonus(event_bonus);
      state.counters["Max EB"] = std::max(state.counters["Max EB"].value, static_cast<double>(eb));
      candidate_team.ReorderTeamForOptimalSkillValue(builder->constraints());
      int skill_value = candidate_team.SkillValue();
      state.counters["Max Skill"] =
          std::max(state.counters["Max Skill"].value, static_cast<double>(skill_value));
      double candidate_ep = estimator.ExpectedEp(power, eb, skill_value, skill_value);
      state.counters["Max EP"] = std::max(state.counters["Max EP"].value, candidate_ep);
      best_team = candidate_team.ToProto(profile, event_bonus, estimator);
    }
    state.ResumeTiming();
  }
  kTeamReporter.Report(state.name(), best_team);
}

template <typename T>
void BM_RecommendTeam(benchmark::State& state) {
  RunBenchmark([]() { return std::make_unique<T>(); }, state);
}

template <typename T>
void BM_RecommendTeamWithConstraints(benchmark::State& state) {
  RunBenchmark(
      []() {
        auto builder = std::make_unique<T>();
        Constraints constraints;
        constraints.SetMinLeadSkill(80);
        for (int i = 1; i < 12; ++i) {
          constraints.AddLeadChar(i);
        }
        builder->AddConstraints(constraints);
        return builder;
      },
      state);
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

BENCHMARK(BM_RecommendTeam<NaiveTeamBuilder>)
    ->Name("Naive (Points)")
    ->Unit(benchmark::kMillisecond);
BENCHMARK(BM_RecommendTeam<NaiveBonusTeamBuilder>)
    ->Name("Naive (Bonus)")
    ->Unit(benchmark::kMillisecond);
BENCHMARK(BM_RecommendTeam<NaivePowerTeamBuilder>)
    ->Name("Naive (Power)")
    ->Unit(benchmark::kMillisecond);
BENCHMARK(BM_RecommendTeam<NaiveSkillTeamBuilder>)
    ->Name("Naive (Skill)")
    ->Unit(benchmark::kMillisecond);
BENCHMARK(BM_RecommendTeam<MaxBonusTeamBuilder>)
    ->Name("Partitioned (Bonus)")
    ->Unit(benchmark::kMillisecond);
BENCHMARK(BM_RecommendTeam<MaxPowerTeamBuilder>)
    ->Name("Partitioned (Power)")
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

BENCHMARK(BM_RecommendTeamWithConstraints<NaiveTeamBuilder>)
    ->Name("Naive (Points, Constrained)")
    ->Unit(benchmark::kMillisecond);
BENCHMARK(BM_RecommendTeamWithConstraints<SimulatedAnnealingTeamBuilder>)
    ->Name("Annealing (Points, Constrained)")
    ->Unit(benchmark::kMillisecond);
BENCHMARK(BM_RecommendTeamWithConstraints<MaxBonusTeamBuilder>)
    ->Name("Partitioned (Bonus, Constrained)")
    ->Unit(benchmark::kMillisecond);
BENCHMARK(BM_RecommendTeamWithConstraints<MaxPowerTeamBuilder>)
    ->Name("Partitioned (Power, Constrained)")
    ->Unit(benchmark::kMillisecond);
BENCHMARK(BM_RecommendTeamWithConstraints<EventTeamBuilder>)
    ->Name("Partitioned (Event, Constrained)")
    ->Unit(benchmark::kMillisecond);

BENCHMARK(BM_RecommendTeamWithConstraints<NaiveBonusTeamBuilder>)
    ->Name("Naive (Bonus, Constrained)")
    ->Unit(benchmark::kMillisecond);
BENCHMARK(BM_RecommendTeamWithConstraints<SimulatedAnnealingBonusTeamBuilder>)
    ->Name("Annealing (Bonus, Constrained)")
    ->Unit(benchmark::kMillisecond);

BENCHMARK(BM_RecommendTeamWithConstraints<NaivePowerTeamBuilder>)
    ->Name("Naive (Power, Constrained)")
    ->Unit(benchmark::kMillisecond);
BENCHMARK(BM_RecommendTeamWithConstraints<SimulatedAnnealingPowerTeamBuilder>)
    ->Name("Annealing (Power, Constrained)")
    ->Unit(benchmark::kMillisecond);

BENCHMARK(BM_RecommendTeamWithConstraints<NaiveSkillTeamBuilder>)
    ->Name("Naive (Skill, Constrained)")
    ->Unit(benchmark::kMillisecond);
BENCHMARK(BM_RecommendTeamWithConstraints<SimulatedAnnealingSkillTeamBuilder>)
    ->Name("Annealing (Skill, Constrained)")
    ->Unit(benchmark::kMillisecond);

}  // namespace
}  // namespace sekai

BENCHMARK_MAIN();
