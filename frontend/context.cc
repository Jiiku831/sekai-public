#include "frontend/context.h"

#include <map>
#include <optional>
#include <string>
#include <vector>

#include "frontend/display_text.h"
#include "frontend/element_id.h"
#include "frontend/proto/context.pb.h"
#include "frontend/util.h"
#include "sekai/array_size.h"
#include "sekai/card.h"
#include "sekai/character.h"
#include "sekai/db/master_db.h"
#include "sekai/db/proto/all.h"
#include "sekai/proto/world_bloom.pb.h"

namespace frontend {
namespace {

using ::sekai::LookupCharacterUnit;
using ::sekai::MaxLevelForRarity;
using ::sekai::TrainableCard;
using ::sekai::WorldBloomVersion;
using ::sekai::db::AreaItem;
using ::sekai::db::Attr;
using ::sekai::db::Card;
using ::sekai::db::CardEpisode;
using ::sekai::db::CardRarityType;
using ::sekai::db::Event;
using ::sekai::db::GameCharacter;
using ::sekai::db::MasterDb;
using ::sekai::db::Unit;
using ::sekai::db::WorldBloom;

constexpr int kDefaultThumbnailRes = 32;
constexpr int kTeamBuilderThumbnailRes = 128;
constexpr int kTeamBuilderSupportThumbnailRes = 64;
constexpr std::string_view kThumbnailPrefix = "/data/assets/";
constexpr std::string_view kThumbnailUntrainedSuffix = "_normal.webp";
constexpr std::string_view kThumbnailTrainedSuffix = "_after_training.webp";

CardContext CreateCardContextFromCardProto(const sekai::CardProto& card,
                                           std::optional<int> thumbnail_res = std::nullopt) {
  bool trained_visual = card.state().special_training();
  if (card.skill_state() == sekai::CardProto::USE_PRIMARY_SKILL) {
    trained_visual = false;
  }
  CardContext context = CreateCardContext(card.db_card(), thumbnail_res,
                                          card.state().special_training(), trained_visual);
  context.mutable_state()->set_level(card.state().level());
  context.mutable_state()->set_master_rank(card.state().master_rank());
  context.mutable_state()->set_skill_level(card.state().skill_level());
  context.mutable_state()->set_team_power_contrib(card.team_power_contrib());
  context.mutable_state()->set_team_bonus_contrib(card.team_bonus_contrib());
  context.mutable_state()->set_team_skill_contrib(card.team_skill_contrib());
  return context;
}

WorldBloomVersionContext CreateWorldBloomVersionContext(WorldBloomVersion version) {
  WorldBloomVersionContext context;
  context.set_display_text(GetWorldBloomVersionDisplayText(version));
  context.set_value(static_cast<int>(version));
  return context;
}

CustomEventContext CreateCustomEventContext() {
  CustomEventContext context;
  for (Attr attr : sekai::EnumValues<Attr, sekai::db::Attr_descriptor>()) {
    if (attr == sekai::db::ATTR_UNKNOWN) continue;
    *context.add_attrs() = CreateAttrContext(attr, /*short_name=*/true);
  }

  std::vector<CharacterContextGroup> groups = CreateCharacterContextGroups();
  *context.mutable_chapter_chars() = {groups.begin(), groups.end()};

  CharacterContextGroup* group = context.add_bonus_chars();
  for (const GameCharacter& character : MasterDb::GetAll<GameCharacter>()) {
    if (!group->chars().empty()) {
      int last_char_id = group->chars().rbegin()->char_id();
      sekai::db::Unit last_unit = sekai::LookupCharacterUnit(last_char_id);
      if (last_unit != LookupCharacterUnit(character.id())) {
        // Fill in subunit VS
        if (last_unit != sekai::db::UNIT_VS) {
          for (const GameCharacter& vs_character : MasterDb::GetAll<GameCharacter>()) {
            if (LookupCharacterUnit(vs_character.id()) != sekai::db::UNIT_VS) {
              continue;
            }
            CharacterContext vs_char_context =
                CreateCharacterContext(vs_character.id(), /*short_name=*/true);
            vs_char_context.set_unit_id(static_cast<int>(last_unit));
            *group->add_chars() = vs_char_context;
          }
        }
        group = context.add_bonus_chars();
      }
    }
    CharacterContext char_context = CreateCharacterContext(character.id(), /*short_name=*/true);
    *group->add_chars() = char_context;
  }

  for (WorldBloomVersion version :
       sekai::EnumValues<WorldBloomVersion, sekai::WorldBloomVersion_descriptor>()) {
    if (version == sekai::WORLD_BLOOM_VERSION_UNKNOWN) continue;
    *context.add_world_bloom_versions() = CreateWorldBloomVersionContext(version);
  }

  return context;
}

std::string GetAttrFramePath(Attr attr) {
  switch (attr) {
    using enum Attr;
    case ATTR_COOL:
      return "images/attr_cool.png";
    case ATTR_CUTE:
      return "images/attr_cute.png";
    case ATTR_HAPPY:
      return "images/attr_happy.png";
    case ATTR_MYST:
      return "images/attr_mysterious.png";
    case ATTR_PURE:
      return "images/attr_pure.png";
    default:
      return "";
  }
}

std::string GetRarityFramePath(CardRarityType rarity, bool trained) {
  switch (rarity) {
    using enum CardRarityType;
    case RARITY_1:
      return "images/frame_1.png";
    case RARITY_2:
      return "images/frame_2.png";
    case RARITY_3:
      return trained ? "images/frame_3_trained.png" : "images/frame_3.png";
    case RARITY_4:
      return trained ? "images/frame_4_trained.png" : "images/frame_4.png";
    case RARITY_BD:
      return "images/frame_birthday.png";
    default:
      return "";
  }
}

}  // namespace

AttrContext CreateAttrContext(Attr attr, bool short_name) {
  AttrContext context;
  context.set_display_text(short_name ? GetAttrDisplayTextShort(attr) : GetAttrDisplayText(attr));
  context.set_value(static_cast<int>(attr));
  return context;
}

CharacterContext CreateCharacterContext(int char_id, bool short_name) {
  CharacterContext context;
  context.set_display_text(short_name ? GetCharacterDisplayTextShort(char_id)
                                      : GetCharacterDisplayText(char_id));
  context.set_char_id(char_id);
  context.set_unit_display_text(GetUnitDisplayText(LookupCharacterUnit(char_id)));
  return context;
}

std::vector<CharacterContextGroup> CreateCharacterContextGroups(bool short_name) {
  std::vector<CharacterContextGroup> groups;
  groups.emplace_back();
  CharacterContextGroup* group = &groups.back();
  for (const GameCharacter& character : MasterDb::GetAll<GameCharacter>()) {
    if (!group->chars().empty()) {
      int last_char_id = group->chars().rbegin()->char_id();
      if (LookupCharacterUnit(last_char_id) != LookupCharacterUnit(character.id())) {
        groups.emplace_back();
        group = &groups.back();
      }
    }
    *group->add_chars() = CreateCharacterContext(character.id(), short_name);
  }
  return groups;
}

std::vector<PowerBonusContext::AreaContext> CreateAreaContext() {
  std::map<int, PowerBonusContext::AreaContext> groups;
  for (const AreaItem& area_item : MasterDb::GetAll<AreaItem>()) {
    PowerBonusContext::AreaContext& area_context = groups[area_item.area_id()];
    area_context.set_display_text(GetAreaDisplayText(area_item.area_id()));
    AreaItemContext* context = area_context.add_area_items();
    context->set_display_text(area_item.name());
    context->set_area_id(area_item.area_id());
    context->set_area_item_id(area_item.id());
  }
  std::vector<PowerBonusContext::AreaContext> flattened_groups;
  for (const auto& [unused, group] : groups) {
    flattened_groups.push_back(group);
  }
  return flattened_groups;
}

CardContext CreateCardContext(const Card& card, std::optional<int> thumbnail_res, bool trained,
                              bool display_trained) {
  CardContext context;
  context.set_card_id(card.id());
  context.set_card_list_row_id(CardListRowId(card.id()));
  context.set_thumbnail_res(thumbnail_res.value_or(kDefaultThumbnailRes));
  context.set_thumbnail_url(MaybeEmbedImage(
      absl::StrCat(kThumbnailPrefix, context.thumbnail_res(), "/", card.assetbundle_name(),
                   display_trained ? kThumbnailTrainedSuffix : kThumbnailUntrainedSuffix)));
  context.set_thumbnail_url_128(MaybeEmbedImage(
      absl::StrCat(kThumbnailPrefix, 128, "/", card.assetbundle_name(),
                   display_trained ? kThumbnailTrainedSuffix : kThumbnailUntrainedSuffix)));
  *context.mutable_character() = CreateCharacterContext(card.character_id());
  *context.mutable_attr() = CreateAttrContext(card.attr());
  *context.mutable_rarity() = CreateRarityContext(card.card_rarity_type());
  context.set_max_level(MaxLevelForRarity(card.card_rarity_type()));
  context.set_trainable(TrainableCard(card.card_rarity_type()));
  context.set_attr_frame(GetAttrFramePath(card.attr()));
  context.set_rarity_frame(GetRarityFramePath(card.card_rarity_type(), trained));

  if (context.trainable()) {
    context.set_alt_thumbnail_url(
        MaybeEmbedImage(absl::StrCat(kThumbnailPrefix, context.thumbnail_res(), "/",
                                     card.assetbundle_name(), kThumbnailTrainedSuffix)));
    context.set_alt_thumbnail_url_128(MaybeEmbedImage(absl::StrCat(
        kThumbnailPrefix, 128, "/", card.assetbundle_name(), kThumbnailTrainedSuffix)));
    context.set_alt_rarity_frame(GetRarityFramePath(card.card_rarity_type(), true));
  }
  for (const CardEpisode* episode : MasterDb::FindAll<CardEpisode>(card.id())) {
    switch (episode->card_episode_part_type()) {
      case CardEpisode::FIRST_PART:
        context.set_card_episode_1(episode->id());
        break;
      case CardEpisode::SECOND_PART:
        context.set_card_episode_2(episode->id());
        break;
      default:
        ABSL_CHECK(false) << "unhandled case";
    }
  }
  return context;
}

RarityContext CreateRarityContext(CardRarityType rarity) {
  RarityContext context;
  context.set_display_text(GetRarityDisplayText(rarity));
  context.set_value(static_cast<int>(rarity));
  return context;
}

ParkingStrategyContext CreateParkingStrategyContext(const sekai::ParkingStrategy& strategy) {
  ParkingStrategyContext strategy_context;
  strategy_context.set_boost(strategy.boost());
  strategy_context.set_multiplier(strategy.multiplier());
  strategy_context.set_base_ep(strategy.base_ep());
  strategy_context.set_total_ep(strategy.total_ep());
  strategy_context.set_score_lb(strategy.score_lb());
  strategy_context.set_score_ub(strategy.score_ub());
  if (strategy.has_plays()) {
    strategy_context.set_plays(strategy.plays());
  }
  if (strategy.has_total_multiplier()) {
    strategy_context.set_total_multiplier(strategy.total_multiplier());
  }
  return strategy_context;
}

TeamContext CreateTeamContext(const sekai::TeamProto& team) {
  TeamContext context;
  for (const sekai::CardProto& card : team.cards()) {
    *context.add_cards() = CreateCardContextFromCardProto(card, kTeamBuilderThumbnailRes);
  }
  context.set_power(team.power());
  *context.mutable_power_detailed() = {team.power_detailed().begin(), team.power_detailed().end()};
  context.set_skill_value(team.skill_value());
  context.set_event_bonus(team.event_bonus());
  context.set_expected_ep(team.expected_ep());
  context.set_expected_score(team.expected_score());
  context.set_best_song_name(team.best_song_name());
  for (const sekai::CardProto& card : team.support_cards()) {
    *context.add_support_cards() =
        CreateCardContextFromCardProto(card, kTeamBuilderSupportThumbnailRes);
  }
  if (team.has_support_bonus()) {
    context.set_support_bonus(team.support_bonus());
    context.set_main_bonus(team.main_bonus());
  }
  if (team.has_target_ep()) {
    ParkingContext& parking_context = *context.mutable_parking_details();
    parking_context.set_target(team.target_ep());
    parking_context.set_max_score(team.max_solo_ebi_score());
    for (const sekai::ParkingStrategy& strategy : team.parking_strategies()) {
      *parking_context.add_strategies() = CreateParkingStrategyContext(strategy);
    }
    for (const sekai::ParkingStrategy& strategy : team.multi_turn_parking()) {
      *parking_context.add_multi_turn_strategies() = CreateParkingStrategyContext(strategy);
    }
  }
  return context;
}

TeamBuilderContext CreateTeamBuilderContext() {
  TeamBuilderContext context;
  TeamBuilderContext::EventContext& default_event_context = *context.add_events();
  default_event_context.set_event_id(0);
  default_event_context.set_event_str("0");
  default_event_context.set_display_text("Custom Event");
  default_event_context.set_selected(true);

  for (const Event& event : MasterDb::GetAll<Event>()) {
    std::string event_name = event.name();
    if (absl::FromUnixMillis(event.start_at()) > absl::Now()) {
      event_name = "TBA (Spoiler Warning)";
    }
    if (event.event_type() == Event::WORLD_BLOOM) {
      for (const WorldBloom* world_bloom : MasterDb::FindAll<WorldBloom>(event.id())) {
        auto character = MasterDb::FindFirst<GameCharacter>(world_bloom->game_character_id());
        TeamBuilderContext::EventContext& event_context = *context.add_events();
        event_context.set_event_id(event.id());
        event_context.set_chapter_id(world_bloom->chapter_no());
        event_context.set_display_text(absl::StrFormat("%d-%d - %s - %s", event.id(),
                                                       world_bloom->chapter_no(), event_name,
                                                       character.given_name()));
        event_context.set_event_str(
            absl::StrFormat("%d-%d", event.id(), world_bloom->chapter_no()));
      }
    } else {
      TeamBuilderContext::EventContext& event_context = *context.add_events();
      event_context.set_event_id(event.id());
      event_context.set_display_text(absl::StrFormat("%d - %s", event.id(), event_name));
      event_context.set_event_str(std::to_string(event.id()));
    }
  }

  std::vector<CharacterContextGroup> groups = CreateCharacterContextGroups();
  *context.mutable_challenge_live_characters() = {groups.begin(), groups.end()};
  *context.mutable_lead_character_constraint() = {groups.begin(), groups.end()};

  for (int kizuna_main : sekai::UniqueCharacterIds()) {
    KizunaContext& kizuna_context = *context.add_kizuna_constraint();
    *kizuna_context.mutable_kizuna_main() = CreateCharacterContext(kizuna_main);
    for (int char_id : sekai::UniqueCharacterIds()) {
      *kizuna_context.add_chars() = CreateCharacterContext(char_id);
    }
  }

  for (CardRarityType rarity :
       sekai::EnumValues<CardRarityType, sekai::db::CardRarityType_descriptor>()) {
    if (rarity == sekai::db::RARITY_UNKNOWN) continue;
    *context.add_rarity_constraint() = CreateRarityContext(rarity);
  }

  context.set_use_old_subunitless_bonus(false);
  *context.mutable_custom_event() = CreateCustomEventContext();
  return context;
}

}  // namespace frontend
