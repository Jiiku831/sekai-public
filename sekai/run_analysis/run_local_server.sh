#!/usr/bin/zsh

autoload -Uz colors && colors

root="${0:a:h}"
cd "$root"

if [[ "$1" = "--build_only" ]]; then
    build_only=1
fi

target_pkg=sekai/run_analysis
target=server
output=sajii.html

src_root="$(mktemp).tar"

echo "$fg[blue]Building binary //$target_pkg:$target$reset_color"
if [[ "$1" = "--ubsan" ]]; then
  include_src=1
  build_opts=(
    --config=wasm-ubsan
    -c dbg
  )
elif [[ "$1" = "--asan" ]]; then
  include_src=1
  build_opts=(
    --config=wasm-asan
    -c dbg
  )
elif [[ "$1" = "--dbg" ]]; then
  include_src=1
  build_opts=(
    --config=wasm-dbg
    -c dbg
  )
else
  build_opts=(
    --config=wasm
    -c opt
  )
fi
BAZEL_CXXOPTS=-std=c++20 bazelisk build "${build_opts[@]}" "//$target_pkg:$target" || exit $?

echo "$fg[blue]Extracting server root...$reset_color"
server_root="$(bazelisk info "${build_opts[@]}" bazel-bin)/$target_pkg/$target.wrangler_root"
server_tar="$(bazelisk info "${build_opts[@]}" bazel-bin)/$target_pkg/$target.tar"
if [[ -d "$server_root" ]]; then
    rm -rf "$server_root" || exit 1
fi
mkdir "$server_root"
tar -xvf "$server_tar" -C "$server_root" || exit 1
if (( include_src )); then
  ln -s "$root" "$server_root/src"
fi

if (( build_only )); then
    exit 0
fi

echo "$fg[blue]Running local server...$reset_color"
echo "$fg[blue]Once running open the page with this link: http://localhost:8000/$output$reset_color"
cd "$server_root"
npx wrangler dev
