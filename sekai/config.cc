#include "sekai/config.h"

#include <cstdlib>
#include <filesystem>
#include <string_view>

#include "absl/log/absl_check.h"
#include "sekai/parse_proto.h"

namespace sekai {
namespace {

#ifdef __cplusplus
#define xstr(exp) str(exp)
#define str(exp) #exp
constexpr std::string_view kCppVersion = xstr(__cplusplus);
#undef xstr
#undef str
#else
constexpr std::string_view kCppVersion = "<unknown>";
#endif

constexpr std::string_view kMainRepoRoot = "_main";

std::filesystem::path RunfilesDir(std::filesystem::path relpath, std::string_view repo_root) {
  char* run_files = std::getenv("RUNFILES_DIR");
  std::filesystem::path path;
  if (run_files == nullptr) {
    path = "./";
    path /= relpath;
  } else {
    path = std::getenv("RUNFILES_DIR");
    path /= repo_root;
    path /= relpath;
  }
  return path;
}

std::string_view SekaiRepoRoot() {
#ifdef SEKAI_REPO_ROOT
  return SEKAI_REPO_ROOT;
#else
  return kMainRepoRoot;
#endif
}

}  // namespace

std::string_view CppVersion() { return kCppVersion; }

const std::filesystem::path& SekaiBestRoot() {
  static const std::filesystem::path* const kPath = [] {
    auto* path = new std::filesystem::path;
    *path = RunfilesDir("data/storage-sekai-best", SekaiRepoRoot());
    return path;
  }();
  return *kPath;
}

const std::filesystem::path& MasterDbRoot() {
  static const std::filesystem::path* const kPath = [] {
    auto* path = new std::filesystem::path;
    *path = RunfilesDir("external/sekai-master-db~", SekaiRepoRoot());
    return path;
  }();
  return *kPath;
}

const std::filesystem::path& SekaiRunfilesRoot() {
  static const std::filesystem::path* const kPath = [] {
    auto* path = new std::filesystem::path;
    *path = RunfilesDir("./", SekaiRepoRoot());
    return path;
  }();
  return *kPath;
}

const std::filesystem::path& MainRunfilesRoot() {
  static const std::filesystem::path* const kPath = [] {
    auto* path = new std::filesystem::path;
    *path = RunfilesDir("./", kMainRepoRoot);
    return path;
  }();
  return *kPath;
}

const WorldBloomConfig& GetWorldBloomConfig(WorldBloomVersion version) {
  static const auto* const kConfig = [] {
    auto arr = std::make_unique<std::array<WorldBloomConfig, WorldBloomVersion_ARRAYSIZE>>();
    (*arr)[WORLD_BLOOM_VERSION_1] = ParseTextProto<WorldBloomConfig>(R"pb(
      support_team_size: 12
      support_char_bonus: 5
      support_wl_card_bonus: 0
      support_team_bonus {
        master_rank_bonus {
          level_bonus: 0
          level_bonus: 0
          level_bonus: 0
          level_bonus: 0
          level_bonus: 0
          level_bonus: 0
        }
        master_rank_bonus {
          level_bonus: 1
          level_bonus: 1.1
          level_bonus: 1.2
          level_bonus: 1.3
          level_bonus: 1.4
          level_bonus: 1.5
        }
        master_rank_bonus {
          level_bonus: 2
          level_bonus: 2.2
          level_bonus: 2.4
          level_bonus: 2.6
          level_bonus: 2.8
          level_bonus: 3
        }
        master_rank_bonus {
          level_bonus: 3
          level_bonus: 3.3
          level_bonus: 3.6
          level_bonus: 3.9
          level_bonus: 4.2
          level_bonus: 4.5
        }
        master_rank_bonus {
          level_bonus: 10
          level_bonus: 10.5
          level_bonus: 11
          level_bonus: 11.5
          level_bonus: 12
          level_bonus: 12.5
        }
        master_rank_bonus {
          level_bonus: 5
          level_bonus: 5.4
          level_bonus: 5.8
          level_bonus: 6.2
          level_bonus: 6.6
          level_bonus: 7
        }
        skill_level_bonus {
          level_bonus: 0
          level_bonus: 0
          level_bonus: 0
          level_bonus: 0
          level_bonus: 0
        }
        skill_level_bonus {
          level_bonus: 0
          level_bonus: 0
          level_bonus: 0.05
          level_bonus: 0.2
          level_bonus: 0.5
        }
        skill_level_bonus {
          level_bonus: 0
          level_bonus: 0
          level_bonus: 0.1
          level_bonus: 0.4
          level_bonus: 1
        }
        skill_level_bonus {
          level_bonus: 0
          level_bonus: 0
          level_bonus: 0.15
          level_bonus: 0.6
          level_bonus: 1.5
        }
        skill_level_bonus {
          level_bonus: 0
          level_bonus: 0
          level_bonus: 0.25
          level_bonus: 1
          level_bonus: 2.5
        }
        skill_level_bonus {
          level_bonus: 0
          level_bonus: 0
          level_bonus: 0.2
          level_bonus: 0.8
          level_bonus: 2
        }
      }
    )pb");

    (*arr)[WORLD_BLOOM_VERSION_2] = ParseTextProto<WorldBloomConfig>(R"pb(
      support_team_size: 20
      support_char_bonus: 5
      support_wl_card_bonus: 20
      support_wl_cards: 785
      support_wl_cards: 786
      support_wl_cards: 787
      support_wl_cards: 788
      support_wl_cards: 848
      support_wl_cards: 849
      support_wl_cards: 850
      support_wl_cards: 851
      support_wl_cards: 884
      support_wl_cards: 885
      support_wl_cards: 886
      support_wl_cards: 887
      support_wl_cards: 921
      support_wl_cards: 922
      support_wl_cards: 923
      support_wl_cards: 924
      support_wl_cards: 961
      support_wl_cards: 962
      support_wl_cards: 963
      support_wl_cards: 964
      support_wl_cards: 979
      support_wl_cards: 980
      support_wl_cards: 981
      support_wl_cards: 982
      support_wl_cards: 983
      support_wl_cards: 984
      support_team_bonus {
        master_rank_bonus {
          level_bonus: 0
          level_bonus: 0
          level_bonus: 0
          level_bonus: 0
          level_bonus: 0
          level_bonus: 0
        }
        master_rank_bonus {
          level_bonus: 0.5
          level_bonus: 0.6
          level_bonus: 0.6
          level_bonus: 0.7
          level_bonus: 0.7
          level_bonus: 0.8
        }
        master_rank_bonus {
          level_bonus: 1
          level_bonus: 1.2
          level_bonus: 1.2
          level_bonus: 1.3
          level_bonus: 1.4
          level_bonus: 1.5
        }
        master_rank_bonus {
          level_bonus: 2
          level_bonus: 2.3
          level_bonus: 2.4
          level_bonus: 2.6
          level_bonus: 2.8
          level_bonus: 3
        }
        master_rank_bonus {
          level_bonus: 7.5
          level_bonus: 8
          level_bonus: 8.5
          level_bonus: 9
          level_bonus: 9.5
          level_bonus: 10
        }
        master_rank_bonus {
          level_bonus: 5
          level_bonus: 5.4
          level_bonus: 5.8
          level_bonus: 6.2
          level_bonus: 6.6
          level_bonus: 7
        }
        skill_level_bonus {
          level_bonus: 0
          level_bonus: 0
          level_bonus: 0
          level_bonus: 0
          level_bonus: 0
        }
        skill_level_bonus {
          level_bonus: 0
          level_bonus: 0
          level_bonus: 0.05
          level_bonus: 0.1
          level_bonus: 0.3
        }
        skill_level_bonus {
          level_bonus: 0
          level_bonus: 0
          level_bonus: 0.1
          level_bonus: 0.2
          level_bonus: 0.5
        }
        skill_level_bonus {
          level_bonus: 0
          level_bonus: 0
          level_bonus: 0.15
          level_bonus: 0.4
          level_bonus: 1
        }
        skill_level_bonus {
          level_bonus: 0
          level_bonus: 0
          level_bonus: 0.25
          level_bonus: 1
          level_bonus: 2.5
        }
        skill_level_bonus {
          level_bonus: 0
          level_bonus: 0
          level_bonus: 0.2
          level_bonus: 0.8
          level_bonus: 2
        }
      }
    )pb");
    return arr.release();
  }();
  ABSL_CHECK_NE(version, WORLD_BLOOM_VERSION_UNKNOWN);
  ABSL_CHECK_LT(version, WorldBloomVersion_ARRAYSIZE);
  ABSL_CHECK_GE(version, 0);
  return (*kConfig)[version];
}

WorldBloomVersion GetWorldBloomVersion(int event_id) {
  int version = 1;
  for (int cutoff : kWorldBloomVersionCutoffs) {
    if (event_id > cutoff) {
      ++version;
    }
  }
  ABSL_CHECK(WorldBloomVersion_IsValid(version)) << "Invalid WL version: " << version;
  ABSL_CHECK_GT(version, 0) << "WL version out of range: " << version;
  return static_cast<WorldBloomVersion>(version);
}

}  // namespace sekai
