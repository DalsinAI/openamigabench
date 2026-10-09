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

int main(void)
{
    char out[512];
    uint32_t tags[4] = { 0x80000021, 5, 0x8000003A, 0x1234 };
    uint8_t raw[4] = { 'T', 'i', 0, 0xFF };
    spy_line l;

    memset(&l, 0, sizeof l);
    l.seq = 42; l.task = "IPrefs"; l.lib = "workbench"; l.func = "WBConfig"; l.offset = -84;
    l.regs[0] = 3; l.regs[1] = 0x0807A2C0;
    l.nraw = 4; l.raw = raw;
    spy_format(out, sizeof out, &l);
    expect("a call with raw bytes", out,
           "000042 [IPrefs         ] workbench WBConfig(-84) d0=$00000003 d1=$0807A2C0 d2=$00000000 a0=$00000000"
           " a1=$00000000 a2=$00000000 a3=$00000000 a4=$00000000 raw=546900FF 'Ti..'");

    memset(&l, 0, sizeof l);
    l.seq = 43; l.task = "Workbench"; l.lib = "icon"; l.func = "GetIconTagList"; l.offset = -180;
    l.regs[3] = 0x00201000; l.str = "SYS:Prefs"; l.ntags = 2; l.tags = tags;
    spy_format(out, sizeof out, &l);
    expect("a call with a name and tags", out,
           "000043 [Workbench      ] icon GetIconTagList(-180) d0=$00000000 d1=$00000000 d2=$00000000 a0=$00201000"
           " a1=$00000000 a2=$00000000 a3=$00000000 a4=$00000000 \"SYS:Prefs\""
           " tags={$80000021=$00000005, $8000003A=$00001234}");

    memset(&l, 0, sizeof l);
    l.seq = 44; l.task = "a task with a very long name"; l.lib = "icon"; l.func = "GetIconTagList"; l.offset = -180;
    l.after = 1; l.regs[0] = 0x00305000;
    spy_format(out, sizeof out, &l);
    expect("a result", out, "000044 [a task with a v] icon GetIconTagList(-180) -> $00305000");

    /* never past the buffer */
    memset(&l, 0, sizeof l);
    l.seq = 1; l.task = "t"; l.lib = "icon"; l.func = "FreeDiskObject"; l.offset = -90;
    spy_format(out, 20, &l);
    expect("cut to the buffer", out, "000001 [t          ");  /* 19 characters and the 0 */

    if (fails) return 1;
    printf("spyfmt: all passed\n");
    return 0;
}
