#include "git/errors.h"

#include "absl/status/status.h"
#include "absl/strings/str_format.h"

extern "C" {
#include "git2/errors.h"
}

namespace git {
namespace {

absl::StatusCode GitErrorClassToStatusCode(int error_class) {
  switch (error_class) {
    case GIT_ERROR_NONE:
      return absl::StatusCode::kOk;
    case GIT_ERROR_INVALID:
      return absl::StatusCode::kFailedPrecondition;
    case GIT_ERROR_NOMEMORY:
      return absl::StatusCode::kResourceExhausted;
    case GIT_ERROR_OS:
    case GIT_ERROR_REFERENCE:
    case GIT_ERROR_ZLIB:
    case GIT_ERROR_REPOSITORY:
    case GIT_ERROR_CONFIG:
    case GIT_ERROR_REGEX:
    case GIT_ERROR_ODB:
    case GIT_ERROR_INDEX:
    case GIT_ERROR_OBJECT:
    case GIT_ERROR_NET:
    case GIT_ERROR_TAG:
    case GIT_ERROR_TREE:
    case GIT_ERROR_INDEXER:
    case GIT_ERROR_SSL:
    case GIT_ERROR_SUBMODULE:
    case GIT_ERROR_THREAD:
    case GIT_ERROR_STASH:
    case GIT_ERROR_CHECKOUT:
    case GIT_ERROR_FETCHHEAD:
    case GIT_ERROR_MERGE:
    case GIT_ERROR_SSH:
    case GIT_ERROR_FILTER:
    case GIT_ERROR_REVERT:
    case GIT_ERROR_CALLBACK:
    case GIT_ERROR_CHERRYPICK:
    default:
      return absl::StatusCode::kUnknown;
  }
}

absl::StatusCode GitReturnCodeToStatusCode(int return_code) {
  switch (return_code) {
    case GIT_OK:
      return absl::StatusCode::kOk;
    case GIT_ERROR:
      return absl::StatusCode::kUnknown;
    case GIT_ENOTFOUND:
      return absl::StatusCode::kNotFound;
    case GIT_EEXISTS:
    case GIT_EAMBIGUOUS:
      return absl::StatusCode::kFailedPrecondition;
    case GIT_EBUFS:
      return absl::StatusCode::kResourceExhausted;
    case GIT_EUSER:
      return absl::StatusCode::kInternal;
    case GIT_EBAREREPO:
    case GIT_EUNBORNBRANCH:
    case GIT_EUNMERGED:
    case GIT_ENONFASTFORWARD:
      return absl::StatusCode::kFailedPrecondition;
    case GIT_EINVALIDSPEC:
      return absl::StatusCode::kInvalidArgument;
    case GIT_EMERGECONFLICT:
    case GIT_ELOCKED:
      return absl::StatusCode::kFailedPrecondition;
    case GIT_EMODIFIED:
      return absl::StatusCode::kAborted;
    case GIT_PASSTHROUGH:
      return absl::StatusCode::kInternal;
    case GIT_ITEROVER:
      return absl::StatusCode::kOutOfRange;
    default:
      return absl::StatusCode::kUnknown;
  }
}

}  // namespace

absl::Status WrapStatus(int return_code) {
  if (return_code >= GIT_OK) {
    return absl::OkStatus();
  }
  const git_error* last_error = git_error_last();
  if (last_error == nullptr) {
    return absl::OkStatus();
  }
  absl::StatusCode status_code = GitReturnCodeToStatusCode(return_code);
  if (status_code == absl::StatusCode::kUnknown) {
    status_code = GitErrorClassToStatusCode(last_error->klass);
  }
  return absl::Status(status_code, absl::StrFormat("%s (code=%d, class=%d)", last_error->message,
                                                   return_code, last_error->klass));
}

}  // namespace git
