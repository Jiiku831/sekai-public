#include "git/init.h"

#include "absl/status/status.h"
#include "git/errors.h"

extern "C" {
#include "git2/global.h"
}

namespace git {

absl::Status Init() { return WrapStatus(git_libgit2_init()); }

}  // namespace git
