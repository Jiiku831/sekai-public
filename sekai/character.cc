#include "sekai/character.h"

#include "sekai/bitset.h"
#include "sekai/db/master_db.h"
#include "sekai/db/proto/game_character.pb.h"

namespace sekai {

db::Unit LookupCharacterUnit(int id) {
  return db::MasterDb::FindFirst<db::GameCharacter>(id).unit();
}

}  // namespace sekai
