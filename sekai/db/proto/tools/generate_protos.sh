#!/usr/bin/zsh

root="$(realpath "${0:a:h}/../../../..")"

echo "Root: $root"

bazelisk build :generate_proto

RUNFILES_DIR="$(bazelisk info bazel-bin)/sekai/db/proto/tools/generate_proto.runfiles"

for file in $RUNFILES_DIR/sekai-master-db~/*.json; do
    echo "processing: $file"
    should_skip=0
    for skip_file in "${skip_files[@]}"; do
      if [[ "$(basename $file)" = "$skip_file" ]]; then
          should_skip=1
          break
      fi
    done
    if (( should_skip )); then
      echo "skipping $(basename $file)..."
      continue
    fi

    should_reprocess=0
    reprocess_files=(
      skills.json
    )
    reprocess_args=""
    for reprocess_file in "${reprocess_files[@]}"; do
      if [[ "$(basename $file)" = "$reprocess_file" ]]; then
          reprocess_args="--overwrite"
          break
      fi
    done
    # if [[ -z "$reprocess_args" ]]; then
    #   echo "skipping $(basename $file)..."
    #   continue
    # fi
    "$(bazelisk info bazel-bin)/sekai/db/proto/tools/generate_proto" \
        --input="$file" \
        --output_dir="$root/sekai/db/proto" \
        "$reprocess_args" || exit 1
done
