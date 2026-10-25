#!/bin/sh
# OpenBench for Cradle's stove_build (target desktop): Cradle checks out this repo alone and sets STOVE and OUT, so
# this fetches the checkouts desktop/build.sh needs beside it (opengadtools, openamigaprefs, openamigafilemanager and,
# when it can, openamigatata) at main into $OUT/deps, says which commit each was, then runs build.sh. A local build
# can still run build.sh itself with OGT, OAP, FILES and TATA set (Team 5, 25 October 2026).
set -eu
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
OUT=${OUT:-$HERE/../build/os3}
STOVE=${STOVE:-$HOME/AmigaChrome/stoves/os32-gcc16}
DEPS=${DEPS:-$OUT/deps}
GIT_BASE=${AMIGACHROME_GIT_BASE:-https://github.com/}
mkdir -p "$DEPS"
fetch() {   # fetch NAME [optional]
    if [ ! -d "$DEPS/$1/.git" ]; then
        if ! git clone -q --depth 1 "${GIT_BASE}DalsinAI/$1" "$DEPS/$1"; then
            [ "${2:-}" = optional ] && { echo "build-amiga.sh: $1 not fetched; built without it"; return 0; }
            echo "build-amiga.sh: couldn't fetch DalsinAI/$1" >&2; exit 1
        fi
    fi
    echo "build-amiga.sh: $1 at $(git -C "$DEPS/$1" rev-parse --short HEAD)"
}
fetch opengadtools
fetch openamigaprefs
fetch openamigafilemanager
fetch openamigatata optional
OS32_GCC16=$STOVE OGT="$DEPS/opengadtools" OAP="$DEPS/openamigaprefs" FILES="$DEPS/openamigafilemanager" \
    TATA="$DEPS/openamigatata" sh "$HERE/build.sh" "$OUT"
rm -rf "$DEPS"   # the job's out/ keeps the program, not the sources
