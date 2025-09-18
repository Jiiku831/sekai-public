#include "sekai/bonus_limit.h"

#include "absl/flags/flag.h"

ABSL_FLAG(
    int, override_mysekai_bonus_limit, -1,
    "If >= 0, then bonus limit is set to this regardless of what is configured in the master db.");

namespace sekai {

int GetMySekaiFixtureBonusLimit() {
  if (absl::GetFlag(FLAGS_override_mysekai_bonus_limit) >= 0) {
    return absl::GetFlag(FLAGS_override_mysekai_bonus_limit);
  }
  constexpr int kMaxMySekaiFixtureBonusLimit = 100;
  return kMaxMySekaiFixtureBonusLimit;
}

}  // namespace sekai
