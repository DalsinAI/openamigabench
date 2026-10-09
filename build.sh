#!/bin/sh
# OpenBench with the os32 stove (m68k-amigaos-gcc, NDK 3.2; the NDK is never
# in this repository). Step R1: our workbench.library and icon.library,
# which forward every call to Hyperion's originals, and wbspy.
#   build.sh [OUT_DIR]   (default build/os3)
# MIT, Copyright (c) 2026 Dalsin Limited.
set -eu
HERE=$(cd "$(dirname "$0")" && pwd)
STOVE=${STOVE:-$HOME/AmigaChrome/stoves/os32}
CC="$STOVE/prefix/bin/m68k-amigaos-gcc"
OUT=${1:-$HERE/build/os3}
mkdir -p "$OUT"
COMMON="-noixemul -m68020 -std=gnu99 -Wall -Werror -O2 -fno-delete-null-pointer-checks -fno-common"
# The libraries: no C library and no start code (and no loop turned into a
# call to the memset they define themselves). Each file's first bytes
# are its own (PROXY_START), so the order of the file's top level is kept.
LIB="$COMMON -ffreestanding -fno-builtin -fno-tree-loop-distribute-patterns -fno-toplevel-reorder -nostartfiles -nostdlib"
# shellcheck disable=SC2086
"$CC" $LIB -o "$OUT/workbench.library" "$HERE/workbench/wblib.c" "$HERE/proxy/proxy.c" -lgcc
# shellcheck disable=SC2086
"$CC" $LIB -o "$OUT/icon.library" "$HERE/icon/iconlib.c" "$HERE/proxy/proxy.c" -lgcc
# shellcheck disable=SC2086
"$CC" $COMMON -o "$OUT/wbspy" "$HERE/tools/wbspy.c" "$HERE/tools/spyfmt.c"
cp "$HERE/lab/Install-R1" "$HERE/lab/Uninstall-R1" "$OUT/"
size() { wc -c < "$1" | tr -d ' '; }
echo "$OUT: workbench.library ($(size "$OUT/workbench.library") bytes), icon.library ($(size "$OUT/icon.library") bytes), wbspy ($(size "$OUT/wbspy") bytes)"
