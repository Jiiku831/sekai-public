#pragma once

#include <filesystem>
#include <fstream>
#include <stack>
#include <string>
#include <string_view>
#include <vector>

#include <google/protobuf/message.h>
#include <nlohmann/json.hpp>

#include "absl/base/nullability.h"
#include "absl/container/flat_hash_map.h"
#include "absl/log/absl_check.h"
#include "absl/log/log.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_format.h"
#include "sekai/config.h"
#include "sekai/db/proto/descriptor.pb.h"

namespace sekai::db {

namespace internal {

using json = ::nlohmann::json;
namespace proto = ::google::protobuf;

template <typename T>
class JsonParser : public json::json_sax_t {
 public:
  bool binary(json::binary_t& val) override {
    status_ = absl::UnimplementedError("binary data not supported.");
    return false;
  }

  bool boolean(bool val) override {
    if (obj_stack_.empty()) {
      status_ = absl::InternalError("empty stack");
      return false;
    }
    auto& cur_obj = obj_stack_.top();
    if (cur_obj.field == nullptr) {
      status_ = absl::InternalError(absl::StrFormat("Field with key '%s' is null", cur_obj.key));
      return false;
    }
    proto::Message* msg = cur_obj.msg;
    const proto::FieldDescriptor* field = cur_obj.field;
    if (field->cpp_type() != proto::FieldDescriptor::CPPTYPE_BOOL) {
      status_ = absl::FailedPreconditionError(
          absl::StrFormat("Case not handled:\n"
                          "Message: %s\n"
                          "Field: %s\n"
                          "Type: %s (expected bool)",
                          cur_obj.descriptor->full_name(), field->name(), field->cpp_type_name()));
      return false;
    }
    const proto::Reflection* reflection = cur_obj.reflection;
    const bool repeated = field->is_repeated();
    if (repeated)
      reflection->AddBool(msg, field, val);
    else
      reflection->SetBool(msg, field, val);
    return true;
  }

  bool end_array() override { return true; }

  bool end_object() override {
    if (obj_stack_.empty()) {
      status_ = absl::InternalError("empty stack");
      return false;
    }
    obj_stack_.pop();
    return true;
  }

  bool key(json::string_t& val) override {
    if (obj_stack_.empty()) {
      status_ = absl::InternalError("empty stack");
      return false;
    }
    auto& cur_obj = obj_stack_.top();
    cur_obj.key = val;
    cur_obj.field = LookupField(*cur_obj.descriptor, val);
    // Accept nullptr here for values that are always empty.
    return true;
  }

  bool null() override { return true; }

  bool number_float(json::number_float_t val, const json::string_t& s) override {
    if (obj_stack_.empty()) {
      status_ = absl::InternalError("empty stack");
      return false;
    }
    auto& cur_obj = obj_stack_.top();
    if (cur_obj.field == nullptr) {
      status_ = absl::InternalError(absl::StrFormat("Field with key '%s' is null", cur_obj.key));
      return false;
    }
    proto::Message* msg = cur_obj.msg;
    const proto::FieldDescriptor* field = cur_obj.field;
    const proto::Reflection* reflection = cur_obj.reflection;
    const bool repeated = field->is_repeated();
    switch (field->cpp_type()) {
      case proto::FieldDescriptor::CPPTYPE_DOUBLE:
        if (repeated)
          reflection->AddDouble(msg, field, val);
        else
          reflection->SetDouble(msg, field, val);
        return true;
      case proto::FieldDescriptor::CPPTYPE_FLOAT:
        if (repeated)
          reflection->AddFloat(msg, field, val);
        else
          reflection->SetFloat(msg, field, val);
        return true;
      default:
        status_ = absl::FailedPreconditionError(absl::StrFormat(
            "Case not handled:\n"
            "Message: %s\n"
            "Field: %s\n"
            "Type: %s (expected float or double)",
            cur_obj.descriptor->full_name(), field->name(), field->cpp_type_name()));
        return false;
    }
  }

  bool number_integer(json::number_integer_t val) override { return number_unsigned(val); }

  bool number_unsigned(json::number_unsigned_t val) override {
    if (obj_stack_.empty()) {
      status_ = absl::InternalError("empty stack");
      return false;
    }
    auto& cur_obj = obj_stack_.top();
    if (cur_obj.field == nullptr) {
      status_ = absl::InternalError(absl::StrFormat("Field with key '%s' is null", cur_obj.key));
      return false;
    }
    proto::Message* msg = cur_obj.msg;
    const proto::FieldDescriptor* field = cur_obj.field;
    const proto::Reflection* reflection = cur_obj.reflection;
    const bool repeated = field->is_repeated();
    switch (field->cpp_type()) {
      case proto::FieldDescriptor::CPPTYPE_INT32:
        if (repeated)
          reflection->AddInt32(msg, field, val);
        else
          reflection->SetInt32(msg, field, val);
        return true;
      case proto::FieldDescriptor::CPPTYPE_INT64:
        if (repeated)
          reflection->AddInt64(msg, field, val);
        else
          reflection->SetInt64(msg, field, val);
        return true;
      case proto::FieldDescriptor::CPPTYPE_UINT32:
        if (repeated)
          reflection->AddUInt32(msg, field, val);
        else
          reflection->SetUInt32(msg, field, val);
        return true;
      case proto::FieldDescriptor::CPPTYPE_UINT64:
        if (repeated)
          reflection->AddUInt64(msg, field, val);
        else
          reflection->SetUInt64(msg, field, val);
        return true;
      case proto::FieldDescriptor::CPPTYPE_DOUBLE:
        if (repeated)
          reflection->AddDouble(msg, field, val);
        else
          reflection->SetDouble(msg, field, val);
        return true;
      case proto::FieldDescriptor::CPPTYPE_FLOAT:
        if (repeated)
          reflection->AddFloat(msg, field, val);
        else
          reflection->SetFloat(msg, field, val);
        return true;
      default:
        status_ = absl::FailedPreconditionError(absl::StrFormat(
            "Case not handled:\n"
            "Message: %s\n"
            "Field: %s\n"
            "Type: %s (expected integral or floating)",
            cur_obj.descriptor->full_name(), field->name(), field->cpp_type_name()));
        return false;
    }
  }

  bool parse_error(std::size_t pos, const std::string& last_token,
                   const json::exception& ex) override {
    status_ = absl::UnimplementedError("unhandled parse error");
    return false;
  }

  bool start_array(std::size_t size) override { return true; }

  bool start_object(std::size_t size) override {
    if (obj_stack_.empty()) {
      objs_.emplace_back();
      obj_stack_.emplace(objs_.back());
      return true;
    }
    auto& cur_obj = obj_stack_.top();
    if (cur_obj.field == nullptr) {
      status_ = absl::InternalError(absl::StrFormat("Field with key '%s' is null", cur_obj.key));
      return false;
    }
    proto::Message* msg = cur_obj.msg;
    const proto::FieldDescriptor* field = cur_obj.field;
    if (field->cpp_type() != proto::FieldDescriptor::CPPTYPE_MESSAGE) {
      status_ = absl::FailedPreconditionError(
          absl::StrFormat("Case not handled:\n"
                          "Message: %s\n"
                          "Field: %s\n"
                          "Type: %s (expected message)",
                          cur_obj.descriptor->full_name(), field->name(), field->cpp_type_name()));
      return false;
    }
    const proto::Reflection* reflection = cur_obj.reflection;
    const bool repeated = field->is_repeated();
    if (repeated) {
      obj_stack_.emplace(*reflection->AddMessage(msg, field));
    } else {
      obj_stack_.emplace(*reflection->MutableMessage(msg, field));
    }
    return true;
  }

  bool string(json::string_t& val) override {
    if (obj_stack_.empty()) {
      status_ = absl::InternalError("empty stack");
      return false;
    }
    auto& cur_obj = obj_stack_.top();
    if (cur_obj.field == nullptr) {
      status_ = absl::InternalError(absl::StrFormat("Field with key '%s' is null", cur_obj.key));
      return false;
    }
    proto::Message* msg = cur_obj.msg;
    const proto::FieldDescriptor* field = cur_obj.field;
    const proto::Reflection* reflection = cur_obj.reflection;
    const bool repeated = field->is_repeated();

    std::string_view empty_value = field->options().GetExtension(string_empty_value);
    if (!empty_value.empty() && val == empty_value) return true;

    switch (field->cpp_type()) {
      case proto::FieldDescriptor::CPPTYPE_ENUM: {
        absl::Nullable<const proto::EnumValueDescriptor*> enum_value =
            LookupEnumValue(*field->enum_type(), val);
        if (enum_value == nullptr) {
          status_ = absl::InvalidArgumentError(
              absl::StrFormat("Unrecognized enum value:\n"
                              "Message: %s\n"
                              "Field: %s\n"
                              "Type: %s\n"
                              "Value: %s\n",
                              cur_obj.descriptor->full_name(), field->name(),
                              field->enum_type()->full_name(), val));
          return false;
        }
        if (repeated)
          reflection->AddEnum(msg, field, enum_value);
        else
          reflection->SetEnum(msg, field, enum_value);
        return true;
      }
      case proto::FieldDescriptor::CPPTYPE_STRING:
        if (repeated)
          reflection->AddString(msg, field, val);
        else
          reflection->SetString(msg, field, val);
        return true;
      default:
        status_ = absl::FailedPreconditionError(absl::StrFormat(
            "Case not handled:\n"
            "Message: %s\n"
            "Field: %s\n"
            "Type: %s (expected string or enum)",
            cur_obj.descriptor->full_name(), field->name(), field->cpp_type_name()));
        return false;
    }
  }

  const std::vector<T>& objs() const { return objs_; }

  const absl::Status& status() const { return status_; }

 private:
  struct StackObj {
    explicit StackObj(proto::Message& msg)
        : msg(&msg), descriptor(msg.GetDescriptor()), reflection(msg.GetReflection()) {}
    absl::Nonnull<proto::Message*> msg;
    absl::Nonnull<const proto::Descriptor*> descriptor;
    absl::Nonnull<const proto::Reflection*> reflection;
    absl::Nullable<const proto::FieldDescriptor*> field = nullptr;
    std::string key;
  };
  std::stack<StackObj> obj_stack_;
  std::vector<T> objs_;
  absl::Status status_ = absl::OkStatus();

  using FieldCache = absl::flat_hash_map<std::string, absl::Nonnull<const proto::FieldDescriptor*>>;
  absl::flat_hash_map<std::string, FieldCache> field_caches_;

  const FieldCache& GetFieldCacheEntry(const proto::Descriptor& descriptor) {
    auto& entry = field_caches_[descriptor.full_name()];
    if (!entry.empty()) {
      return entry;
    }
    absl::flat_hash_map<std::string, absl::Nonnull<const proto::FieldDescriptor*>> cache;
    for (int i = 0; i < descriptor.field_count(); ++i) {
      absl::Nonnull<const proto::FieldDescriptor*> field = descriptor.field(i);
      std::string_view key_name = field->options().GetExtension(json_name);
      cache[key_name] = field;
    }
    return entry = std::move(cache);
  }

  absl::Nullable<const proto::FieldDescriptor*> LookupField(const proto::Descriptor& descriptor,
                                                            std::string_view json_key) {
    const FieldCache& cache = GetFieldCacheEntry(descriptor);
    auto it = cache.find(json_key);
    if (it == cache.end()) return nullptr;
    return it->second;
  }

  using EnumValueCache =
      absl::flat_hash_map<std::string, absl::Nonnull<const proto::EnumValueDescriptor*>>;
  absl::flat_hash_map<std::string, EnumValueCache> enum_value_caches_;

  const EnumValueCache& GetEnumValueCacheEntry(const proto::EnumDescriptor& descriptor) {
    auto& entry = enum_value_caches_[descriptor.full_name()];
    if (!entry.empty()) {
      return entry;
    }
    absl::flat_hash_map<std::string, absl::Nonnull<const proto::EnumValueDescriptor*>> cache;
    for (int i = 0; i < descriptor.value_count(); ++i) {
      absl::Nonnull<const proto::EnumValueDescriptor*> value = descriptor.value(i);
      std::string_view key_name = value->options().GetExtension(json_value);
      cache[key_name] = value;
    }
    return entry = std::move(cache);
  }

  absl::Nullable<const proto::EnumValueDescriptor*> LookupEnumValue(
      const proto::EnumDescriptor& descriptor, std::string_view json_value) {
    const EnumValueCache& cache = GetEnumValueCacheEntry(descriptor);
    auto it = cache.find(json_value);
    if (it == cache.end()) return nullptr;
    return it->second;
  }
};

}  // namespace internal

template <typename T>
absl::StatusOr<std::vector<T>> ParseJsonFromString(std::string_view json_str) {
  internal::JsonParser<T> parser;
  bool status = nlohmann::json::sax_parse(json_str, &parser);
  if (status) {
    return parser.objs();
  }
  if (parser.status().ok()) {
    return absl::InternalError("Parser failed but no error status.");
  }
  return parser.status();
}

template <typename T>
absl::StatusOr<std::vector<T>> ParseJsonFromFile(const std::filesystem::path path) {
  if (!std::filesystem::exists(path)) {
    LOG(INFO) << "File not found: " << path;
    return std::vector<T>{};
  }
  std::ifstream ifs;
  internal::JsonParser<T> parser;
  ifs.open(path, std::ifstream::in);
  ABSL_CHECK(ifs.is_open()) << path;
  bool status = nlohmann::json::sax_parse(ifs, &parser);
  if (status) {
    return parser.objs();
  }
  if (parser.status().ok()) {
    return absl::InternalError("Parser failed but no error status.");
  }
  return parser.status();
}

template <typename T>
absl::StatusOr<std::vector<T>> ParseJson() {
  std::filesystem::path path;
  if (!T::GetDescriptor()->options().GetExtension(master_db_file).empty()) {
    path = MasterDbRoot() / T::GetDescriptor()->options().GetExtension(master_db_file);
  }
  if (!T::GetDescriptor()->options().GetExtension(sekai_best_file).empty()) {
    path = SekaiBestRoot() / T::GetDescriptor()->options().GetExtension(sekai_best_file);
  }
  if (path.empty()) {
    return absl::FailedPreconditionError("No configured data source.");
  }
  absl::StatusOr<std::vector<T>> result = ParseJsonFromFile<T>(path);
  if (!result.ok()) {
    LOG(FATAL) << "While parsing " << path << " got error: " << result.status();
  }
  return result;
}

}  // namespace sekai::db
