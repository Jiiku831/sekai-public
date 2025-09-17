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
  explicit Commit(git_commit* absl_nonnull repo);

  std::string Message() const;
  std::string RawMessage() const;

 private:
  std::unique_ptr<git_commit, decltype(&git_commit_free) absl_nonnull> commit_;
};

}  // namespace git
