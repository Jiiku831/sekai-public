#pragma once

#include <tuple>

#include "sekai/db/proto/action_set.pb.h"
#include "sekai/db/proto/area_item.pb.h"
#include "sekai/db/proto/area_item_level.pb.h"
#include "sekai/db/proto/card.pb.h"
#include "sekai/db/proto/card_episode.pb.h"
#include "sekai/db/proto/challenge_live_stage.pb.h"
#include "sekai/db/proto/character2d.pb.h"
#include "sekai/db/proto/character_archive_voice.pb.h"
#include "sekai/db/proto/character_mission_v2.pb.h"
#include "sekai/db/proto/character_mission_v2_area_item.pb.h"
#include "sekai/db/proto/character_mission_v2_ex.pb.h"
#include "sekai/db/proto/character_mission_v2_parameter_group.pb.h"
#include "sekai/db/proto/character_rank.pb.h"
#include "sekai/db/proto/costume3d.pb.h"
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
#include "sekai/db/proto/music_vocal.pb.h"
#include "sekai/db/proto/records.pb.h"
#include "sekai/db/proto/skill.pb.h"
#include "sekai/db/proto/stamp.pb.h"
#include "sekai/db/proto/world_bloom.pb.h"
#include "sekai/db/proto/world_bloom_different_attribute_bonus.pb.h"
#include "sekai/db/proto/world_bloom_support_deck_bonus.pb.h"

namespace sekai::db {

using AllRecordTypes =
    std::tuple<ActionSet, AreaItem, AreaItemLevel, Card, CardEpisode, ChallengeLiveStage,
               Character2D, CharacterArchiveVoice, CharacterMissionV2, CharacterMissionV2AreaItem,
               CharacterMissionV2Ex, CharacterMissionV2ParameterGroup, CharacterRank, Costume3D,
               Event, EventCard, EventDeckBonus, EventRarityBonusRate, GameCharacter,
               GameCharacterUnit, MasterLesson, Music, MusicMeta, MusicVocal, Skill, Stamp,
               WorldBloom, WorldBloomDifferentAttributeBonus, WorldBloomSupportDeckBonus>;

}  // namespace sekai::db
