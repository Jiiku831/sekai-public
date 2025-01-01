#pragma once

#include <array>
#include <span>

#include "sekai/array_size.h"
#include "sekai/card.h"
#include "sekai/db/proto/enums.pb.h"

namespace sekai {

std::array<std::vector<const Card*>, kCharacterArraySize> PartitionCardPoolByCharacters(
    std::span<const Card* const> pool);

std::array<std::vector<const Card*>, db::Attr_ARRAYSIZE> PartitionCardPoolByAttribute(
    std::span<const Card* const> pool);

std::vector<int> SortCharactersByMaxEventBonus(const EventBonus& event_bonus);

std::vector<const Card*> SortCardsByBonus(std::span<const Card* const> pool);

std::vector<const Card*> SortCardsByPower(std::span<const Card* const> pool, bool attr_match,
                                          bool primary_unit_match, bool secondary_unit_match);

std::vector<const Card*> FilterCardsByUnit(db::Unit unit, std::span<const Card* const> pool);

std::vector<const Card*> FilterCardsByAttr(db::Attr attr, std::span<const Card* const> pool);

std::vector<const Card*> FilterCards(db::Attr attr, db::Unit unit,
                                     std::span<const Card* const> pool);

std::vector<const Card*> GetSortedSupportPool(std::span<const Card* const> pool);

}  // namespace sekai
