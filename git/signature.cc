#include "git/signature.h"

#include "absl/base/nullability.h"
#include "absl/log/absl_check.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/time/civil_time.h"
#include "absl/time/time.h"
#include "git/errors.h"

extern "C" {
#include "git2/signature.h"
#include "git2/types.h"
}

namespace git {

Signature::Signature(absl::Nonnull<git_signature*> sig)
    : sig_(
          sig, +[](git_signature* ptr) { git_signature_free(ptr); }) {
  ABSL_CHECK_NE(sig_, nullptr);
}

absl::StatusOr<Signature> Signature::New(const std::string& name, const std::string& email,
                                         absl::Time time, absl::TimeZone tz) {
  int64_t ts = absl::ToUnixSeconds(time);
  constexpr absl::CivilSecond ref(2000, 1, 1, 0, 0, 0);
  int64_t offset =
      (absl::FromCivil(ref, absl::UTCTimeZone()) - absl::FromCivil(ref, tz)) / absl::Minutes(1);
  git_signature* sig = nullptr;
  GIT_RETURN_IF_ERROR(git_signature_new(&sig, name.c_str(), email.c_str(), ts, offset));
  if (sig == nullptr) {
    return absl::UnknownError("git_signature is null after new signature");
  }
  return Signature(sig);
}

}  // namespace git
