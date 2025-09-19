#!/usr/bin/zsh

autoload -Uz colors && colors

root="${0:a:h}"
cd "$root"

targets=($(bazelisk query 'kind("cc_test", //...)'))

BAZEL_CXXOPTS=-std=c++23 bazelisk build -c opt --keep_going ${targets[@]}
BAZEL_CXXOPTS=-std=c++23 bazelisk run -c opt //:refresh_compile_commands

# Ignore failures
exit 0
