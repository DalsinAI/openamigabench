#!/bin/sh
# For Cradle's stove builds: Cradle sets STOVE (its os32 stove) and OUT
# (where the build goes) and runs this. The build itself is build.sh.
#   STOVE=... OUT=... sh build-amiga.sh
# MIT, Copyright (c) 2026 Dalsin Limited.
set -eu
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
STOVE=${STOVE:-"$HOME/AmigaChrome/stoves/os32"}
OUT=${OUT:-"$HERE/build/os3"}
export STOVE
sh "$HERE/build.sh" "$OUT"
sha256sum "$OUT/workbench.library" "$OUT/icon.library" "$OUT/wbspy"
