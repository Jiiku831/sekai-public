#!/usr/bin/zsh

autoload -Uz colors && colors

root="${0:a:h}"
cd "$root"

targets=($(bazelisk query 'kind("cc_test", //...)'))

BAZEL_CXXOPTS=-std=c++23 bazelisk build -c opt -j 4 --keep_going ${targets[@]}
BAZEL_CXXOPTS=-std=c++23 bazelisk run -c opt -j 4 //:refresh_compile_commands

# Ignore failures
exit 0
