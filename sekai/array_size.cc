#include "sekai/array_size.h"

#include <algorithm>
#include <memory>

#include "sekai/db/master_db.h"

namespace sekai {
namespace {

using ::sekai::db::MasterDb;

template <typename T>
int MaxId() {
  int max_id = 0;
  for (const auto& c : MasterDb::GetAll<T>()) {
    max_id = std::max(max_id, c.id());
  }
  ABSL_CHECK_LT(max_id, 9999);
  return max_id;
}

std::unique_ptr<std::vector<int>> GetUniqueCharacterIds() {
  std::set<int> ids;
  for (const auto& c : MasterDb::GetAll<db::GameCharacter>()) {
    ids.insert(c.id());
  }
  return std::make_unique<std::vector<int>>(ids.begin(), ids.end());
}

int MaxCharacterId() {
  int max_id = 0;
  for (const auto& c : MasterDb::GetAll<db::GameCharacter>()) {
    max_id = std::max(max_id, c.id());
  }
  ABSL_CHECK_LT(max_id, 999);
  return max_id;
}

int MaxMySekaiGateId() {
  int max_id = 0;
  for (const auto& c : MasterDb::GetAll<db::MySekaiGate>()) {
    max_id = std::max(max_id, c.id());
  }
  ABSL_CHECK_LT(max_id, 999);
  return max_id;
}

}  // namespace

int AreaItemArraySize() {
  static int kArraySize = MaxId<db::AreaItem>() + 1;
  return kArraySize;
}

int CardArraySize() {
  static int kArraySize = MaxId<db::Card>() + 1;
  return kArraySize;
}

const std::vector<int>& UniqueCharacterIds() {
  static const std::vector<int>* const kUniqueIds = GetUniqueCharacterIds().release();
  return *kUniqueIds;
}

int CharacterArraySize() {
  static const int kArraySize = MaxCharacterId() + 1;
  return kArraySize;
}

int MySekaiGateArraySize() {
  static const int kArraySize = MaxMySekaiGateId() + 1;
  return kArraySize;
}

}  // namespace sekai
