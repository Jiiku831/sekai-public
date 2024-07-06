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
#include "sekai/array_size.h"
#include "sekai/card.h"
#include "sekai/challenge_live_estimator.h"
#include "sekai/config.h"
#include "sekai/db/master_db.h"
#include "sekai/db/proto/all.h"
#include "sekai/estimator_base.h"
#include "sekai/event_bonus.h"
#include "sekai/html/report.h"
#include "sekai/html/team.h"
#include "sekai/profile.h"
#include "sekai/proto/profile.pb.h"
#include "sekai/proto_util.h"
#include "sekai/team.h"
#include "sekai/team_builder/challenge_live_team_builder.h"

ABSL_FLAG(std::string, output, "", "output path");

using namespace sekai;

Profile LoadProfile() {
  auto profile_proto = ParseTextProtoFile<ProfileProto>("data/profile/profile.textproto");
  Profile profile(profile_proto);
  profile.LoadCardsFromCsv(MainRunfilesRoot() / "data/profile/cards.csv");
  return profile;
}

std::vector<Team> GetBestTeams(int char_id, std::span<const Card* const> cards,
                               const Profile& profile, const EventBonus& event_bonus,
                               const EstimatorBase& estimator) {
  ChallengeLiveTeamBuilder builder({.enable_progress = true}, char_id);
  return builder.RecommendTeams(cards, profile, event_bonus, estimator);
}

std::optional<Team> GetBestTeam(int char_id, std::span<const Card* const> cards,
                                const Profile& profile, const EventBonus& event_bonus,
                                const EstimatorBase& estimator) {
  std::vector<Team> teams = GetBestTeams(char_id, cards, profile, event_bonus, estimator);
  if (teams.empty()) {
    return std::nullopt;
  }
  return teams.front();
}

std::vector<std::pair<std::string, TeamProto>> BuildTeams() {
  Profile profile = LoadProfile();
  EventBonus event_bonus;
  profile.ApplyEventBonus(event_bonus);
  std::vector<const Card*> cards = profile.CardPtrs();
  ChallengeLiveEstimator estimator;

  std::vector<std::pair<std::string, TeamProto>> team_protos;
  for (int char_id : UniqueCharacterIds()) {
    std::optional<Team> team = GetBestTeam(char_id, cards, profile, event_bonus, estimator);
    TeamProto team_proto;
    if (team.has_value()) {
      team->ReorderTeamForOptimalSkillValue();
      team_proto = team->ToProto(profile, event_bonus, estimator);
    }
    auto character = db::MasterDb::FindFirst<db::GameCharacter>(char_id);
    team_protos.emplace_back(character.given_name(), team_proto);
  }
  return team_protos;
}

int main(int argc, char** argv) {
  Init(argc, argv);

  ABSL_CHECK(!absl::GetFlag(FLAGS_output).empty());

  std::vector<std::pair<std::string, TeamProto>> teams = BuildTeams();

  CTML::Node root("div");
  for (const auto& [title, team] : teams) {
    root.AppendChild(CTML::Node("h3", title));
    root.AppendChild(html::Team(team));
  }
  std::ofstream fout(absl::GetFlag(FLAGS_output));
  fout << html::CreateReport(root);
}
