#pragma once

#include <filesystem>
#include <fstream>
#include <ios>
#include <optional>

#include <google/protobuf/descriptor.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/text_format.h>

#include "absl/log/absl_check.h"
#include "base/zstd.h"
#include "sekai/file_util.h"
#include "sekai/parse_proto.h"

namespace sekai {

template <typename T, auto Descriptor>
std::vector<T> EnumValues() {
  std::vector<T> values;
  const google::protobuf::EnumDescriptor* desc = Descriptor();
  for (int i = 0; i < desc->value_count(); ++i) {
    values.push_back(static_cast<T>(desc->value(i)->number()));
  }
  return values;
}

template <typename T, auto Descriptor>
std::vector<T> EnumValuesExcludingDefault() {
  std::vector<T> values;
  const google::protobuf::EnumDescriptor* desc = Descriptor();
  for (int i = 0; i < desc->value_count(); ++i) {
    if (desc->value(i)->number() == 0) {
      continue;
    }
    values.push_back(static_cast<T>(desc->value(i)->number()));
  }
  return values;
}

template <typename T>
T ParseTextProtoFile(std::filesystem::path path) {
  std::ifstream fs;
  fs.open(path);
  google::protobuf::io::IstreamInputStream input_stream{&fs};
  T msg;
  ABSL_CHECK(google::protobuf::TextFormat::Parse(&input_stream, &msg));
  return msg;
}

template <typename T>
T ReadCompressedBinaryProto(const std::string& data) {
  std::string decompressed_data = zstd::Decompressor{}(data);
  T msg;
  ABSL_CHECK(msg.ParseFromString(decompressed_data));
  return msg;
}

template <typename T>
T ReadCompressedBinaryProtoFile(std::filesystem::path path) {
  std::string data = GetFileContents(path, std::ios::binary);
  return ReadCompressedBinaryProto<T>(data);
}

template <typename T>
void WriteCompressedBinaryProtoFile(std::filesystem::path path, const T& msg) {
  std::string data;
  msg.SerializeToString(&data);
  data = zstd::Compressor{}(data);
  std::ofstream fout(path, std::ios::binary);
  fout.write(data.c_str(), data.size());
}

template <typename T>
T ReadBinaryProtoFile(std::filesystem::path path) {
  std::string data = GetFileContents(path, std::ios::binary);
  T msg;
  ABSL_CHECK(msg.ParseFromString(data));
  return msg;
}

template <typename T>
void WriteBinaryProtoFile(std::filesystem::path path, const T& msg) {
  std::string data;
  msg.SerializeToString(&data);
  std::ofstream fout(path, std::ios::binary);
  fout.write(data.c_str(), data.size());
}

}  // namespace sekai
