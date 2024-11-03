#pragma once

#include <string>

#include "sekai/db/proto/all.h"

namespace frontend {

std::string GetRarityDisplayText(sekai::db::CardRarityType attr);
std::string GetRarityDisplayTextShort(sekai::db::CardRarityType attr);
std::string GetAttrDisplayText(sekai::db::Attr attr);
std::string GetAttrDisplayTextShort(sekai::db::Attr attr);
std::string GetAreaDisplayText(int area_id);
std::string GetAreaDisplayTextShort(int area_id);
std::string GetCharacterDisplayText(int char_id);
std::string GetCharacterDisplayTextShort(int char_id);
std::string GetUnitDisplayText(int unit_id);
std::string GetUnitDisplayTextShort(int unit_id);

}  // namespace frontend
