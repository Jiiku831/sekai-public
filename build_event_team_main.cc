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
#include "sekai/proto/profile.pb.h"
#include "sekai/proto_util.h"
#include "sekai/team.h"
#include "sekai/team_builder/event_team_builder.h"
#include "sekai/team_builder/max_bonus_team_builder.h"

ABSL_FLAG(int, event_id, 0, "event_id");
ABSL_FLAG(std::string, output, "", "output path");
ABSL_FLAG(std::optional<int>, max_power, std::nullopt, "max power of any team");
ABSL_FLAG(std::optional<int>, min_power_in_max_power_team, std::nullopt,
          "the power of the weakest card in your max power team");
ABSL_FLAG(std::optional<int>, max_skill, std::nullopt, "max skill of any team");

using namespace sekai;

Profile LoadProfile() {
  auto profile_proto = ParseTextProtoFile<ProfileProto>("data/profile/profile.textproto");
  Profile profile(profile_proto);
  profile.LoadCardsFromCsv(MainRunfilesRoot() / "data/profile/cards.csv");
  return profile;
}

std::optional<Team> GetMaxBonusTeam(std::span<const Card* const> cards, const Profile& profile,
                                    const EventBonus& event_bonus, const Estimator& estimator) {
  MaxBonusTeamBuilder builder;
  std::vector<Team> teams = builder.RecommendTeams(cards, profile, event_bonus, estimator);
  if (teams.empty()) return std::nullopt;
  return teams.front();
}

int main(int argc, char** argv) {
  Init(argc, argv);

  ABSL_CHECK(!absl::GetFlag(FLAGS_output).empty());

  EventBonus event_bonus;
  Estimator estimator = RandomExEstimator(Estimator::Mode::kMulti);
  if (absl::GetFlag(FLAGS_event_id) > 0) {
    const db::Event& event = db::MasterDb::FindFirst<db::Event>(absl::GetFlag(FLAGS_event_id));
    event_bonus = EventBonus(event);
    estimator = RandomExEstimator(event.event_type() == db::Event::CHEERFUL_CARNIVAL
                                      ? Estimator::Mode::kCheerful
                                      : Estimator::Mode::kMulti);
  } else {
    // 01 - Ichika   02 - Saki    03 - Honami  04 - Shiho
    // 05 - Minori   06 - Haruka  07 - Airi    08 - Shizuku
    // 09 - Kohane   10 - An      11 - Akito   12 - Toya
    // 13 - Tsukasa  14 - Emu     15 - Nene    16 - Rui
    // 17 - Kanade   18 - Mafuyu  19 - Ena     20 - Mizuki
    // 21 - Miku     22 - Rin     23 - Len     24 - Luka
    // 25 - MEIKO    26 - KAITO
    SimpleEventBonus simple_event_bonus = sekai::ParseTextProto<SimpleEventBonus>(R"pb(
      attr: ATTR_PURE
      chars {char_id: 17}
      chars {char_id: 18}
      chars {char_id: 19}
      chars {char_id: 20}
      chars {char_id: 21 unit: UNIT_25}
      chars {char_id: 22 unit: UNIT_25}
      chars {char_id: 23 unit: UNIT_25}
      chars {char_id: 24 unit: UNIT_25}
      chars {char_id: 25 unit: UNIT_25}
      chars {char_id: 26 unit: UNIT_25}
    )pb");
    event_bonus = EventBonus(simple_event_bonus);
  }
  Profile profile = LoadProfile();
  profile.ApplyEventBonus(event_bonus);

  std::vector<const Card*> cards = profile.CardPtrs();
  std::cout << "Pool size: " << cards.size() << std::endl;

  EventTeamBuilder::Options options{
      .enable_progress = true,
      .prune_every_n_steps = 1000,
  };
  if (absl::GetFlag(FLAGS_max_power).has_value()) {
    options.max_power = *absl::GetFlag(FLAGS_max_power);
  }
  if (absl::GetFlag(FLAGS_min_power_in_max_power_team).has_value()) {
    options.min_power_in_max_power_team = *absl::GetFlag(FLAGS_min_power_in_max_power_team);
  }
  if (absl::GetFlag(FLAGS_max_skill).has_value()) {
    options.max_skill = *absl::GetFlag(FLAGS_max_skill);
  }
  std::optional<Team> max_bonus_team = GetMaxBonusTeam(cards, profile, event_bonus, estimator);
  if (max_bonus_team.has_value()) {
    options.max_bonus = max_bonus_team->EventBonus(event_bonus);
    options.min_bonus_in_max_bonus_team = max_bonus_team->MinBonusContrib();
    int skill_value = max_bonus_team->MaxSkillValue();
    options.min_ep = estimator.ExpectedEp(max_bonus_team->Power(profile), options.max_bonus,
                                          skill_value, skill_value);
  }

  EventTeamBuilder builder{options};
  absl::Time start_time = absl::Now();
  std::vector<Team> teams = builder.RecommendTeams(cards, profile, event_bonus, estimator);
  absl::Duration duration = absl::Now() - start_time;
  std::cout << "Cards pruned: " << builder.stats().cards_pruned << std::endl;
  std::cout << absl::StrFormat("Evaluated %llu out of %llu teams in %s\n",
                               builder.stats().teams_evaluated, builder.stats().teams_considered,
                               absl::FormatDuration(duration));

  CTML::Node root("div");
  if (max_bonus_team.has_value()) {
    root.AppendChild(CTML::Node("h3", "Max Bonus"));
    max_bonus_team->ReorderTeamForOptimalSkillValue();
    root.AppendChild(html::Team(max_bonus_team->ToProto(profile, event_bonus, estimator)));
  }
  root.AppendChild(CTML::Node("h3", "Best Teams"));
  for (Team& team : teams) {
    team.ReorderTeamForOptimalSkillValue();
    root.AppendChild(html::Team(team.ToProto(profile, event_bonus, estimator)));
  }
  std::ofstream fout(absl::GetFlag(FLAGS_output));
  fout << html::CreateReport(root);
}
