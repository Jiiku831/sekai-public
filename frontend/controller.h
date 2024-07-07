#pragma once

#include <array>
#include <string>
#include <variant>

#include "sekai/bitset.h"
#include "sekai/challenge_live_estimator.h"
#include "sekai/db/proto/all.h"
#include "sekai/estimator.h"
#include "sekai/event_bonus.h"
#include "sekai/profile.h"
#include "sekai/proto/profile.pb.h"
#include "sekai/team_builder/constraints.h"

namespace frontend {

constexpr int kNumTeams = 2;
constexpr int kTeamSize = 5;

struct FilterState {
  FilterState() {
    attr.set();
    character.set();
    rarity.set();
  }

  // TODO: add unit and subunit
  sekai::Attr attr;
  sekai::Character character;
  sekai::CardRarityType rarity;
  bool unreleased_cards = false;
  bool owned_cards = false;

  bool Eval(const sekai::db::Card& card, bool owned) const;
};

class Controller {
 public:
  Controller();

  void ReadSaveData();
  void WriteSaveData() const;
  void DeleteSaveData() const;

  void SetAttrFilterState(int attr, bool state);
  void SetCharacterFilterState(int char_id, bool state);
  void SetRarityFilterState(int rarity, bool state);
  void SetUnreleasedContentFilterState(bool state);
  void SetOwnedCardsFilterState(bool state);
  void SetAreaItemLevel(int area_item_id, int level);
  void SetCharacterRank(int char_id, int rank);
  void SetTitleBonus(int bonus);

  void ImportCardsFromCsv(const std::string& cards_csv);
  void SetCardOwned(int card_id, bool state);
  void SetCardLevel(int card_id, int level);
  void SetCardMasterRank(int card_id, int level);
  void SetCardSkillLevel(int card_id, int level);
  void SetCardTrained(int card_id, bool state);
  void SetCardEpisodeRead(int card_id, int episode_id, bool state);

  void SetEventBonusByEvent(int event_id, int chapter_id);
  void SetCustomEventAttr(int attr);
  void SetCustomEventCharacter(int char_id, int unit_id, bool state);
  void SetCustomEventChapter(int char_id);
  void SetLeadConstraint(int char_id, bool state);
  void SetKizunaConstraint(int char_1, int char_2, bool state);
  void SetRarityConstraint(int rarity, bool state);
  void SetMinLeadSkill(int value);

  void BuildEventTeam();
  void BuildChallengeLiveTeam(int char_id);

  bool IsValidCard(int card_id) const;
  void SetTeamCard(int team_index, int card_index, int card_id);
  void ClearTeamCard(int team_index, int card_index);

  void RefreshTeams() const;

  void SetUseOldSubunitlessBonus(bool state);

 private:
  sekai::ProfileProto profile_proto_ = sekai::EmptyProfileProto();
  sekai::Profile profile_{profile_proto_};
  FilterState card_list_filter_state_;
  std::array<std::array<int, kTeamSize>, kNumTeams> teams_{};
  sekai::Estimator::Mode estimator_mode_ = sekai::Estimator::Mode::kMulti;
  sekai::Estimator estimator_ = sekai::RandomExEstimator(sekai::Estimator::Mode::kMulti);
  sekai::Estimator cc_estimator_ = sekai::RandomExEstimator(sekai::Estimator::Mode::kCheerful);
  sekai::ChallengeLiveEstimator cl_estimator_;
  sekai::EventBonus event_bonus_;
  sekai::Constraints constraints_;

  const sekai::Estimator& estimator() const {
    return estimator_mode_ == sekai::Estimator::Mode::kCheerful ? cc_estimator_ : estimator_;
  }

  const std::variant<sekai::EventId, sekai::SimpleEventBonus> event_bonus_source() const {
    if (profile_proto_.event_id().event_id() > 0) return profile_proto_.event_id();
    return profile_proto_.custom_event();
  };

  void UpdateCardListVisibilities(FilterState new_state, bool force_update = false);
  void UpdateProfile();
  sekai::CardState* GetCardState(int card_id);
  void RefreshTeam(int team_index) const;
};

}  // namespace frontend
