#!/bin/sh
# OpenBench with the os32 stove (m68k-amigaos-gcc, NDK 3.2; the NDK is never
# in this repository). Step R1: our workbench.library and icon.library,
# which forward every call to Hyperion's originals, and wbspy.
#   build.sh [OUT_DIR]   (default build/os3)
# MIT, Copyright (c) 2026 AmigaChrome Team.
set -eu
HERE=$(cd "$(dirname "$0")" && pwd)
# Soft float: the specs file links libnix's soft libm020 (maths through the
# mathieee libraries), not libm881, so one build runs on every 68040.
SPECS="-specs=$HERE/toolchain/m68k-soft-m68040.specs"
STOVE=${STOVE:-$HOME/AmigaChrome/stoves/os32}
CC="$STOVE/prefix/bin/m68k-amigaos-gcc"
OUT=${1:-$HERE/build/os3}
mkdir -p "$OUT"
COMMON="-noixemul -m68040 $SPECS -std=gnu99 -Wall -Werror -O2 -fno-delete-null-pointer-checks -fno-common"
# The libraries: no C library and no start code (and no loop turned into a
# call to the memset they define themselves). Each file's first bytes
# are its own (PROXY_START), so the order of the file's top level is kept.
LIB="$COMMON -ffreestanding -fno-builtin -fno-tree-loop-distribute-patterns -fno-toplevel-reorder -nostartfiles -nostdlib"
# shellcheck disable=SC2086
"$CC" $LIB -o "$OUT/workbench.library" "$HERE/workbench/wblib.c" "$HERE/proxy/proxy.c" -lgcc
# shellcheck disable=SC2086
"$CC" $LIB -o "$OUT/icon.library" "$HERE/icon/iconlib.c" "$HERE/proxy/proxy.c" -lgcc
# Step R2: icon.library's own calls, not switched on: openicon.library
# (icon.library's table under a test name) and iconprobe, which calls it
# and Hyperion's side by side.
# shellcheck disable=SC2086
"$CC" $LIB -Wextra -c -o /dev/null "$HERE/icon/icon_core.c"
# shellcheck disable=SC2086
"$CC" $LIB -o "$OUT/openicon.library" "$HERE/icon/openicon.c" "$HERE/icon/icon_core.c" -lgcc
und=$("$STOVE/prefix/bin/m68k-amigaos-nm" -u "$OUT/openicon.library" || true)
[ -z "$und" ] || { echo "undefined in openicon.library: $und" >&2; exit 1; }
# shellcheck disable=SC2086
"$CC" $COMMON -o "$OUT/iconprobe" "$HERE/tools/iconprobe.c"
# the host tests' icons, for iconprobe in a lab (SYS:Storage/OpenBench-Icons)
if command -v python3 >/dev/null; then
  rm -rf "$OUT/OpenBench-Icons"
  python3 "$HERE/tests/host/icon_fixtures.py" "$OUT/OpenBench-Icons" >/dev/null
  rm -f "$OUT/OpenBench-Icons/"*.want "$OUT/OpenBench-Icons/manifest.txt" "$OUT/OpenBench-Icons/deflate.bin"
fi
# Step L1 (docs/design/Design-Open-Workbench-Library.md): workbench.library's
# own lists, not switched on: openbench.library (workbench.library's table
# under a test name) and WorkbenchABITest, which calls it and the system's
# side by side.
# shellcheck disable=SC2086
"$CC" $LIB -Wextra -c -o /dev/null "$HERE/workbench/wb_core.c"
# shellcheck disable=SC2086
"$CC" $LIB -o "$OUT/openbench.library" "$HERE/workbench/openbench.c" "$HERE/workbench/wb_core.c" -lgcc
und=$("$STOVE/prefix/bin/m68k-amigaos-nm" -u "$OUT/openbench.library" || true)
[ -z "$und" ] || { echo "undefined in openbench.library: $und" >&2; exit 1; }
# shellcheck disable=SC2086
"$CC" $COMMON -o "$OUT/WorkbenchABITest" "$HERE/tests/amiga/WorkbenchABITest.c"
# shellcheck disable=SC2086
"$CC" $COMMON -o "$OUT/wbspy" "$HERE/tools/wbspy.c" "$HERE/tools/spyfmt.c"
# r1swap: lets OS 3.2's own Workbench start under R1, for step L2 (lab only)
# shellcheck disable=SC2086
"$CC" $COMMON -o "$OUT/r1swap" "$HERE/tools/r1swap.c"
# alohaprobe: what Intuition tells a Workbench, and when (the native screen mode's step 4; lab only)
# shellcheck disable=SC2086
"$CC" $COMMON -o "$OUT/alohaprobe" "$HERE/tools/alohaprobe.c"
cp "$HERE/lab/Install-R1" "$HERE/lab/Uninstall-R1" "$OUT/"
size() { wc -c < "$1" | tr -d ' '; }
echo "$OUT: workbench.library ($(size "$OUT/workbench.library") bytes), icon.library ($(size "$OUT/icon.library") bytes), wbspy ($(size "$OUT/wbspy") bytes), alohaprobe ($(size "$OUT/alohaprobe") bytes), openicon.library ($(size "$OUT/openicon.library") bytes), iconprobe ($(size "$OUT/iconprobe") bytes), openbench.library ($(size "$OUT/openbench.library") bytes), WorkbenchABITest ($(size "$OUT/WorkbenchABITest") bytes)"
sha256sum "$OUT/openicon.library" "$OUT/iconprobe" "$OUT/openbench.library" "$OUT/WorkbenchABITest" 2>/dev/null || true
