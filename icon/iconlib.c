/* Our icon.library, step R1: every entry of 3.2.3's (icon_lib.sfd 47.1,
 * 30 entries, bias 30) forwarded to Hyperion's original, loaded privately
 * (proxy/proxy.c). It reports exactly 47.5, the version it implements
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
#define REVISION 5

static const char name[] = "icon.library";
static const char id[] = "icon.library 47.5 (9.10.2026) OpenBench R1 (forwards to the original)\r\n";
static const char ver[] __attribute__((used)) = "$VER: icon.library 47.5 (9.10.2026) OpenBench R1, MIT, Copyright (c) 2026 Dalsin Limited";

/* Each entry: its name, and what the spy copies (proxy.h). */
static const struct spy_func funcs[] = {
    { "OBSOLETEGetWBObject",        R_A0, -1,   -1, 0 },    /*  -30 private: name a0 */
    { "OBSOLETEPutWBObject",        R_A0, -1,   -1, 0 },    /*  -36 private: name a0, object a1 */
    { "GetIcon",                    R_A0, -1,   -1, 0 },    /*  -42 private: name a0, icon a1, freelist a2 */
    { "PutIcon",                    R_A0, -1,   -1, 0 },    /*  -48 private: name a0, icon a1 */
    { "FreeFreeList",               -1,   -1,   -1, 0 },    /*  -54 a0 */
    { "OBSOLETEFreeWBObject",       -1,   -1,   -1, 0 },    /*  -60 private: a0 */
    { "OBSOLETEAllocWBObject",      -1,   -1,   -1, 0 },    /*  -66 private */
    { "AddFreeList",                -1,   -1,   -1, 0 },    /*  -72 freelist a0, mem a1, size a2 */
    { "GetDiskObject",              R_A0, -1,   -1, 0 },    /*  -78 name a0 */
    { "PutDiskObject",              R_A0, -1,   -1, 0 },    /*  -84 name a0, icon a1 */
    { "FreeDiskObject",             -1,   -1,   -1, 0 },    /*  -90 a0 */
    { "FindToolType",               R_A1, -1,   -1, 0 },    /*  -96 array a0, type a1 */
    { "MatchToolValue",             R_A0, -1,   -1, 0 },    /* -102 typeString a0, value a1 */
    { "BumpRevision",               R_A1, -1,   -1, 0 },    /* -108 newname a0, oldname a1 */
    { "FreeAlloc",                  -1,   -1,   -1, 0 },    /* -114 freelist a0, len a1, type a2 */
    { "GetDefDiskObject",           -1,   -1,   -1, 0 },    /* -120 type d0 */
    { "PutDefDiskObject",           -1,   -1,   -1, 0 },    /* -126 icon a0 */
    { "GetDiskObjectNew",           R_A0, -1,   -1, 0 },    /* -132 name a0 */
    { "DeleteDiskObject",           R_A0, -1,   -1, 0 },    /* -138 name a0 */
    { "FreeFree",                   -1,   -1,   -1, 0 },    /* -144 freelist a0, address a1 */
    { "DupDiskObjectA",             -1,   R_A1, -1, 0 },    /* -150 icon a0, tags a1 */
    { "IconControlA",               -1,   R_A1, -1, 0 },    /* -156 icon a0, tags a1 */
    { "DrawIconStateA",             R_A2, R_A3, -1, 0 },    /* -162 rp a0, icon a1, label a2, x d0, y d1, state d2, tags a3 */
    { "GetIconRectangleA",          R_A2, R_A4, -1, 0 },    /* -168 rp a0, icon a1, label a2, rect a3, tags a4 */
    { "NewDiskObject",              -1,   -1,   -1, 0 },    /* -174 type d0 */
    { "GetIconTagList",             R_A0, R_A1, -1, 0 },    /* -180 name a0, tags a1 */
    { "PutIconTagList",             R_A0, R_A2, -1, 0 },    /* -186 name a0, icon a1, tags a2 */
    { "LayoutIconA",                -1,   R_A2, -1, 0 },    /* -192 icon a0, screen a1, tags a2 */
    { "ChangeToSelectedIconColor",  -1,   -1,   -1, 0 },    /* -198 cr a0 */
    { "BumpRevisionLength",         R_A1, -1,   -1, 0 },    /* -204 newname a0, oldname a1, length d0 */
};
#define NFUNCS (sizeof funcs / sizeof funcs[0])

const struct ProxyDef proxy_def = {
    name, "OpenUp.original.icon.library", id, VERSION, REVISION, NFUNCS, funcs
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
PROXY_STUB(19, 144);
PROXY_STUB(20, 150);
PROXY_STUB(21, 156);
PROXY_STUB(22, 162);
PROXY_STUB(23, 168);
PROXY_STUB(24, 174);
PROXY_STUB(25, 180);
PROXY_STUB(26, 186);
PROXY_STUB(27, 192);
PROXY_STUB(28, 198);
PROXY_STUB(29, 204);

void stub_0(void), stub_1(void), stub_2(void), stub_3(void), stub_4(void), stub_5(void), stub_6(void),
     stub_7(void), stub_8(void), stub_9(void), stub_10(void), stub_11(void), stub_12(void), stub_13(void),
     stub_14(void), stub_15(void), stub_16(void), stub_17(void), stub_18(void), stub_19(void), stub_20(void),
     stub_21(void), stub_22(void), stub_23(void), stub_24(void), stub_25(void), stub_26(void), stub_27(void),
     stub_28(void), stub_29(void);

struct Library *proxy_init(), *proxy_open();
BPTR proxy_close(), proxy_expunge();
ULONG proxy_null(void);

static const APTR functable[] = {
    (APTR)proxy_open, (APTR)proxy_close, (APTR)proxy_expunge, (APTR)proxy_null,
    (APTR)stub_0, (APTR)stub_1, (APTR)stub_2, (APTR)stub_3, (APTR)stub_4, (APTR)stub_5, (APTR)stub_6,
    (APTR)stub_7, (APTR)stub_8, (APTR)stub_9, (APTR)stub_10, (APTR)stub_11, (APTR)stub_12, (APTR)stub_13,
    (APTR)stub_14, (APTR)stub_15, (APTR)stub_16, (APTR)stub_17, (APTR)stub_18, (APTR)stub_19, (APTR)stub_20,
    (APTR)stub_21, (APTR)stub_22, (APTR)stub_23, (APTR)stub_24, (APTR)stub_25, (APTR)stub_26, (APTR)stub_27,
    (APTR)stub_28, (APTR)stub_29,
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
