#include "sekai/server.h"

#include "absl/log/absl_check.h"
#include "sekai/db/proto/server.pb.h"

namespace sekai {

extern const int kServerId;

db::Server GetServer() {
  ABSL_CHECK(db::Server_IsValid(kServerId));
  return static_cast<db::Server>(kServerId);
}

}  // namespace sekai
