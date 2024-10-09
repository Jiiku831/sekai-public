#include "frontend/controller_base.h"

#include <filesystem>

#include <emscripten.h>
#include <emscripten/bind.h>

#include "absl/log/log.h"
#include "frontend/log.h"
#include "sekai/profile.h"
#include "sekai/proto/profile.pb.h"
#include "sekai/proto_util.h"

namespace frontend {

using ::emscripten::class_;
using ::sekai::ProfileProto;
using ::sekai::ReadBinaryProtoFile;

void ControllerBase::ReadSaveData() {
  if (!std::filesystem::exists(kSaveDataPath)) {
    LOG(INFO) << "No existing save data!";
    return;
  }
  profile_proto_ = ReadBinaryProtoFile<ProfileProto>(kSaveDataPath);
  LOG(INFO) << "Successfully read save data.";
  UpdateProfile();
  OnSaveDataRead();
}

void ControllerBase::WriteSaveData() const {
  // TODO: add option for multiple slots
  LogDebugProto("Save data: ", profile_proto_);
  WriteBinaryProtoFile<ProfileProto>(kSaveDataPath, profile_proto_);

  EM_ASM(
      FS.syncfs(false, function (err) {
          assert(!err);
          console.log("Written save data!");
        });
  );
}

void ControllerBase::DeleteSaveData() const {
  std::error_code err;
  if (!std::filesystem::remove(kSaveDataPath, err)) {
    LOG(ERROR) << "Failed to delete saved data with code " << err.value() << ": " << err.message();
    return;
  }
  EM_ASM(
      FS.syncfs(false, function (err) {
          assert(!err);
          console.log("Deleted save data!");
        });
  );
}

void ControllerBase::UpdateProfile() {
  profile_ = sekai::Profile(profile_proto_);
  OnProfileUpdate();
}

EMSCRIPTEN_BINDINGS(controller_base) {
  class_<ControllerBase>("ControllerBase")
      .function("ReadSaveData", &ControllerBase::ReadSaveData)
      .function("WriteSaveData", &ControllerBase::WriteSaveData)
      .function("DeleteSaveData", &ControllerBase::DeleteSaveData);
}

}  // namespace frontend
