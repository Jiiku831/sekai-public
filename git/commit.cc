#include "git/commit.h"

#include "absl/base/nullability.h"
#include "absl/log/absl_check.h"

extern "C" {
#include "git2/commit.h"
#include "git2/types.h"
}

namespace git {

Commit::Commit(git_commit* absl_nonnull commit)
    : commit_(commit, +[](git_commit* ptr) { git_commit_free(ptr); }) {
  ABSL_CHECK_NE(commit_, nullptr);
}

std::string Commit::Message() const { return std::string(git_commit_message(commit_.get())); }

std::string Commit::RawMessage() const {
  return std::string(git_commit_message_raw(commit_.get()));
}

}  // namespace git
