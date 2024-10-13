#pragma once

#include <span>

#include <ctml.hpp>

#include "sekai/proto/max_character_rank.pb.h"

namespace sekai::html {

CTML::Node MaxCrTable(std::span<const MaxCharacterRank> max_cr);

}  // namespace sekai::html
