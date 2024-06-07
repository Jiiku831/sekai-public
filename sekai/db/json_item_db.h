#pragma once

#include "sekai/db/item_db.h"
#include "sekai/db/json/parser.h"

namespace sekai::db {

template <typename T>
class JsonItemDb : public ItemDb<T> {
 public:
  JsonItemDb() : ItemDb<T>(ParseJson<T>()) {}
};

}  // namespace sekai::db
