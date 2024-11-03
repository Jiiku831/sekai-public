#!/usr/bin/zsh

autoload -Uz colors && colors

root="${0:a:h}"
cd "$root"

echo "$fg[blue]Building targets$reset_color"
target="//sekai/history:history_analyzer_main"
repo_target="//sekai/history:sekai_master_db_repo"
BAZEL_CXXOPTS=-std=c++20 bazelisk build -c opt "$target" "$repo_target" || exit $?

echo "$fg[blue]Unpacking ${repo_target} to $tmpdir $reset_color"
target_path="$(echo $target | sed 's/:/\//')"
repo_target_path="$(echo $repo_target | sed 's/:/\//')"
tmpdir="$(mktemp -d)"
unzip "$(bazelisk info -c opt bazel-bin)/$repo_target_path" -d "$tmpdir" || exit $?

echo "$fg[blue]Running ${target}$reset_color"
args=(
    "--repo_path=$tmpdir"
    "--stderrthreshold=0"
)
runfiles="$(bazelisk info -c opt bazel-bin)/${target_path}.runfiles"
RUNFILES_DIR="$runfiles" \
    "$(bazelisk info -c opt bazel-bin)/${target_path}" \
    "${args[@]}" "$@"
exit_code="$?"

echo "$fg[blue]Cleaning up ${target}$reset_color"
rm -rf "$tmpdir"

if [[ "$exit_code" -ne 0 ]]; then
    echo "$fg[red]$target exited with code $exit_code$reset_color"
else
    echo "$fg[green]Done$reset_color"
fi
