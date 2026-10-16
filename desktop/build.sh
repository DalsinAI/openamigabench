#!/bin/sh
# OpenBench, the desktop (desktop/openbench.c), with opengadtools' drawer window (workbench/wb_*.c) and
# its theme and drawing (lib/), from an opengadtools checkout: OGT, default ../../opengadtools.
#   desktop/build.sh [OUT_DIR]   (default build/os3)
# MIT, Copyright (c) 2026 AmigaChrome Team.
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
# Look: OpenPrefs Look (openamigaprefs Look/src/look.c), SYS:Prefs/Look; its theme reader is lib/ogt_theme.c, linked below
# shellcheck disable=SC2086
"$CC" $FLAGS -I"$L" -I"$C" -DIN_OPENBENCH -Dmain=look_main -c -o "$TMP/look.o" "$OAP/Look/src/look.c"
BUILTIN_OBJS="$BUILTIN_OBJS $TMP/look.o"
BUILTIN_DEFS="$BUILTIN_DEFS -DOB_HAVE_LOOK"
# Menus: OpenPrefs Menus, OpenMenus' editor (openamigaprefs Menus/src/menus.c), SYS:Prefs/Menus; its theme
# reader is lib/ogt_theme.c, linked below. OpenMenus' settings code (om_prefs.c) is compiled and linked once for
# every built-in that reads it: a block that needs it repeats these two lines. OpenMenus itself (Menus/engine,
# with om_kbd.c) is a background program, not an editor: it isn't built in.
# shellcheck disable=SC2086
"$CC" $FLAGS -I"$OAP/Menus/src" -I"$L" -DIN_OPENBENCH -Dmain=menusprefs_main -c -o "$TMP/menus.o" "$OAP/Menus/src/menus.c"
# shellcheck disable=SC2086
[ -f "$TMP/om_prefs.o" ] || "$CC" $FLAGS -I"$OAP/Menus/src" -c -o "$TMP/om_prefs.o" "$OAP/Menus/src/om_prefs.c"
case " $BUILTIN_OBJS " in *" $TMP/om_prefs.o "*) ;; *) BUILTIN_OBJS="$BUILTIN_OBJS $TMP/om_prefs.o" ;; esac
BUILTIN_OBJS="$BUILTIN_OBJS $TMP/menus.o"
BUILTIN_DEFS="$BUILTIN_DEFS -DOB_HAVE_MENUS"
# OpenDateTime (openamigaprefs DateTime/src): SYS:Prefs/OpenDateTime
DT="$OAP/DateTime/src"
# shellcheck disable=SC2086
"$CC" $FLAGS -DIN_OPENBENCH -Dmain=opendatetime_main -c -o "$TMP/opendatetime.o" "$DT/opendatetime.c"
BUILTIN_OBJS="$BUILTIN_OBJS $TMP/opendatetime.o"
BUILTIN_DEFS="$BUILTIN_DEFS -DOB_HAVE_OPENDATETIME"
# OpenDock: the dock's editor (openamigaprefs Dock/src/dockprefs.c), SYS:Prefs/OpenDock; entry dockprefs_main,
# as opendock_main is the dock's. Its settings code is the dock's od_dock.o, linked below.
# shellcheck disable=SC2086
"$CC" $FLAGS -I"$D" -DIN_OPENBENCH -Dmain=dockprefs_main -c -o "$TMP/dockprefs.o" "$D/dockprefs.c"
BUILTIN_OBJS="$BUILTIN_OBJS $TMP/dockprefs.o"
BUILTIN_DEFS="$BUILTIN_DEFS -DOB_HAVE_OPENDOCK"
# OpenGamepads (openamigaprefs Gamepads/src, with its copy of openinput.library's headers): SYS:Prefs/OpenGamepads
GP="$OAP/Gamepads"
for f in gamepads gp_core gp_pad gp_draw; do
    # shellcheck disable=SC2086
    "$CC" $FLAGS -I"$GP/src" -I"$GP/include" -DIN_OPENBENCH -Dmain=opengamepads_main -c -o "$TMP/opengamepads_$f.o" "$GP/src/$f.c"
    BUILTIN_OBJS="$BUILTIN_OBJS $TMP/opengamepads_$f.o"
done
BUILTIN_DEFS="$BUILTIN_DEFS -DOB_HAVE_OPENGAMEPADS"
# OpenInput (openamigaprefs Input/src; Common/obmsg.h for its Gamepads...): SYS:Prefs/OpenInput
INP="$OAP/Input/src"
# shellcheck disable=SC2086
"$CC" $FLAGS -I"$C" -DIN_OPENBENCH -Dmain=openinput_main -c -o "$TMP/openinput.o" "$INP/openinput.c"
BUILTIN_OBJS="$BUILTIN_OBJS $TMP/openinput.o"
BUILTIN_DEFS="$BUILTIN_DEFS -DOB_HAVE_OPENINPUT"
# OpenSerial: the serial port's editor (openamigaprefs Serial/src/openserial.c), SYS:Prefs/OpenSerial
# shellcheck disable=SC2086
"$CC" $FLAGS -DIN_OPENBENCH -Dmain=openserial_main -c -o "$TMP/openserial.o" "$OAP/Serial/src/openserial.c"
BUILTIN_OBJS="$BUILTIN_OBJS $TMP/openserial.o"
BUILTIN_DEFS="$BUILTIN_DEFS -DOB_HAVE_OPENSERIAL"
# OpenSound (openamigaprefs Sound/src, the editor: sound.c and its sp_*.c; not OpenSpeaker): SYS:Prefs/OpenSound
SND="$OAP/Sound/src"
for f in sound sp_core sp_amiga sp_test; do
    # shellcheck disable=SC2086
    "$CC" $FLAGS -I"$SND" -I"$C" -DIN_OPENBENCH -Dmain=opensound_main -c -o "$TMP/opensound_$f.o" "$SND/$f.c"
    BUILTIN_OBJS="$BUILTIN_OBJS $TMP/opensound_$f.o"
done
BUILTIN_DEFS="$BUILTIN_DEFS -DOB_HAVE_OPENSOUND"
# OpenTata: Tata's window (openamigatata src/prefs/tataprefs.c), SYS:Prefs/OpenTata, with Tata's inspection
# (src/tata_inspect.c and the code it shares with C:Tata) linked in for openbench.c (OB_HAVE_TATA): before
# OpenBench runs a program or opens a project it asks Tata itself, with no patch (the Team, 11 October 2026:
# Tata a built-in feature of OpenBench). From an openamigatata checkout: TATA, default ../../openamigatata;
# without one OpenBench is built without Tata, and says so. Tata's list helpers are compiled once, here.
TATA=${TATA:-$HERE/../../openamigatata}
OB_TATA_DEFS=""
if [ -f "$TATA/src/tata_inspect.c" ]; then
    TS="$TATA/src"
    for f in tata_db tata_files tata_inspect tata_ui; do
        # shellcheck disable=SC2086
        "$CC" $FLAGS -I"$TS" -DIN_OPENBENCH -c -o "$TMP/tata_$f.o" "$TS/$f.c"
        BUILTIN_OBJS="$BUILTIN_OBJS $TMP/tata_$f.o"
    done
    # the hashes at -O1, as Tata's own build.sh (GCC 6.5's -O2 has broken loops of their shape)
    # shellcheck disable=SC2086
    "$CC" $(echo $FLAGS | sed 's/-O2/-O1/') -fno-lto -I"$TS" -DIN_OPENBENCH -c -o "$TMP/tata_hash.o" "$TS/tata_hash.c"
    # shellcheck disable=SC2086
    "$CC" $FLAGS -Wno-pointer-sign -I"$TS" -I"$L" -DIN_OPENBENCH -Dmain=opentata_main -c -o "$TMP/opentata.o" "$TS/prefs/tataprefs.c"
    # its toolbar (the other built-ins draw none: compiled here, once)
    # shellcheck disable=SC2086
    "$CC" $FLAGS -I"$L" -c -o "$TMP/ogt_toolbar.o" "$L/ogt_toolbar.c"
    BUILTIN_OBJS="$BUILTIN_OBJS $TMP/tata_hash.o $TMP/opentata.o $TMP/ogt_toolbar.o"
    BUILTIN_DEFS="$BUILTIN_DEFS -DOB_HAVE_OPENTATA"
    OB_TATA_DEFS="-DOB_HAVE_TATA -I$TS"
    echo "OpenBench: Tata built in, from $TATA ($(cd "$TATA" && git rev-parse --short HEAD 2>/dev/null || echo 'not a git checkout'))"
else
    echo "OpenBench: Tata not built in (no openamigatata at $TATA: set TATA)"
fi
# OpenTypes: which program opens each kind of file (openamigaprefs OpenTypes/src/opentypes.c), SYS:Prefs/OpenTypes
# shellcheck disable=SC2086
"$CC" $FLAGS -DIN_OPENBENCH -Dmain=opentypes_main -c -o "$TMP/opentypes.o" "$OAP/OpenTypes/src/opentypes.c"
BUILTIN_OBJS="$BUILTIN_OBJS $TMP/opentypes.o"
BUILTIN_DEFS="$BUILTIN_DEFS -DOB_HAVE_OPENTYPES"
# OpenUp-Setup: the first-start wizard (openamigaprefs Setup/src/setup.c), SYS:Prefs/OpenUp-Setup, and
# SYS:WBStartup/OpenUp-Setup at the first start
# shellcheck disable=SC2086
"$CC" $FLAGS -DIN_OPENBENCH -Dmain=openup_setup_main -c -o "$TMP/openup_setup.o" "$OAP/Setup/src/setup.c"
BUILTIN_OBJS="$BUILTIN_OBJS $TMP/openup_setup.o"
BUILTIN_DEFS="$BUILTIN_DEFS -DOB_HAVE_OPENUP_SETUP"
# TitleBar: OpenPrefs Title bar (openamigaprefs TitleBar/src/titlebar.c), SYS:Prefs/TitleBar
# shellcheck disable=SC2086
"$CC" $FLAGS -I"$OAP/TitleBar/src" -I"$C" -DIN_OPENBENCH -Dmain=titlebar_main -c -o "$TMP/titlebar.o" "$OAP/TitleBar/src/titlebar.c"
BUILTIN_OBJS="$BUILTIN_OBJS $TMP/titlebar.o"
BUILTIN_DEFS="$BUILTIN_DEFS -DOB_HAVE_TITLEBAR"
# Windows: OpenPrefs Windows (openamigaprefs Windows/src/windows.c), SYS:Prefs/Windows
# shellcheck disable=SC2086
"$CC" $FLAGS -I"$C" -DIN_OPENBENCH -Dmain=windows_main -c -o "$TMP/windows.o" "$OAP/Windows/src/windows.c"
BUILTIN_OBJS="$BUILTIN_OBJS $TMP/windows.o"
BUILTIN_DEFS="$BUILTIN_DEFS -DOB_HAVE_WINDOWS"
# --- built-ins: end ---
# shellcheck disable=SC2086
"$CC" $FLAGS $BUILTIN_DEFS -I"$HERE" -c -o "$TMP/ob_builtin.o" "$HERE/ob_builtin.c"
# shellcheck disable=SC2086
"$CC" $FLAGS -c -o "$TMP/ob_malloc.o" "$HERE/ob_malloc.c"
# shellcheck disable=SC2086
# shellcheck disable=SC2086
"$CC" $FLAGS -c -o "$TMP/ob_info.o" "$HERE/ob_info.c"
# shellcheck disable=SC2086
"$CC" $FLAGS $OB_TATA_DEFS -I"$W" -I"$L" -I"$C" -o "$OUT/OpenBench" "$HERE/openbench.c" "$TMP/ob_builtin.o" "$TMP/ob_malloc.o" "$TMP/ob_info.o" $BUILTIN_OBJS \
    "$W/wb_drawer.c" "$W/wb_dir.c" "$W/wb_fmt.c" "$W/wb_pic.c" "$W/wb_glyph.c" "$W/wb_thumb.c" \
    "$L/ogt_theme.c" "$L/ogt_draw.c" "$L/ogt_icons.c" "$L/ogt_font.c" "$TMP/opendock.o" "$TMP/od_dock.o" -lamiga
rm -rf "$TMP"
echo "$OUT/OpenBench ($(wc -c < "$OUT/OpenBench") bytes)"
