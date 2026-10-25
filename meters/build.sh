#!/bin/sh
# OpenMeters, the desktop widget (meters/openmeters.c), for AmigaOS 3.x with the os32-gcc16 stove.
# The compiler: OS32_GCC16, else STOVE, else ~/AmigaChrome/stoves/os32-gcc16 (as desktop/build.sh); it stops
# when given an older GCC (the os32 stove's GCC 6.5.0b merges a word clear and the long clear after it into one
# clr.l at -O2), and builds -fno-tree-loop-distribute-patterns (GCC 16.2.0b's loop-shift trap).
#   meters/build.sh [OUT_DIR]   (default build/os3): OpenMeters and its icon, OpenMeters.info
# 0.2 draws its panel as the dock draws its shelf, in the theme's colours: it links opengadtools' theme reader
# (lib/ogt_theme.c; OGT, default ../../opengadtools) and includes openamigaprefs' Common/wbclose.h (OAP, default
# ../../openamigaprefs), as desktop/build.sh does.
# 0.2.5 draws in the icons' font: with no Workbench icon font in Font prefs, the theme's, through opengadtools'
# lib/ogt_font.c, as OpenBench's icon labels.
# MIT, Copyright (c) 2026 AmigaChrome Team.
set -eu
HERE=$(cd "$(dirname "$0")" && pwd)
# Soft float: the specs file links libnix's soft libm020 (maths through the
# mathieee libraries), not libm881, so one build runs on every 68040.
SPECS="-specs=$(cd "$HERE/.." && pwd)/toolchain/m68k-soft-m68040.specs"
OGT=${OGT:-$HERE/../../opengadtools}
OAP=${OAP:-$HERE/../../openamigaprefs}
STOVE=${OS32_GCC16:-${STOVE:-$HOME/AmigaChrome/stoves/os32-gcc16}}
CC="$STOVE/prefix/bin/m68k-amigaos-gcc"
GCCV=$("$CC" -dumpversion 2>/dev/null || echo 0)
[ "${GCCV%%.*}" -ge 16 ] 2>/dev/null || { echo "$CC is GCC $GCCV: this needs GCC 16 (the os32-gcc16 stove; set OS32_GCC16)"; exit 2; }
OUT=${1:-$HERE/../build/os3}
mkdir -p "$OUT"
# GCC 16.2.0-open3's late-combine pass has a wrong-code bug (GCC PR 123853): wbclose.h's strcpy(r->name,
# "OpenWBNotify") came out as a copy that steps a1 and stores through a1 again, one byte late, so the semaphore was
# named "". Off until OpenAmigaGCC open4 (openamigagcc #29), on a compiler that knows the option (as desktop/build.sh).
LATE=""
if "$CC" -fno-late-combine-instructions -S -o /dev/null -x c /dev/null 2>/dev/null; then LATE="-fno-late-combine-instructions"; fi
"$CC" -noixemul -m68040 $SPECS -std=gnu99 -Wall -Werror -O2 -fno-tree-loop-distribute-patterns -fno-delete-null-pointer-checks -fno-common $LATE \
    -I"$OGT/lib" -I"$OAP/Common" -o "$OUT/OpenMeters" "$HERE/openmeters.c" "$OGT/lib/ogt_theme.c" "$OGT/lib/ogt_font.c" -lamiga
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
