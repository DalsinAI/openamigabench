#!/bin/sh
# OpenBench host tests: wbspy's lines (tools/spyfmt.c).
# MIT, Copyright (c) 2026 Dalsin Limited.
set -eu
HERE=$(cd "$(dirname "$0")/../.." && pwd)
OUT=${TMPDIR:-/tmp}/openbench-host-tests
mkdir -p "$OUT"
cc -std=c99 -Wall -Wextra -Werror -O1 -I"$HERE/tools" "$HERE/tools/spyfmt.c" "$HERE/tests/host/test_spyfmt.c" -o "$OUT/test_spyfmt"
"$OUT/test_spyfmt"
