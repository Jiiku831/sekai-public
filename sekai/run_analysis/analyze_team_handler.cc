#include "sekai/run_analysis/analyze_team_handler.h"

#include <memory>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_format.h"
#include "base/util.h"
#include "google/protobuf/repeated_field.h"
#include "sekai/card.h"
#include "sekai/config.h"
#include "sekai/db/master_db.h"
#include "sekai/event_bonus.h"
#include "sekai/proto/card_state.pb.h"
#include "sekai/run_analysis/handler.h"
#include "sekai/run_analysis/proto/service.pb.h"
#include "sekai/team.h"

namespace sekai::run_analysis {

namespace {

using ::google::protobuf::RepeatedField;
using ::google::protobuf::RepeatedPtrField;
using ::sekai::db::MasterDb;

struct ConstructedTeam {
  std::vector<std::unique_ptr<Card>> cards;
  std::vector<const Card*> card_ptrs;
  Team team;
};
absl::StatusOr<ConstructedTeam> ConstructTeam(const RepeatedPtrField<DeckCard>& deck_cards,
                                              const EventBonus& event_bonus, const Profile& profile,
                                              int skill_level = kSkillLevelMin) {
  std::vector<std::unique_ptr<Card>> cards;
  std::vector<const Card*> card_ptrs;
  for (const DeckCard& card : deck_cards) {
    CardState card_state;
    // Level does not matter for the purpose of this analysis.
    card_state.set_level(1);
    card_state.set_skill_level(skill_level);
    card_state.set_master_rank(card.master_rank());
    if (card_state.master_rank() < 0 || card_state.master_rank() > kMasterRankMax) {
      return absl::InvalidArgumentError(
          absl::StrFormat("Expected master rank to be within [0, 5], got %d for card %d",
                          card_state.master_rank(), card.card_id()));
    }
    ASSIGN_OR_RETURN(const auto* db_card, MasterDb::FindFirstOrError<db::Card>(card.card_id()));
    cards.push_back(std::make_unique<Card>(*db_card, card_state));
    if (card.trained_state() && cards.back()->HasSecondarySkill()) {
      cards.back()->UseSecondarySkill(true);
    }
    cards.back()->ApplyEventBonus(event_bonus);
    cards.back()->ApplyProfilePowerBonus(profile);
    card_ptrs.push_back(cards.back().get());
  }
  auto res = ConstructedTeam{
      .cards = std::move(cards),
      .card_ptrs = card_ptrs,
      .team = Team(card_ptrs),
  };
  return res;
}

absl::StatusOr<float> GetSkillLevelWithProfile(int skill_level, const EventBonus& event_bonus,
                                               const Profile& profile,
                                               const RepeatedPtrField<DeckCard>& cards,
                                               RepeatedField<int>& card_skills) {
  ASSIGN_OR_RETURN(ConstructedTeam team, ConstructTeam(cards, event_bonus, profile, skill_level));
  card_skills.Reserve(cards.size());
  std::vector<int> skill_values = team.team.GetSkillValues();
  card_skills = {skill_values.begin(), skill_values.end()};
  return team.team.SkillValue();
}

absl::StatusOr<AnalyzeTeamResponse::SkillDetails> GetSkillDetails(
    const RepeatedPtrField<DeckCard>& cards, const EventBonus& event_bonus) {
  AnalyzeTeamResponse::SkillDetails details;
  ASSIGN_OR_RETURN(float min_skill,
                   GetSkillLevelWithProfile(kSkillLevelMin, event_bonus, Profile::Min(), cards,
                                            *details.mutable_skill_lower_bounds()));
  details.set_skill_value_lower_bound(min_skill);
  ASSIGN_OR_RETURN(float max_skill,
                   GetSkillLevelWithProfile(kSkillLevelMax, event_bonus, Profile::Max(), cards,
                                            *details.mutable_skill_upper_bounds()));
  details.set_skill_value_upper_bound(max_skill);
  return details;
}

}  // namespace

absl::StatusOr<AnalyzeTeamResponse> AnalyzeTeamHandler::Run(
    const AnalyzeTeamRequest& request) const {
  if (request.cards().size() != kTeamSize) {
    return absl::InvalidArgumentError(
        absl::StrCat("Expected 5 cards, got ", request.cards().size()));
  }

  ASSIGN_OR_RETURN(const auto* event, MasterDb::FindFirstOrError<db::Event>(request.event_id()));
  EventBonus event_bonus(event);
  ASSIGN_OR_RETURN(ConstructedTeam team,
                   ConstructTeam(request.cards(), event_bonus, Profile::Min()));

  AnalyzeTeamResponse response;
  response.set_event_bonus(team.team.EventBonus(event_bonus));
  ASSIGN_OR_RETURN(*response.mutable_skill_details(),
                   GetSkillDetails(request.cards(), event_bonus));
  return response;
}

}  // namespace sekai::run_analysis
