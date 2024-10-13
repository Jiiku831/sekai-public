#pragma once

#include <string_view>

#include "sekai/profile.h"
#include "sekai/proto/profile.pb.h"

namespace frontend {

constexpr std::string_view kSaveDataPath = "/idbfs/profile";

class ControllerBase {
 public:
  virtual ~ControllerBase() = default;

  void ReadSaveData();
  void WriteSaveData() const;
  void DeleteSaveData() const;

 protected:
  sekai::ProfileProto profile_proto_ = sekai::EmptyProfileProto();
  sekai::Profile profile_{profile_proto_};

  void UpdateProfile();
  virtual void OnProfileUpdate() = 0;
  virtual void OnSaveDataRead() = 0;
};

}  // namespace frontend
