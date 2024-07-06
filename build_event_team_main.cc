#include <fstream>
#include <optional>
#include <string>
#include <vector>

#include <ctml.hpp>

#include "absl/flags/flag.h"
#include "absl/strings/str_format.h"
#include "absl/time/clock.h"
#include "absl/time/time.h"
#include "base/init.h"
#include "sekai/card.h"
#include "sekai/config.h"
#include "sekai/db/master_db.h"
#include "sekai/db/proto/all.h"
#include "sekai/estimator.h"
#include "sekai/event_bonus.h"
#include "sekai/html/report.h"
#include "sekai/html/team.h"
#include "sekai/profile.h"
#include "sekai/proto/card_state.pb.h"
#include "sekai/proto/constraints.pb.h"
#include "sekai/proto/profile.pb.h"
#include "sekai/proto_util.h"
#include "sekai/team.h"
#include "sekai/team_builder/constraints.h"
#include "sekai/team_builder/event_team_builder.h"
#include "sekai/team_builder/max_bonus_team_builder.h"
#include "sekai/team_builder/max_power_team_builder.h"
#include "sekai/team_builder/optimization_objective.h"
#include "sekai/team_builder/simulated_annealing_team_builder.h"

ABSL_FLAG(int, event_id, 0, "event id");
ABSL_FLAG(int, chapter_id, 0, "chapter id");
ABSL_FLAG(std::string, output, "", "output path");
ABSL_FLAG(std::optional<int>, max_skill, std::nullopt, "max skill of any team");
ABSL_FLAG(bool, use_simulated_annealing, false,
          "whether or not to use simulated annealing. note this method is non-exact");

constexpr std::string_view kBestTeamTitle = "Best Team";

using namespace sekai;

Profile LoadProfile() {
  auto profile_proto = ParseTextProtoFile<ProfileProto>("data/profile/profile.textproto");
  Profile profile(profile_proto);
  profile.LoadCardsFromCsv(MainRunfilesRoot() / "data/profile/cards.csv");
  return profile;
}

Constraints LoadConstraints() {
  auto constraints_proto =
      ParseTextProtoFile<TeamConstraints>("data/profile/constraints.textproto");
  return Constraints{constraints_proto};
}

std::optional<Team> GetMaxBonusTeam(std::span<const Card* const> cards, const Profile& profile,
                                    const EventBonus& event_bonus, const Estimator& estimator,
                                    const Constraints& constraints) {
  MaxBonusTeamBuilder builder;
  builder.AddConstraints(constraints);
  std::vector<Team> teams = builder.RecommendTeams(cards, profile, event_bonus, estimator);
  if (teams.empty()) return std::nullopt;
  return teams.front();
}

std::optional<Team> GetMaxPowerTeam(std::span<const Card* const> cards, const Profile& profile,
                                    const EventBonus& event_bonus, const Estimator& estimator,
                                    const Constraints& constraints) {
  MaxPowerTeamBuilder builder;
  builder.AddConstraints(constraints);
  std::vector<Team> teams = builder.RecommendTeams(cards, profile, event_bonus, estimator);
  if (teams.empty()) return std::nullopt;
  return teams.front();
}

std::vector<Team> GetMaxTeamSimulatedAnnealing(
    OptimizationObjective obj, std::span<const Card* const> cards, const Profile& profile,
    const EventBonus& event_bonus, const Estimator& estimator, const Constraints& constraints) {
  SimulatedAnnealingTeamBuilder builder(obj);
  builder.AddConstraints(constraints);
  return builder.RecommendTeams(cards, profile, event_bonus, estimator);
}

std::optional<Team> GetMaxTeamSimulatedAnnealingOptional(
    OptimizationObjective obj, std::span<const Card* const> cards, const Profile& profile,
    const EventBonus& event_bonus, const Estimator& estimator, const Constraints& constraints) {
  std::vector<Team> teams =
      GetMaxTeamSimulatedAnnealing(obj, cards, profile, event_bonus, estimator, constraints);
  if (teams.empty()) {
    return std::nullopt;
  }
  return teams.front();
}

struct BestTeams {
  std::optional<Team> max_bonus_team;
  std::optional<Team> max_power_team;
  std::optional<Team> max_skill_team;
  std::vector<Team> recommended_teams;
};

BestTeams GetBestTeamsSimulatedAnnealing(std::span<const Card* const> cards, const Profile& profile,
                                         const EventBonus& event_bonus, const Estimator& estimator,
                                         const Constraints& constraints, bool compute_max) {
  BestTeams teams;
  if (compute_max) {
    teams.max_bonus_team = GetMaxTeamSimulatedAnnealingOptional(
        OptimizationObjective::kOptimizeBonus, cards, profile, event_bonus, estimator, constraints);
    teams.max_power_team = GetMaxTeamSimulatedAnnealingOptional(
        OptimizationObjective::kOptimizePower, cards, profile, event_bonus, estimator, constraints);
    teams.max_skill_team = GetMaxTeamSimulatedAnnealingOptional(
        OptimizationObjective::kOptimizeSkill, cards, profile, event_bonus, estimator, constraints);
  }
  teams.recommended_teams = GetMaxTeamSimulatedAnnealing(
      OptimizationObjective::kOptimizePoints, cards, profile, event_bonus, estimator, constraints);
  return teams;
}

BestTeams GetBestTeamsExhaustive(std::span<const Card* const> cards, const Profile& profile,
                                 const EventBonus& event_bonus, const Estimator& estimator,
                                 const Constraints& constraints) {
  BestTeams teams;
  EventTeamBuilder::Options options{
      .enable_progress = true,
      .prune_every_n_steps = 1000,
  };
  if (absl::GetFlag(FLAGS_max_skill).has_value()) {
    options.max_skill = *absl::GetFlag(FLAGS_max_skill);
  }
  teams.max_power_team = GetMaxPowerTeam(cards, profile, event_bonus, estimator, constraints);
  if (teams.max_power_team.has_value()) {
    options.max_power = teams.max_power_team->Power(profile);
    options.min_power_in_max_power_team = teams.max_power_team->MinPowerContrib(profile);
    int skill_value = teams.max_power_team->ConstrainedMaxSkillValue(constraints).skill_value;
    options.min_ep =
        std::max(static_cast<int>(estimator.ExpectedEp(
                     teams.max_power_team->Power(profile),
                     teams.max_power_team->EventBonus(event_bonus), skill_value, skill_value)),
                 options.min_ep);
  }
  teams.max_bonus_team = GetMaxBonusTeam(cards, profile, event_bonus, estimator, constraints);
  if (teams.max_bonus_team.has_value()) {
    options.max_bonus = teams.max_bonus_team->EventBonus(event_bonus);
    options.min_bonus_in_max_bonus_team = teams.max_bonus_team->MinBonusContrib();
    int skill_value = teams.max_bonus_team->ConstrainedMaxSkillValue(constraints).skill_value;
    options.min_ep = std::max(
        static_cast<int>(estimator.ExpectedEp(teams.max_bonus_team->Power(profile),
                                              options.max_bonus, skill_value, skill_value)),
        options.min_ep);
  }

  EventTeamBuilder builder{options};
  builder.AddConstraints(constraints);
  absl::Time start_time = absl::Now();
  teams.recommended_teams = builder.RecommendTeams(cards, profile, event_bonus, estimator);
  absl::Duration duration = absl::Now() - start_time;
  std::cout << "Cards pruned: " << builder.stats().cards_pruned << std::endl;
  std::cout << absl::StrFormat("Evaluated %llu out of %llu teams in %s\n",
                               builder.stats().teams_evaluated, builder.stats().teams_considered,
                               absl::FormatDuration(duration));
  return teams;
}

std::vector<std::pair<std::string, TeamProto>> BuildEventTeamProtos(const EventBonus& event_bonus,
                                                                    const Estimator& estimator,
                                                                    bool compute_max) {
  Constraints constraints = LoadConstraints();
  Profile profile = LoadProfile();
  profile.ApplyEventBonus(event_bonus);

  std::vector<const Card*> cards = profile.CardPtrs();
  std::cout << "Pool size: " << cards.size() << std::endl;

  BestTeams teams =
      absl::GetFlag(FLAGS_use_simulated_annealing)
          ? GetBestTeamsSimulatedAnnealing(cards, profile, event_bonus, estimator, constraints,
                                           compute_max)
          : GetBestTeamsExhaustive(cards, profile, event_bonus, estimator, constraints);

  std::vector<std::pair<std::string, TeamProto>> team_protos;
  for (Team& team : teams.recommended_teams) {
    team.ReorderTeamForOptimalSkillValue(constraints);
    team_protos.emplace_back(kBestTeamTitle, team.ToProto(profile, event_bonus, estimator));
    // NOTE: need to change return type if we want multiple recommended teams.
    break;
  }
  if (teams.max_bonus_team.has_value()) {
    teams.max_bonus_team->ReorderTeamForOptimalSkillValue(constraints);
    team_protos.emplace_back("Max Bonus",
                             teams.max_bonus_team->ToProto(profile, event_bonus, estimator));
  }
  if (teams.max_power_team.has_value()) {
    teams.max_power_team->ReorderTeamForOptimalSkillValue(constraints);
    team_protos.emplace_back("Max Power",
                             teams.max_power_team->ToProto(profile, event_bonus, estimator));
  }
  if (teams.max_skill_team.has_value()) {
    teams.max_skill_team->ReorderTeamForOptimalSkillValue(constraints);
    team_protos.emplace_back("Max Skill",
                             teams.max_skill_team->ToProto(profile, event_bonus, estimator));
  }
  return team_protos;
}

std::vector<std::pair<std::string, TeamProto>> BuildAllWorldBloomTeams() {
  std::vector<std::pair<std::string, TeamProto>> teams;
  Estimator estimator = RandomExEstimator(Estimator::Mode::kMulti);
  EventId event_id;
  event_id.set_event_id(absl::GetFlag(FLAGS_event_id));
  for (const auto* world_bloom : db::MasterDb::FindAll<db::WorldBloom>(event_id.event_id())) {
    event_id.set_chapter_id(world_bloom->chapter_no());
    EventBonus event_bonus{event_id};

    std::vector<std::pair<std::string, TeamProto>> chapter_teams =
        BuildEventTeamProtos(event_bonus, estimator, /*compute_max=*/false);
    auto chapter_char =
        db::MasterDb::FindFirst<db::GameCharacter>(world_bloom->game_character_id());
    std::string title = absl::StrCat("Chapter ", chapter_char.given_name());
    TeamProto team_proto;
    if (!chapter_teams.empty() && chapter_teams.front().first == kBestTeamTitle) {
      team_proto = chapter_teams.front().second;
    }
    teams.emplace_back(title, team_proto);
  }
  return teams;
}

int main(int argc, char** argv) {
  Init(argc, argv);

  ABSL_CHECK(!absl::GetFlag(FLAGS_output).empty());

  EventBonus event_bonus;
  bool is_wl_no_chapter_id = false;
  Estimator estimator = RandomExEstimator(Estimator::Mode::kMulti);
  if (absl::GetFlag(FLAGS_event_id) > 0) {
    EventId event_id;
    event_id.set_event_id(absl::GetFlag(FLAGS_event_id));
    if (absl::GetFlag(FLAGS_chapter_id) > 0) {
      event_id.set_chapter_id(absl::GetFlag(FLAGS_chapter_id));
    }
    event_bonus = EventBonus(event_id);
    const db::Event& event = db::MasterDb::FindFirst<db::Event>(absl::GetFlag(FLAGS_event_id));
    estimator = RandomExEstimator(event.event_type() == db::Event::CHEERFUL_CARNIVAL
                                      ? Estimator::Mode::kCheerful
                                      : Estimator::Mode::kMulti);
    if (event.event_type() == db::Event::WORLD_BLOOM && absl::GetFlag(FLAGS_chapter_id) <= 0) {
      is_wl_no_chapter_id = true;
    }
  } else {
    //  1 - Ichika    2 - Saki     3 - Honami   4 - Shiho
    //  5 - Minori    6 - Haruka   7 - Airi     8 - Shizuku
    //  9 - Kohane   10 - An      11 - Akito   12 - Toya
    // 13 - Tsukasa  14 - Emu     15 - Nene    16 - Rui
    // 17 - Kanade   18 - Mafuyu  19 - Ena     20 - Mizuki
    // 21 - Miku     22 - Rin     23 - Len     24 - Luka
    // 25 - MEIKO    26 - KAITO
    SimpleEventBonus simple_event_bonus = sekai::ParseTextProto<SimpleEventBonus>(R"pb(
      attr: ATTR_MYST
      chars {char_id: 9}
      chars {char_id: 10}
      chars {char_id: 11}
      chars {char_id: 12}
      chars {char_id: 21 unit: UNIT_VBS}
      chars {char_id: 22 unit: UNIT_VBS}
      chars {char_id: 23 unit: UNIT_VBS}
      chars {char_id: 24 unit: UNIT_VBS}
      chars {char_id: 25 unit: UNIT_VBS}
      chars {char_id: 26 unit: UNIT_VBS}
    )pb");
    event_bonus = EventBonus(simple_event_bonus);
  }
  std::vector<std::pair<std::string, TeamProto>> teams =
      is_wl_no_chapter_id ? BuildAllWorldBloomTeams()
                          : BuildEventTeamProtos(event_bonus, estimator, /*compute_max=*/true);

  CTML::Node root("div");
  for (const auto& [title, team] : teams) {
    root.AppendChild(CTML::Node("h3", title));
    root.AppendChild(html::Team(team));
  }
  std::ofstream fout(absl::GetFlag(FLAGS_output));
  fout << html::CreateReport(root);
}
