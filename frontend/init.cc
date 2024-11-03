#include "frontend/init.h"

#include <emscripten.h>
#include <google/protobuf/util/json_util.h>

#include "absl/log/absl_check.h"
#include "absl/strings/str_cat.h"
#include "frontend/context.h"
#include "frontend/display_text.h"
#include "frontend/element_id.h"
#include "frontend/proto/context.pb.h"
#include "sekai/card.h"
#include "sekai/character.h"
#include "sekai/db/master_db.h"
#include "sekai/db/proto/all.h"
#include "sekai/proto_util.h"

namespace frontend {

namespace {

using ::google::protobuf::util::MessageToJsonString;
using ::sekai::EnumValues;
using ::sekai::LookupCharacterUnit;
using ::sekai::db::Attr;
using ::sekai::db::Card;
using ::sekai::db::CardEpisode;
using ::sekai::db::CardRarityType;
using ::sekai::db::GameCharacter;
using ::sekai::db::MasterDb;

EM_JS(void, CallInitialRender, (const char* context), {
    const parsed_context = JSON.parse(UTF8ToString(context));
    InitialRender(parsed_context);
});

void CreateAttrFilters(CardListContext& list) {
  for (Attr attr : EnumValues<Attr, sekai::db::Attr_descriptor>()) {
    if (attr == sekai::db::ATTR_UNKNOWN) continue;
    *list.mutable_list_filter()->add_attrs() = CreateAttrContext(attr, /*short_name=*/true);
  }
}

void CreateRarityFilters(CardListContext& list) {
  for (CardRarityType rarity : EnumValues<CardRarityType, sekai::db::CardRarityType_descriptor>()) {
    if (rarity == sekai::db::RARITY_UNKNOWN) continue;
    *list.mutable_list_filter()->add_rarities() = CreateRarityContext(rarity);
  }
}

void CreateCharacterFilters(CardListContext& list) {
  std::vector<CharacterContextGroup> groups = CreateCharacterContextGroups();
  *list.mutable_list_filter()->mutable_char_rows() = {groups.begin(), groups.end()};
}

void CreateListFilters(CardListContext& list) {
  CreateAttrFilters(list);
  CreateRarityFilters(list);
  CreateCharacterFilters(list);
}

CardListContext CreateEmptyCardList() {
  CardListContext list;
  for (const Card& card : MasterDb::GetAll<Card>()) {
    *list.add_cards() = CreateCardContext(card);
  }
  CreateListFilters(list);
  return list;
}

PowerBonusContext CreatePowerBonusContext() {
  PowerBonusContext context;
  std::vector<PowerBonusContext::AreaContext> areas = CreateAreaContext();
  *context.mutable_areas() = {areas.begin(), areas.end()};

  std::vector<CharacterContextGroup> groups = CreateCharacterContextGroups();
  *context.mutable_char_rows() = {groups.begin(), groups.end()};
  return context;
}

InitialRenderContext CreateInitialRenderContext() {
  InitialRenderContext context;
  *context.mutable_card_list() = CreateEmptyCardList();
  *context.mutable_power_bonus() = CreatePowerBonusContext();
  *context.mutable_team_builder() = CreateTeamBuilderContext();
  return context;
}

}  // namespace

void InitializePage() {
  std::string json;
  (void)MessageToJsonString(CreateInitialRenderContext(), &json);
  CallInitialRender(json.c_str());
}

}  // namespace frontend
