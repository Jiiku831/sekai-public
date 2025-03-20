#pragma once

#include <string>

#include "sekai/run_analysis/proto/service.pb.h"

namespace sekai::run_analysis {

template <typename StatusType>
void ToStatusProto(const StatusType& status, Status& status_proto) {
  status_proto.set_code(static_cast<int>(status.code()));
  if (!status.ok()) {
    status_proto.set_msg(std::string(status.message()));
  }
}

}  // namespace sekai::run_analysis
