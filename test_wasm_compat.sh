#!/usr/bin/bash
if git rev-parse --abbrev-ref HEAD | grep -E "(tw|en|kr)" > /dev/null; then
  exit 0
fi
BAZEL_CXXOPTS=-std=c++23 bazelisk build --config=wasm -c opt \
    //frontend/build:sajii_wasm \
    //frontend/build:ohnyajii_wasm \
    //sekai/run_analysis:server_wasm \
    //sekai/run_analysis/testing:load_data_wasm
