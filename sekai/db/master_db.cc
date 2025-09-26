#include "sekai/db/master_db.h"

#include <string>

#include "absl/flags/flag.h"

ABSL_FLAG(
    std::string, full_db_path, "",
    "db exact location, will prefer if provided. otherwise will attempt to automatically locate.");

namespace sekai::db {
std::string GetFullDbPath() { return absl::GetFlag(FLAGS_full_db_path); }

}  // namespace sekai::db
