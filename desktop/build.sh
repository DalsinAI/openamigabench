#!/bin/sh
# OpenBench, the desktop (desktop/openbench.c), with opengadtools' drawer window (workbench/wb_*.c) and
# its theme and drawing (lib/), from an opengadtools checkout: OGT, default ../../opengadtools.
#   desktop/build.sh [OUT_DIR]   (default build/os3)
# MIT, Copyright (c) 2026 Dalsin Limited.
set -eu
HERE=$(cd "$(dirname "$0")" && pwd)
OGT=${OGT:-$HERE/../../opengadtools}
STOVE=${STOVE:-$HOME/AmigaChrome/stoves/os32}
CC="$STOVE/prefix/bin/m68k-amigaos-gcc"
OUT=${1:-$HERE/../build/os3}
mkdir -p "$OUT"
# The dock is OpenDock's code (openamigaprefs Dock/src), built in: OAP, default ../../openamigaprefs.
OAP=${OAP:-$HERE/../../openamigaprefs}
W="$OGT/workbench"
L="$OGT/lib"
D="$OAP/Dock/src"
C="$OAP/Common"
FLAGS="-noixemul -m68020 -std=gnu99 -Wall -Werror -O2 -fno-delete-null-pointer-checks -fno-common"
TMP=$(mktemp -d)
# shellcheck disable=SC2086
"$CC" $FLAGS -I"$D" -I"$C" -DOPENDOCK_IN_OPENBENCH -Dmain=opendock_main -c -o "$TMP/opendock.o" "$D/opendock.c"
# shellcheck disable=SC2086
"$CC" $FLAGS -I"$D" -I"$C" -c -o "$TMP/od_dock.o" "$D/od_dock.c"
# shellcheck disable=SC2086
# The programs built in (ob_builtin.h, ob_builtin_list.h): each block compiles one program's code with
# -DIN_OPENBENCH and its main() renamed into $TMP, adds its objects to BUILTIN_OBJS and its -DOB_HAVE_<NAME>
# to BUILTIN_DEFS. Keep the blocks in alphabetical order.
BUILTIN_OBJS=""
BUILTIN_DEFS=""
# --- built-ins: begin ---
# --- built-ins: end ---
# shellcheck disable=SC2086
"$CC" $FLAGS $BUILTIN_DEFS -I"$HERE" -c -o "$TMP/ob_builtin.o" "$HERE/ob_builtin.c"
# shellcheck disable=SC2086
"$CC" $FLAGS -c -o "$TMP/ob_malloc.o" "$HERE/ob_malloc.c"
# shellcheck disable=SC2086
"$CC" $FLAGS -I"$W" -I"$L" -I"$C" -o "$OUT/OpenBench" "$HERE/openbench.c" "$TMP/ob_builtin.o" "$TMP/ob_malloc.o" $BUILTIN_OBJS \
    "$W/wb_drawer.c" "$W/wb_dir.c" "$W/wb_fmt.c" "$W/wb_pic.c" "$W/wb_glyph.c" \
    "$L/ogt_theme.c" "$L/ogt_draw.c" "$L/ogt_icons.c" "$L/ogt_font.c" "$TMP/opendock.o" "$TMP/od_dock.o" -lamiga
rm -rf "$TMP"
echo "$OUT/OpenBench ($(wc -c < "$OUT/OpenBench") bytes)"
