#include <string>

#include "absl/flags/flag.h"
#include "absl/log/absl_check.h"
#include "absl/log/log.h"
#include "base/init.h"
#include "base/util.h"
#include "git/commit.h"
#include "git/init.h"
#include "git/object_id.h"
#include "git/reference.h"
#include "git/repository.h"

ABSL_FLAG(std::string, repo_path, "", "path to clone the repo to");

int LogAndReturnStatusCode(absl::Status status) {
  LOG(ERROR) << status;
  return static_cast<int>(status.code());
}

int main(int argc, char** argv) {
  Init(argc, argv);
  ABSL_CHECK_OK(git::Init());

  ABSL_CHECK(!absl::GetFlag(FLAGS_repo_path).empty());
  ASSIGN_OR_RETURN_MAP(git::Repository repo, git::Repository::Open(absl::GetFlag(FLAGS_repo_path)),
                       LogAndReturnStatusCode);
  ASSIGN_OR_RETURN_MAP(git::Reference ref_head, repo.LookupReference("refs/heads/main"),
                       LogAndReturnStatusCode);
  ASSIGN_OR_RETURN_MAP(git::ObjectId head_oid, ref_head.Target(), LogAndReturnStatusCode);
  ASSIGN_OR_RETURN_MAP(git::Commit head_commit, repo.LookupCommit(head_oid),
                       LogAndReturnStatusCode);
  LOG(INFO) << head_commit.RawMessage();
  return 0;
}
