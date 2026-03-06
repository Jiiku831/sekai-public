#pragma once

#include <string_view>

#include <google/protobuf/util/json_util.h>

#include "absl/log/log.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "sekai/run_analysis/proto/service.pb.h"
#include "sekai/run_analysis/proto_util.h"

namespace sekai::run_analysis {

namespace internal {

template <typename ResponseType>
std::string SerializeResponse(const ResponseType& response) {
  std::string output;
  if (auto status = google::protobuf::util::MessageToJsonString(response, &output); !status.ok()) {
    output = absl::StrCat("JSON serialization failed: ", status.message());
    output.insert(0, 1, static_cast<char>(status.code()));
  } else {
    output.insert(0, 1, static_cast<char>(response.status().code()));
  }
  return output;
}

template <typename ResponseType>
std::string SerializeResponseBinary(const ResponseType& response) {
  std::string output;
  if (!response.SerializeToString(&output)) {
    output = "Response serialization failed";
    output.insert(0, 1, static_cast<char>(absl::StatusCode::kInternal));
  } else {
    output.insert(0, 1, 0);
  }
  return output;
}

}  // namespace internal

class HandlerBase {
 public:
  ~HandlerBase() = default;
  virtual std::string Run(std::string_view request) const = 0;
  virtual std::string RunBinary(std::string_view request) const = 0;
};

template <typename RequestType, typename ResponseType>
class Handler : public HandlerBase {
 public:
  ~Handler() = default;

  using request_type = RequestType;
  using response_type = ResponseType;

  std::string Run(std::string_view request) const override {
    RequestType request_msg;
    if (auto status = google::protobuf::util::JsonStringToMessage(request, &request_msg);
        !status.ok()) {
      ResponseType response;
      ToStatusProto(status, *response.mutable_status());
      return internal::SerializeResponse(response);
    }
    absl::StatusOr<ResponseType> response = Run(request_msg);
    if (!response.ok()) {
      ResponseType response_msg;
      ToStatusProto(response.status(), *response_msg.mutable_status());
      return internal::SerializeResponse(response_msg);
    }
    ToStatusProto(response.status(), *response->mutable_status());
    return internal::SerializeResponse(*response);
  }

  std::string RunBinary(std::string_view request) const override {
    RequestType request_msg;
    if (!request_msg.ParseFromString(request)) {
      ResponseType response;
      ToStatusProto(absl::InvalidArgumentError("Failed to parse request"),
                    *response.mutable_status());
      return internal::SerializeResponseBinary(response);
    }
    absl::StatusOr<ResponseType> response = Run(request_msg);
    if (!response.ok()) {
      ResponseType response_msg;
      ToStatusProto(response.status(), *response_msg.mutable_status());
      return internal::SerializeResponseBinary(response_msg);
    }
    ToStatusProto(response.status(), *response->mutable_status());
    return internal::SerializeResponseBinary(*response);
  }

  virtual absl::StatusOr<ResponseType> Run(const RequestType& request) const = 0;
};

}  // namespace sekai::run_analysis
