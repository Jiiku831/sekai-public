#include "sekai/max_character_rank.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <limits>
#include <optional>
#include <span>
#include <vector>

#include "absl/log/absl_check.h"
#include "absl/time/time.h"
#include "sekai/array_size.h"
#include "sekai/challenge_live.h"
#include "sekai/character.h"
#include "sekai/config.h"
#include "sekai/db/master_db.h"
#include "sekai/proto/max_character_rank.pb.h"
#include "sekai/proto_util.h"
#include "sekai/version.h"

namespace sekai {
namespace {

constexpr int kMaxChallengePts = 1030;
constexpr int kPre4thAnniExPts = 9300;
constexpr int kPre4thAnniCap = 101;
constexpr int kCharacterRankXpIncrement = 10;
constexpr std::array kCharacterRankXpRequirement = {
    1,  2,  4,  7,  10,  14,  18,  22,  26,  30,  35,  40,  45,  50,  55,  61,  67,  73,
    79, 85, 92, 99, 106, 113, 120, 128, 136, 144, 152, 160, 169, 178, 187, 196, 205,
};

int GetMaxChallengeLiveStage(int char_id, absl::Time time) {
  std::vector<int> pt_reqs = GetChallengeLiveStagePointRequirement(char_id);
  int num_days_since_4th_anni_uncap =
      time < Get4thAnniReleaseTime()
          ? 0
          : std::max(std::ceil((time - Get4thAnniResetTime()) / absl::Hours(24)), 1.0);
  ABSL_CHECK_LT(static_cast<std::size_t>(kPre4thAnniCap), pt_reqs.size());
  int pre_4th_anni_max_pts = pt_reqs[kPre4thAnniCap] + kPre4thAnniExPts - 1;
  int max_pt_gain = num_days_since_4th_anni_uncap * kMaxChallengePts;
  int max_theoretical_pts = pre_4th_anni_max_pts + max_pt_gain;
  for (std::size_t rank = 0; rank < pt_reqs.size(); ++rank) {
    if (max_theoretical_pts < pt_reqs[rank]) {
      return rank - 1;
    }
  }
  return pt_reqs.size() - 1;
}

bool WorldLink2ChapterStarted(int char_id, absl::Time time) {
  std::span<const db::WorldBloom> chapters = db::MasterDb::GetAll<db::WorldBloom>();
  for (const db::WorldBloom& chapter : chapters) {
    absl::Time start_time = absl::FromUnixMillis(chapter.chapter_start_at());
    if (chapter.event_id() < 160) continue;
    if (chapter.event_id() > 180) continue;
    if (start_time > time) continue;
    if (chapter.game_character_id() != char_id) continue;
    return true;
  }
  return false;
}

int GetProgress(int char_id, CharacterRankSource::OtherSource source, absl::Time time) {
  switch (source) {
    case CharacterRankSource::OTHER_SOURCE_CHALLENGE_LIVE:
      return GetMaxChallengeLiveStage(char_id, time);
    case CharacterRankSource::OTHER_SOURCE_ANNI_2_STAMP:
      return GetAssetVersionAt(time) >= kAnni2AssetVersion ? 3 : 0;
    case CharacterRankSource::OTHER_SOURCE_ANNI_3_STAMP:
      return GetAssetVersionAt(time) >= kAnni3AssetVersion ? 3 : 0;
    case CharacterRankSource::OTHER_SOURCE_WORLD_LINK:
      return GetAssetVersionAt(time) >= kEndOfWlAssetVersion ? 2 : 0;
    case CharacterRankSource::OTHER_SOURCE_ANNI_4_STAMP:
      return GetAssetVersionAt(time) >= kAnni4AssetVersion ? 2 : 0;
    case CharacterRankSource::OTHER_SOURCE_ANNI_4_MEMORIAL_SELECT:
      return GetAssetVersionAt(time) >= kAnni4AssetVersion ? 1 : 0;
    case CharacterRankSource::OTHER_SOURCE_MOVIE_STAMP:
      return GetAssetVersionAt(time) >= kMovieAssetVersion ? 2 : 0;
    case CharacterRankSource::OTHER_SOURCE_ANNI_4_5_STAMP:
      return GetAssetVersionAt(time) >= kAnni4p5AssetVersion ? 2 : 0;
    case CharacterRankSource::OTHER_SOURCE_WORLD_LINK_2:
      return WorldLink2ChapterStarted(char_id, time) ? 2 : 0;
    case CharacterRankSource::OTHER_SOURCE_ANNI_5_STAMP:
      return GetAssetVersionAt(time) >= kAnni5AssetVersion ? 2 : 0;
    case CharacterRankSource::OTHER_SOURCE_ANNI_5_MEMORIAL_SELECT:
      return GetAssetVersionAt(time) >= kAnni5AssetVersion ? 1 : 0;
    default:
      ABSL_CHECK(false) << "unhandled case";
  }
  return 0;
}

std::optional<int> GetMaxProgress(int char_id, CharacterRankSource::OtherSource source,
                                  absl::Time time) {
  switch (source) {
    case CharacterRankSource::OTHER_SOURCE_CHALLENGE_LIVE:
      return GetChallengeLiveStagePointRequirement(char_id).size() - 1;
    case CharacterRankSource::OTHER_SOURCE_ANNI_2_STAMP:
      return GetAssetVersionAt(time) >= kAnni2AssetVersion ? 3 : 0;
    case CharacterRankSource::OTHER_SOURCE_ANNI_3_STAMP:
      return GetAssetVersionAt(time) >= kAnni3AssetVersion ? 3 : 0;
    case CharacterRankSource::OTHER_SOURCE_WORLD_LINK:
      return GetAssetVersionAt(time) >= kEndOfWlAssetVersion ? 2 : 0;
    case CharacterRankSource::OTHER_SOURCE_ANNI_4_STAMP:
      return GetAssetVersionAt(time) >= kAnni4AssetVersion ? 2 : 0;
    case CharacterRankSource::OTHER_SOURCE_ANNI_4_MEMORIAL_SELECT:
      return GetAssetVersionAt(time) >= kAnni4AssetVersion ? 1 : 0;
    case CharacterRankSource::OTHER_SOURCE_MOVIE_STAMP:
      return GetAssetVersionAt(time) >= kMovieAssetVersion ? 2 : 0;
    case CharacterRankSource::OTHER_SOURCE_ANNI_4_5_STAMP:
      return GetAssetVersionAt(time) >= kAnni4p5AssetVersion ? 2 : 0;
    case CharacterRankSource::OTHER_SOURCE_WORLD_LINK_2:
      return GetAssetVersionAt(time) >= kAnni4AssetVersion ? 2 : 0;
    case CharacterRankSource::OTHER_SOURCE_ANNI_5_STAMP:
      return GetAssetVersionAt(time) >= kAnni5AssetVersion ? 2 : 0;
    case CharacterRankSource::OTHER_SOURCE_ANNI_5_MEMORIAL_SELECT:
      return GetAssetVersionAt(time) >= kAnni5AssetVersion ? 1 : 0;
    default:
      ABSL_CHECK(false) << "unhandled case";
  }
  return std::nullopt;
}

int ProgressToXp(int char_id, CharacterRankSource::OtherSource source, int progress) {
  switch (source) {
    case CharacterRankSource::OTHER_SOURCE_CHALLENGE_LIVE:
      return progress - 1;
    case CharacterRankSource::OTHER_SOURCE_ANNI_2_STAMP:
    case CharacterRankSource::OTHER_SOURCE_ANNI_3_STAMP:
    case CharacterRankSource::OTHER_SOURCE_WORLD_LINK:
    case CharacterRankSource::OTHER_SOURCE_ANNI_4_STAMP:
    case CharacterRankSource::OTHER_SOURCE_ANNI_4_MEMORIAL_SELECT:
    case CharacterRankSource::OTHER_SOURCE_MOVIE_STAMP:
    case CharacterRankSource::OTHER_SOURCE_ANNI_4_5_STAMP:
    case CharacterRankSource::OTHER_SOURCE_WORLD_LINK_2:
    case CharacterRankSource::OTHER_SOURCE_ANNI_5_STAMP:
    case CharacterRankSource::OTHER_SOURCE_ANNI_5_MEMORIAL_SELECT:
      return progress;
    default:
      ABSL_CHECK(false) << "unhandled case";
  }
  return 0;
}

std::vector<int> GetValidAreaItemsForMission(int char_id, db::CharacterMissionType type) {
  db::Unit unit = LookupCharacterUnit(char_id);
  std::vector<const db::CharacterMissionV2AreaItem*> items =
      db::MasterDb::FindAll<db::CharacterMissionV2AreaItem>(type);
  std::vector<int> area_item_ids;
  for (const db::CharacterMissionV2AreaItem* item : items) {
    if ((item->has_character_id() && item->character_id() != char_id) ||
        (item->has_unit() && item->unit() != unit)) {
      continue;
    }
    area_item_ids.push_back(item->area_item_id());
  }
  return area_item_ids;
}

int GetMaxAreaItemLevelForMission(int char_id, db::CharacterMissionType type) {
  std::vector<int> area_item_ids = GetValidAreaItemsForMission(char_id, type);
  ABSL_CHECK(!area_item_ids.empty());
  int total_levels = 0;
  for (int area_item_id : area_item_ids) {
    std::vector<const db::AreaItemLevel*> levels =
        db::MasterDb::FindAll<db::AreaItemLevel>(area_item_id);
    total_levels += levels.size();
  }
  return total_levels;
}

int IsExMission(db::CharacterMissionType source) {
  switch (source) {
    case db::CHARACTER_MISSION_TYPE_PLAY_LIVE_EX:
    case db::CHARACTER_MISSION_TYPE_WAITING_ROOM_EX:
      return true;
    default:
      return false;
  }
}

int CountAlts(int char_id, absl::Time time) {
  std::span<const db::MusicVocal> vocals = db::MasterDb::GetAll<db::MusicVocal>();
  int count = 0;
  for (const db::MusicVocal& vocal : vocals) {
    if (vocal.music_vocal_type() != db::MusicVocal::ANOTHER_VOCAL) continue;
    absl::Time publish_time = absl::FromUnixMillis(vocal.archive_published_at());
    if (publish_time > time) continue;
    for (const db::MusicVocal::Character& character : vocal.characters()) {
      if (character.character_type() == db::MusicVocal::Character::GAME_CHARACTER &&
          character.character_id() == char_id) {
        ++count;
        break;
      }
    }
  }
  return count;
}

int CountCostumes(int char_id, absl::Time time) {
  std::vector<const db::Costume3D*> costumes = db::MasterDb::FindAll<db::Costume3D>(char_id);
  int count = 0;
  for (const db::Costume3D* costume : costumes) {
    absl::Time publish_time = absl::FromUnixMillis(costume->published_at());
    if (publish_time > time) continue;
    if (costume->part_type() != db::Costume3D::BODY) continue;
    ++count;
  }
  return count;
}

absl::flat_hash_map<db::CardRarityType, int> CardCount(int char_id, absl::Time time) {
  std::span<const db::Card> cards = db::MasterDb::GetAll<db::Card>();
  absl::flat_hash_map<db::CardRarityType, int> count;
  for (const db::Card& card : cards) {
    absl::Time publish_time = absl::FromUnixMillis(card.release_at());
    if (publish_time > time) continue;
    if (char_id != card.character_id()) continue;
    ++count[card.card_rarity_type()];
  }
  return count;
}

int CountTrainedOnlyCards(int char_id, absl::Time time) {
  std::span<const db::Card> cards = db::MasterDb::GetAll<db::Card>();
  int count = 0;
  for (const db::Card& card : cards) {
    absl::Time publish_time = absl::FromUnixMillis(card.release_at());
    if (publish_time > time) continue;
    if (char_id != card.character_id()) continue;
    if (card.initial_special_training_status() != db::Card::INITIAL_SPECIAL_TRAINING_TRUE) continue;
    ++count;
  }
  return count;
}

int AlbumRarityFactor(db::CardRarityType rarity) {
  switch (rarity) {
    case db::RARITY_1:
    case db::RARITY_2:
    case db::RARITY_BD:
      return 1;
    case db::RARITY_3:
    case db::RARITY_4:
      return 2;
    default:
      ABSL_CHECK(false) << "unhandled case";
      return 0;
  }
}

int IsRare(db::CardRarityType rarity) {
  switch (rarity) {
    case db::RARITY_BD:
    case db::RARITY_4:
      return true;
    default:
      return false;
  }
}

int IsCommon(db::CardRarityType rarity) {
  switch (rarity) {
    case db::RARITY_1:
    case db::RARITY_2:
    case db::RARITY_3:
      return true;
    default:
      return false;
  }
}

int CountAlbumMember(int char_id, absl::Time time) {
  absl::flat_hash_map<db::CardRarityType, int> counts = CardCount(char_id, time);
  int total = 0;
  for (const auto& [rarity, count] : counts) {
    total += count * AlbumRarityFactor(rarity);
  }
  return total - CountTrainedOnlyCards(char_id, time);
}

int CountRareMember(int char_id, absl::Time time) {
  absl::flat_hash_map<db::CardRarityType, int> counts = CardCount(char_id, time);
  int total = 0;
  for (const auto& [rarity, count] : counts) {
    if (!IsRare(rarity)) {
      continue;
    }
    total += count;
  }
  return total;
}

int CountCommonMember(int char_id, absl::Time time) {
  absl::flat_hash_map<db::CardRarityType, int> counts = CardCount(char_id, time);
  int total = 0;
  for (const auto& [rarity, count] : counts) {
    if (!IsCommon(rarity)) {
      continue;
    }
    total += count;
  }
  return total;
}

int CountStamps(int char_id, absl::Time time) {
  std::span<const db::Stamp> stamps = db::MasterDb::GetAll<db::Stamp>();
  int count = 0;
  for (const db::Stamp& stamp : stamps) {
    absl::Time publish_time = absl::FromUnixMillis(stamp.archive_published_at());
    if (publish_time > time) continue;
    if (stamp.character_id1() == char_id || stamp.character_id2() == char_id) {
      ++count;
    }
  }
  return count;
}

int CountAreaConvos(int char_id, absl::Time time) {
  std::span<const db::ActionSet> action_sets = db::MasterDb::GetAll<db::ActionSet>();
  int count = 0;
  for (const db::ActionSet& action_set : action_sets) {
    absl::Time publish_time = absl::FromUnixMillis(action_set.archive_published_at());
    if (publish_time > time) continue;
    for (int character2d_id : action_set.character2d_ids()) {
      const auto& character = db::MasterDb::FindFirst<db::Character2D>(character2d_id);
      if (character.character_id() == char_id) {
        ++count;
        break;
      }
    }
  }
  return count;
}

int CountCardEpisodes(int char_id, db::CardEpisode::PartType part, absl::Time time) {
  std::span<const db::Card> cards = db::MasterDb::GetAll<db::Card>();
  int count = 0;
  for (const db::Card& card : cards) {
    absl::Time publish_time = absl::FromUnixMillis(card.release_at());
    if (publish_time > time) continue;
    if (char_id != card.character_id()) continue;

    std::vector<const db::CardEpisode*> episodes =
        db::MasterDb::FindAll<db::CardEpisode>(card.id());
    for (const db::CardEpisode* episode : episodes) {
      if (episode->card_episode_part_type() == part) {
        ++count;
      }
    }
  }
  return count;
}

int CountVoiceLine(int char_id, absl::Time time) {
  std::vector<const db::CharacterArchiveVoice*> voices =
      db::MasterDb::FindAll<db::CharacterArchiveVoice>(char_id);
  int count = 0;
  for (const db::CharacterArchiveVoice* voice : voices) {
    absl::Time publish_time = absl::FromUnixMillis(voice->display_start_at());
    if (publish_time > time) continue;
    if (voice->voice_type() == db::CharacterArchiveVoice::COMMENT_LIVE_TOP ||
        voice->voice_type() == db::CharacterArchiveVoice::COMMENT_LOGIN_BONUS ||
        voice->voice_type() == db::CharacterArchiveVoice::PRACTICE) {
      continue;
    }
    ++count;
  }
  return count;
}

std::vector<int> GetGameCharacterUnitIds(const db::MySekaiGameCharacterUnitGroup& grp) {
  std::vector<int> ids;
  if (grp.has_game_character_unit_id1()) {
    ids.push_back(grp.game_character_unit_id1());
  }
  if (grp.has_game_character_unit_id2()) {
    ids.push_back(grp.game_character_unit_id2());
  }
  if (grp.has_game_character_unit_id3()) {
    ids.push_back(grp.game_character_unit_id3());
  }
  if (grp.has_game_character_unit_id4()) {
    ids.push_back(grp.game_character_unit_id4());
  }
  if (grp.has_game_character_unit_id5()) {
    ids.push_back(grp.game_character_unit_id5());
  }
  return ids;
}

int CountMySekaiTalks(int char_id) {
  // TODO: check if there is an archive for this as well
  absl::flat_hash_set<std::string> unique_talks;
  for (const auto& talk : db::MasterDb::GetAll<db::MySekaiCharacterTalk>()) {
    bool match = false;
    for (int game_character_unit_id :
         GetGameCharacterUnitIds(db::MasterDb::FindFirst<db::MySekaiGameCharacterUnitGroup>(
             talk.game_character_unit_group_id()))) {
      const auto& game_character_unit =
          db::MasterDb::FindFirst<db::GameCharacterUnit>(game_character_unit_id);
      if (game_character_unit.game_character_id() == char_id) {
        match = true;
        break;
      }
    }
    if (!match) continue;

    // Check is fixture talk
    const auto& condition_group =
        db::MasterDb::FindFirst<db::MySekaiCharacterTalkConditionGroup>(talk.condition_group_id());
    const auto& condition =
        db::MasterDb::FindFirst<db::MySekaiCharacterTalkCondition>(condition_group.condition_id());
    if (condition.condition_type() == db::MySekaiCharacterTalkCondition::MYSEKAI_FIXTURE_ID) {
      unique_talks.insert(talk.lua());
    }
  }
  return unique_talks.size();
}

std::vector<int> GetFixtureTagIds(const db::MySekaiFixture& fixture) {
  std::vector<int> ids;
  ids.reserve(4);
  if (fixture.mysekai_fixture_tag_group().has_mysekai_fixture_tag_id1()) {
    ids.push_back(fixture.mysekai_fixture_tag_group().mysekai_fixture_tag_id1());
  }
  if (fixture.mysekai_fixture_tag_group().has_mysekai_fixture_tag_id2()) {
    ids.push_back(fixture.mysekai_fixture_tag_group().mysekai_fixture_tag_id2());
  }
  if (fixture.mysekai_fixture_tag_group().has_mysekai_fixture_tag_id3()) {
    ids.push_back(fixture.mysekai_fixture_tag_group().mysekai_fixture_tag_id3());
  }
  if (fixture.mysekai_fixture_tag_group().has_mysekai_fixture_tag_id4()) {
    ids.push_back(fixture.mysekai_fixture_tag_group().mysekai_fixture_tag_id4());
  }
  return ids;
}

int CountMySekaiFixtures(int char_id) {
  // TODO: check if there is an archive for this as well
  int count = 0;
  for (const auto& fixture : db::MasterDb::GetAll<db::MySekaiFixture>()) {
    for (int tag_id : GetFixtureTagIds(fixture)) {
      const auto& tag = db::MasterDb::FindFirst<db::MySekaiFixtureTag>(tag_id);
      if (tag.tag_type() == db::MySekaiFixtureTag::GAME_CHARACTER && tag.external_id() == char_id) {
        ++count;
        break;
      }
    }
  }
  return count;
}

int CountMembers(int char_id, absl::Time time) {
  return CountRareMember(char_id, time) + CountCommonMember(char_id, time);
}

int GetProgress(int char_id, db::CharacterMissionType source, absl::Time time) {
  switch (source) {
    case db::CHARACTER_MISSION_TYPE_AREA_ITEM_LEVEL_UP_CHARACTER:
    case db::CHARACTER_MISSION_TYPE_AREA_ITEM_LEVEL_UP_REALITY_WORLD:
    case db::CHARACTER_MISSION_TYPE_AREA_ITEM_LEVEL_UP_UNIT:
      return GetMaxAreaItemLevelForMission(char_id, source);
    case db::CHARACTER_MISSION_TYPE_PLAY_LIVE:
    case db::CHARACTER_MISSION_TYPE_PLAY_LIVE_EX:
    case db::CHARACTER_MISSION_TYPE_WAITING_ROOM:
    case db::CHARACTER_MISSION_TYPE_WAITING_ROOM_EX:
      return std::numeric_limits<int>::max();
    case db::CHARACTER_MISSION_TYPE_COLLECT_ANOTHER_VOCAL:
      return CountAlts(char_id, time);
    case db::CHARACTER_MISSION_TYPE_COLLECT_COSTUME_3D:
      return CountCostumes(char_id, time);
    case db::CHARACTER_MISSION_TYPE_COLLECT_MEMBER:
      return CountAlbumMember(char_id, time);
    case db::CHARACTER_MISSION_TYPE_COLLECT_STAMP:
      return CountStamps(char_id, time);
    case db::CHARACTER_MISSION_TYPE_MASTER_RANK_UP_RARE:
      return CountRareMember(char_id, time) * kMasterRankMax;
    case db::CHARACTER_MISSION_TYPE_MASTER_RANK_UP_STANDARD:
      return CountCommonMember(char_id, time) * kMasterRankMax;
    case db::CHARACTER_MISSION_TYPE_READ_AREA_TALK:
      return CountAreaConvos(char_id, time);
    case db::CHARACTER_MISSION_TYPE_READ_CARD_EPISODE_FIRST:
      return CountCardEpisodes(char_id, db::CardEpisode::FIRST_PART, time);
    case db::CHARACTER_MISSION_TYPE_READ_CARD_EPISODE_SECOND:
      return CountCardEpisodes(char_id, db::CardEpisode::SECOND_PART, time);
    case db::CHARACTER_MISSION_TYPE_SKILL_LEVEL_UP_RARE:
      return CountRareMember(char_id, time) * (kSkillLevelMax - 1);
    case db::CHARACTER_MISSION_TYPE_SKILL_LEVEL_UP_STANDARD:
      return CountCommonMember(char_id, time) * (kSkillLevelMax - 1);
    case db::CHARACTER_MISSION_TYPE_COLLECT_CHARACTER_ARCHIVE_VOICE:
      return CountVoiceLine(char_id, time);
    case db::CHARACTER_MISSION_TYPE_COLLECT_MYSEKAI_FIXTURE:
      return CountMySekaiFixtures(char_id);
    case db::CHARACTER_MISSION_TYPE_COLLECT_MYSEKAI_CANVAS:
      return CountMembers(char_id, time);
    case db::CHARACTER_MISSION_TYPE_READ_MYSEKAI_FIXTURE_TALK:
      return CountMySekaiTalks(char_id);
    default:
      ABSL_CHECK(false) << "unhandled case";
  }
  return 0;
}

std::vector<db::CharacterMissionV2ParameterGroup> ExpandGroup(
    std::span<const db::CharacterMissionV2ParameterGroup*> params, bool is_ex_mission) {
  std::sort(
      params.begin(), params.end(),
      [](const db::CharacterMissionV2ParameterGroup* lhs,
         const db::CharacterMissionV2ParameterGroup* rhs) { return lhs->seq() < rhs->seq(); });

  std::vector<db::CharacterMissionV2ParameterGroup> new_params;
  int seq = 1;
  std::vector<int> cumulative_req;
  for (const db::CharacterMissionV2ParameterGroup* param : params) {
    while (seq < param->seq()) {
      ABSL_CHECK(!new_params.empty()) << "no param to replicate";
      const db::CharacterMissionV2ParameterGroup& last_params = new_params.back();
      new_params.push_back(last_params);
      new_params.back().set_seq(seq);
      if (is_ex_mission) {
        cumulative_req.push_back(cumulative_req.back() + new_params.back().requirement());
      }
      ++seq;
    }
    if (param->exp() == 0) {
      break;
    }
    new_params.push_back(*param);
    if (is_ex_mission) {
      cumulative_req.push_back((cumulative_req.empty() ? 0 : cumulative_req.back()) +
                               new_params.back().requirement());
    }
    ++seq;
  }
  if (is_ex_mission) {
    ABSL_CHECK_EQ(new_params.size(), cumulative_req.size());
    for (std::size_t i = 0; i < new_params.size(); ++i) {
      new_params[i].set_requirement(cumulative_req[i]);
    }
  }
  return new_params;
}

std::optional<int> GetMaxProgressFromMissionParams(int char_id, db::CharacterMissionType source) {
  std::vector<const db::CharacterMissionV2*> missions =
      db::MasterDb::FindAll<db::CharacterMissionV2>(source);
  const db::CharacterMissionV2* char_mission = nullptr;
  for (const db::CharacterMissionV2* mission : missions) {
    if (mission->character_id() == char_id) {
      char_mission = mission;
      break;
    }
  }
  if (char_mission == nullptr) {
    LOG(WARNING) << "Cannot find character " << char_id << " mission with type "
                 << db::CharacterMissionType_Name(source);
    return std::nullopt;
  }
  std::vector<const db::CharacterMissionV2ParameterGroup*> params =
      db::MasterDb::FindAll<db::CharacterMissionV2ParameterGroup>(
          char_mission->parameter_group_id());
  ABSL_CHECK(!params.empty()) << "Cannot find character " << char_id << " mission param group "
                              << char_mission->parameter_group_id() << " with type "
                              << db::CharacterMissionType_Name(source);
  std::vector<db::CharacterMissionV2ParameterGroup> expanded_params =
      ExpandGroup(std::span(params), IsExMission(source));
  ABSL_CHECK(!expanded_params.empty()) << "unexpected error";
  return expanded_params.back().requirement();
}

std::optional<int> GetMaxProgress(int char_id, db::CharacterMissionType source, absl::Time time) {
  switch (source) {
    case db::CHARACTER_MISSION_TYPE_AREA_ITEM_LEVEL_UP_CHARACTER:
    case db::CHARACTER_MISSION_TYPE_AREA_ITEM_LEVEL_UP_REALITY_WORLD:
    case db::CHARACTER_MISSION_TYPE_AREA_ITEM_LEVEL_UP_UNIT:
      return GetMaxAreaItemLevelForMission(char_id, source);
    case db::CHARACTER_MISSION_TYPE_PLAY_LIVE:
    case db::CHARACTER_MISSION_TYPE_PLAY_LIVE_EX:
    case db::CHARACTER_MISSION_TYPE_WAITING_ROOM:
    case db::CHARACTER_MISSION_TYPE_WAITING_ROOM_EX:
    case db::CHARACTER_MISSION_TYPE_COLLECT_ANOTHER_VOCAL:
    case db::CHARACTER_MISSION_TYPE_COLLECT_COSTUME_3D:
    case db::CHARACTER_MISSION_TYPE_COLLECT_MEMBER:
    case db::CHARACTER_MISSION_TYPE_COLLECT_STAMP:
    case db::CHARACTER_MISSION_TYPE_MASTER_RANK_UP_RARE:
    case db::CHARACTER_MISSION_TYPE_MASTER_RANK_UP_STANDARD:
    case db::CHARACTER_MISSION_TYPE_READ_AREA_TALK:
    case db::CHARACTER_MISSION_TYPE_READ_CARD_EPISODE_FIRST:
    case db::CHARACTER_MISSION_TYPE_READ_CARD_EPISODE_SECOND:
    case db::CHARACTER_MISSION_TYPE_SKILL_LEVEL_UP_RARE:
    case db::CHARACTER_MISSION_TYPE_SKILL_LEVEL_UP_STANDARD:
    case db::CHARACTER_MISSION_TYPE_COLLECT_CHARACTER_ARCHIVE_VOICE:
    case db::CHARACTER_MISSION_TYPE_COLLECT_MYSEKAI_FIXTURE:
    case db::CHARACTER_MISSION_TYPE_COLLECT_MYSEKAI_CANVAS:
    case db::CHARACTER_MISSION_TYPE_READ_MYSEKAI_FIXTURE_TALK:
      return GetMaxProgressFromMissionParams(char_id, source);
    default:
      ABSL_CHECK(false) << "unhandled case";
  }
  return std::nullopt;
}

int ProgressToXp(int char_id, db::CharacterMissionType source, int progress) {
  std::vector<const db::CharacterMissionV2*> missions =
      db::MasterDb::FindAll<db::CharacterMissionV2>(source);
  const db::CharacterMissionV2* char_mission = nullptr;
  for (const db::CharacterMissionV2* mission : missions) {
    if (mission->character_id() == char_id) {
      char_mission = mission;
      break;
    }
  }
  ABSL_CHECK_NE(char_mission, nullptr)
      << "Cannot find character " << char_id << " mission with type "
      << db::CharacterMissionType_Name(source);
  std::vector<const db::CharacterMissionV2ParameterGroup*> params =
      db::MasterDb::FindAll<db::CharacterMissionV2ParameterGroup>(
          char_mission->parameter_group_id());
  ABSL_CHECK(!params.empty()) << "Cannot find character " << char_id << " mission param group "
                              << char_mission->parameter_group_id() << " with type "
                              << db::CharacterMissionType_Name(source);
  std::vector<db::CharacterMissionV2ParameterGroup> expanded_params =
      ExpandGroup(std::span(params), IsExMission(source));
  int xp = 0;
  for (const db::CharacterMissionV2ParameterGroup& param : expanded_params) {
    if (progress < param.requirement()) {
      break;
    }
    xp += param.exp();
  }
  return xp;
}

void SetSource(CharacterRankSource::OtherSource source, CharacterRankSource& cr_source) {
  cr_source.set_other_source(source);
}

void SetSource(db::CharacterMissionType source, CharacterRankSource& cr_source) {
  cr_source.set_character_mission_source(source);
}

template <typename T>
std::optional<CharacterRankSource> GetMaxCharacterRankForSource(int char_id, T source,
                                                                absl::Time time) {
  std::optional<int> max_progress = GetMaxProgress(char_id, source, time);
  if (!max_progress.has_value()) {
    return std::nullopt;
  }
  CharacterRankSource cr_source;
  SetSource(source, cr_source);
  cr_source.set_progress(GetProgress(char_id, source, time));
  cr_source.set_max_progress(*max_progress);
  cr_source.set_current_xp(ProgressToXp(char_id, source, cr_source.progress()));
  cr_source.set_max_xp(ProgressToXp(char_id, source, cr_source.max_progress()));
  return cr_source;
}

MaxCharacterRank::Rank GetRank(int xp) {
  int rank = 1;
  int last_req = 0;
  for (int req : kCharacterRankXpRequirement) {
    if (xp < req) {
      break;
    }
    last_req = req;
    ++rank;
  }
  int next_req = kCharacterRankXpRequirement.back() + kCharacterRankXpIncrement;
  while (rank < kMaxCharacterRank && xp >= next_req) {
    ++rank;
    last_req = next_req;
    next_req += kCharacterRankXpIncrement;
  }
  MaxCharacterRank::Rank rank_proto;
  rank_proto.set_rank(rank);
  rank_proto.set_excess_xp(xp - last_req);
  return rank_proto;
}

MaxCharacterRank GetMaxCharacterRank(int char_id, absl::Time time) {
  MaxCharacterRank max_rank;
  max_rank.set_character_id(char_id);

  int total_xp = 0;
  int total_max_xp = 0;
  for (db::CharacterMissionType source :
       EnumValuesExcludingDefault<db::CharacterMissionType,
                                  db::CharacterMissionType_descriptor>()) {
    std::optional<CharacterRankSource> cr_source =
        GetMaxCharacterRankForSource(char_id, source, time);
    if (!cr_source.has_value()) {
      continue;
    }
    total_xp += cr_source->current_xp();
    total_max_xp += cr_source->max_xp();
    *max_rank.add_sources() = *std::move(cr_source);
  }
  for (CharacterRankSource::OtherSource source :
       EnumValuesExcludingDefault<CharacterRankSource::OtherSource,
                                  CharacterRankSource::OtherSource_descriptor>()) {
    std::optional<CharacterRankSource> cr_source =
        GetMaxCharacterRankForSource(char_id, source, time);
    if (!cr_source.has_value()) {
      continue;
    }
    total_xp += cr_source->current_xp();
    total_max_xp += cr_source->max_xp();
    *max_rank.add_sources() = *std::move(cr_source);
  }
  max_rank.set_current_xp(total_xp);
  max_rank.set_max_xp(total_max_xp);
  *max_rank.mutable_current_rank() = GetRank(total_xp);
  *max_rank.mutable_max_rank() = GetRank(total_max_xp);

  // Adjust for 168 stamp.
  if (max_rank.current_rank().rank() < 168 ||
      (max_rank.current_rank().rank() == 168 && max_rank.current_rank().excess_xp() == 0)) {
    for (CharacterRankSource& source : *max_rank.mutable_sources()) {
      if (source.character_mission_source() == db::CHARACTER_MISSION_TYPE_COLLECT_STAMP) {
        source.set_progress(source.progress() - 1);
        source.set_current_xp(source.current_xp() - 1);
      }
    }
    total_xp -= 1;
    *max_rank.mutable_current_rank() = GetRank(total_xp);
  }

  return max_rank;
}

}  // namespace

std::vector<MaxCharacterRank> GetMaxCharacterRanks(absl::Time time) {
  std::vector<MaxCharacterRank> ranks(CharacterArraySize());
  for (int char_id : UniqueCharacterIds()) {
    ranks[char_id] = GetMaxCharacterRank(char_id, time);
  }
  return ranks;
}

std::optional<CharacterRankSource> GetCharacterRankSource(const MaxCharacterRank& rank,
                                                          CharacterRankSource::OtherSource source) {
  for (const CharacterRankSource& cr_source : rank.sources()) {
    if (cr_source.other_source() == source) {
      return cr_source;
    }
  }
  return std::nullopt;
}

std::optional<CharacterRankSource> GetCharacterRankSource(const MaxCharacterRank& rank,
                                                          db::CharacterMissionType mission) {
  for (const CharacterRankSource& cr_source : rank.sources()) {
    if (cr_source.character_mission_source() == mission) {
      return cr_source;
    }
  }
  return std::nullopt;
}

std::string SourceDescription(CharacterRankSource::OtherSource source) {
  switch (source) {
    case CharacterRankSource::OTHER_SOURCE_CHALLENGE_LIVE:
      return "CL";
    case CharacterRankSource::OTHER_SOURCE_ANNI_2_STAMP:
      return "2nd Anni";
    case CharacterRankSource::OTHER_SOURCE_ANNI_3_STAMP:
      return "3rd Anni";
    case CharacterRankSource::OTHER_SOURCE_WORLD_LINK:
      return "WL";
    case CharacterRankSource::OTHER_SOURCE_ANNI_4_STAMP:
      return "4th Anni";
    case CharacterRankSource::OTHER_SOURCE_ANNI_4_MEMORIAL_SELECT:
      return "4th Gacha";
    case CharacterRankSource::OTHER_SOURCE_MOVIE_STAMP:
      return "Movie";
    case CharacterRankSource::OTHER_SOURCE_ANNI_4_5_STAMP:
      return "4.5 Anni";
    case CharacterRankSource::OTHER_SOURCE_WORLD_LINK_2:
      return "WL2";
    case CharacterRankSource::OTHER_SOURCE_ANNI_5_STAMP:
      return "5th Anni";
    case CharacterRankSource::OTHER_SOURCE_ANNI_5_MEMORIAL_SELECT:
      return "5th Gacha";
    default:
      ABSL_CHECK(false) << "unhandled case";
  }
  return "";
}

std::string SourceDescription(db::CharacterMissionType source) {
  switch (source) {
    case db::CHARACTER_MISSION_TYPE_AREA_ITEM_LEVEL_UP_CHARACTER:
      return "Char Items";
    case db::CHARACTER_MISSION_TYPE_AREA_ITEM_LEVEL_UP_REALITY_WORLD:
      return "Plants";
    case db::CHARACTER_MISSION_TYPE_AREA_ITEM_LEVEL_UP_UNIT:
      return "Unit Items";
    case db::CHARACTER_MISSION_TYPE_PLAY_LIVE:
      return "Lives";
    case db::CHARACTER_MISSION_TYPE_PLAY_LIVE_EX:
      return "Lives (EX)";
    case db::CHARACTER_MISSION_TYPE_WAITING_ROOM:
      return "Dupes";
    case db::CHARACTER_MISSION_TYPE_WAITING_ROOM_EX:
      return "Dupes (EX)";
    case db::CHARACTER_MISSION_TYPE_COLLECT_ANOTHER_VOCAL:
      return "Alts";
    case db::CHARACTER_MISSION_TYPE_COLLECT_COSTUME_3D:
      return "Costumes";
    case db::CHARACTER_MISSION_TYPE_COLLECT_MEMBER:
      return "Album";
    case db::CHARACTER_MISSION_TYPE_COLLECT_STAMP:
      return "Stamps";
    case db::CHARACTER_MISSION_TYPE_MASTER_RANK_UP_RARE:
      return "MR (4*)";
    case db::CHARACTER_MISSION_TYPE_MASTER_RANK_UP_STANDARD:
      return "MR (3*)";
    case db::CHARACTER_MISSION_TYPE_READ_AREA_TALK:
      return "Area Convos";
    case db::CHARACTER_MISSION_TYPE_READ_CARD_EPISODE_FIRST:
      return "Story (1st)";
    case db::CHARACTER_MISSION_TYPE_READ_CARD_EPISODE_SECOND:
      return "Story (2nd)";
    case db::CHARACTER_MISSION_TYPE_SKILL_LEVEL_UP_RARE:
      return "SL (4*)";
    case db::CHARACTER_MISSION_TYPE_SKILL_LEVEL_UP_STANDARD:
      return "SL (3*)";
    case db::CHARACTER_MISSION_TYPE_COLLECT_CHARACTER_ARCHIVE_VOICE:
      return "Voice Lines";
    case db::CHARACTER_MISSION_TYPE_COLLECT_MYSEKAI_FIXTURE:
      return "Furnitures";
    case db::CHARACTER_MISSION_TYPE_COLLECT_MYSEKAI_CANVAS:
      return "Canvases";
    case db::CHARACTER_MISSION_TYPE_READ_MYSEKAI_FIXTURE_TALK:
      return "MySaki Convos";
    default:
      ABSL_CHECK(false) << "unhandled case";
      return "";
  }
}

}  // namespace sekai
