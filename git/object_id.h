#pragma once

extern "C" {
#include "git2/types.h"
}

namespace git {

class ObjectId {
 public:
  explicit ObjectId(const git_oid& oid);
  ObjectId(const ObjectId& other);
  ObjectId(ObjectId&& other);

  const git_oid& oid() const { return oid_; }

 private:
  git_oid oid_;
};

}  // namespace git
