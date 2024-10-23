#pragma once

#include <filesystem>
#include <memory>
#include <string>

#include "absl/base/nullability.h"
#include "absl/status/statusor.h"
#include "git/commit.h"
#include "git/index.h"
#include "git/object_id.h"
#include "git/reference.h"

extern "C" {
#include "git2/repository.h"
#include "git2/types.h"
}

namespace git {

class Repository {
 public:
  // Transfers ownership of the underlying git repository.
  explicit Repository(absl::Nonnull<git_repository*> repo);

  static absl::StatusOr<Repository> Init(std::filesystem::path path);
  static absl::StatusOr<Repository> Open(std::filesystem::path path);

  absl::StatusOr<Commit> LookupCommit(const ObjectId& oid);
  absl::StatusOr<Reference> LookupReference(const std::string& name);

  absl::StatusOr<Index> RepositoryIndex();

 private:
  absl::Nonnull<std::unique_ptr<git_repository, decltype(&git_repository_free)>> repo_;
};

}  // namespace git
