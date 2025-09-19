#include "git/repository.h"

#include "absl/base/nullability.h"
#include "absl/log/absl_check.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "git/commit.h"
#include "git/errors.h"

extern "C" {
#include "git2/commit.h"
#include "git2/refs.h"
#include "git2/repository.h"
#include "git2/types.h"
}

namespace git {

Repository::Repository(git_repository* absl_nonnull repo)
    : repo_(repo, +[](git_repository* ptr) { git_repository_free(ptr); }) {
  ABSL_CHECK_NE(repo_, nullptr);
}

absl::StatusOr<Commit> Repository::LookupCommit(const ObjectId& oid) {
  git_commit* commit = nullptr;
  GIT_RETURN_IF_ERROR(git_commit_lookup(&commit, repo_.get(), &oid.oid()));
  if (commit == nullptr) {
    return absl::UnknownError("git_commit is null after lookup");
  }
  return Commit(commit);
}

absl::StatusOr<Reference> Repository::LookupReference(const std::string& name) {
  git_reference* ref = nullptr;
  GIT_RETURN_IF_ERROR(git_reference_lookup(&ref, repo_.get(), name.c_str()));
  if (ref == nullptr) {
    return absl::UnknownError("git_reference is null after lookup");
  }
  return Reference(ref);
}

absl::StatusOr<Repository> Repository::Init(std::filesystem::path path) {
  git_repository* repo = nullptr;
  GIT_RETURN_IF_ERROR(git_repository_init(&repo, path.c_str(), false));
  if (repo == nullptr) {
    return absl::UnknownError("git_repository is null after init");
  }
  return Repository(repo);
}

absl::StatusOr<Repository> Repository::Open(std::filesystem::path path) {
  git_repository* repo = nullptr;
  GIT_RETURN_IF_ERROR(git_repository_open(&repo, path.c_str()));
  if (repo == nullptr) {
    return absl::UnknownError("git_repository is null after open");
  }
  return Repository(repo);
}

absl::StatusOr<Index> Repository::RepositoryIndex() {
  git_index* index = nullptr;
  GIT_RETURN_IF_ERROR(git_repository_index(&index, repo_.get()));
  if (index == nullptr) {
    return absl::UnknownError("git_index is null after git_repository_index");
  }
  return Index(index);
}

}  // namespace git
