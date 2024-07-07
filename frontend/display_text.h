#pragma once

#include <string>

#include "sekai/db/proto/all.h"

namespace frontend {

std::string GetRarityDisplayText(sekai::db::CardRarityType attr);
std::string GetAttrDisplayText(sekai::db::Attr attr);
std::string GetAreaDisplayText(int area_id);
std::string GetCharacterDisplayText(int char_id);
std::string GetUnitDisplayText(int unit_id);

}  // namespace frontend
