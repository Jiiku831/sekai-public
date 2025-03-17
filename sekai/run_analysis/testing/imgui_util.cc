#include "sekai/run_analysis/testing/imgui_util.h"

#include <string_view>

#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>

#include "imgui.h"

namespace sekai::run_analysis {

using ::google::protobuf::Descriptor;
using ::google::protobuf::FieldDescriptor;
using ::google::protobuf::Message;
using ::google::protobuf::Reflection;

namespace {

void PrintMessageImpl(const google::protobuf::Message& msg, int& next_id);
void PrintMessageNode(const char* title, const google::protobuf::Message& msg, int& next_id) {
  ImGui::PushID(next_id++);
  ImGui::Unindent();
  if (ImGui::TreeNodeEx("%s", ImGuiTreeNodeFlags_DefaultOpen, "%s:", title)) {
    ImGui::SameLine();
    PrintMessageImpl(msg, next_id);
    ImGui::TreePop();
  } else {
    ImGui::SameLine();
    ImGui::Text("{...}");
  }
  ImGui::Indent();
  ImGui::PopID();
}

void PrintField(const FieldDescriptor& field, const Reflection& reflection, const Message& msg,
                int& next_id) {
  if (field.has_presence() && !reflection.HasField(msg, &field)) {
    return;
  }
  std::string field_name = std::string(field.name());
  if (field.is_repeated()) {
    int size = reflection.FieldSize(msg, &field);
    for (int i = 0; i < size; ++i) {
      switch (field.cpp_type()) {
        case FieldDescriptor::CPPTYPE_INT32:
          ImGui::Text("%s: %d", field_name.c_str(), reflection.GetRepeatedInt32(msg, &field, i));
          break;
        case FieldDescriptor::CPPTYPE_INT64:
          ImGui::Text("%s: %ld", field_name.c_str(), reflection.GetRepeatedInt64(msg, &field, i));
          break;
        case FieldDescriptor::CPPTYPE_UINT32:
          ImGui::Text("%s: %u", field_name.c_str(), reflection.GetRepeatedUInt32(msg, &field, i));
          break;
        case FieldDescriptor::CPPTYPE_UINT64:
          ImGui::Text("%s: %lu", field_name.c_str(), reflection.GetRepeatedUInt64(msg, &field, i));
          break;
        case FieldDescriptor::CPPTYPE_DOUBLE:
          ImGui::Text("%s: %f", field_name.c_str(), reflection.GetRepeatedDouble(msg, &field, i));
          break;
        case FieldDescriptor::CPPTYPE_FLOAT:
          ImGui::Text("%s: %f", field_name.c_str(), reflection.GetRepeatedFloat(msg, &field, i));
          break;
        case FieldDescriptor::CPPTYPE_BOOL:
          ImGui::Text("%s: %s", field_name.c_str(),
                      reflection.GetRepeatedBool(msg, &field, i) ? "true" : "false");
          break;
        case FieldDescriptor::CPPTYPE_ENUM:
          ImGui::Text("%s: %s", field_name.c_str(),
                      std::string(reflection.GetRepeatedEnum(msg, &field, i)->name()).c_str());
          break;
        case FieldDescriptor::CPPTYPE_STRING:
          ImGui::Text("%s: %s", field_name.c_str(),
                      reflection.GetRepeatedString(msg, &field, i).c_str());
          break;
        case FieldDescriptor::CPPTYPE_MESSAGE:
          PrintMessageNode(field_name.c_str(), reflection.GetRepeatedMessage(msg, &field, i),
                           next_id);
          break;
        default:
          ImGui::Text("UNIMPLEMENTED");
          break;
      }
    }
  } else if (field.is_map()) {
    ImGui::Text("%s: UNIMPLEMENTED", field_name.c_str());
  } else {
    switch (field.cpp_type()) {
      case FieldDescriptor::CPPTYPE_INT32:
        ImGui::Text("%s: %d", field_name.c_str(), reflection.GetInt32(msg, &field));
        break;
      case FieldDescriptor::CPPTYPE_INT64:
        ImGui::Text("%s: %ld", field_name.c_str(), reflection.GetInt64(msg, &field));
        break;
      case FieldDescriptor::CPPTYPE_UINT32:
        ImGui::Text("%s: %u", field_name.c_str(), reflection.GetUInt32(msg, &field));
        break;
      case FieldDescriptor::CPPTYPE_UINT64:
        ImGui::Text("%s: %lu", field_name.c_str(), reflection.GetUInt64(msg, &field));
        break;
      case FieldDescriptor::CPPTYPE_DOUBLE:
        ImGui::Text("%s: %f", field_name.c_str(), reflection.GetDouble(msg, &field));
        break;
      case FieldDescriptor::CPPTYPE_FLOAT:
        ImGui::Text("%s: %f", field_name.c_str(), reflection.GetFloat(msg, &field));
        break;
      case FieldDescriptor::CPPTYPE_BOOL:
        ImGui::Text("%s: %s", field_name.c_str(),
                    reflection.GetBool(msg, &field) ? "true" : "false");
        break;
      case FieldDescriptor::CPPTYPE_ENUM:
        ImGui::Text("%s: %s", field_name.c_str(),
                    std::string(reflection.GetEnum(msg, &field)->name()).c_str());
        break;
      case FieldDescriptor::CPPTYPE_STRING:
        ImGui::Text("%s: %s", field_name.c_str(), reflection.GetString(msg, &field).c_str());
        break;
      case FieldDescriptor::CPPTYPE_MESSAGE:
        PrintMessageNode(field_name.c_str(), reflection.GetMessage(msg, &field), next_id);
        break;
      default:
        ImGui::Text("UNIMPLEMENTED");
        break;
    }
  }
}

void PrintMessageImpl(const google::protobuf::Message& msg, int& next_id) {
  const Descriptor& descriptor = *msg.GetDescriptor();
  const Reflection& reflection = *msg.GetReflection();
  ImGui::Text("{");
  ImGui::Indent();
  for (int i = 0; i < descriptor.field_count(); ++i) {
    PrintField(*descriptor.field(i), reflection, msg, next_id);
  }
  ImGui::Unindent();
  ImGui::Text("}");
}

}  // namespace

void PrintMessage(const char* title, const google::protobuf::Message& msg) {
  int next_id = 0;
  std::string msg_name(msg.GetDescriptor()->name());
  if (ImGui::TreeNode("%s", "%s = %s", title, msg_name.c_str())) {
    ImGui::SameLine();
    PrintMessageImpl(msg, next_id);
    ImGui::TreePop();
  } else {
    ImGui::SameLine();
    ImGui::Text("{...}");
  }
}

}  // namespace sekai::run_analysis
