#pragma once

#include <bitset>
#include <cstddef>

#include <google/protobuf/message.h>

#include "absl/strings/str_format.h"
#include "absl/strings/str_join.h"
#include "sekai/db/proto/enums.pb.h"

namespace sekai {

#define BITSET_FORWARD_MUTATING_FN(op)      \
  ProtoEnumBitset<T, N, Descriptor>& op() { \
    data_.op();                             \
    return *this;                           \
  }

#define BITSET_FORWARD_MUTATING_OP(op)                                                    \
  ProtoEnumBitset<T, N, Descriptor>& op(const ProtoEnumBitset<T, N, Descriptor>& other) { \
    data_.op(other.data_);                                                                \
    return *this;                                                                         \
  }

#define BITSET_FORWARD_MUTATING_SHIFT_OP(op)               \
  ProtoEnumBitset<T, N, Descriptor>& op(std::size_t pos) { \
    data_.op(pos);                                         \
    return *this;                                          \
  }

#define BITSET_FORWARD_COPYING_UNARY_OP(op)               \
  ProtoEnumBitset<T, N, Descriptor> op() const {          \
    return ProtoEnumBitset<T, N, Descriptor>(data_.op()); \
  }

#define BITSET_FORWARD_COPYING_BINARY_OP(op, op2)                                              \
  ProtoEnumBitset<T, N, Descriptor> op(const ProtoEnumBitset<T, N, Descriptor>& other) const { \
    return ProtoEnumBitset<T, N, Descriptor>(data_ op2 other.data_);                           \
  }

#define BITSET_FORWARD_COPYING_SHIFT_OP(op)                     \
  ProtoEnumBitset<T, N, Descriptor> op(std::size_t pos) const { \
    return ProtoEnumBitset<T, N, Descriptor>(data_.op(pos));    \
  }

template <typename T, size_t N, auto Descriptor>
class ProtoEnumBitset {
 public:
  using reference = typename std::bitset<N>::reference;
  constexpr ProtoEnumBitset() = default;
  ProtoEnumBitset(T v) { data_.set(v); }
  ProtoEnumBitset(std::bitset<N>&& data) : data_(std::move(data)) {}
  constexpr ProtoEnumBitset(uint64_t val) : data_(val) {}

  bool operator==(const ProtoEnumBitset<T, N, Descriptor>& rhs) const { return data_ == rhs.data_; }

  bool operator[](T val) const { return data_[static_cast<std::size_t>(val)]; }
  reference operator[](T val) { return data_[static_cast<std::size_t>(val)]; }
  bool test(T val) const { return data_.test(static_cast<std::size_t>(val)); }

  bool all() const { return data_.all(); }
  bool any() const { return data_.any(); }
  bool none() const { return data_.none(); }
  std::size_t count() const { return data_.count(); }
  std::size_t size() const { return data_.size(); }

  BITSET_FORWARD_MUTATING_OP(operator&=);
  BITSET_FORWARD_MUTATING_OP(operator|=);
  BITSET_FORWARD_MUTATING_OP(operator^=);
  BITSET_FORWARD_COPYING_UNARY_OP(operator~);
  BITSET_FORWARD_MUTATING_SHIFT_OP(operator<<=);
  BITSET_FORWARD_MUTATING_SHIFT_OP(operator>>=);
  BITSET_FORWARD_COPYING_SHIFT_OP(operator<<);
  BITSET_FORWARD_COPYING_SHIFT_OP(operator>>);
  BITSET_FORWARD_MUTATING_FN(set);
  BITSET_FORWARD_MUTATING_FN(reset);
  BITSET_FORWARD_MUTATING_FN(flip);
  BITSET_FORWARD_COPYING_BINARY_OP(operator&, &);
  BITSET_FORWARD_COPYING_BINARY_OP(operator|, |);
  BITSET_FORWARD_COPYING_BINARY_OP(operator^, ^);

  ProtoEnumBitset<T, N, Descriptor> set(T v) {
    data_.set(v);
    return *this;
  }

  ProtoEnumBitset<T, N, Descriptor> set(T v, bool x) {
    data_.set(v, x);
    return *this;
  }

  ProtoEnumBitset<T, N, Descriptor> reset(T v) {
    data_.reset(v);
    return *this;
  }

  template <typename Sink>
  friend void AbslStringify(Sink& sink, const ProtoEnumBitset<T, N, Descriptor>& bitset) {
    static const google::protobuf::EnumDescriptor* const descriptor = Descriptor();
    std::vector<std::string> values;
    for (int i = 0; i < descriptor->value_count(); ++i) {
      const google::protobuf::EnumValueDescriptor* value = descriptor->value(i);
      if (bitset.data_.test(value->number())) {
        values.push_back(absl::StrFormat("%s (%d)", value->name(), value->number()));
      }
    }
    absl::Format(&sink, "%s: %s", bitset.data_.to_string('-'), absl::StrJoin(values, " | "));
  }

 private:
  std::bitset<N> data_;
};

#undef BITSET_FORWARD_MUTATING_FN
#undef BITSET_FORWARD_MUTATING_OP
#undef BITSET_FORWARD_MUTATING_SHIFT_OP
#undef BITSET_FORWARD_COPYING_UNARY_OP
#undef BITSET_FORWARD_COPYING_BINARY_OP
#undef BITSET_FORWARD_COPYING_SHIFT_OP

#define DECLARE_BITSET(name) \
  using name = ProtoEnumBitset<db::name, db::name##_ARRAYSIZE, db::name##_descriptor>;

DECLARE_BITSET(Attr)
DECLARE_BITSET(CardParameterType)
DECLARE_BITSET(CardRarityType)
DECLARE_BITSET(Unit)

#undef DECLARE_BITSET

using Character = std::bitset<27>;

}  // namespace sekai
