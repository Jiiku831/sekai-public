#!/usr/bin/bash
BAZEL_CXXOPTS=-std=c++23 bazelisk run @hedron_compile_commands//:refresh_all

# Ignore failures
exit 0
