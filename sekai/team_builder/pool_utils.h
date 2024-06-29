#pragma once

#include <array>
#include <span>

#include "sekai/array_size.h"
#include "sekai/card.h"

namespace sekai {

std::array<std::vector<const Card*>, kCharacterArraySize> PartitionCardPoolByCharacters(
    std::span<const Card* const> pool);

std::vector<int> SortCharactersByMaxEventBonus(const EventBonus& event_bonus);

}  // namespace sekai
