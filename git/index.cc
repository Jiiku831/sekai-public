#include "git/index.h"

#include "absl/base/nullability.h"
#include "absl/log/absl_check.h"
#include "absl/status/statusor.h"
#include "git/errors.h"
#include "git/object_id.h"

extern "C" {
#include "git2/index.h"
#include "git2/types.h"
}

namespace git {

Index::Index(absl::Nonnull<git_index*> index)
    : index_(
          index, +[](git_index* ptr) { git_index_free(ptr); }) {
  ABSL_CHECK_NE(index_, nullptr);
}

absl::StatusOr<ObjectId> Index::WriteTree() {
  git_oid oid;
  GIT_RETURN_IF_ERROR(git_index_write_tree(&oid, index_.get()));
  return ObjectId(oid);
}

}  // namespace git
