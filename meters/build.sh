#!/bin/sh
# OpenMeters, the desktop widget (meters/openmeters.c), for AmigaOS 3.x with the os32 stove.
#   meters/build.sh [OUT_DIR]   (default build/os3)
# 0.2 draws its panel as the dock draws its shelf, in the theme's colours: it links opengadtools' theme reader
# (lib/ogt_theme.c; OGT, default ../../opengadtools) and includes openamigaprefs' Common/wbclose.h (OAP, default
# ../../openamigaprefs), as desktop/build.sh does.
# MIT, Copyright (c) 2026 Dalsin Limited.
set -eu
HERE=$(cd "$(dirname "$0")" && pwd)
OGT=${OGT:-$HERE/../../opengadtools}
OAP=${OAP:-$HERE/../../openamigaprefs}
STOVE=${STOVE:-$HOME/AmigaChrome/stoves/os32}
CC="$STOVE/prefix/bin/m68k-amigaos-gcc"
OUT=${1:-$HERE/../build/os3}
mkdir -p "$OUT"
"$CC" -noixemul -m68020 -std=gnu99 -Wall -Werror -O2 -fno-delete-null-pointer-checks -fno-common \
    -I"$OGT/lib" -I"$OAP/Common" -o "$OUT/OpenMeters" "$HERE/openmeters.c" "$OGT/lib/ogt_theme.c" -lamiga
echo "OpenMeters: $(wc -c < "$OUT/OpenMeters") bytes"
