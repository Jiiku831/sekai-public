#!/usr/bin/zsh

root="${0:a:h}"
cd "$root"

git submodule update --remote
