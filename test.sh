#!/usr/bin/bash
if [[ "$(git rev-parse --abbrev-ref HEAD)" = "en" ]]; then
  exit 0
fi
BAZEL_CXXOPTS=-std=c++20 bazelisk test -c opt \
    --build_tests_only --test_tag_filters=-manual --test_output=summary ...
