#include "sekai/challenge_live_estimator.h"

#include <functional>
#include <vector>

#include "absl/base/no_destructor.h"
#include "absl/container/flat_hash_set.h"
#include "absl/log/absl_check.h"
#include "sekai/db/master_db.h"
#include "sekai/db/proto/all.h"
#include "sekai/team.h"

namespace sekai {

namespace {

using ::sekai::db::MasterDb;

constexpr int kTeamSize = 5;

std::string DifficultyToDisplayText(db::Difficulty diff) {
  switch (diff) {
    case db::DIFF_EASY:
      return "EASY";
    case db::DIFF_NORMAL:
      return "NORMAL";
    case db::DIFF_HARD:
      return "HARD";
    case db::DIFF_EXPERT:
      return "EX";
    case db::DIFF_MASTER:
      return "MAS";
    case db::DIFF_APPEND:
      return "AP";
    default:
      ABSL_CHECK(false) << "unhandled case";
  }
}

void SafeAddMetas(std::span<absl::Nonnull<const db::MusicMeta* const>> metas,
                  std::vector<ChallengeLiveSongEstimator>& dst) {
  for (const db::MusicMeta* meta : metas) {
    if (MasterDb::SafeFindFirst<db::Music>(meta->music_id()) == nullptr) {
      LOG(WARNING) << "Music " << meta->music_id() << " not found in master db. skipping";
      continue;
    }
    dst.emplace_back(meta);
  }
}

}  // namespace

ChallengeLiveSongEstimator::ChallengeLiveSongEstimator(const db::MusicMeta* meta) : meta_(meta) {
  ABSL_CHECK_EQ(meta->skill_score_solo_size(), kTeamSize + 1);
  sorted_skill_factor_ = {meta->skill_score_solo().begin(),
                          meta->skill_score_solo().begin() + kTeamSize};
  std::sort(sorted_skill_factor_.begin(), sorted_skill_factor_.end(), std::greater<float>());
  sorted_skill_factor_.push_back(meta->skill_score_solo(kTeamSize));
  base_factor_ = meta->base_score();
}

double ChallengeLiveSongEstimator::ExpectedValue(int power,
                                                 std::span<const int> sorted_skills) const {
  float skill_factor_total = 0;
  ABSL_CHECK_EQ(sorted_skills.size(), sorted_skill_factor_.size());
  for (std::size_t i = 0; i < sorted_skills.size(); ++i) {
    skill_factor_total += sorted_skill_factor_[i] * sorted_skills[i] / 100;
  }
  return 4 * power * (skill_factor_total + base_factor_);
}

ChallengeLiveEstimator::ChallengeLiveEstimator() {
  // TODO: check if we can just evaluate all songs
  absl::flat_hash_set<std::pair<int, db::Difficulty>> meta_songs = {
      {104, db::DIFF_MASTER},  // Cendrillon
      // {154, db::DIFF_APPEND},  // Chikyuu
      {72, db::DIFF_MASTER},   // BYB
      {11, db::DIFF_MASTER},   // Biba
      {62, db::DIFF_MASTER},   // JSG
      {91, db::DIFF_MASTER},   // Dramaturgy
      {410, db::DIFF_MASTER},  // Marshall Maximizer
      {74, db::DIFF_MASTER},   // Ebi
  };
  std::vector<const db::MusicMeta*> metas =
      MasterDb::GetIf<db::MusicMeta>([&](const db::MusicMeta& meta) {
        return meta_songs.contains(std::make_pair(meta.music_id(), meta.difficulty()));
      });
  SafeAddMetas(metas, metas_);
}

ChallengeLiveEstimator::ChallengeLiveEstimator(
    std::span<absl::Nonnull<const db::MusicMeta* const>> songs) {
  SafeAddMetas(songs, metas_);
}

ChallengeLiveEstimator::ValueDetail ChallengeLiveEstimator::ExpectedValueImpl(
    const Profile& profile, const EventBonus& event_bonus, const Team& team) const {
  std::vector<int> skills = team.GetSkillValues();
  std::sort(skills.begin(), skills.end(), std::greater<int>());
  while (skills.size() < kTeamSize) {
    skills.push_back(0);
  }
  skills.push_back(skills.front());
  int power = team.Power(profile);
  double max_score = 0;
  const db::MusicMeta* db_meta = nullptr;
  for (const ChallengeLiveSongEstimator& meta : metas_) {
    double candidate_score = meta.ExpectedValue(power, skills);
    if (db_meta == nullptr || max_score < candidate_score) {
      db_meta = meta.meta();
      max_score = candidate_score;
    }
  }
  ABSL_CHECK_NE(db_meta, nullptr);
  return {
      .value = max_score,
      .meta = db_meta,
  };
}

double ChallengeLiveEstimator::ExpectedValue(const Profile& profile, const EventBonus& event_bonus,
                                             const Team& team) const {
  return ExpectedValueImpl(profile, event_bonus, team).value;
}

double ChallengeLiveEstimator::MaxExpectedValue(const Profile& profile,
                                                const EventBonus& event_bonus, const Team& team,
                                                Character lead_chars) const {
  return ExpectedValue(profile, event_bonus, team);
}

void ChallengeLiveEstimator::AnnotateTeamProto(const Profile& profile,
                                               const EventBonus& event_bonus, const Team& team,
                                               TeamProto& team_proto) const {
  ChallengeLiveEstimator::ValueDetail detail = ExpectedValueImpl(profile, event_bonus, team);
  auto music = MasterDb::FindFirst<db::Music>(detail.meta->music_id());
  team_proto.set_expected_score(detail.value);
  team_proto.set_best_song_name(
      absl::StrFormat("%s %s", music.title(), DifficultyToDisplayText(detail.meta->difficulty())));
}

const EstimatorBase& SoloEbiMasEstimator() {
  static const absl::NoDestructor<ChallengeLiveEstimator> kEstimator(
      MasterDb::GetIf<db::MusicMeta>([](const db::MusicMeta& meta) {
        return meta.difficulty() == db::DIFF_MASTER && meta.music_id() == 74;
      }));
  return *kEstimator;
}

}  // namespace sekai
