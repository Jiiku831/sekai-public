#include "sekai/run_analysis/parser.h"

#include <string_view>

#include <nlohmann/json.hpp>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_format.h"
#include "base/util.h"
#include "sekai/db/json/parser.h"
#include "sekai/run_analysis/proto/run_data.pb.h"

namespace sekai::run_analysis {
namespace {

using json = ::nlohmann::json;

class RunDataParserV2 : public json::json_sax_t {
 public:
  bool binary(json::binary_t& val) override {
    status_ = absl::UnimplementedError("binary data not supported");
    return false;
  }

  bool boolean(bool val) override {
    status_ = absl::UnimplementedError("boolean data not supported");
    return false;
  }

  bool end_array() override { return true; }
  bool end_object() override {
    current_key_ = "";
    return true;
  };

  bool key(json::string_t& val) override {
    current_key_ = val;
    return true;
  }

  bool null() override {
    status_ = absl::UnimplementedError("null not supported");
    return false;
  }

  bool number_float(json::number_float_t val, const json::string_t& s) override {
    status_ = absl::UnimplementedError("float not supported");
    return false;
  }

  bool number_integer(json::number_integer_t val) override { return number_unsigned(val); }

  bool number_unsigned(json::number_unsigned_t val) override {
    if (current_key_ == "x") {
      run_data_.mutable_user_graph()->mutable_overall()->add_timestamps(val * 1000);
    } else if (current_key_ == "y") {
      run_data_.mutable_user_graph()->mutable_overall()->add_points(val);
    } else {
      status_ = absl::UnimplementedError(absl::StrCat("Unexpected key: ", current_key_));
      return false;
    }
    return true;
  }

  bool parse_error(std::size_t pos, const std::string& last_token,
                   const json::exception& ex) override {
    status_ = absl::UnimplementedError(absl::StrCat("unhandled parse error: ", ex.what()));
    return false;
  }

  bool start_array(std::size_t size) override { return true; }

  bool start_object(std::size_t size) override { return true; }

  bool string(json::string_t& val) override {
    status_ = absl::UnimplementedError("string data not supported");
    return false;
  }

  const RunData& run_data() const { return run_data_; }
  const absl::Status& status() const { return status_; }

 private:
  std::string current_key_;
  RunData run_data_;
  absl::Status status_ = absl::OkStatus();
};

}  // namespace

absl::StatusOr<RunData> ParseRunData(std::string_view json_str) {
  ASSIGN_OR_RETURN(std::vector<RunData> data, sekai::db::ParseJsonFromString<RunData>(json_str));
  if (data.size() != 1) {
    return absl::InvalidArgumentError(absl::StrFormat(
        "Expected input to contain exactly one RunData. Got %d instead.", data.size()));
  }
  return data[0];
}

absl::StatusOr<RunData> ParseRunDataV2(std::string_view json_str) {
  RunDataParserV2 parser;
  bool status = nlohmann::json::sax_parse(json_str, &parser);
  if (status && parser.status().ok()) {
    return parser.run_data();
  }
  if (parser.status().ok()) {
    return absl::InternalError("Parser failed but no error status");
  }
  return parser.status();
}

}  // namespace sekai::run_analysis
