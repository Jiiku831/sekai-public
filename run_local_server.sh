#!/usr/bin/zsh

autoload -Uz colors && colors

root="${0:a:h}"
cd "$root"

if [[ "$1" = "--build_only" ]]; then
    build_only=1
fi

target_pkg=frontend/build
target=server
output=sajii.html

echo "$fg[blue]Building binary //$target_pkg:$target$reset_color"
BAZEL_CXXOPTS=-std=c++20 bazelisk build --config=wasm -c opt "//$target_pkg:$target" || exit $?

echo "$fg[blue]Extracting server root...$reset_color"
server_root="$(bazelisk info --config=wasm -c opt bazel-bin)/$target_pkg/$target.runfiles/_main/$target_pkg/server_root"
if [[ -d "$server_root" ]]; then
    rm -rf "$server_root" || exit 1
fi
mkdir "$server_root"
tar -xvf "$server_root.tar" -C "$server_root" || exit 1

if (( build_only )); then
    exit 0
fi

echo "$fg[blue]Running local server...$reset_color"
echo "$fg[blue]Once running open the page with this link: http://localhost:8000/$output$reset_color"
EXEC="$(bazelisk info --config=wasm -c opt bazel-bin)/$target_pkg/$target"
cd "$server_root"
"$EXEC"
