#pragma once

#include <memory>

#include "absl/base/nullability.h"
#include "absl/status/statusor.h"
#include "absl/time/time.h"

extern "C" {
#include "git2/signature.h"
#include "git2/types.h"
}

namespace git {

class Signature {
 public:
  // Transfers ownership of the underlying pointer.
  explicit Signature(absl::Nonnull<git_signature*> sig);

  static absl::StatusOr<Signature> New(const std::string& name, const std::string& email,
                                       absl::Time time = absl::Now(),
                                       absl::TimeZone tz = absl::LocalTimeZone());

 private:
  absl::Nonnull<std::unique_ptr<git_signature, decltype(&git_signature_free)>> sig_;
};

}  // namespace git
