#pragma once

#include <span>

#include "sekai/db/proto/all.h"

namespace sekai {

std::span<const db::MySekaiFixture* const> FixturesWithCharBonuses();

}  // namespace sekai
