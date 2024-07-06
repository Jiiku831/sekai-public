#pragma once

#include <tuple>

#include "sekai/db/proto/area_item.pb.h"
#include "sekai/db/proto/area_item_level.pb.h"
#include "sekai/db/proto/card.pb.h"
#include "sekai/db/proto/card_episode.pb.h"
#include "sekai/db/proto/character_rank.pb.h"
#include "sekai/db/proto/descriptor.pb.h"
#include "sekai/db/proto/enums.pb.h"
#include "sekai/db/proto/event.pb.h"
#include "sekai/db/proto/event_card.pb.h"
#include "sekai/db/proto/event_deck_bonus.pb.h"
#include "sekai/db/proto/event_rarity_bonus_rate.pb.h"
#include "sekai/db/proto/game_character.pb.h"
#include "sekai/db/proto/game_character_unit.pb.h"
#include "sekai/db/proto/master_lesson.pb.h"
#include "sekai/db/proto/music.pb.h"
#include "sekai/db/proto/music_meta.pb.h"
#include "sekai/db/proto/records.pb.h"
#include "sekai/db/proto/skill.pb.h"
#include "sekai/db/proto/world_bloom.pb.h"
#include "sekai/db/proto/world_bloom_different_attribute_bonus.pb.h"
#include "sekai/db/proto/world_bloom_support_deck_bonus.pb.h"

namespace sekai::db {

using AllRecordTypes =
    std::tuple<AreaItem, AreaItemLevel, Card, CardEpisode, CharacterRank, Event, EventCard,
               EventDeckBonus, EventRarityBonusRate, GameCharacter, GameCharacterUnit, MasterLesson,
               Music, MusicMeta, Skill, WorldBloom, WorldBloomDifferentAttributeBonus,
               WorldBloomSupportDeckBonus>;

}  // namespace sekai::db
