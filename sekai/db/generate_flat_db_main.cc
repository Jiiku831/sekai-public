#include <filesystem>
#include <fstream>
#include <ios>
#include <string>
#include <string_view>
#include <tuple>

#include "absl/container/flat_hash_map.h"
#include "absl/flags/flag.h"
#include "absl/log/absl_check.h"
#include "absl/log/log.h"
#include "absl/strings/escaping.h"
#include "base/init.h"
#include "base/zstd.h"
#include "sekai/config.h"
#include "sekai/db/json_item_db.h"
#include "sekai/db/proto/all.h"
#include "sekai/file_util.h"
#include "sekai/proto_util.h"

ABSL_FLAG(std::string, output, "", "output path");
ABSL_FLAG(bool, enable_thumbnails, false, "whether or not to enabled thumbnails");

using namespace ::sekai::db;

constexpr std::string_view kDataUriPrefix = "data:image/webp;base64,";

template <typename... Ts>
class FlatDbGenerator {
 public:
  explicit FlatDbGenerator(std::tuple<Ts...> unused) {}

  void Write(std::filesystem::path path) {
    Records records;
    std::apply([&records](auto&&... items) { ((records.MergeFrom(items.ToRecords())), ...); },
               items_);
    if (absl::GetFlag(FLAGS_enable_thumbnails)) {
      AddEmbeddedThumbnails(records);
    }
    sekai::WriteCompressedBinaryProtoFile(path, records);
  }

 private:
  void AddEmbeddedThumbnails(Records& records) {
    for (const auto& [src, dst] : thumbnail_mapping_) {
      AddEmbeddedThumbnails(src, dst, records);
    }
  }

  void AddEmbeddedThumbnails(std::filesystem::path src, std::filesystem::path dst,
                             Records& records) {
    for (const auto& entry : std::filesystem::directory_iterator(sekai::MainRunfilesRoot() / src)) {
      if (!entry.is_regular_file()) {
        LOG(WARNING) << "Skipping non-regular file: " << entry.path();
        continue;
      }
      std::filesystem::path src_file = entry.path();
      // Need to add support for other file types if needed.
      ABSL_CHECK_EQ(src_file.extension(), ".webp");
      std::filesystem::path dst_file = dst / src_file.filename();
      AddEmbeddedThumbnail(src_file, dst_file, records);
    }
  }

  void AddEmbeddedThumbnail(std::filesystem::path src, std::filesystem::path dst,
                            Records& records) {
    std::string data = sekai::GetFileContents(src, std::ios::binary);
    (*records.mutable_thumbnails())[dst] = absl::StrCat(kDataUriPrefix, absl::Base64Escape(data));
  }

  std::tuple<JsonItemDb<Ts>...> items_;
  absl::flat_hash_map<std::string, std::string> thumbnail_mapping_ = {
      {"data/storage-sekai-best/sekai-assets/thumbnail/chara_32/", "/data/assets/32"},
  };
};

int main(int argc, char** argv) {
  Init(argc, argv);
  ABSL_CHECK(!absl::GetFlag(FLAGS_output).empty());

  FlatDbGenerator generator{AllRecordTypes{}};
  generator.Write(absl::GetFlag(FLAGS_output));
}
