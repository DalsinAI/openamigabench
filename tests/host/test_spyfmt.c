/* Host test: wbspy's lines (tools/spyfmt.c).
 * MIT, Copyright (c) 2026 Dalsin Limited. */
#include <stdio.h>
#include <string.h>
#include "spyfmt.h"

static int fails;

static void expect(const char *what, const char *got, const char *want)
{
    if (strcmp(got, want)) {
        printf("FAIL %s\n  got:  %s\n  want: %s\n", what, got, want);
        fails++;
    }
}

#define ZERO_REGS " d0=$00000000 d1=$00000000 d2=$00000000 a0=$00000000 a1=$00000000 a2=$00000000 a3=$00000000 a4=$00000000"

int main(void)
{
    char out[600];
    uint32_t tags[4] = { 0x80000021, 5, 0x8000003A, 0x1234 };
    uint8_t raw[5] = { 'T', 'i', '\'', 0, 0xFF };
    spy_line l;

    memset(&l, 0, sizeof l);
    l.seq = 42; l.time_ms = 12; l.time_us = 345; l.task = "IPrefs"; l.lib = "workbench"; l.func = "WBConfig"; l.offset = -84;
    l.regs[0] = 3; l.regs[1] = 0x0807A2C0;
    l.returned = 1; l.result = 1;
    l.nraw = 5; l.raw = raw;
    spy_format(out, sizeof out, &l);
    expect("a call with its result and raw bytes", out,
           "000042       12.345ms [IPrefs         ] workbench WBConfig(-84) d0=$00000003 d1=$0807A2C0 d2=$00000000"
           " a0=$00000000 a1=$00000000 a2=$00000000 a3=$00000000 a4=$00000000 -> $00000001 raw=54692700FF 'Ti\\'..'");

    memset(&l, 0, sizeof l);
    l.seq = 43; l.time_ms = -1; l.task = "Workbench"; l.lib = "icon"; l.func = "GetIconTagList"; l.offset = -180;
    l.str = "SYS:Prefs"; l.ntags = 2; l.tags = tags;
    spy_format(out, sizeof out, &l);
    expect("a call with a name and tags, still running", out,
           "000043 [Workbench      ] icon GetIconTagList(-180)" ZERO_REGS " \"SYS:Prefs\""
           " tags={$80000021=$00000005, $8000003A=$00001234} -> (no result recorded)");

    memset(&l, 0, sizeof l);
    l.seq = 44; l.time_ms = -1; l.task = "a task with a very long name"; l.lib = "icon"; l.func = "GetDiskObject";
    l.offset = -78; l.str = "Say \"hi\"\n\\ok\x01" "A"; l.returned = 1; l.result = 0x00305000;
    spy_format(out, sizeof out, &l);
    expect("a string with quotes, a newline, a backslash and a control character", out,
           "000044 [a task with a v] icon GetDiskObject(-78)" ZERO_REGS " \"Say \\\"hi\\\"\\n\\\\ok\\001A\" -> $00305000");

    /* a name with a newline stays on one line */
    memset(&l, 0, sizeof l);
    l.seq = 45; l.time_ms = 2210000; l.time_us = 7; l.task = "Bad\nName"; l.lib = "workbench"; l.func = "WBInfo";
    l.offset = -90; l.returned = 1; l.result = 1;
    spy_format(out, sizeof out, &l);
    expect("a caller's name escaped, and a time past 36 minutes", out,
           "000045  2210000.007ms [Bad\\nName      ] workbench WBInfo(-90)" ZERO_REGS " -> $00000001");

    /* a name cut only between escapes */
    memset(&l, 0, sizeof l);
    l.seq = 46; l.time_ms = -1; l.task = "fourteen chars\nmore"; l.lib = "icon"; l.func = "FreeDiskObject"; l.offset = -90;
    l.returned = 1;
    spy_format(out, sizeof out, &l);
    expect("a 15-column name never ends inside an escape", out,
           "000046 [fourteen chars ] icon FreeDiskObject(-90)" ZERO_REGS " -> $00000000");

    /* never past the buffer */
    memset(&l, 0, sizeof l);
    l.seq = 1; l.time_ms = -1; l.task = "t"; l.lib = "icon"; l.func = "FreeDiskObject"; l.offset = -90;
    spy_format(out, 20, &l);
    expect("cut to the buffer", out, "000001 [t          ");  /* 19 characters and the 0 */

    if (fails) return 1;
    printf("spyfmt: all passed\n");
    return 0;
}
