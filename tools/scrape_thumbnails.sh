#!/usr/bin/zsh

root="$(realpath "${0:a:h}/..")"

cards_path=$(mktemp)

wget -O "$cards_path" https://raw.githubusercontent.com/Sekai-World/sekai-master-db-diff/main/cards.json

bazelisk build //tools:scrape_thumbnails

mkdir -p "$root/data/storage-sekai-best/sekai-assets/thumbnail/chara_rip/"
"$(bazelisk info bazel-bin)/tools/scrape_thumbnails" \
    --cards="$cards_path" |
    gallery-dl -i - -f /O -D "$root/data/storage-sekai-best/sekai-assets/thumbnail/chara_rip"

mkdir -p "$root/data/storage-sekai-best/sekai-assets/thumbnail/chara_16/"
mkdir -p "$root/data/storage-sekai-best/sekai-assets/thumbnail/chara_32/"
mkdir -p "$root/data/storage-sekai-best/sekai-assets/thumbnail/chara_48/"
mkdir -p "$root/data/storage-sekai-best/sekai-assets/thumbnail/chara_64/"
mkdir -p "$root/data/storage-sekai-best/sekai-assets/thumbnail/chara_128/"
cd "$root/data/storage-sekai-best/sekai-assets/thumbnail"

for img in chara_rip/*; do
    base="$(basename "$img")"
    if ! [[ -f "chara_16/$base" ]]; then
        echo convert "$img" -resize 16x16 "chara_16/$base"
        convert "$img" -resize 16x16 "chara_16/$base"
    fi
    if ! [[ -f "chara_32/$base" ]]; then
        echo convert "$img" -resize 32x32 "chara_32/$base"
        convert "$img" -resize 32x32 "chara_32/$base"
    fi
    if ! [[ -f "chara_48/$base" ]]; then
        echo convert "$img" -resize 48x48 "chara_48/$base"
        convert "$img" -resize 48x48 "chara_48/$base"
    fi
    if ! [[ -f "chara_64/$base" ]]; then
        echo convert "$img" -resize 64x64 "chara_64/$base"
        convert "$img" -resize 64x64 "chara_64/$base"
    fi
    if ! [[ -f "chara_128/$base" ]]; then
        echo convert "$img" -resize 128x128 "chara_128/$base"
        convert "$img" -resize 128x128 "chara_128/$base"
    fi
done

mkdir -p "$root/data/images/frame"
frame_images=(
  "attr_cool.png"
  "attr_cute.png"
  "attr_happy.png"
  "attr_mysterious.png"
  "attr_pure.png"
  "frame_1.png"
  "frame_2.png"
  "frame_3.png"
  "frame_4.png"
  "frame_birthday.png"
  "frame_3_trained.png"
  "frame_4_trained.png"
)
for img in "${frame_images[@]}"; do
  if ! [[ -f "$root/data/images/frame/$img" ]]; then
    wget -O "$root/data/images/frame/$img" "https://raw.githubusercontent.com/Sekaipedia/card-thumbnail-generator/master/src/images/$img"
  fi
done

if ! [[ -f "$root/data/images/favicon.png" ]]; then
  wget -O "$root/data/images/favicon.png" "https://jiiku.pages.dev/images/favicon.png"
fi
