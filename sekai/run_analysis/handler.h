#pragma once

#include <string_view>

#include <google/protobuf/util/json_util.h>

#include "absl/log/log.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "sekai/run_analysis/proto/service.pb.h"

namespace sekai::run_analysis {

namespace internal {

template <typename StatusType>
void ToStatusProto(const StatusType& status, Status& status_proto) {
  status_proto.set_code(static_cast<int>(status.code()));
  if (!status.ok()) {
    status_proto.set_msg(std::string(status.message()));
  }
}

template <typename ResponseType>
std::string SerializeResponse(const ResponseType& response) {
  std::string output;
  if (auto status = google::protobuf::util::MessageToJsonString(response, &output); !status.ok()) {
    output = absl::StrCat("JSON serialization failed: ", status.message().as_string());
    output.insert(0, 1, static_cast<char>(status.code()));
  } else {
    output.insert(0, 1, static_cast<char>(response.status().code()));
  }
  return output;
}

}  // namespace internal

class HandlerBase {
 public:
  ~HandlerBase() = default;
  virtual std::string Run(std::string_view request) const = 0;
};

template <typename RequestType, typename ResponseType>
class Handler : public HandlerBase {
 public:
  ~Handler() = default;

  std::string Run(std::string_view request) const override {
    RequestType request_msg;
    if (auto status = google::protobuf::util::JsonStringToMessage(request, &request_msg);
        !status.ok()) {
      ResponseType response;
      internal::ToStatusProto(status, *response.mutable_status());
      return internal::SerializeResponse(response);
    }
    absl::StatusOr<ResponseType> response = Run(request_msg);
    if (!response.ok()) {
      ResponseType response_msg;
      internal::ToStatusProto(response.status(), *response_msg.mutable_status());
      return internal::SerializeResponse(response_msg);
    }
    internal::ToStatusProto(response.status(), *response->mutable_status());
    return internal::SerializeResponse(*response);
  }

  virtual absl::StatusOr<ResponseType> Run(const RequestType& request) const = 0;
};

}  // namespace sekai::run_analysis
