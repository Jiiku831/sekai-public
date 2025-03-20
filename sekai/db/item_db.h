#pragma once

#include <span>
#include <vector>

#include <google/protobuf/message.h>

#include "absl/base/nullability.h"
#include "absl/container/flat_hash_map.h"
#include "absl/functional/any_invocable.h"
#include "absl/log/absl_check.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_format.h"
#include "sekai/db/proto/descriptor.pb.h"
#include "sekai/db/proto/records.pb.h"

namespace sekai::db {

template <typename T>
class ItemDb {
 public:
  ItemDb(absl::StatusOr<std::vector<T>>&& objs) {
    if (objs.ok()) {
      objs_ = *std::move(objs);
    } else {
      status_ = objs.status();
    }
    MaybeIndexObjs();
  }
  ItemDb(const ItemDb&) = delete;
  ItemDb(ItemDb&&) = delete;

  const absl::Status status() const { return status_; }

  const T& FindFirst(int64_t key) const {
    const T* obj = SafeFindFirst(key);
    ABSL_CHECK_NE(obj, nullptr) << "While looking up item type " << typeid(T).name() << " with key "
                                << key;
    return *obj;
  }

  const T* SafeFindFirst(int64_t key) const {
    auto it = indexed_objs_.find(key);
    if (it == indexed_objs_.end() || it->second.empty() || it->second.front() == nullptr) {
      return nullptr;
    }
    return it->second.front();
  }

  const std::vector<absl::Nonnull<const T*>>& FindAll(int64_t key) const {
    auto it = indexed_objs_.find(key);
    if (it == indexed_objs_.end()) return empty_;
    return it->second;
  }

  std::span<const T> GetAll() const { return objs_; }

  std::vector<absl::Nonnull<const T*>> GetIf(absl::AnyInvocable<bool(const T&) const> pred) const {
    std::vector<absl::Nonnull<const T*>> matching;
    for (const T& obj : objs_) {
      if (pred(obj)) {
        matching.push_back(&obj);
      }
    }
    return matching;
  }

  Records ToRecords(std::span<const std::string> allowlist) const {
    Records records;
    if (!allowlist.empty() && !absl::c_linear_search(allowlist, T::GetDescriptor()->name())) {
      return records;
    }
    for (const T& obj : objs_) {
      records.add_records()->PackFrom(obj, "");
    }
    return records;
  }

  std::string DebugString() const {
    return absl::StrFormat("%-33s: %s (%lu records, %lu keys)", T::GetDescriptor()->name(),
                           status_.ToString(), objs_.size(), indexed_objs_.size());
  }

 private:
  void MaybeIndexObjs() {
    absl::Nullable<const google::protobuf::FieldDescriptor*> primary_key_field = GetPrimaryKey();
    if (primary_key_field == nullptr) return;
    if (primary_key_field->is_repeated() || primary_key_field->is_map()) {
      status_ = absl::FailedPreconditionError(
          absl::StrFormat("Primary key %s must be singular", primary_key_field->name()));
    }
    const google::protobuf::Reflection* reflection = T::GetReflection();
    for (const T& obj : objs_) {
      std::optional<int64_t> int_key;
      switch (primary_key_field->cpp_type()) {
        case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
          int_key = reflection->GetInt32(obj, primary_key_field);
          break;
        case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
          int_key = reflection->GetInt64(obj, primary_key_field);
          break;
        case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
          int_key = reflection->GetEnumValue(obj, primary_key_field);
          break;
        default:
          status_ = absl::FailedPreconditionError(absl::StrFormat(
              "Primary key %s has unsupported type %s in %s", primary_key_field->name(),
              primary_key_field->cpp_type_name(), T::GetDescriptor()->full_name()));
          return;
      }
      if (int_key.has_value()) {
        indexed_objs_[*int_key].push_back(&obj);
      }
    }
  }

  absl::Nullable<const google::protobuf::FieldDescriptor*> GetPrimaryKey() {
    static absl::Nullable<const google::protobuf::FieldDescriptor*> const primary_key_field =
        []() -> absl::Nullable<const google::protobuf::FieldDescriptor*> {
      const google::protobuf::Descriptor* descriptor = T::GetDescriptor();
      for (int i = 0; i < descriptor->field_count(); ++i) {
        const google::protobuf::FieldDescriptor* field = descriptor->field(i);
        if (field->options().GetExtension(primary_key)) {
          return field;
        }
      }
      return nullptr;
    }();
    return primary_key_field;
  }

  absl::Status status_ = absl::OkStatus();
  std::vector<T> objs_;
  absl::flat_hash_map<int64_t, std::vector<absl::Nonnull<const T*>>> indexed_objs_;
  std::vector<absl::Nonnull<const T*>> empty_;
};

}  // namespace sekai::db
