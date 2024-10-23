#pragma once

#include <algorithm>

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

#define ASSIGN_OR_RETURN(dst, expr) ASSIGN_OR_RETURN_IMPL(CONCAT_NAME(tmp, __COUNTER__), dst, expr)

#define ASSIGN_OR_RETURN_MAP(dst, expr, fn) \
  ASSIGN_OR_RETURN_MAP_IMPL(CONCAT_NAME(tmp, __COUNTER__), dst, expr, fn)
