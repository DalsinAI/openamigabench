#!/bin/sh
# OpenMeters, the desktop widget (meters/openmeters.c), for AmigaOS 3.x with the os32 stove.
#   meters/build.sh [OUT_DIR]   (default build/os3): OpenMeters and its icon, OpenMeters.info
# 0.2 draws its panel as the dock draws its shelf, in the theme's colours: it links opengadtools' theme reader
# (lib/ogt_theme.c; OGT, default ../../opengadtools) and includes openamigaprefs' Common/wbclose.h (OAP, default
# ../../openamigaprefs), as desktop/build.sh does.
# MIT, Copyright (c) 2026 AmigaChrome Team.
set -eu
HERE=$(cd "$(dirname "$0")" && pwd)
# Soft float: the specs file links libnix's soft libm020 (maths through the
# mathieee libraries), not libm881, so one build runs on every 68040.
SPECS="-specs=$(cd "$HERE/.." && pwd)/toolchain/m68k-soft-m68040.specs"
OGT=${OGT:-$HERE/../../opengadtools}
OAP=${OAP:-$HERE/../../openamigaprefs}
STOVE=${STOVE:-$HOME/AmigaChrome/stoves/os32}
CC="$STOVE/prefix/bin/m68k-amigaos-gcc"
OUT=${1:-$HERE/../build/os3}
mkdir -p "$OUT"
"$CC" -noixemul -m68040 $SPECS -std=gnu99 -Wall -Werror -O2 -fno-delete-null-pointer-checks -fno-common \
    -I"$OGT/lib" -I"$OAP/Common" -o "$OUT/OpenMeters" "$HERE/openmeters.c" "$OGT/lib/ogt_theme.c" -lamiga
echo "OpenMeters: $(wc -c < "$OUT/OpenMeters") bytes"
# its icon, OpenMeters.info beside it: a frosted panel of bars, a GlowIcon in the Open family's style, with the
# tooltypes MAKEICON writes (meters/make_icon.py; needs Python with Pillow and an amigachrome checkout,
# AMIGACHROME, for its icon writer)
AMIGACHROME=${AMIGACHROME:-$HOME/AmigaChrome}
[ -f "$AMIGACHROME/scripts/amiga_icon.py" ] || AMIGACHROME=$HERE/../../amigachrome
if [ -f "$AMIGACHROME/scripts/amiga_icon.py" ]; then
    python3 "$HERE/make_icon.py" "$AMIGACHROME" "$OUT/OpenMeters.info" 16384
else
    echo "$OUT/OpenMeters.info not made: no amigachrome checkout (set AMIGACHROME)" >&2
fi
