#pragma once

#include <optional>
#include <vector>

#include "frontend/proto/context.pb.h"
#include "sekai/db/proto/all.h"
#include "sekai/proto/team.pb.h"

namespace frontend {

std::vector<PowerBonusContext::AreaContext> CreateAreaContext();
AttrContext CreateAttrContext(sekai::db::Attr attr, bool short_name = false);
CardContext CreateCardContext(const sekai::db::Card& card,
                              std::optional<int> thumbnail_res = std::nullopt, bool trained = false,
                              bool display_trained = false);
CharacterContext CreateCharacterContext(int char_id, bool short_name = false);
std::vector<CharacterContextGroup> CreateCharacterContextGroups(bool short_name = false);
RarityContext CreateRarityContext(sekai::db::CardRarityType rarity);
TeamContext CreateTeamContext(const sekai::TeamProto& team);

TeamBuilderContext CreateTeamBuilderContext();

}  // namespace frontend
