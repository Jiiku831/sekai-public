#include <benchmark/benchmark.h>

#include "sekai/card.h"
#include "sekai/db/master_db.h"
#include "sekai/db/proto/all.h"
#include "sekai/event_bonus.h"
#include "sekai/profile.h"
#include "sekai/proto/profile.pb.h"
#include "sekai/team.h"
#include "testing/util.h"

namespace sekai {
namespace {

using ::testing::ParseTextProto;

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

std::vector<const Card*> MakeCardPtrs(std::span<const Card> cards) {
  std::vector<const Card*> card_ptrs;
  for (const Card& card : cards) {
    card_ptrs.push_back(&card);
  }
  return card_ptrs;
}

Card CreateCard(const Profile& profile, int card_id, int level, int master_rank = 0) {
  CardState state;
  state.set_level(level);
  state.set_master_rank(master_rank);
  state.set_special_training(true);
  for (const auto* episode : db::MasterDb::FindAll<db::CardEpisode>(card_id)) {
    state.add_card_episodes_read(episode->id());
  }
  Card card{db::MasterDb::FindFirst<db::Card>(card_id), state};
  card.ApplyProfilePowerBonus(profile);
  return card;
}

void BM_TeamComputePower(benchmark::State& state) {
  Profile profile{TestProfile()};
  std::array cards = {
      CreateCard(profile, /*card_id=*/404, /*level=*/60, /*master_rank=*/5),
      CreateCard(profile, /*card_id=*/139, /*level=*/60),
      CreateCard(profile, /*card_id=*/115, /*level=*/60),
      CreateCard(profile, /*card_id=*/511, /*level=*/60),
      CreateCard(profile, /*card_id=*/787, /*level=*/60),
  };
  auto card_ptrs = MakeCardPtrs(cards);
  for (auto _ : state) {
    Team team{card_ptrs};
    benchmark::DoNotOptimize(team.PowerDetailed(profile).sum());
  }
}

void BM_TeamComputePowerDirect(benchmark::State& state) {
  Profile profile{TestProfile()};
  std::array cards = {
      CreateCard(profile, /*card_id=*/404, /*level=*/60, /*master_rank=*/5),
      CreateCard(profile, /*card_id=*/139, /*level=*/60),
      CreateCard(profile, /*card_id=*/115, /*level=*/60),
      CreateCard(profile, /*card_id=*/511, /*level=*/60),
      CreateCard(profile, /*card_id=*/787, /*level=*/60),
  };
  auto card_ptrs = MakeCardPtrs(cards);
  for (auto _ : state) {
    Team team{card_ptrs};
    benchmark::DoNotOptimize(team.Power(profile));
  }
}

void BM_TeamComputeEventBonusNoDiffAttr(benchmark::State& state) {
  Profile profile{TestProfile()};
  std::array cards = {
      CreateCard(profile, /*card_id=*/404, /*level=*/60, /*master_rank=*/5),
      CreateCard(profile, /*card_id=*/139, /*level=*/60),
      CreateCard(profile, /*card_id=*/115, /*level=*/60),
      CreateCard(profile, /*card_id=*/511, /*level=*/60),
      CreateCard(profile, /*card_id=*/787, /*level=*/60),
  };
  auto event_id = ParseTextProto<EventId>(R"pb(event_id: 113)pb");
  EventBonus bonus(event_id);
  for (Card& card : cards) {
    card.ApplyEventBonus(bonus);
  }
  auto card_ptrs = MakeCardPtrs(cards);
  for (auto _ : state) {
    Team team{card_ptrs};
    benchmark::DoNotOptimize(team.EventBonus(bonus));
  }
}

void BM_TeamComputeEventBonusWithDiffAttr(benchmark::State& state) {
  Profile profile{TestProfile()};
  std::array cards = {
      CreateCard(profile, /*card_id=*/404, /*level=*/60, /*master_rank=*/5),
      CreateCard(profile, /*card_id=*/139, /*level=*/60),
      CreateCard(profile, /*card_id=*/115, /*level=*/60),
      CreateCard(profile, /*card_id=*/511, /*level=*/60),
      CreateCard(profile, /*card_id=*/787, /*level=*/60),
  };
  auto event_id = ParseTextProto<EventId>(R"pb(event_id: 112 chapter_id: 1)pb");
  EventBonus bonus(event_id);
  for (Card& card : cards) {
    card.ApplyEventBonus(bonus);
  }
  auto card_ptrs = MakeCardPtrs(cards);
  for (auto _ : state) {
    Team team{card_ptrs};
    benchmark::DoNotOptimize(team.EventBonus(bonus));
  }
}

void BM_TeamComputeSkillValueNoUScorer(benchmark::State& state) {
  Profile profile{TestProfile()};
  std::array cards = {
      CreateCard(profile, /*card_id=*/403, /*level=*/60, /*master_rank=*/5),
      CreateCard(profile, /*card_id=*/139, /*level=*/60),
      CreateCard(profile, /*card_id=*/115, /*level=*/60),
      CreateCard(profile, /*card_id=*/511, /*level=*/60),
      CreateCard(profile, /*card_id=*/787, /*level=*/60),
  };
  auto card_ptrs = MakeCardPtrs(cards);
  for (auto _ : state) {
    Team team{card_ptrs};
    benchmark::DoNotOptimize(team.SkillValue());
  }
}

void BM_TeamComputeSkillValueWithUScorer(benchmark::State& state) {
  Profile profile{TestProfile()};
  std::array cards = {
      CreateCard(profile, /*card_id=*/404, /*level=*/60, /*master_rank=*/5),
      CreateCard(profile, /*card_id=*/139, /*level=*/60),
      CreateCard(profile, /*card_id=*/115, /*level=*/60),
      CreateCard(profile, /*card_id=*/511, /*level=*/60),
      CreateCard(profile, /*card_id=*/787, /*level=*/60),
  };
  auto card_ptrs = MakeCardPtrs(cards);
  for (auto _ : state) {
    Team team{card_ptrs};
    benchmark::DoNotOptimize(team.SkillValue());
  }
}

void BM_TeamComputeAllBestCase(benchmark::State& state) {
  Profile profile{TestProfile()};
  std::array cards = {
      CreateCard(profile, /*card_id=*/403, /*level=*/60, /*master_rank=*/5),
      CreateCard(profile, /*card_id=*/139, /*level=*/60),
      CreateCard(profile, /*card_id=*/115, /*level=*/60),
      CreateCard(profile, /*card_id=*/511, /*level=*/60),
      CreateCard(profile, /*card_id=*/787, /*level=*/60),
  };
  auto event_id = ParseTextProto<EventId>(R"pb(event_id: 111)pb");
  EventBonus bonus(event_id);
  for (Card& card : cards) {
    card.ApplyEventBonus(bonus);
  }
  auto card_ptrs = MakeCardPtrs(cards);
  for (auto _ : state) {
    Team team{card_ptrs};
    benchmark::DoNotOptimize(team.Power(profile));
    benchmark::DoNotOptimize(team.EventBonus(bonus));
    benchmark::DoNotOptimize(team.SkillValue());
  }
}

void BM_TeamComputeAllWorstCase(benchmark::State& state) {
  Profile profile{TestProfile()};
  std::array cards = {
      CreateCard(profile, /*card_id=*/404, /*level=*/60, /*master_rank=*/5),
      CreateCard(profile, /*card_id=*/139, /*level=*/60),
      CreateCard(profile, /*card_id=*/115, /*level=*/60),
      CreateCard(profile, /*card_id=*/511, /*level=*/60),
      CreateCard(profile, /*card_id=*/787, /*level=*/60),
  };
  auto event_id = ParseTextProto<EventId>(R"pb(event_id: 112 chapter_id: 1)pb");
  EventBonus bonus(event_id);
  for (Card& card : cards) {
    card.ApplyEventBonus(bonus);
  }
  auto card_ptrs = MakeCardPtrs(cards);
  for (auto _ : state) {
    Team team{card_ptrs};
    benchmark::DoNotOptimize(team.Power(profile));
    benchmark::DoNotOptimize(team.EventBonus(bonus));
    benchmark::DoNotOptimize(team.SkillValue());
  }
}

// -----------------------------------------------------------------
// Benchmark                       Time             CPU   Iterations
// -----------------------------------------------------------------
// int power + branches         28.7 ns         28.7 ns     24412377  (deviation: -8)
// int power + shifts           30.0 ns         30.0 ns     21484347  (deviation: -17)
// int power + lookup table     27.1 ns         27.1 ns     25698994  (deviation: +3)
// int power + lookup table v2  26.0 ns         26.0 ns     27122511  (deviation: +3)
// int power + lookup table v3  24.2 ns         24.2 ns     29166642  (deviation: +3)
// float power + branches       30.3 ns         30.3 ns     23044638  (deviation: +10)
// float power + mults          28.7 ns         28.7 ns     23965667  (deviation: +10)

BENCHMARK(BM_TeamComputePower);
BENCHMARK(BM_TeamComputePowerDirect);
BENCHMARK(BM_TeamComputeEventBonusNoDiffAttr);
BENCHMARK(BM_TeamComputeEventBonusWithDiffAttr);
BENCHMARK(BM_TeamComputeSkillValueNoUScorer);
BENCHMARK(BM_TeamComputeSkillValueWithUScorer);
BENCHMARK(BM_TeamComputeAllBestCase);
BENCHMARK(BM_TeamComputeAllWorstCase);

}  // namespace
}  // namespace sekai

BENCHMARK_MAIN();
