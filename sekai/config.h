#pragma once

#include <filesystem>
#include <string_view>

#include "sekai/proto/world_bloom.pb.h"

namespace sekai {

inline constexpr int kTeamSize = 5;

inline constexpr int kMasterRankMin = 0;
inline constexpr int kMasterRankMax = 5;
inline constexpr int kMasterRankArraySize = kMasterRankMax + 1;

inline constexpr int kSkillLevelMin = 1;
inline constexpr int kSkillLevelMax = 4;
inline constexpr int kSkillLevelArraySize = kSkillLevelMax + 1;

inline constexpr int kMaxPower = 400'000;
inline constexpr int kMaxScore = 3'000'000;
inline constexpr int kMaxEventBonus = 750;
inline constexpr int kMaxBaseEventPoint = 3000;
inline constexpr int kMinSkillValue = 20 + (20 / 5) * 4;
inline constexpr int kMaxSkillValue = 160 + (160 / 5) * 4;

inline constexpr std::array kReferenceScoreBoostCaps = {0.f, 120.f, 130.f, 140.f, 140.f};

inline constexpr int kMaxTitleBonus = 310;
inline constexpr int kMaxCharacterRank = 160;

inline constexpr std::array kBoostMultipliers = {1, 5, 10, 15, 20, 25, 27, 29, 31, 33, 35};

inline constexpr WorldBloomVersion kDefaultWorldBloomVersion = WORLD_BLOOM_VERSION_2;
inline constexpr std::array kWorldBloomVersionCutoffs = {152};

inline constexpr int kEbiBaseFactor = 100;
inline constexpr int kSoloScoreStep = 20'000;

std::string_view CppVersion();
const std::filesystem::path& SekaiBestRoot();
const std::filesystem::path& MasterDbRoot();
const std::filesystem::path& SekaiRunfilesRoot();
const std::filesystem::path& MainRunfilesRoot();

const WorldBloomConfig& GetWorldBloomConfig(WorldBloomVersion version);
WorldBloomVersion GetWorldBloomVersion(int event_id);

}  // namespace sekai
