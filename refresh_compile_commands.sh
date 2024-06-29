#!/usr/bin/bash
BAZEL_CXXOPTS=-std=c++20 bazelisk run @hedron_compile_commands//:refresh_all

# Ignore failures
exit 0
