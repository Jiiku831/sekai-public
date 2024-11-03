#pragma once

#include <memory>

#include "absl/base/nullability.h"
#include "absl/status/statusor.h"
#include "git/object_id.h"

extern "C" {
#include "git2/index.h"
#include "git2/types.h"
}

namespace git {

class Index {
 public:
  // Transfers ownership of the underlying pointer.
  explicit Index(absl::Nonnull<git_index*> index);

  absl::StatusOr<ObjectId> WriteTree();

 private:
  absl::Nonnull<std::unique_ptr<git_index, decltype(&git_index_free)>> index_;
};

}  // namespace git
