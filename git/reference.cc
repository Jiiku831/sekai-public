#include "git/reference.h"

#include "absl/base/nullability.h"
#include "absl/log/absl_check.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "git/object_id.h"

extern "C" {
#include "git2/refs.h"
#include "git2/types.h"
}

namespace git {

Reference::Reference(git_reference* absl_nonnull ref)
    : ref_(ref, +[](git_reference* ptr) { git_reference_free(ptr); }) {
  ABSL_CHECK_NE(ref_, nullptr);
}

absl::StatusOr<ObjectId> Reference::Target() const {
  const git_oid* oid = git_reference_target(ref_.get());
  if (oid == nullptr) {
    return absl::InvalidArgumentError("reference target not available, see git_reference_target.");
  }
  return ObjectId(*oid);
}

}  // namespace git
