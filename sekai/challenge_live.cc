#include "sekai/challenge_live.h"

#include "absl/log/absl_check.h"
#include "sekai/db/master_db.h"
#include "sekai/db/proto/all.h"

namespace sekai {

std::vector<int> GetChallengeLiveStagePointRequirement(int char_id) {
  std::vector<const db::ChallengeLiveStage*> stages =
      db::MasterDb::FindAll<db::ChallengeLiveStage>(char_id);
  std::sort(stages.begin(), stages.end(),
            [](const db::ChallengeLiveStage* lhs, const db::ChallengeLiveStage* rhs) {
              return lhs->rank() < rhs->rank();
            });
  std::vector<int> pts;
  pts.push_back(0);
  pts.push_back(0);

  int rank = 1;
  int cumulative_pts = 0;
  for (const db::ChallengeLiveStage* stage : stages) {
    ABSL_CHECK_EQ(rank, stage->rank())
        << "Expected next stage to be " << rank << ". Got " << stage->rank() << " instead.";
    cumulative_pts += stage->next_stage_challenge_point();
    pts.push_back(cumulative_pts);
    ++rank;
  }
  return pts;
}

}  // namespace sekai
