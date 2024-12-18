#pragma once

#include <memory>

#include "absl/base/nullability.h"
#include "absl/status/statusor.h"
#include "git/object_id.h"

extern "C" {
#include "git2/refs.h"
#include "git2/types.h"
}

namespace git {

class Reference {
 public:
  // Transfers ownership of the underlying pointer.
  explicit Reference(absl::Nonnull<git_reference*> ref);

  absl::StatusOr<ObjectId> Target() const;

 private:
  absl::Nonnull<std::unique_ptr<git_reference, decltype(&git_reference_free)>> ref_;
};

}  // namespace git
