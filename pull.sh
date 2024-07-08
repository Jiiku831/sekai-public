#!/usr/bin/zsh

root="${0:a:h}"
cd "$root"

git submodule update --remote

$root/tools/scrape_thumbnails.sh

metas_path="$root/data/storage-sekai-best/music_metas.json"
wget -O - "https://storage.sekai.best/sekai-best-assets/music_metas.json" | jq . > "$metas_path"
