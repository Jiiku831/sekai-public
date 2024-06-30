#pragma once

#include <filesystem>
#include <string_view>

namespace sekai {

inline constexpr int kMasterRankMax = 5;
inline constexpr int kMasterRankArraySize = kMasterRankMax + 1;

inline constexpr int kSkillLevelMax = 4;
inline constexpr int kSkillLevelArraySize = kSkillLevelMax + 1;

inline constexpr int kMaxPower = 400'000;
inline constexpr int kMaxEventBonus = 750;
inline constexpr int kMinSkillValue = 20 + (20 / 5) * 4;
inline constexpr int kMaxSkillValue = 160 + (160 / 5) * 4;

inline constexpr float kReferenceScoreBoostCap = 140.0;

std::string_view CppVersion();
const std::filesystem::path& SekaiBestRoot();
const std::filesystem::path& MasterDbRoot();
const std::filesystem::path& SekaiRunfilesRoot();
const std::filesystem::path& MainRunfilesRoot();

}  // namespace sekai
