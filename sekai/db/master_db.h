#pragma once

#include <algorithm>
#include <concepts>
#include <filesystem>
#include <memory>
#include <string_view>
#include <tuple>
#include <vector>

#include <google/protobuf/any.h>

#include "absl/container/flat_hash_map.h"
#include "absl/functional/any_invocable.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_join.h"
#include "absl/time/clock.h"
#include "absl/time/time.h"
#include "sekai/config.h"
#include "sekai/db/item_db.h"
#include "sekai/db/proto/all.h"
#include "sekai/proto_util.h"

namespace sekai::db {

namespace internal {

constexpr std::string_view kFlatDbPath = "data/flat-db";
constexpr std::string_view kMinimalFlatDbPath = "data/flat-db-minimal";
constexpr std::string_view kEmptyString = "";

template <typename... Ts>
class MasterDbImpl {
 public:
  MasterDbImpl(absl::Time start_load, absl::flat_hash_map<std::string, std::string>&& thumbnails,
               std::vector<Ts>&&... items)
      : thumbnails_(thumbnails),
        items_(std::make_tuple(std::move(items)...)),
        load_duration_(absl::Now() - start_load) {}

  template <typename T>
  const ItemDb<T>& Get() const {
    return std::get<ItemDb<T>>(items_);
  }

  std::vector<absl::Status> Status() const {
    std::vector<absl::Status> out;
    std::apply([&out](const auto&... items) { ((out.push_back(items.status())), ...); }, items_);
    return out;
  }

  std::string DebugString() const {
    std::string thumbnail_status = absl::StrFormat(
        "%-33s: OK (%lu records, %lu keys)", "Thumbnails", thumbnails_.size(), thumbnails_.size());
    return absl::StrFormat(
        "DB loaded in %s\n%s\n%s", absl::FormatDuration(load_duration_),
        std::apply(
            [](const auto&... items) { return absl::StrJoin({items.DebugString()...}, "\n"); },
            items_),
        thumbnail_status);
  }

  // Forward methods from ItemDb.
  template <typename T>
  const T& FindFirst(int64_t key) {
    return Get<T>().FindFirst(key);
  }

  template <typename T>
  const std::vector<absl::Nonnull<const T*>>& FindAll(int64_t key) {
    return Get<T>().FindAll(key);
  }

  template <typename T>
  std::span<const T> GetAll() {
    return Get<T>().GetAll();
  }

  template <typename T>
  std::vector<absl::Nonnull<const T*>> GetIf(absl::AnyInvocable<bool(const T&) const> pred) {
    return Get<T>().GetIf(std::move(pred));
  }

  // Returns empty string if not found.
  std::string_view FindThumbnail(std::string path) const {
    auto thumb = thumbnails_.find(path);
    if (thumb == thumbnails_.end()) {
      return kEmptyString;
    }
    return thumb->second;
  }

 private:
  MasterDbImpl() = default;
  absl::flat_hash_map<std::string, std::string> thumbnails_;
  std::tuple<ItemDb<Ts>...> items_;
  absl::Duration load_duration_;
};

template <typename T, typename... Ts>
void TryUnpack(const google::protobuf::Any& msg, std::tuple<std::vector<Ts>...>& out) {
  if (msg.Is<T>()) {
    std::vector<T>& msg_vec = std::get<std::vector<T>>(out);
    msg_vec.emplace_back();
    msg.UnpackTo(&msg_vec.back());
  }
}

template <typename... Ts>
std::pair<absl::Time, std::tuple<std::vector<Ts>...>> LoadItemsFromFlatDb(
    std::tuple<Ts...> unused, absl::flat_hash_map<std::string, std::string>& thumbnails,
    const std::string& data) {
  absl::Time start = absl::Now();
  std::filesystem::path db_path = MainRunfilesRoot() / kFlatDbPath;
  if (!data.empty()) {
    LOG(INFO) << "Loading data from string.";
  } else if (!std::filesystem::exists(db_path)) {
    LOG(INFO) << "Full flat-db not found, trying minimal flat-db.";
    db_path = MainRunfilesRoot() / kMinimalFlatDbPath;
  }
  auto records = data.empty() ? ReadCompressedBinaryProtoFile<Records>(db_path)
                              : ReadCompressedBinaryProto<Records>(data);
  std::tuple<std::vector<Ts>...> out;
  for (const google::protobuf::Any& record : records.records()) {
    ((TryUnpack<Ts>(record, out)), ...);
  }
  thumbnails = {records.thumbnails().begin(), records.thumbnails().end()};
  return std::make_pair(start, out);
}

template <typename... Ts>
std::unique_ptr<MasterDbImpl<Ts...>> CreateMasterDbImplFromItems(
    absl::Time start_load, absl::flat_hash_map<std::string, std::string>&& thumbnails,
    std::tuple<std::vector<Ts>...>&& items) {
  return std::apply(
      [start_load, &thumbnails](auto&&... items) {
        return std::make_unique<MasterDbImpl<Ts...>>(start_load, std::move(thumbnails),
                                                     std::move(items)...);
      },
      items);
}

}  // namespace internal

// Empty class for backwards compatibility.
class MasterDb {
 public:
  static auto Create(const std::string& data = "") {
    absl::flat_hash_map<std::string, std::string> thumbnails;
    auto [start_load, items] = internal::LoadItemsFromFlatDb(AllRecordTypes{}, thumbnails, data);
    return internal::CreateMasterDbImplFromItems(start_load, std::move(thumbnails),
                                                 std::move(items));
  }

  static const auto& Get(const std::string& data = "") {
    static const auto* const kMasterDb = MasterDb::Create(data).release();
    return *kMasterDb;
  }

  // Forward methods from ItemDb for the default static instance.
  template <typename T>
  static const T& FindFirst(int64_t key) {
    return Get().template Get<T>().FindFirst(key);
  }

  template <typename T>
  static const T* SafeFindFirst(int64_t key) {
    return Get().template Get<T>().SafeFindFirst(key);
  }

  template <typename T>
  static absl::StatusOr<const T*> FindFirstOrError(int64_t key) {
    auto res = SafeFindFirst<T>(key);
    if (res == nullptr) {
      return absl::NotFoundError(
          absl::StrFormat("Failed to find %s with key %lld.", T::GetDescriptor()->name(), key));
    }
    return res;
  }

  template <typename T>
  static const std::vector<absl::Nonnull<const T*>>& FindAll(int64_t key) {
    return Get().template Get<T>().FindAll(key);
  }

  template <typename T>
  static std::span<const T> GetAll() {
    return Get().template Get<T>().GetAll();
  }

  template <typename T>
  static std::vector<absl::Nonnull<const T*>> GetIf(absl::AnyInvocable<bool(const T&) const> pred) {
    return Get().template Get<T>().GetIf(std::move(pred));
  }

  // Returns empty string if not found.
  static std::string_view FindThumbnail(std::string path) { return Get().FindThumbnail(path); }
};

}  // namespace sekai::db
