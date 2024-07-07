#include "frontend/util.h"

#include <string>
#include <string_view>

#include "sekai/db/master_db.h"

namespace frontend {

using ::sekai::db::MasterDb;

std::string MaybeEmbedImage(const std::string& path) {
  std::string embedded_data = std::string(MasterDb::FindThumbnail(path));
  if (embedded_data.empty()) return path;
  return embedded_data;
}

}  // namespace frontend
