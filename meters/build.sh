#!/bin/sh
# OpenMeters, the desktop widget (meters/openmeters.c), for AmigaOS 3.x with the os32 stove.
#   meters/build.sh [OUT_DIR]   (default build/os3)
# MIT, Copyright (c) 2026 Dalsin Limited.
set -eu
HERE=$(cd "$(dirname "$0")" && pwd)
STOVE=${STOVE:-$HOME/AmigaChrome/stoves/os32}
CC="$STOVE/prefix/bin/m68k-amigaos-gcc"
OUT=${1:-$HERE/../build/os3}
mkdir -p "$OUT"
"$CC" -noixemul -m68020 -std=gnu99 -Wall -Werror -O2 -fno-delete-null-pointer-checks -fno-common \
    -o "$OUT/OpenMeters" "$HERE/openmeters.c" -lamiga
echo "OpenMeters: $(wc -c < "$OUT/OpenMeters") bytes"
