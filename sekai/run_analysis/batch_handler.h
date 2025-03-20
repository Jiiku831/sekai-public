#pragma once

#include "absl/log/log.h"
#include "absl/status/statusor.h"
#include "sekai/run_analysis/handler.h"
#include "sekai/run_analysis/proto/service.pb.h"

namespace sekai::run_analysis {

template <typename HandlerType, typename RequestType, typename ResponseType>
class BatchHandler : public Handler<RequestType, ResponseType> {
 public:
  absl::StatusOr<ResponseType> Run(const RequestType& request) const override {
    ResponseType response;
    for (const auto& subrequest : request.requests()) {
      auto subresponse = handler_.Run(subrequest);
      if (subresponse.ok()) {
        *response.add_responses() = *std::move(subresponse);
      } else {
        typename HandlerType::response_type subresponse_msg;
        ToStatusProto(subresponse.status(), *subresponse_msg.mutable_status());
        *response.add_responses() = subresponse_msg;
      }
    }
    return response;
  }

 private:
  HandlerType handler_;
};

}  // namespace sekai::run_analysis
