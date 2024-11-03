#pragma once

#include "absl/status/status.h"
#include "base/util.h"

#define GIT_RETURN_IF_ERROR(expr) RETURN_IF_ERROR(WrapStatus(expr))

namespace git {

absl::Status WrapStatus(int return_code);

}  // namespace git
