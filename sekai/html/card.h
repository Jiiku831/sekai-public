#pragma once

#include <ctml.hpp>

#include "sekai/db/proto/all.h"
#include "sekai/proto/card.pb.h"

namespace sekai::html {

CTML::Node Card(const db::Card& card, bool trained = false);
CTML::Node Card(const CardProto& card);

}  // namespace sekai::html
