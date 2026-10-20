#!/bin/sh
# Packs dist/<game>-<target>.tar.gz (.zip for Windows) for each game, for
# av: the program at the top, with its levels (series/) where it has
# them, and COPYING, README (cgames' and the game's), PATCHES.md and the
# man page; csokoban also gets LEVELS.txt, the credits for its levels.
#
#   asciiverse/package.sh x86_64-unknown-linux-musl
set -eu
cd "$(dirname "$0")/.."
target="$1"
case "$target" in *windows*) exe=.exe ;; *) exe= ;; esac
mkdir -p dist
for game in cmines csokoban cblocks; do
  stage=stage/$game
  rm -rf "$stage" && mkdir -p "$stage"
  cp "out/$game$exe" "$stage/"
  [ -d "$game/series" ] && cp -r "$game/series" "$stage/"
  cp COPYING PATCHES.md "$stage/"
  cp README "$stage/README-cgames"
  cp "$game/README" "$stage/README"
  cp "$game/$game.6" "$stage/"
  [ "$game" = csokoban ] && cp asciiverse/csokoban-LEVELS.txt "$stage/LEVELS.txt"
  if [ -n "$exe" ]; then
    (cd "$stage" && zip -qr "../../dist/$game-$target.zip" .)
  else
    tar -czf "dist/$game-$target.tar.gz" -C "$stage" .
  fi
done
ls dist
