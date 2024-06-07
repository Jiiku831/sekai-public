#!/usr/bin/zsh

root="$(realpath "${0:a:h}/../../../..")"

echo "Root: $root"

bazelisk build :generate_proto

for file in $root/data/master-db/*.json; do
    echo "processing: $file"
    skip_files=(
      areaItems.json
      areaItemLevels.json
      assetList.json
      cards.json
      cardEpisodes.json
      characterRanks.json
      events.json
      eventCards.json
      eventDeckBonuses.json
      eventRarityBonusRates.json
      gameCharacters.json
      gameCharacterUnits.json
      masterLessons.json
      skills.json
      worldBlooms.json
      worldBloomDifferentAttributeBonuses.json
      worldBloomSupportDeckBonus.json
    )
    should_skip=0
    for skip_file in "${skip_files[@]}"; do
      if [[ "$(basename $file)" = "$skip_file" ]]; then
          should_skip=1
          break
      fi
    done
    if (( should_skip )); then
      echo "skipping..."
      continue
    fi
    "$(bazelisk info bazel-bin)/sekai/db/proto/tools/generate_proto" \
        --overwrite --input="$file" \
        --output_dir="$root/sekai/db/proto" || exit 1
done
