#!/bin/sh
# icon_core on a big-endian 68k: the host test built with an m68k Linux
# cross compiler at -O0 and -O2 for the 68020 and 68040 and run under
# qemu-m68k, on run.sh's fixtures (run run.sh first). Skipped without them.
# MIT, Copyright (c) 2026 Dalsin Limited.
set -eu
HERE=$(cd "$(dirname "$0")/../.." && pwd)
OUT=${TMPDIR:-/tmp}/openbench-host-tests
CC=${M68K_CC:-m68k-linux-gnu-gcc}
QEMU=${QEMU_M68K:-qemu-m68k}
command -v "$CC" >/dev/null && command -v "$QEMU" >/dev/null || { echo "skipped: no $CC or $QEMU"; exit 0; }
[ -f "$OUT/icons/manifest.txt" ] || { echo "run tests/host/run.sh first"; exit 1; }
bad=0
for cpu in 68020 68040; do
  for opt in -O0 -O2; do
    "$CC" -m$cpu $opt -std=gnu99 -Wall -Wextra -Werror -static -I"$HERE/icon" \
      "$HERE/icon/icon_core.c" "$HERE/tests/host/test_icon_core.c" -o "$OUT/test_icon_core.m68k"
    "$QEMU" -cpu m68040 "$OUT/test_icon_core.m68k" "$OUT/icons" > "$OUT/m68k.log" || { tail -20 "$OUT/m68k.log"; bad=1; }
    while read -r name exact; do
      cmp -s "$OUT/icons/$name.want" "$OUT/icons/$name.got" || { echo "FAIL $cpu $opt $name"; bad=1; }
    done < "$OUT/icons/manifest.txt"
    echo "m$cpu $opt: $(tail -n 1 "$OUT/m68k.log")"
  done
done
exit $bad
