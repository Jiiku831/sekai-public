#pragma once

#include <filesystem>
#include <memory>
#include <string>

#include "absl/base/nullability.h"

extern "C" {
#include "git2/commit.h"
#include "git2/types.h"
}

namespace git {

class Commit {
 public:
  // Transfers ownership of the underlying git repository.
  explicit Commit(absl::Nonnull<git_commit*> repo);

  std::string Message() const;
  std::string RawMessage() const;

 private:
  absl::Nonnull<std::unique_ptr<git_commit, decltype(&git_commit_free)>> commit_;
};

}  // namespace git
