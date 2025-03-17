#!/usr/bin/bash
BAZEL_CXXOPTS=-std=c++23 bazelisk run //:refresh_compile_commands

# Ignore failures
exit 0
