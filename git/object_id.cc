#include "git/object_id.h"

extern "C" {
#include "git2/oid.h"
#include "git2/types.h"
}

namespace git {

ObjectId::ObjectId(const git_oid& oid) { git_oid_cpy(&oid_, &oid); }

ObjectId::ObjectId(const ObjectId& other) { git_oid_cpy(&oid_, &other.oid_); }

ObjectId::ObjectId(ObjectId&& other) { git_oid_cpy(&oid_, &other.oid_); }

}  // namespace git
