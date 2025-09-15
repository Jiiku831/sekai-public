#pragma once

#include <algorithm>

#include "absl/base/attributes.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"

#define RETURN_IF_ERROR(expr) \
  if (absl::Status status = expr; !status.ok()) return status

#define RETURN_IF_ERROR_MAP(expr, fn) \
  if (absl::Status status = expr; !status.ok()) return fn(status)

template <typename T>
absl::Status AssignOrReturn(T& lhs, absl::StatusOr<T> rhs) {
  if (rhs.ok()) {
    lhs = *std::move(rhs);
  }
  return rhs.status();
}

#define CONCAT_NAME_INNER(x, y) x##y
#define CONCAT_NAME(x, y) CONCAT_NAME_INNER(x, y)

#define ASSIGN_OR_RETURN_VOID_IMPL(tmp, dst, expr) \
  auto tmp = expr;                                 \
  if (!tmp.ok()) {                                 \
    return;                                        \
  }                                                \
  dst = *std::move(tmp);

#define ASSIGN_OR_RETURN_IMPL(tmp, dst, expr) \
  auto tmp = expr;                            \
  if (!tmp.ok()) {                            \
    return tmp.status();                      \
  }                                           \
  dst = *std::move(tmp);

#define ASSIGN_OR_RETURN_MAP_IMPL(tmp, dst, expr, fn) \
  auto tmp = expr;                                    \
  if (!tmp.ok()) {                                    \
    return fn(tmp.status());                          \
  }                                                   \
  dst = *std::move(tmp);

#define ASSIGN_OR_RETURN_VOID(dst, expr) \
  ASSIGN_OR_RETURN_VOID_IMPL(CONCAT_NAME(tmp, __COUNTER__), dst, expr)

#define ASSIGN_OR_RETURN(dst, expr) ASSIGN_OR_RETURN_IMPL(CONCAT_NAME(tmp, __COUNTER__), dst, expr)

#define ASSIGN_OR_RETURN_MAP(dst, expr, fn) \
  ASSIGN_OR_RETURN_MAP_IMPL(CONCAT_NAME(tmp, __COUNTER__), dst, expr, fn)

inline void ReturnVoid(const absl::Status& status) {}

template <typename T>
struct ReturnValue {
 public:
  explicit ReturnValue(const T& v ABSL_ATTRIBUTE_LIFETIME_BOUND) : v_(v) {}

  const T& operator()(const absl::Status& status) { return v_; }

 private:
  const T& v_;
};

template <typename T, typename Fn>
absl::StatusOr<typename std::invoke_result_t<Fn, T>> MapStatusOr(Fn fn,
                                                                 const absl::StatusOr<T>& val) {
  RETURN_IF_ERROR(val.status());
  return fn(*val);
}

template <typename T, typename Fn>
absl::StatusOr<typename std::invoke_result_t<Fn, T>> MapStatusOr(Fn fn, absl::StatusOr<T>&& val) {
  RETURN_IF_ERROR(val.status());
  return fn(*std::move(val));
}

template <typename T>
absl::StatusOr<T> ReduceStatusOr(const absl::StatusOr<absl::StatusOr<T>>& val) {
  RETURN_IF_ERROR(val.status());
  return *val;
}

template <typename T>
absl::StatusOr<T> ReduceStatusOr(absl::StatusOr<absl::StatusOr<T>>&& val) {
  RETURN_IF_ERROR(val.status());
  return *std::move(val);
}

template <typename T, typename Fn>
absl::StatusOr<typename std::invoke_result_t<Fn, T>::value_type> BindStatusOr(
    Fn fn, const absl::StatusOr<T>& val) {
  return ReduceStatusOr(MapStatusOr(fn, val));
}

template <typename T, typename Fn>
absl::StatusOr<typename std::invoke_result_t<Fn, T>::value_type> BindStatusOr(
    Fn fn, absl::StatusOr<T>&& val) {
  return ReduceStatusOr(MapStatusOr(fn, val));
}
