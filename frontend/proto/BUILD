load("@rules_proto//proto:defs.bzl", "proto_library")
load("@rules_cc//cc:defs.bzl", "cc_proto_library")

proto_library(
    name = "context_proto",
    srcs = ["context.proto"],
)

cc_proto_library(
    name = "context_cc_proto",
    deps = [":context_proto"],
)
