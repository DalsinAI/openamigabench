/* Our workbench.library, step R1: every entry of 3.2.3's (wb_lib.sfd 47.1,
 * 19 entries, bias 30) forwarded to Hyperion's original, loaded privately
 * (proxy/proxy.c). It reports exactly 47.42, the version it implements
 * (amigachrome docs/design/Design-Workbench-Replacement.md, section 6).
 *
 * MIT, Copyright (c) 2026 Dalsin Limited. */
#include <exec/types.h>
#include <exec/resident.h>
#include <exec/nodes.h>

#include "../proxy/proxy.h"
#include "../proxy/stub.h"

/* First in the file, so the library's first bytes return at once when run. */
PROXY_START();

#define VERSION 47
#define REVISION 42

static const char name[] = "workbench.library";
static const char id[] = "workbench.library 47.42 (9.10.2026) OpenBench R1 (forwards to the original)\r\n";
static const char ver[] __attribute__((used)) = "$VER: workbench.library 47.42 (9.10.2026) OpenBench R1, MIT, Copyright (c) 2026 Dalsin Limited";

/* Each entry: its name, and what the spy copies (proxy.h). */
static const struct spy_func funcs[] = {
    { "UpdateWorkbench",            R_A0, -1,   -1,   0 },  /*  -30 name a0, lock a1, action d0 */
    { "QuoteWorkbench",             -1,   -1,   -1,   1 },  /*  -36 private: d0; the result's bytes */
    { "StartWorkbench",             -1,   -1,   R_D1, 0 },  /*  -42 private: flags d0, ptr d1 */
    { "AddAppWindowA",              -1,   R_A2, -1,   0 },  /*  -48 id d0, userdata d1, window a0, port a1, tags a2 */
    { "RemoveAppWindow",            -1,   -1,   -1,   0 },  /*  -54 a0 */
    { "AddAppIconA",                R_A0, R_A4, -1,   0 },  /*  -60 id d0, ud d1, text a0, port a1, lock a2, icon a3, tags a4 */
    { "RemoveAppIcon",              -1,   -1,   -1,   0 },  /*  -66 a0 */
    { "AddAppMenuItemA",            R_A0, R_A2, -1,   0 },  /*  -72 id d0, ud d1, text a0, port a1, tags a2 */
    { "RemoveAppMenuItem",          -1,   -1,   -1,   0 },  /*  -78 a0 */
    { "WBConfig",                   -1,   -1,   R_D1, 0 },  /*  -84 private: tag d0, value d1 */
    { "WBInfo",                     R_A1, -1,   -1,   0 },  /*  -90 lock a0, name a1, screen a2 */
    { "OpenWorkbenchObjectA",       R_A0, R_A1, -1,   0 },  /*  -96 name a0, tags a1 */
    { "CloseWorkbenchObjectA",      R_A0, R_A1, -1,   0 },  /* -102 name a0, tags a1 */
    { "WorkbenchControlA",          R_A0, R_A1, -1,   0 },  /* -108 name a0, tags a1 */
    { "AddAppWindowDropZoneA",      -1,   R_A1, -1,   0 },  /* -114 aw a0, id d0, ud d1, tags a1 */
    { "RemoveAppWindowDropZone",    -1,   -1,   -1,   0 },  /* -120 aw a0, zone a1 */
    { "ChangeWorkbenchSelectionA",  R_A0, R_A2, -1,   0 },  /* -126 name a0, hook a1, tags a2 */
    { "MakeWorkbenchObjectVisibleA", R_A0, R_A1, -1,  0 },  /* -132 name a0, tags a1 */
    { "WhichWorkbenchObjectA",      -1,   R_A1, -1,   0 },  /* -138 window a0, x d0, y d1, tags a1 */
};
#define NFUNCS (sizeof funcs / sizeof funcs[0])

const struct ProxyDef proxy_def = {
    name, "OpenUp.original.workbench.library", id, VERSION, REVISION, NFUNCS, funcs
};

PROXY_STUB(0, 30);
PROXY_STUB(1, 36);
PROXY_STUB(2, 42);
PROXY_STUB(3, 48);
PROXY_STUB(4, 54);
PROXY_STUB(5, 60);
PROXY_STUB(6, 66);
PROXY_STUB(7, 72);
PROXY_STUB(8, 78);
PROXY_STUB(9, 84);
PROXY_STUB(10, 90);
PROXY_STUB(11, 96);
PROXY_STUB(12, 102);
PROXY_STUB(13, 108);
PROXY_STUB(14, 114);
PROXY_STUB(15, 120);
PROXY_STUB(16, 126);
PROXY_STUB(17, 132);
PROXY_STUB(18, 138);

void stub_0(void), stub_1(void), stub_2(void), stub_3(void), stub_4(void), stub_5(void), stub_6(void),
     stub_7(void), stub_8(void), stub_9(void), stub_10(void), stub_11(void), stub_12(void), stub_13(void),
     stub_14(void), stub_15(void), stub_16(void), stub_17(void), stub_18(void);

struct Library *proxy_init(), *proxy_open();
BPTR proxy_close(), proxy_expunge();
ULONG proxy_null(void);

static const APTR functable[] = {
    (APTR)proxy_open, (APTR)proxy_close, (APTR)proxy_expunge, (APTR)proxy_null,
    (APTR)stub_0, (APTR)stub_1, (APTR)stub_2, (APTR)stub_3, (APTR)stub_4, (APTR)stub_5, (APTR)stub_6,
    (APTR)stub_7, (APTR)stub_8, (APTR)stub_9, (APTR)stub_10, (APTR)stub_11, (APTR)stub_12, (APTR)stub_13,
    (APTR)stub_14, (APTR)stub_15, (APTR)stub_16, (APTR)stub_17, (APTR)stub_18,
    (APTR)-1
};
typedef char check_count[sizeof functable / sizeof functable[0] == 4 + NFUNCS + 1 ? 1 : -1];

/* RTF_AUTOINIT's table: the base's size, the functions, no data table, init */
static const APTR inittable[4] = {
    (APTR)sizeof(struct ProxyBase), (APTR)functable, NULL, (APTR)proxy_init
};

const struct Resident romtag __attribute__((used)) = {
    RTC_MATCHWORD, (struct Resident *)&romtag, (APTR)(&romtag + 1), RTF_AUTOINIT, VERSION, NT_LIBRARY, 0,
    (char *)name, (char *)id, (APTR)inittable
};
