#!/bin/sh
# OpenBench host tests: wbspy's lines (tools/spyfmt.c), workbench.library's
# lists (workbench/wb_core.c), and icon.library's core (icon/icon_core.c)
# against icons made by Python's own zlib.
# MIT, Copyright (c) 2026 AmigaChrome Team.
set -eu
HERE=$(cd "$(dirname "$0")/../.." && pwd)
OUT=${TMPDIR:-/tmp}/openbench-host-tests
mkdir -p "$OUT"
cc -std=c99 -Wall -Wextra -Werror -O1 -I"$HERE/tools" "$HERE/tools/spyfmt.c" "$HERE/tests/host/test_spyfmt.c" -o "$OUT/test_spyfmt"
"$OUT/test_spyfmt"

# icon_core, with the address and undefined-behaviour checkers where the
# compiler has them
SAN="-fsanitize=address,undefined -fno-sanitize-recover=all"
echo 'int main(void){return 0;}' > "$OUT/san.c"
cc $SAN "$OUT/san.c" -o "$OUT/san" 2>/dev/null && "$OUT/san" || SAN=""
# shellcheck disable=SC2086
cc -std=gnu99 -Wall -Wextra -Werror -O1 -g $SAN -I"$HERE/icon" "$HERE/icon/icon_core.c" \
  "$HERE/tests/host/test_icon_core.c" -o "$OUT/test_icon_core"
# wb_core: workbench.library's lists (step L1)
# shellcheck disable=SC2086
cc -std=c99 -Wall -Wextra -Werror -O1 -g $SAN -I"$HERE/workbench" "$HERE/workbench/wb_core.c" \
  "$HERE/tests/host/test_wb_core.c" -o "$OUT/test_wb_core"
"$OUT/test_wb_core"

rm -rf "$OUT/icons"
python3 "$HERE/tests/host/icon_fixtures.py" "$OUT/icons"
"$OUT/test_icon_core" "$OUT/icons"
bad=0
while read -r name exact; do
  if ! cmp -s "$OUT/icons/$name.want" "$OUT/icons/$name.got"; then
    echo "FAIL $name: what was read differs from what was written"
    diff "$OUT/icons/$name.want" "$OUT/icons/$name.got" | head -20
    bad=1
  fi
done < "$OUT/icons/manifest.txt"
[ "$bad" = 0 ] && echo "icon dumps: all match"
exit $bad
