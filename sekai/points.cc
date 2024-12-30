#include "sekai/points.h"

#include "sekai/config.h"

namespace sekai {

int SoloEbiPoints(int score, double eb) {
  return static_cast<int>(kEbiBaseFactor * (100 + static_cast<int>(score / kSoloScoreStep)) *
                          (10000 + static_cast<int>(eb * 100)) / 1000'000.00);
}

}  // namespace sekai
