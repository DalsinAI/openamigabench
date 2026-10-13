/* WorkbenchABITest: the Team's workbench.library lists beside OS 3.2's (step L1
 * of docs/design/Design-Open-Workbench-Library.md). It opens the system's
 * workbench.library and openwb.library (the Team's, under its test name), makes
 * the same calls through each, and prints each difference, then a count.
 * For lab instances only: with OS 3.2's Workbench running, its AppIcons
 * and Tools menu items appear for a moment while the test runs.
 *
 *   WorkbenchABITest [LIB path] [ALL] [NOLIMITS] [RISKY]
 *
 * LIB: the Team's library (LIBS:openwb.library by default). ALL: print the cases
 * that match too. NOLIMITS: skip filling the menus to their limits.
 * RISKY: also the calls that may crash OS 3.2's library (NULL labels,
 * NULL DiskObjects, a drop zone on no AppWindow, a handle removed twice).
 *
 * Each library runs the whole sequence in turn and its answers are kept
 * by case name; then the two are compared. Handles are compared by
 * whether they are NULL, keys by whether they are 0, BOOLs and numbers by
 * value. INFO lines are printed and not compared.
 *
 * MIT, Copyright (c) 2026 Dalsin Limited. */
#include <stdio.h>
#include <string.h>
#include <exec/types.h>
#include <exec/execbase.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/rdargs.h>
#include <intuition/intuition.h>
#include <workbench/workbench.h>
#include <workbench/icon.h>
#include <utility/tagitem.h>
#include <utility/hooks.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/icon.h>
#include <proto/wb.h>

static const char version[] __attribute__((used)) =
    "$VER: WorkbenchABITest 0.1 (13.10.2026) OpenBench L1, MIT, Copyright (c) 2026 Dalsin Limited";

struct Library *WorkbenchBase, *IconBase;
struct IntuitionBase *IntuitionBase;

#define TEMPLATE "LIB/K,ALL/S,NOLIMITS/S,RISKY/S"
enum { A_LIB, A_ALL, A_NOLIMITS, A_RISKY, A_COUNT };

#define MAXREC 400
struct rec { char name[60]; LONG v; };
struct run { struct rec r[MAXREC]; int n; };
static struct run runs[2];
static struct run *cur;
static int risky, limits = 1;
static int wb_tools_items;
static struct Library *ours_base;      /* items in OS 3.2's Tools menu before the test (dump_wb_menus) */

static void rec(const char *name, LONG v)
{
    if (cur->n < MAXREC) {
        strncpy(cur->r[cur->n].name, name, sizeof cur->r[0].name - 1);
        cur->r[cur->n].v = v;
        cur->n++;
    }
}

/* RISKY: each risky case's name is written to SYS:Salvor/workbenchlib/progress.txt
 * before it is made, so a lab that hangs says which call hung it. */
static const char *who;
static void step(const char *name)
{
    BPTR f = Open((CONST_STRPTR)"SYS:Salvor/workbenchlib/progress.txt", MODE_READWRITE);
    if (f) {
        Seek(f, 0, OFFSET_END);
        FPrintf(f, (CONST_STRPTR)"%s: %s\n", (ULONG)who, (ULONG)name);
        Close(f);
    }
}

static void recp(const char *name, APTR p) { rec(name, p != NULL); }
static void recb(const char *name, BOOL b) { rec(name, (LONG)b); }

static void recerr(const char *name)
{
    char n[72];
    sprintf(n, "%s IoErr", name);
    rec(n, IoErr());
}

static void info(const char *what, LONG v)
{
    printf("INFO %s %s: %ld\n", who, what, (long)v);
}

static int same_nocase(const char *a, const char *b)
{
    while (*a && *b) {
        int x = *a++, y = *b++;
        if (x >= 'a' && x <= 'z') x -= 32;
        if (y >= 'a' && y <= 'z') y -= 32;
        if (x != y) return 0;
    }
    return *a == *b;
}

/* a setup/cleanup and drop zone hook that does nothing, in case a
 * Workbench calls it */
static ULONG hook_entry(void) { return 0; }
static struct Hook dummy_hook = { { NULL, NULL }, (void *)hook_entry, NULL, NULL };

#define UNKNOWN_TAG (WBA_Dummy + 0xFF)

/* ---- the sequence, through WorkbenchBase ---- */

static void appwindows(struct Window *win, struct MsgPort *port)
{
    struct AppWindow *aw1, *aw2, *x;
    struct AppWindowDropZone *z1, *z2, *z3, *z4, *zx;
    struct TagItem unknown[] = { { UNKNOWN_TAG, 1 }, { TAG_END, 0 } };
    struct IBox box = { 60, 0, 40, 20 };
    struct TagItem t1[] = { { WBDZA_Left, 0 }, { WBDZA_Top, 0 }, { WBDZA_Width, 50 }, { WBDZA_Height, 20 }, { TAG_END, 0 } };
    struct TagItem t2[] = { { WBDZA_Box, (ULONG)&box }, { TAG_END, 0 } };
    struct TagItem t3[] = { { WBDZA_RelRight, (ULONG)-40 }, { WBDZA_RelBottom, (ULONG)-20 }, { WBDZA_RelWidth, (ULONG)-10 },
                            { WBDZA_RelHeight, (ULONG)-10 }, { WBDZA_Hook, (ULONG)&dummy_hook }, { TAG_END, 0 } };

    aw1 = AddAppWindowA(1, 2, win, port, NULL);
    recp("AddAppWindowA", aw1);
    x = AddAppWindowA(1, 2, NULL, port, NULL);
    recp("AddAppWindowA NULL window", x);
    if (x) recb("RemoveAppWindow NULL-window one", RemoveAppWindow(x));
    x = AddAppWindowA(1, 2, win, NULL, NULL);
    recp("AddAppWindowA NULL port", x);
    if (x) recb("RemoveAppWindow NULL-port one", RemoveAppWindow(x));
    aw2 = AddAppWindowA(3, 4, win, port, unknown);
    recp("AddAppWindowA unknown tag", aw2);
    if (aw1 && aw2) rec("aw1 != aw2", aw1 != aw2);

    if (aw1) {
        z1 = AddAppWindowDropZoneA(aw1, 11, 12, t1);
        recp("AddAppWindowDropZoneA box tags", z1);
        z2 = AddAppWindowDropZoneA(aw1, 21, 22, t2);
        recp("AddAppWindowDropZoneA WBDZA_Box", z2);
        z3 = AddAppWindowDropZoneA(aw1, 31, 32, t3);
        recp("AddAppWindowDropZoneA relative, hook", z3);
        SetIoErr(0);
        z4 = AddAppWindowDropZoneA(aw1, 41, 42, NULL);
        recp("AddAppWindowDropZoneA no tags", z4);
        if (!z4) recerr("AddAppWindowDropZoneA no tags");
        {   /* which tags a zone needs */
            static const struct { const char *name; ULONG t[10]; } v[] = {
                { "AddAppWindowDropZoneA Left, Top only", { WBDZA_Left, 5, WBDZA_Top, 5, TAG_END } },
                { "AddAppWindowDropZoneA Width, Height only", { WBDZA_Width, 30, WBDZA_Height, 10, TAG_END } },
                { "AddAppWindowDropZoneA Width only", { WBDZA_Width, 30, TAG_END } },
                { "AddAppWindowDropZoneA Height only", { WBDZA_Height, 10, TAG_END } },
                { "AddAppWindowDropZoneA Width 0", { WBDZA_Width, 0, WBDZA_Height, 10, TAG_END } },
                { "AddAppWindowDropZoneA Height 0", { WBDZA_Width, 30, WBDZA_Height, 0, TAG_END } },
                { "AddAppWindowDropZoneA negative Width", { WBDZA_Width, (ULONG)-5, WBDZA_Height, 10, TAG_END } },
                { "AddAppWindowDropZoneA RelWidth, RelHeight only", { WBDZA_RelWidth, 0, WBDZA_RelHeight, 0, TAG_END } },
                { "AddAppWindowDropZoneA outside the window", { WBDZA_Left, 900, WBDZA_Top, 900, WBDZA_Width, 30, WBDZA_Height, 10, TAG_END } },
                { "AddAppWindowDropZoneA bigger than the window", { WBDZA_Left, 0, WBDZA_Top, 0, WBDZA_Width, 2000, WBDZA_Height, 2000, TAG_END } },
                { "AddAppWindowDropZoneA all four, Width 0", { WBDZA_Left, 0, WBDZA_Top, 0, WBDZA_Width, 0, WBDZA_Height, 10, TAG_END } },
                { "AddAppWindowDropZoneA all four, Height 0", { WBDZA_Left, 0, WBDZA_Top, 0, WBDZA_Width, 10, WBDZA_Height, 0, TAG_END } },
                { "AddAppWindowDropZoneA all four, Width -5", { WBDZA_Left, 0, WBDZA_Top, 0, WBDZA_Width, (ULONG)-5, WBDZA_Height, 10, TAG_END } },
                { "AddAppWindowDropZoneA all four, Left -5", { WBDZA_Left, (ULONG)-5, WBDZA_Top, 0, WBDZA_Width, 10, WBDZA_Height, 10, TAG_END } },
                { "AddAppWindowDropZoneA Left, RelBottom, Width, RelHeight", { WBDZA_Left, 0, WBDZA_RelBottom, (ULONG)-10, WBDZA_Width, 10, WBDZA_RelHeight, 0, TAG_END } },
                { "AddAppWindowDropZoneA three of four", { WBDZA_Left, 0, WBDZA_Top, 0, WBDZA_Width, 10, TAG_END } },
                { "AddAppWindowDropZoneA Box and Width", { WBDZA_Width, 10, WBDZA_Box, 0, TAG_END } },
            };
            unsigned k;
            for (k = 0; k < sizeof v / sizeof v[0]; k++) {
                ULONG t[10];
                memcpy(t, v[k].t, sizeof t);
                if (t[2] == WBDZA_Box) t[3] = (ULONG)&box;    /* "Box and Width" */
                zx = AddAppWindowDropZoneA(aw1, 50 + k, 0, (struct TagItem *)t);
                recp(v[k].name, zx);
                if (zx)
                    RemoveAppWindowDropZone(aw1, zx);
            }
        }
        if (aw2 && z1) {
            SetIoErr(0);
            recb("RemoveAppWindowDropZone wrong window", RemoveAppWindowDropZone(aw2, z1));
            recerr("RemoveAppWindowDropZone wrong window");
        }
        if (z1) {
            SetIoErr(0);
            recb("RemoveAppWindowDropZone NULL window", RemoveAppWindowDropZone(NULL, z1));
            recerr("RemoveAppWindowDropZone NULL window");
        }
        SetIoErr(0);
        recb("RemoveAppWindowDropZone NULL zone", RemoveAppWindowDropZone(aw1, NULL));
        recerr("RemoveAppWindowDropZone NULL zone");
        if (z2) {
            SetIoErr(0);
            recb("RemoveAppWindowDropZone", RemoveAppWindowDropZone(aw1, z2));
            recerr("RemoveAppWindowDropZone");
            if (risky) {
                step("RemoveAppWindowDropZone again");
                SetIoErr(0);
                recb("RemoveAppWindowDropZone again", RemoveAppWindowDropZone(aw1, z2));
                recerr("RemoveAppWindowDropZone again");
            }
        }
        /* z1, z3 and z4 are still on aw1: removing it removes them */
        recb("RemoveAppWindow with zones", RemoveAppWindow(aw1));
        if (risky)
        {
            /* Removing an AppWindow a second time hangs OS 3.2's library
             * (measured, lab wblib-a): asked of the Team's only. */
            if (WorkbenchBase == ours_base)
                info("RemoveAppWindow again (hangs OS 3.2's)", RemoveAppWindow(aw1));
        }
    }
    if (risky) {
        step("AddAppWindowDropZoneA NULL window");
        zx = AddAppWindowDropZoneA(NULL, 1, 2, t1);
        recp("AddAppWindowDropZoneA NULL window", zx);
    }
    recb("RemoveAppWindow NULL", RemoveAppWindow(NULL));
    if (aw2)
        recb("RemoveAppWindow aw2", RemoveAppWindow(aw2));
}

static void appicons(struct MsgPort *port, struct DiskObject *dobj)
{
    struct AppIcon *a, *b, *x;
    struct TagItem t[] = {
        { WBAPPICONA_SupportsOpen, FALSE }, { WBAPPICONA_SupportsCopy, TRUE }, { WBAPPICONA_SupportsInformation, TRUE },
        { WBAPPICONA_SupportsEmptyTrash, TRUE }, { WBAPPICONA_SupportsEjectDisk, TRUE },
        { WBAPPICONA_NotifySelectState, TRUE }, { TAG_END, 0 }
    };
    a = AddAppIconA(1, 2, (CONST_STRPTR)"WBABITest", port, 0, dobj, NULL);
    recp("AddAppIconA", a);
    b = AddAppIconA(3, 4, (CONST_STRPTR)"WBABITest 2", port, 0, dobj, t);
    recp("AddAppIconA tags", b);
    if (a && b) rec("ai1 != ai2", a != b);
    x = AddAppIconA(5, 6, (CONST_STRPTR)"WBABITest port", NULL, 0, dobj, NULL);
    recp("AddAppIconA NULL port", x);
    if (x) recb("RemoveAppIcon NULL-port one", RemoveAppIcon(x));
    if (risky) {
        step("AddAppIconA NULL text");
        x = AddAppIconA(5, 6, NULL, port, 0, dobj, NULL);
        recp("AddAppIconA NULL text", x);
        if (x) recb("RemoveAppIcon NULL-text one", RemoveAppIcon(x));
        step("AddAppIconA NULL DiskObject");
        x = AddAppIconA(5, 6, (CONST_STRPTR)"WBABITest dobj", port, 0, NULL, NULL);
        recp("AddAppIconA NULL DiskObject", x);
        if (x) recb("RemoveAppIcon NULL-DiskObject one", RemoveAppIcon(x));
    }
    if (a) {
        recb("RemoveAppIcon", RemoveAppIcon(a));
        if (risky)
        {
            /* Removing an AppIcon a second time hangs OS 3.2's library
             * (measured, lab wblib-a): asked of the Team's only. */
            if (WorkbenchBase == ours_base)
                info("RemoveAppIcon again (hangs OS 3.2's)", RemoveAppIcon(a));
        }
    }
    if (b) recb("RemoveAppIcon tags one", RemoveAppIcon(b));
    recb("RemoveAppIcon NULL", RemoveAppIcon(NULL));
}

static void appmenus(struct MsgPort *port)
{
    struct AppMenuItem *m, *sep, *ck, *parent, *sub, *title, *under, *x;
    ULONG key = 0, tkey = 0;
    struct TagItem cmd[] = { { WBAPPMENUA_CommandKeyString, (ULONG)"Y" }, { TAG_END, 0 } };
    struct TagItem empty[] = { { WBAPPMENUA_CommandKeyString, (ULONG)"" }, { TAG_END, 0 } };
    struct TagItem getkey[] = { { WBAPPMENUA_GetKey, (ULONG)&key }, { TAG_END, 0 } };
    struct TagItem gettitle[] = { { WBAPPMENUA_GetTitleKey, (ULONG)&tkey }, { TAG_END, 0 } };
    struct TagItem usekey[] = { { WBAPPMENUA_UseKey, 0 }, { TAG_END, 0 } };

    m = AddAppMenuItemA(1, 2, (CONST_STRPTR)"WBABITest item", port, NULL);
    recp("AddAppMenuItemA", m);
    sep = AddAppMenuItemA(1, 2, (CONST_STRPTR)"---", port, NULL);
    recp("AddAppMenuItemA separator", sep);
    ck = AddAppMenuItemA(1, 2, (CONST_STRPTR)"WBABITest key", port, cmd);
    recp("AddAppMenuItemA command key", ck);
    x = AddAppMenuItemA(1, 2, (CONST_STRPTR)"WBABITest empty key", port, empty);
    recp("AddAppMenuItemA empty command key", x);
    if (x) recb("RemoveAppMenuItem empty-key one", RemoveAppMenuItem(x));
    x = AddAppMenuItemA(1, 2, (CONST_STRPTR)"WBABITest port", NULL, NULL);
    recp("AddAppMenuItemA NULL port", x);
    if (x) recb("RemoveAppMenuItem NULL-port one", RemoveAppMenuItem(x));
    if (risky) {
        step("AddAppMenuItemA NULL text");
        x = AddAppMenuItemA(1, 2, NULL, port, NULL);
        recp("AddAppMenuItemA NULL text", x);
        if (x) recb("RemoveAppMenuItem NULL-text one", RemoveAppMenuItem(x));
    }

    parent = AddAppMenuItemA(3, 4, (CONST_STRPTR)"WBABITest parent", port, getkey);
    recp("AddAppMenuItemA GetKey", parent);
    rec("GetKey key != 0", key != 0);
    usekey[0].ti_Data = key;
    sub = key ? AddAppMenuItemA(5, 6, (CONST_STRPTR)"WBABITest sub", port, usekey) : NULL;
    recp("AddAppMenuItemA UseKey (sub item)", sub);
    usekey[0].ti_Data = 0x7FFF1234;
    x = AddAppMenuItemA(5, 6, (CONST_STRPTR)"WBABITest bad key", port, usekey);
    recp("AddAppMenuItemA UseKey unknown", x);
    if (x) recb("RemoveAppMenuItem unknown-key one", RemoveAppMenuItem(x));

    title = AddAppMenuItemA(7, 8, (CONST_STRPTR)"WBABITest", port, gettitle);
    recp("AddAppMenuItemA GetTitleKey", title);
    rec("GetTitleKey key != 0", tkey != 0);
    if (key && tkey) rec("GetTitleKey key != GetKey key", tkey != key);
    usekey[0].ti_Data = tkey;
    under = tkey ? AddAppMenuItemA(9, 10, (CONST_STRPTR)"WBABITest under title", port, usekey) : NULL;
    recp("AddAppMenuItemA UseKey (title)", under);

    if (parent) recb("RemoveAppMenuItem parent first", RemoveAppMenuItem(parent));
    if (sub) {
        recb("RemoveAppMenuItem sub after parent", RemoveAppMenuItem(sub));
        if (risky)
        {
            step("RemoveAppMenuItem sub again");
            recb("RemoveAppMenuItem sub again", RemoveAppMenuItem(sub));
        }
    }
    if (under) recb("RemoveAppMenuItem under title", RemoveAppMenuItem(under));
    if (title) recb("RemoveAppMenuItem title", RemoveAppMenuItem(title));
    if (m) recb("RemoveAppMenuItem", RemoveAppMenuItem(m));
    if (sep) recb("RemoveAppMenuItem separator", RemoveAppMenuItem(sep));
    if (ck) recb("RemoveAppMenuItem command key", RemoveAppMenuItem(ck));
    recb("RemoveAppMenuItem NULL", RemoveAppMenuItem(NULL));
}

#define FILL 90
static struct AppMenuItem *filled[FILL];

static int fill(const char *text, struct MsgPort *port, struct TagItem *tags, int cap)
{
    int n;
    for (n = 0; n < cap; n++)
        if (!(filled[n] = AddAppMenuItemA((ULONG)n, 0, (CONST_STRPTR)text, port, tags)))
            break;
    return n;
}

static void unfill(int n)
{
    while (n > 0)
        RemoveAppMenuItem(filled[--n]);
}

static void menu_limits(struct MsgPort *port)
{
    ULONG key = 0, tkey = 0;
    struct TagItem getkey[] = { { WBAPPMENUA_GetKey, (ULONG)&key }, { TAG_END, 0 } };
    struct TagItem gettitle[] = { { WBAPPMENUA_GetTitleKey, (ULONG)&tkey }, { TAG_END, 0 } };
    struct TagItem usekey[] = { { WBAPPMENUA_UseKey, 0 }, { TAG_END, 0 } };
    struct AppMenuItem *parent, *title;
    int n;

    /* The limit counts every program's items. OS 3.2's Tools menu has
     * Workbench's own ResetWB and whatever other programs added (OpenUpMenu's
     * two on Instance-32), so those are counted back in for OS 3.2; the Team's
     * starts empty. */
    n = fill("WBABITest tool", port, NULL, FILL);
    rec("Tools menu holds, counting other programs' items",
        n + ((WorkbenchBase != ours_base && wb_tools_items > 0) ? wb_tools_items - 1 : 0));
    unfill(n);

    parent = AddAppMenuItemA(0, 0, (CONST_STRPTR)"WBABITest parent", port, getkey);
    usekey[0].ti_Data = key;
    n = (parent && key) ? fill("WBABITest sub", port, usekey, 40) : -1;
    rec("sub items under one item", n);
    if (n > 0) unfill(n);
    if (parent) RemoveAppMenuItem(parent);

    title = AddAppMenuItemA(0, 0, (CONST_STRPTR)"WBTitle", port, gettitle);
    usekey[0].ti_Data = tkey;
    n = (title && tkey) ? fill("WBABITest under", port, usekey, FILL) : -1;
    rec("items under one new title", n);
    if (n > 0) unfill(n);
    if (title) RemoveAppMenuItem(title);

    for (n = 0; n < 40; n++) {
        tkey = 0;
        if (!(filled[n] = AddAppMenuItemA((ULONG)n, 0, (CONST_STRPTR)"WBT", port, gettitle)))
            break;
    }
    rec("new menu titles", n);
    unfill(n);
}

static void control(void)
{
    ULONG stack = 0, restart = 0, flags = 0, v = 0;
    struct List *hidden = NULL;
    struct Node *nd;
    APTR hook;
    LONG found, count;
    struct TagItem unknown[] = { { UNKNOWN_TAG, 1 }, { TAG_END, 0 } };
    struct TagItem none[] = { { TAG_END, 0 } };
    BOOL ok;

    ok = WorkbenchControl(NULL, WBCTRLA_GetDefaultStackSize, (ULONG)&stack, TAG_END);
    recb("WBCTRLA_GetDefaultStackSize ok", ok);
    info("default stack size", (LONG)stack);
    ok = WorkbenchControl(NULL, WBCTRLA_SetDefaultStackSize, 12344, TAG_END);
    recb("WBCTRLA_SetDefaultStackSize ok", ok);
    v = 0;
    WorkbenchControl(NULL, WBCTRLA_GetDefaultStackSize, (ULONG)&v, TAG_END);
    rec("default stack size after setting 12344", (LONG)v);
    WorkbenchControl(NULL, WBCTRLA_SetDefaultStackSize, stack, TAG_END);

    ok = WorkbenchControl(NULL, WBCTRLA_GetTypeRestartTime, (ULONG)&restart, TAG_END);
    recb("WBCTRLA_GetTypeRestartTime ok", ok);
    info("type restart time", (LONG)restart);
    WorkbenchControl(NULL, WBCTRLA_SetTypeRestartTime, 7, TAG_END);
    v = 0;
    WorkbenchControl(NULL, WBCTRLA_GetTypeRestartTime, (ULONG)&v, TAG_END);
    rec("type restart time after setting 7", (LONG)v);
    WorkbenchControl(NULL, WBCTRLA_SetTypeRestartTime, restart, TAG_END);

    ok = WorkbenchControl(NULL, WBCTRLA_GetGlobalFlags, (ULONG)&flags, TAG_END);
    recb("WBCTRLA_GetGlobalFlags ok", ok);
    info("global flags", (LONG)flags);
    WorkbenchControl(NULL, WBCTRLA_SetGlobalFlags, flags ^ WBF_OLDDATESFIRST, TAG_END);
    v = 0;
    WorkbenchControl(NULL, WBCTRLA_GetGlobalFlags, (ULONG)&v, TAG_END);
    rec("global flags change as set", (LONG)(v == (flags ^ WBF_OLDDATESFIRST)));
    WorkbenchControl(NULL, WBCTRLA_SetGlobalFlags, flags, TAG_END);

    ok = WorkbenchControl(NULL, WBCTRLA_AddHiddenDeviceName, (ULONG)"WBABI9", TAG_END);
    recb("WBCTRLA_AddHiddenDeviceName ok", ok);
    ok = WorkbenchControl(NULL, WBCTRLA_GetHiddenDeviceList, (ULONG)&hidden, TAG_END);
    recb("WBCTRLA_GetHiddenDeviceList ok", ok);
    found = 0;
    count = 0;
    if (hidden) {
        for (nd = hidden->lh_Head; nd->ln_Succ; nd = nd->ln_Succ) {
            count++;
            if (nd->ln_Name && !strcmp(nd->ln_Name, "WBABI9"))
                found = 1;
        }
        WorkbenchControl(NULL, WBCTRLA_FreeHiddenDeviceList, (ULONG)hidden, TAG_END);
    }
    rec("hidden list has the name added", found);
    info("hidden devices", count);
    ok = WorkbenchControl(NULL, WBCTRLA_AddHiddenDeviceName, (ULONG)"wbabi9", TAG_END);
    recb("WBCTRLA_AddHiddenDeviceName again, other case ok", ok);
    ok = WorkbenchControl(NULL, WBCTRLA_RemoveHiddenDeviceName, (ULONG)"WBABI9", TAG_END);
    recb("WBCTRLA_RemoveHiddenDeviceName ok", ok);
    ok = WorkbenchControl(NULL, WBCTRLA_RemoveHiddenDeviceName, (ULONG)"WBABI9", TAG_END);
    recb("WBCTRLA_RemoveHiddenDeviceName again ok", ok);
    hidden = NULL;
    WorkbenchControl(NULL, WBCTRLA_GetHiddenDeviceList, (ULONG)&hidden, TAG_END);
    found = 0;
    if (hidden) {
        for (nd = hidden->lh_Head; nd->ln_Succ; nd = nd->ln_Succ)
            if (nd->ln_Name && same_nocase(nd->ln_Name, "WBABI9"))
                found++;
        WorkbenchControl(NULL, WBCTRLA_FreeHiddenDeviceList, (ULONG)hidden, TAG_END);
    }
    rec("hidden list after removing (any case)", found);

    hook = (APTR)1;
    ok = WorkbenchControl(NULL, WBCTRLA_GetCopyHook, (ULONG)&hook, TAG_END);
    recb("WBCTRLA_GetCopyHook ok", ok);
    rec("copy hook is NULL", hook == NULL);
    hook = (APTR)1;
    WorkbenchControl(NULL, WBCTRLA_GetDeleteHook, (ULONG)&hook, TAG_END);
    rec("delete hook is NULL", hook == NULL);
    hook = (APTR)1;
    WorkbenchControl(NULL, WBCTRLA_GetTextInputHook, (ULONG)&hook, TAG_END);
    rec("text input hook is NULL", hook == NULL);
    hook = (APTR)1;
    WorkbenchControl(NULL, WBCTRLA_GetDiskInfoHook, (ULONG)&hook, TAG_END);
    rec("disk info hook is NULL", hook == NULL);

    ok = WorkbenchControl(NULL, WBCTRLA_AddSetupCleanupHook, (ULONG)&dummy_hook, TAG_END);
    recb("WBCTRLA_AddSetupCleanupHook ok", ok);
    ok = WorkbenchControl(NULL, WBCTRLA_RemSetupCleanupHook, (ULONG)&dummy_hook, TAG_END);
    recb("WBCTRLA_RemSetupCleanupHook ok", ok);
    ok = WorkbenchControl(NULL, WBCTRLA_RemSetupCleanupHook, (ULONG)&dummy_hook, TAG_END);
    recb("WBCTRLA_RemSetupCleanupHook again ok", ok);

    /* the questions that need a desktop (the Team's answers them from step L3) */
    {
        LONG open = -1;
        BPTR path = 0;
        struct List *l;
        ok = WorkbenchControl((CONST_STRPTR)"SYS:", WBCTRLA_IsOpen, (ULONG)&open, TAG_END);
        recb("desktop: WBCTRLA_IsOpen ok", ok);
        if (ok) info("IsOpen SYS:", open);
        l = NULL;
        ok = WorkbenchControl(NULL, WBCTRLA_GetProgramList, (ULONG)&l, TAG_END);
        recb("desktop: WBCTRLA_GetProgramList ok", ok);
        if (ok && l) WorkbenchControl(NULL, WBCTRLA_FreeProgramList, (ULONG)l, TAG_END);
        l = NULL;
        ok = WorkbenchControl(NULL, WBCTRLA_GetSelectedIconList, (ULONG)&l, TAG_END);
        recb("desktop: WBCTRLA_GetSelectedIconList ok", ok);
        if (ok && l) WorkbenchControl(NULL, WBCTRLA_FreeSelectedIconList, (ULONG)l, TAG_END);
        l = NULL;
        ok = WorkbenchControl(NULL, WBCTRLA_GetOpenDrawerList, (ULONG)&l, TAG_END);
        recb("desktop: WBCTRLA_GetOpenDrawerList ok", ok);
        if (ok && l) WorkbenchControl(NULL, WBCTRLA_FreeOpenDrawerList, (ULONG)l, TAG_END);
        ok = WorkbenchControl(NULL, WBCTRLA_DuplicateSearchPath, (ULONG)&path, TAG_END);
        recb("desktop: WBCTRLA_DuplicateSearchPath ok", ok);
        if (ok && path) WorkbenchControl(NULL, WBCTRLA_FreeSearchPath, (ULONG)path, TAG_END);
    }

    recb("WorkbenchControlA unknown tag", WorkbenchControlA(NULL, unknown));
    recb("WorkbenchControlA no tags", WorkbenchControlA(NULL, none));
    recb("WorkbenchControlA NULL tags", WorkbenchControlA(NULL, NULL));
}

static void sequence(struct Library *base, const char *name, struct Window *win, struct MsgPort *port, struct DiskObject *dobj)
{
    struct AppMessage *m;
    WorkbenchBase = base;
    who = name;
    appwindows(win, port);
    appicons(port, dobj);
    appmenus(port);
    if (limits)
        menu_limits(port);
    control();
    /* nothing was dropped or chosen, so nothing should have come */
    Delay(5);
    while ((m = (struct AppMessage *)GetMsg(port))) {
        info("an AppMessage of type", m->am_Type);
        ReplyMsg((struct Message *)m);
    }
}

/* What OS 3.2's Workbench has in its menus now (its root window's menu
 * strip), to explain the Tools count: items other programs added count
 * against the same limit. Read under Forbid into a buffer, printed after. */
static void dump_wb_menus(struct Task *wb)
{
    static char buf[2048];
    char *p = buf, *end = buf + sizeof buf - 80;
    struct Screen *scr;
    struct Window *w;
    struct Menu *m;
    struct MenuItem *it;
    if (!wb || !(scr = LockPubScreen((CONST_STRPTR)"Workbench")))
        return;
    buf[0] = 0;
    Forbid();
    for (w = scr->FirstWindow; w; w = w->NextWindow)
        if (w->UserPort && w->UserPort->mp_SigTask == wb && w->MenuStrip)
            break;
    if (w)
        for (m = w->MenuStrip; m && p < end; m = m->NextMenu) {
            int n = 0;
            for (it = m->FirstItem; it; it = it->NextItem)
                n++;
            p += sprintf(p, "INFO OS 3.2's Workbench menu \"%.20s\": %d items", m->MenuName ? (char *)m->MenuName : "", n);
            if (m->MenuName && !strcmp((char *)m->MenuName, "Tools"))
                wb_tools_items = n;
            if (m->MenuName && !strcmp((char *)m->MenuName, "Tools"))
                for (it = m->FirstItem; it && p < end; it = it->NextItem)
                    p += sprintf(p, (it->Flags & ITEMTEXT) && it->ItemFill ? " [%.24s]" : " [-]",
                                 (it->Flags & ITEMTEXT) && it->ItemFill ? (char *)((struct IntuiText *)it->ItemFill)->IText : "");
            *p++ = '\n';
            *p = 0;
        }
    Permit();
    UnlockPubScreen(NULL, scr);
    printf("%s", buf);
}

static struct rec *find(struct run *r, const char *name)
{
    int i;
    for (i = 0; i < r->n; i++)
        if (!strcmp(r->r[i].name, name))
            return &r->r[i];
    return NULL;
}

int main(void)
{
    LONG args[A_COUNT] = { 0 };
    struct RDArgs *rda;
    struct Library *os = NULL, *ours = NULL;
    const char *path = "LIBS:openwb.library";
    struct Window *win;
    struct MsgPort *port;
    struct DiskObject *dobj;
    struct Task *wb;
    int i, show_all, cases = 0, diffs = 0;

    if (!(rda = ReadArgs((CONST_STRPTR)TEMPLATE, args, NULL))) {
        PrintFault(IoErr(), (CONST_STRPTR)"WorkbenchABITest");
        return RETURN_FAIL;
    }
    if (args[A_LIB]) path = (const char *)args[A_LIB];
    show_all = args[A_ALL] != 0;
    limits = args[A_NOLIMITS] == 0;
    risky = args[A_RISKY] != 0;

    IntuitionBase = (struct IntuitionBase *)OpenLibrary((CONST_STRPTR)"intuition.library", 39);
    IconBase = OpenLibrary((CONST_STRPTR)"icon.library", 44);
    os = OpenLibrary((CONST_STRPTR)"workbench.library", 44);
    ours = OpenLibrary((CONST_STRPTR)path, 44);
    if (!IntuitionBase || !IconBase || !os || !ours) {
        printf("cannot open: intuition %p, icon %p, workbench.library %p, %s %p\n",
               (void *)IntuitionBase, (void *)IconBase, (void *)os, path, (void *)ours);
        goto out;
    }
    Forbid();
    wb = FindTask((CONST_STRPTR)"Workbench");
    Permit();
    printf("OS 3.2: %s %ld.%ld (%s)", os->lib_Node.ln_Name, (long)os->lib_Version, (long)os->lib_Revision,
           os->lib_IdString ? (char *)os->lib_IdString : "");
    printf("the Team's: %s %ld.%ld\n", ours->lib_Node.ln_Name, (long)ours->lib_Version, (long)ours->lib_Revision);
    printf("a process called Workbench: %s\n", wb ? "yes" : "no");
    printf("risky cases %s, menu limits %s\n", risky ? "on" : "off", limits ? "on" : "off");
    ours_base = ours;
    dump_wb_menus(wb);

    win = OpenWindowTags(NULL, WA_Left, 20, WA_Top, 40, WA_Width, 240, WA_Height, 80,
                         WA_Title, (ULONG)"WorkbenchABITest", WA_DragBar, TRUE, WA_Activate, FALSE, TAG_END);
    port = CreateMsgPort();
    dobj = GetDefDiskObject(WBTOOL);
    if (!win || !port || !dobj) {
        printf("cannot make the window, port or DiskObject\n");
    } else {
        cur = &runs[0];
        sequence(os, "OS 3.2", win, port, dobj);
        cur = &runs[1];
        sequence(ours, "the Team's", win, port, dobj);

        for (i = 0; i < runs[0].n; i++) {
            struct rec *a = &runs[0].r[i], *b = find(&runs[1], a->name);
            cases++;
            if (!b) {
                diffs++;
                printf("DIFF %s: OS 3.2 %ld, the Team's (not run)\n", a->name, (long)a->v);
            } else if (a->v != b->v) {
                diffs++;
                printf("DIFF %s: OS 3.2 %ld, the Team's %ld\n", a->name, (long)a->v, (long)b->v);
            } else if (show_all) {
                printf("SAME %s: %ld\n", a->name, (long)a->v);
            }
        }
        for (i = 0; i < runs[1].n; i++)
            if (!find(&runs[0], runs[1].r[i].name)) {
                cases++;
                diffs++;
                printf("DIFF %s: OS 3.2 (not run), the Team's %ld\n", runs[1].r[i].name, (long)runs[1].r[i].v);
            }
        printf("WorkbenchABITest: %d checks, %d differ\n", cases, diffs);
    }
    if (dobj) FreeDiskObject(dobj);
    if (port) DeleteMsgPort(port);
    if (win) CloseWindow(win);
out:
    if (ours) CloseLibrary(ours);
    if (os) CloseLibrary(os);
    if (IconBase) CloseLibrary(IconBase);
    if (IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);
    FreeArgs(rda);
    return diffs ? RETURN_WARN : RETURN_OK;
}
