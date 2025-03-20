#include "sekai/run_analysis/analyze_team_handler.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "absl/status/status_matchers.h"
#include "sekai/proto_util.h"
#include "sekai/run_analysis/proto/service.pb.h"
#include "testing/util.h"

namespace sekai::run_analysis {
namespace {

using ::absl_testing::IsOkAndHolds;
using ::testing::ParseTextProto;
using ::testing::ProtoEquals;

class AnalyzeTeamTest : public ::testing::Test {
 protected:
  absl::StatusOr<AnalyzeTeamResponse> Run(const AnalyzeTeamRequest& request) {
    return handler_.Run(request);
  }

  AnalyzeTeamHandler handler_;
};

TEST_F(AnalyzeTeamTest, RespondsWithEventBonus) {
  auto request = ParseTextProto<AnalyzeTeamRequest>(R"pb(
    event_id: 159
    cards {card_id: 707 master_rank: 5}
    cards {card_id: 427 master_rank: 5}
    cards {card_id: 480}
    cards {card_id: 306}
    cards {card_id: 355}
    team_power: 291320
  )pb");
  auto expected_response = ParseTextProto<AnalyzeTeamResponse>(R"pb(
    event_bonus: 105
    skill_details {
      skill_lower_bounds: 80
      skill_lower_bounds: 60
      skill_lower_bounds: 60
      skill_lower_bounds: 60
      skill_lower_bounds: 60
      skill_upper_bounds: 100
      skill_upper_bounds: 80
      skill_upper_bounds: 80
      skill_upper_bounds: 80
      skill_upper_bounds: 80
      skill_value_lower_bound: 128
      skill_value_upper_bound: 164
    }
  )pb");
  EXPECT_THAT(Run(request), IsOkAndHolds(ProtoEquals(expected_response)));
}

}  // namespace
}  // namespace sekai::run_analysis
