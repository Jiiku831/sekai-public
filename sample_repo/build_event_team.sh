#!/usr/bin/zsh

autoload -Uz colors && colors

root="${0:a:h}"
cd "$root"

target=":build_event_team_main"
args=(
    "--output=$root/reports/event_team.html"
    "--stderrthreshold=0"
)

BAZEL_CXXOPTS=-std=c++23 bazelisk build -c opt "$target" || exit $?

target_path="$(echo $target | sed 's/:/\//')"
runfiles="$(bazelisk info -c opt bazel-bin)/${target_path}.runfiles"

echo "$fg[blue]Running ${target}$reset_color"
RUNFILES_DIR="$runfiles" \
    "$(bazelisk info -c opt bazel-bin)/${target_path}" \
    "${args[@]}" "$@"
exit_code="$?"

if [[ "$exit_code" -ne 0 ]]; then
    echo "$fg[red]$target exited with code $exit_code$reset_color"
else
    echo "$fg[green]Done$reset_color"
fi
