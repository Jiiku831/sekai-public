#pragma once

#include <ctml.hpp>

#include "sekai/proto/team.pb.h"

namespace sekai::html {

CTML::Node Team(const TeamProto& team);

}  // namespace sekai::html
