#include "sekai/max_character_rank.h"

#include <limits>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "absl/time/time.h"
#include "sekai/array_size.h"
#include "testing/util.h"

namespace sekai {
namespace {

TEST(GetMaxCharacterRanksTest, ReturnsForEachCharacter) {
  std::vector<MaxCharacterRank> max_character_ranks = GetMaxCharacterRanks(absl::Now());
  ASSERT_LE(static_cast<std::size_t>(CharacterArraySize()), max_character_ranks.size());
  EXPECT_FALSE(max_character_ranks.empty());
  for (int char_id : sekai::UniqueCharacterIds()) {
    EXPECT_EQ(max_character_ranks[char_id].character_id(), char_id);
  }
}

TEST(GetMaxCharacterRanksTest, ComputeMaxPlantAreaItemsForMiku) {
  std::vector<MaxCharacterRank> max_character_ranks = GetMaxCharacterRanks(
      absl::FromCivil(absl::CivilSecond(2024, 9, 27, 1, 59, 59), absl::UTCTimeZone()));
  ASSERT_LT(21ull, max_character_ranks.size());
  std::optional<CharacterRankSource> cl_source = GetCharacterRankSource(
      max_character_ranks[21], db::CHARACTER_MISSION_TYPE_AREA_ITEM_LEVEL_UP_REALITY_WORLD);
  ASSERT_TRUE(cl_source.has_value());
  EXPECT_EQ(cl_source->progress(), 200);
  EXPECT_EQ(cl_source->max_progress(), cl_source->progress());
  EXPECT_EQ(cl_source->current_xp(), 40);
  EXPECT_EQ(cl_source->max_xp(), cl_source->current_xp());
}

TEST(GetMaxCharacterRanksTest, ComputeMaxCharAreaItemsForMiku) {
  std::vector<MaxCharacterRank> max_character_ranks = GetMaxCharacterRanks(
      absl::FromCivil(absl::CivilSecond(2024, 9, 27, 1, 59, 59), absl::UTCTimeZone()));
  ASSERT_LT(21ull, max_character_ranks.size());
  std::optional<CharacterRankSource> cl_source = GetCharacterRankSource(
      max_character_ranks[21], db::CHARACTER_MISSION_TYPE_AREA_ITEM_LEVEL_UP_CHARACTER);
  ASSERT_TRUE(cl_source.has_value());
  EXPECT_EQ(cl_source->progress(), 100);
  EXPECT_EQ(cl_source->max_progress(), cl_source->progress());
  EXPECT_EQ(cl_source->current_xp(), 20);
  EXPECT_EQ(cl_source->max_xp(), cl_source->current_xp());
}

TEST(GetMaxCharacterRanksTest, ComputeMaxUnitAreaItemsForMiku) {
  std::vector<MaxCharacterRank> max_character_ranks = GetMaxCharacterRanks(
      absl::FromCivil(absl::CivilSecond(2024, 9, 27, 1, 59, 59), absl::UTCTimeZone()));
  ASSERT_LT(21ull, max_character_ranks.size());
  std::optional<CharacterRankSource> cl_source = GetCharacterRankSource(
      max_character_ranks[21], db::CHARACTER_MISSION_TYPE_AREA_ITEM_LEVEL_UP_UNIT);
  ASSERT_TRUE(cl_source.has_value());
  EXPECT_EQ(cl_source->progress(), 100);
  EXPECT_EQ(cl_source->max_progress(), cl_source->progress());
  EXPECT_EQ(cl_source->current_xp(), 20);
  EXPECT_EQ(cl_source->max_xp(), cl_source->current_xp());
}

TEST(GetMaxCharacterRanksTest, ComputeMaxLeaderLivesForMiku) {
  std::vector<MaxCharacterRank> max_character_ranks = GetMaxCharacterRanks(
      absl::FromCivil(absl::CivilSecond(2024, 9, 27, 1, 59, 59), absl::UTCTimeZone()));
  ASSERT_LT(21ull, max_character_ranks.size());
  std::optional<CharacterRankSource> cl_source =
      GetCharacterRankSource(max_character_ranks[21], db::CHARACTER_MISSION_TYPE_PLAY_LIVE);
  ASSERT_TRUE(cl_source.has_value());
  EXPECT_EQ(cl_source->progress(), std::numeric_limits<int>::max());
  EXPECT_EQ(cl_source->max_progress(), 50000);
  EXPECT_EQ(cl_source->current_xp(), 140);
  EXPECT_EQ(cl_source->max_xp(), 140);
}

TEST(GetMaxCharacterRanksTest, ComputeMaxLeaderLivesExForMiku) {
  std::vector<MaxCharacterRank> max_character_ranks = GetMaxCharacterRanks(
      absl::FromCivil(absl::CivilSecond(2024, 9, 27, 1, 59, 59), absl::UTCTimeZone()));
  ASSERT_LT(21ull, max_character_ranks.size());
  std::optional<CharacterRankSource> cl_source =
      GetCharacterRankSource(max_character_ranks[21], db::CHARACTER_MISSION_TYPE_PLAY_LIVE_EX);
  ASSERT_TRUE(cl_source.has_value());
  EXPECT_EQ(cl_source->progress(), std::numeric_limits<int>::max());
  EXPECT_EQ(cl_source->max_progress(), 28500);
  EXPECT_EQ(cl_source->current_xp(), 30);
  EXPECT_EQ(cl_source->max_xp(), 30);
}

TEST(GetMaxCharacterRanksTest, ComputeMaxVocalsForMiku) {
  std::vector<MaxCharacterRank> max_character_ranks = GetMaxCharacterRanks(
      absl::FromCivil(absl::CivilSecond(2024, 9, 27, 1, 59, 59), absl::UTCTimeZone()));
  ASSERT_LT(21ull, max_character_ranks.size());
  std::optional<CharacterRankSource> cl_source = GetCharacterRankSource(
      max_character_ranks[21], db::CHARACTER_MISSION_TYPE_COLLECT_ANOTHER_VOCAL);
  ASSERT_TRUE(cl_source.has_value());
  EXPECT_GE(cl_source->progress(), 14);
}

TEST(GetMaxCharacterRanksTest, ComputeMaxCostumeForMiku) {
  std::vector<MaxCharacterRank> max_character_ranks = GetMaxCharacterRanks(
      absl::FromCivil(absl::CivilSecond(2024, 9, 27, 1, 59, 59), absl::UTCTimeZone()));
  ASSERT_LT(21ull, max_character_ranks.size());
  std::optional<CharacterRankSource> cl_source = GetCharacterRankSource(
      max_character_ranks[21], db::CHARACTER_MISSION_TYPE_COLLECT_COSTUME_3D);
  ASSERT_TRUE(cl_source.has_value());
  EXPECT_GE(cl_source->progress(), 1308);
}

TEST(GetMaxCharacterRanksTest, ComputeMaxAlbumForMiku) {
  std::vector<MaxCharacterRank> max_character_ranks = GetMaxCharacterRanks(
      absl::FromCivil(absl::CivilSecond(2024, 9, 27, 1, 59, 59), absl::UTCTimeZone()));
  ASSERT_LT(21ull, max_character_ranks.size());
  std::optional<CharacterRankSource> cl_source =
      GetCharacterRankSource(max_character_ranks[21], db::CHARACTER_MISSION_TYPE_COLLECT_MEMBER);
  ASSERT_TRUE(cl_source.has_value());
  EXPECT_GE(cl_source->progress(), 77);
}

TEST(GetMaxCharacterRanksTest, ComputeMaxStampForMiku) {
  std::vector<MaxCharacterRank> max_character_ranks = GetMaxCharacterRanks(
      absl::FromCivil(absl::CivilSecond(2024, 9, 27, 1, 59, 59), absl::UTCTimeZone()));
  ASSERT_LT(21ull, max_character_ranks.size());
  std::optional<CharacterRankSource> cl_source =
      GetCharacterRankSource(max_character_ranks[21], db::CHARACTER_MISSION_TYPE_COLLECT_STAMP);
  ASSERT_TRUE(cl_source.has_value());
  EXPECT_GE(cl_source->progress(), 50);
}

TEST(GetMaxCharacterRanksTest, ComputeMaxChallengeLiveForMiku) {
  std::vector<MaxCharacterRank> max_character_ranks = GetMaxCharacterRanks(
      absl::FromCivil(absl::CivilSecond(2024, 9, 27, 1, 59, 59), absl::UTCTimeZone()));
  ASSERT_LT(21ull, max_character_ranks.size());
  std::optional<CharacterRankSource> cl_source = GetCharacterRankSource(
      max_character_ranks[21], CharacterRankSource::OTHER_SOURCE_CHALLENGE_LIVE);
  ASSERT_TRUE(cl_source.has_value());
  EXPECT_EQ(cl_source->current_xp(), 100);
  EXPECT_EQ(cl_source->progress(), 101);

  max_character_ranks = GetMaxCharacterRanks(
      absl::FromCivil(absl::CivilSecond(2024, 9, 27, 2, 0, 0), absl::UTCTimeZone()));
  ASSERT_LT(21ull, max_character_ranks.size());
  cl_source = GetCharacterRankSource(max_character_ranks[21],
                                     CharacterRankSource::OTHER_SOURCE_CHALLENGE_LIVE);
  ASSERT_TRUE(cl_source.has_value());
  EXPECT_EQ(cl_source->current_xp(), 101);
  EXPECT_EQ(cl_source->progress(), 102);

  max_character_ranks = GetMaxCharacterRanks(
      absl::FromCivil(absl::CivilSecond(2024, 10, 7, 19, 0, 0), absl::UTCTimeZone()));
  ASSERT_LT(21ull, max_character_ranks.size());
  cl_source = GetCharacterRankSource(max_character_ranks[21],
                                     CharacterRankSource::OTHER_SOURCE_CHALLENGE_LIVE);
  ASSERT_TRUE(cl_source.has_value());
  EXPECT_EQ(cl_source->current_xp(), 102);
  EXPECT_EQ(cl_source->progress(), 103);
}

// TODO: add tests for mysekai CR missions

}  // namespace
}  // namespace sekai
