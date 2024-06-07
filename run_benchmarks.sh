#!/usr/bin/zsh

autoload -Uz colors && colors

root="${0:a:h}"
cd "$root"

targets=(
    sekai:team_benchmark
    sekai/db:master_db_benchmark
    sekai/team_builder:team_builder_benchmark
)

BAZEL_CXXOPTS=-std=c++20 bazelisk build -c opt "${targets[@]}" || exit $?

for target in "${targets[@]}"; do
    target_path="$(echo $target | sed 's/:/\//')"
    runfiles="$(bazelisk info -c opt bazel-bin)/${target_path}.runfiles"
    echo "$fg[blue]Running benchmark //${target}$reset_color"
    RUNFILES_DIR="$runfiles" "$(bazelisk info -c opt bazel-bin)/${target_path}" \
      --benchmark_counters_tabular=true
    exit_code="$?"
    if [[ "$exit_code" -ne 0 ]]; then
        echo "$fg[red]Benchmark exited with code $exit_code$reset_color"
    else
        echo "$fg[green]Done$reset_color"
    fi
done
