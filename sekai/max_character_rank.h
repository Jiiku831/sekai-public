#pragma once

#include <optional>
#include <vector>

#include "absl/time/time.h"
#include "sekai/proto/max_character_rank.pb.h"

namespace sekai {

// Returns the max character ranks at the given time, assuming the time is after the 4th
// anniversary.
//
// For now, the max CL is assumed to be +1030/d.
std::vector<MaxCharacterRank> GetMaxCharacterRanks(absl::Time time);

std::optional<CharacterRankSource> GetCharacterRankSource(const MaxCharacterRank& rank,
                                                          CharacterRankSource::OtherSource source);
std::optional<CharacterRankSource> GetCharacterRankSource(const MaxCharacterRank& rank,
                                                          db::CharacterMissionType mission);

std::string SourceDescription(CharacterRankSource::OtherSource source);
std::string SourceDescription(db::CharacterMissionType source);

}  // namespace sekai
