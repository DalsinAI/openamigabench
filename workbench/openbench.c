/* openbench.library: the Team's workbench.library list calls, at
 * workbench.library's LVOs, under its test name (step L1 of
 * docs/design/Design-Open-Workbench-Library.md). workbench.library is
 * OpenBench itself, version 48 (the user's decisions of 13 October 2026);
 * this test build holds only its lists. It is opened beside the system's
 * workbench.library by tests/amiga/WorkbenchABITest, which makes every call
 * through both and lists each difference. Nothing opens it by the name
 * workbench.library, it forwards nothing, and it hooks nothing: no
 * SetFunction, no AlohaWorkbench, no public port, no process.
 *
 * Done here (L1), in wb_core.c: AddAppWindowA, RemoveAppWindow,
 * AddAppIconA, RemoveAppIcon, AddAppMenuItemA, RemoveAppMenuItem,
 * AddAppWindowDropZoneA, RemoveAppWindowDropZone, and WorkbenchControlA's
 * settings (the default stack size, the type restart time, the global
 * flags, the hidden devices, the copy, delete, text input and disk info
 * hooks, the setup and cleanup hooks).
 *
 * Not here: everything that needs a desktop (UpdateWorkbench, WBInfo,
 * OpenWorkbenchObjectA, CloseWorkbenchObjectA, ChangeWorkbenchSelectionA,
 * MakeWorkbenchObjectVisibleA, WhichWorkbenchObjectA, WorkbenchControlA's
 * questions about windows and icons) and the private calls
 * (QuoteWorkbench, StartWorkbench, WBConfig). They answer failure, as 3.2's
 * do while no Workbench runs, and change nothing. Step L3 and on.
 *
 * App objects are kept whether or not a desktop runs (the user's decision
 * 3): the desktop shows them once it is up. OS 3.2's refuses them with no
 * Workbench running, so WorkbenchABITest differs there, by design.
 *
 * MIT, Copyright (c) 2026 AmigaChrome Team. */
#include <stddef.h>
#include <exec/types.h>
#include <exec/resident.h>
#include <exec/libraries.h>
#include <exec/memory.h>
#include <exec/semaphores.h>
#include <exec/lists.h>
#include <exec/execbase.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <intuition/intuition.h>
#include <workbench/workbench.h>
#include <utility/tagitem.h>
#include <utility/hooks.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>

#include "wb_core.h"

#define REG(r, decl) register decl __asm(#r)

#ifndef LIBNAME
#define LIBNAME "openbench.library"
#endif
#define VERSION 48                  /* OS 3.2's 19 entries, the Team's after them (the user's decision 6) */
#define REVISION 1

/* First in the file: run as a program, the library returns at once. */
__asm("\t.text\n\t.globl _owb_start\n_owb_start:\n\tmoveq #-1,d0\n\trts\n");

static const char lib_name[] = LIBNAME;
static const char lib_id[] = "openbench.library 48.1 (13.10.2026) OpenBench L1 (the Team's lists, for the side-by-side test)\r\n";
static const char lib_ver[] __attribute__((used)) =
    "$VER: openbench.library 48.1 (13.10.2026) OpenBench L1, MIT, Copyright (c) 2026 AmigaChrome Team";

/* wb_core's AppMessage offsets are the NDK's */
#define CHECK_OFF(f, o) typedef char check_##f[offsetof(struct AppMessage, f) == (o) ? 1 : -1]
typedef char check_am_size[sizeof(struct AppMessage) == WBC_APPMSG_SIZE ? 1 : -1];
CHECK_OFF(am_Type, WBC_AM_OFF_TYPE);
CHECK_OFF(am_UserData, WBC_AM_OFF_USERDATA);
CHECK_OFF(am_ID, WBC_AM_OFF_ID);
CHECK_OFF(am_NumArgs, WBC_AM_OFF_NUMARGS);
CHECK_OFF(am_ArgList, WBC_AM_OFF_ARGLIST);
CHECK_OFF(am_Version, WBC_AM_OFF_VERSION);
CHECK_OFF(am_Class, WBC_AM_OFF_CLASS);
CHECK_OFF(am_MouseX, WBC_AM_OFF_MOUSEX);
CHECK_OFF(am_MouseY, WBC_AM_OFF_MOUSEY);
CHECK_OFF(am_Seconds, WBC_AM_OFF_SECONDS);
CHECK_OFF(am_Micros, WBC_AM_OFF_MICROS);
CHECK_OFF(am_Reserved, WBC_AM_OFF_RESERVED);
typedef char check_tag_base[WBA_Dummy == WBC_TAG_BASE ? 1 : -1];
typedef char check_types[AMTYPE_APPWINDOW == WBC_AMTYPE_APPWINDOW && AMTYPE_APPICON == WBC_AMTYPE_APPICON &&
                         AMTYPE_APPMENUITEM == WBC_AMTYPE_APPMENUITEM && AMTYPE_APPWINDOWZONE == WBC_AMTYPE_APPWINDOWZONE &&
                         AM_VERSION == WBC_AM_VERSION ? 1 : -1];
/* a handle is the library's own object, and starts with the stub's word */
typedef char check_stub[offsetof(struct wbc_obj, priv) == 0 ? 1 : -1];

struct ExecBase *SysBase;
struct DosLibrary *DOSBase;
struct Library *UtilityBase;

struct OWBBase {
    struct Library lib;
    UWORD pad;
    BPTR seglist;
    struct SignalSemaphore lock;    /* everything in st */
    struct wbc_state st;
    struct wbc_env env;
};

static struct OWBBase *Base;

/* No C library: the compiler may still call these for structure copies. */
void *memset(void *d, int c, size_t n)
{
    UBYTE *p = d;
    while (n--) *p++ = (UBYTE)c;
    return d;
}

void *memcpy(void *d, const void *s, size_t n)
{
    UBYTE *p = d;
    const UBYTE *q = s;
    while (n--) *p++ = *q++;
    return d;
}

static void *env_alloc(struct wbc_env *e, uint32_t size)
{
    (void)e;
    return AllocVec(size, MEMF_ANY | MEMF_CLEAR);
}

static void env_free(struct wbc_env *e, void *p)
{
    (void)e;
    FreeVec(p);
}

/* ---- calls into the core, under the lock ---- */

static void lock(void) { ObtainSemaphore(&Base->lock); }
static void unlock(void) { ReleaseSemaphore(&Base->lock); }

/* A program's tag list as the core's pairs, walked with NextTagItem
 * (TAG_MORE, TAG_SKIP and TAG_IGNORE as utility.library does them).
 * WBDZA_Box is given as its four parts. NULL if there is no memory. */
static struct wbc_tag *core_tags(const struct TagItem *tags)
{
    struct TagItem *walk = (struct TagItem *)tags, *t;
    struct wbc_tag *out;
    ULONG n = 0, i = 0;
    while ((t = NextTagItem(&walk)))
        n += t->ti_Tag == WBDZA_Box ? 4 : 1;
    if (!(out = AllocVec((n + 1) * sizeof *out, MEMF_ANY | MEMF_CLEAR)))
        return NULL;
    walk = (struct TagItem *)tags;
    while ((t = NextTagItem(&walk))) {
        if (t->ti_Tag == WBDZA_Box) {
            struct IBox *b = (struct IBox *)t->ti_Data;
            if (!b)
                continue;
            out[i].tag = WBDZA_Left; out[i++].data = (UWORD)b->Left;
            out[i].tag = WBDZA_Top; out[i++].data = (UWORD)b->Top;
            out[i].tag = WBDZA_Width; out[i++].data = (UWORD)b->Width;
            out[i].tag = WBDZA_Height; out[i++].data = (UWORD)b->Height;
        } else {
            out[i].tag = t->ti_Tag;
            out[i++].data = t->ti_Data;
        }
    }
    out[i].tag = 0;
    return out;
}

static LONG io_error(int r)
{
    switch (r) {
    case WBC_ERR_NOMEM: return ERROR_NO_FREE_STORE;
    case WBC_ERR_UNKNOWN: return ERROR_OBJECT_NOT_FOUND;
    case WBC_ERR_ARGS: return ERROR_REQUIRED_ARG_MISSING;
    default: return 0;
    }
}

/* ---- the list calls ---- */

static struct AppWindow *L_AddAppWindowA(REG(d0, ULONG id), REG(d1, ULONG userdata), REG(a0, struct Window *window),
                                         REG(a1, struct MsgPort *port), REG(a2, struct TagItem *tags))
{
    struct wbc_tag *t = core_tags(tags);
    struct wbc_obj *o = NULL;
    if (!t)
        return NULL;
    lock();
    wbc_add_appwindow(&Base->st, id, userdata, window, port, t, &o);
    unlock();
    FreeVec(t);
    return (struct AppWindow *)o;
}

static BOOL L_RemoveAppWindow(REG(a0, struct AppWindow *aw))
{
    int r;
    lock();
    r = wbc_remove_appwindow(&Base->st, aw);
    unlock();
    return r == WBC_OK;
}

static struct AppIcon *L_AddAppIconA(REG(d0, ULONG id), REG(d1, ULONG userdata), REG(a0, CONST_STRPTR text),
                                     REG(a1, struct MsgPort *port), REG(a2, BPTR lk), REG(a3, struct DiskObject *dobj),
                                     REG(a4, struct TagItem *tags))
{
    struct wbc_tag *t = core_tags(tags);
    struct wbc_obj *o = NULL;
    if (!t)
        return NULL;
    lock();
    wbc_add_appicon(&Base->st, id, userdata, (const char *)text, port, (uint32_t)lk, dobj, t, &o);
    unlock();
    FreeVec(t);
    return (struct AppIcon *)o;
}

static BOOL L_RemoveAppIcon(REG(a0, struct AppIcon *ai))
{
    int r;
    lock();
    r = wbc_remove_appicon(&Base->st, ai);
    unlock();
    return r == WBC_OK;
}

static struct AppMenuItem *L_AddAppMenuItemA(REG(d0, ULONG id), REG(d1, ULONG userdata), REG(a0, CONST_STRPTR text),
                                             REG(a1, struct MsgPort *port), REG(a2, struct TagItem *tags))
{
    struct wbc_tag *t = core_tags(tags);
    struct wbc_obj *o = NULL;
    uint32_t key = 0;
    int given = 0;
    ULONG *where;
    if (!t)
        return NULL;
    lock();
    wbc_add_appmenu(&Base->st, id, userdata, (const char *)text, port, t, &o, &key, &given);
    unlock();
    FreeVec(t);
    if (o && given) {
        /* the key goes where the program asked for it */
        where = (ULONG *)GetTagData(WBAPPMENUA_GetTitleKey, 0, tags);
        if (!where || ((struct wbc_obj *)o)->u.mi.role != WBC_MENU_TITLE)
            where = (ULONG *)GetTagData(WBAPPMENUA_GetKey, 0, tags);
        if (where)
            *where = key;
    }
    return (struct AppMenuItem *)o;
}

static BOOL L_RemoveAppMenuItem(REG(a0, struct AppMenuItem *mi))
{
    lock();
    wbc_remove_appmenu(&Base->st, mi);
    unlock();
    /* OS 3.2 answers TRUE for any item that isn't NULL, even one already
     * removed (WorkbenchABITest RISKY, lab wblib-a), so this one does too. */
    return mi != NULL;
}

static struct AppWindowDropZone *L_AddAppWindowDropZoneA(REG(a0, struct AppWindow *aw), REG(d0, ULONG id),
                                                         REG(d1, ULONG userdata), REG(a1, struct TagItem *tags))
{
    struct wbc_tag *t = core_tags(tags);
    struct wbc_obj *o = NULL;
    int r;
    if (!t)
        return NULL;
    lock();
    r = wbc_add_dropzone(&Base->st, aw, id, userdata, t, &o);
    unlock();
    FreeVec(t);
    if (r != WBC_OK)
        SetIoErr(io_error(r));
    return (struct AppWindowDropZone *)o;
}

static BOOL L_RemoveAppWindowDropZone(REG(a0, struct AppWindow *aw), REG(a1, struct AppWindowDropZone *dz))
{
    int r;
    lock();
    r = wbc_remove_dropzone(&Base->st, aw, dz);
    unlock();
    if (r != WBC_OK)
        SetIoErr(io_error(r));          /* "The reason for the failure can be obtained using dos.library/IoErr" */
    return r == WBC_OK;
}

/* ---- WorkbenchControlA: the settings; the rest needs a desktop ---- */

/* The hidden devices as a list programs walk: a struct List of Nodes, each
 * with its name; freed with WBCTRLA_FreeHiddenDeviceList. */
static struct List *hidden_list(void)
{
    struct List *l = AllocVec(sizeof *l, MEMF_ANY | MEMF_CLEAR);
    struct wbc_name *n;
    if (!l)
        return NULL;
    l->lh_Head = (struct Node *)&l->lh_Tail;    /* an empty list */
    l->lh_Tail = NULL;
    l->lh_TailPred = (struct Node *)&l->lh_Head;
    for (n = Base->st.set.hidden; n; n = n->next) {
        ULONG len = 0;
        struct Node *nd;
        while (n->name[len]) len++;
        if (!(nd = AllocVec(sizeof *nd + len + 1, MEMF_ANY | MEMF_CLEAR)))
            break;
        nd->ln_Name = (char *)(nd + 1);
        CopyMem(n->name, nd->ln_Name, len + 1);
        AddTail(l, nd);
    }
    return l;
}

static void free_list(struct List *l)
{
    struct Node *n;
    if (!l)
        return;
    while ((n = RemHead(l)))
        FreeVec(n);
    FreeVec(l);
}

static BOOL L_WorkbenchControlA(REG(a0, CONST_STRPTR name), REG(a1, struct TagItem *tags))
{
    struct TagItem *walk = tags, *t;
    struct wbc_settings *s = &Base->st.set;
    BOOL ok = TRUE;
    (void)name;
    lock();
    while ((t = NextTagItem(&walk))) {
        ULONG d = t->ti_Data;
        switch (t->ti_Tag) {
        case WBCTRLA_GetDefaultStackSize: if (d) *(ULONG *)d = s->default_stack; break;
        case WBCTRLA_SetDefaultStackSize: s->default_stack = d; wbc_note(&Base->st, WBC_NOTE_SETTINGS); break;
        case WBCTRLA_GetTypeRestartTime: if (d) *(ULONG *)d = s->type_restart; break;
        case WBCTRLA_SetTypeRestartTime: s->type_restart = d; wbc_note(&Base->st, WBC_NOTE_SETTINGS); break;
        case WBCTRLA_GetGlobalFlags: if (d) *(ULONG *)d = s->global_flags; break;
        case WBCTRLA_SetGlobalFlags: s->global_flags = d; wbc_note(&Base->st, WBC_NOTE_SETTINGS); break;
        case WBCTRLA_GetCopyHook: if (d) *(APTR *)d = s->copy_hook; break;
        case WBCTRLA_SetCopyHook: s->copy_hook = (APTR)d; break;
        case WBCTRLA_GetDeleteHook: if (d) *(APTR *)d = s->delete_hook; break;
        case WBCTRLA_SetDeleteHook: s->delete_hook = (APTR)d; break;
        case WBCTRLA_GetTextInputHook: if (d) *(APTR *)d = s->text_input_hook; break;
        case WBCTRLA_SetTextInputHook: s->text_input_hook = (APTR)d; break;
        case WBCTRLA_GetDiskInfoHook: if (d) *(APTR *)d = s->disk_info_hook; break;
        case WBCTRLA_SetDiskInfoHook: s->disk_info_hook = (APTR)d; break;
        case WBCTRLA_AddSetupCleanupHook: if (wbc_add_setup_cleanup(&Base->st, (void *)d) != WBC_OK) ok = FALSE; break;
        case WBCTRLA_RemSetupCleanupHook: if (wbc_rem_setup_cleanup(&Base->st, (void *)d) != WBC_OK) ok = FALSE; break;
        case WBCTRLA_AddHiddenDeviceName: if (wbc_add_hidden(&Base->st, (const char *)d) == WBC_ERR_NOMEM) ok = FALSE; break;
        /* a name that isn't there: TRUE, as 3.2's (WorkbenchABITest, lab wblib-a) */
        case WBCTRLA_RemoveHiddenDeviceName: wbc_remove_hidden(&Base->st, (const char *)d); break;
        case WBCTRLA_GetHiddenDeviceList:
            if (d && !(*(struct List **)d = hidden_list()))
                ok = FALSE;
            break;
        case WBCTRLA_FreeHiddenDeviceList: free_list((struct List *)d); break;
        case WBCTRLA_IsOpen:
            /* no desktop, so no drawer is open: TRUE, and FALSE in the LONG, as
             * 3.2's while no Workbench runs (WorkbenchABITest, lab wblib-a) */
            if (d) *(LONG *)d = FALSE;
            break;
        case WBCTRLA_DuplicateSearchPath:
        case WBCTRLA_FreeSearchPath:
        case WBCTRLA_RedrawAppIcon:
        case WBCTRLA_GetProgramList:
        case WBCTRLA_FreeProgramList:
        case WBCTRLA_GetSelectedIconList:
        case WBCTRLA_FreeSelectedIconList:
        case WBCTRLA_GetOpenDrawerList:
        case WBCTRLA_FreeOpenDrawerList:
            ok = FALSE;                 /* a question for the desktop: step L3 */
            break;
        default:
            break;                      /* a tag it doesn't know: ignored, TRUE, as 3.2's (WorkbenchABITest) */
        }
    }
    unlock();
    return ok;
}

/* ---- the calls that need a desktop, and the private ones: not yet ---- */

static void L_UpdateWorkbench(REG(a0, CONST_STRPTR name), REG(a1, BPTR lk), REG(d0, LONG action))
{
    (void)name; (void)lk; (void)action;
}

static ULONG L_Zero(void) { return 0; }

/* ---- the library ---- */

static struct Library *lib_init(REG(d0, struct OWBBase *b), REG(a0, BPTR seg), REG(a6, struct ExecBase *sys))
{
    SysBase = sys;
    b->seglist = seg;
    b->lib.lib_Node.ln_Type = NT_LIBRARY;
    b->lib.lib_Node.ln_Name = (char *)lib_name;
    b->lib.lib_Flags = LIBF_SUMUSED | LIBF_CHANGED;
    b->lib.lib_Version = VERSION;
    b->lib.lib_Revision = REVISION;
    b->lib.lib_IdString = (APTR)lib_id;
    /* OS 3.2 (V47) and a 68020 or better, as the design's section 6 says */
    if (sys->LibNode.lib_Version < 47 || !(sys->AttnFlags & AFF_68020))
        goto fail;
    DOSBase = (struct DosLibrary *)OpenLibrary((CONST_STRPTR)"dos.library", 47);
    UtilityBase = OpenLibrary((CONST_STRPTR)"utility.library", 47);
    if (!DOSBase || !UtilityBase)
        goto fail;
    InitSemaphore(&b->lock);
    b->env.alloc = env_alloc;
    b->env.free = env_free;
    wbc_init(&b->st, &b->env);
    Base = b;
    return &b->lib;
fail:
    if (DOSBase)
        CloseLibrary((struct Library *)DOSBase);
    if (UtilityBase)
        CloseLibrary(UtilityBase);
    FreeMem((UBYTE *)b - b->lib.lib_NegSize, b->lib.lib_NegSize + b->lib.lib_PosSize);
    return NULL;
}

static struct Library *lib_open(REG(a6, struct OWBBase *b))
{
    b->lib.lib_OpenCnt++;
    b->lib.lib_Flags &= ~LIBF_DELEXP;
    return &b->lib;
}

static BPTR lib_expunge(REG(a6, struct OWBBase *b))
{
    BPTR seg;
    /* never while a program holds one of the library's objects */
    if (b->lib.lib_OpenCnt || !wbc_empty(&b->st)) {
        b->lib.lib_Flags |= LIBF_DELEXP;
        return 0;
    }
    seg = b->seglist;
    Remove(&b->lib.lib_Node);
    wbc_free_all(&b->st);
    CloseLibrary(UtilityBase);
    CloseLibrary((struct Library *)DOSBase);
    FreeMem((UBYTE *)b - b->lib.lib_NegSize, b->lib.lib_NegSize + b->lib.lib_PosSize);
    return seg;
}

static BPTR lib_close(REG(a6, struct OWBBase *b))
{
    if (b->lib.lib_OpenCnt)
        b->lib.lib_OpenCnt--;
    if (!b->lib.lib_OpenCnt && (b->lib.lib_Flags & LIBF_DELEXP))
        return lib_expunge(b);
    return 0;
}

static ULONG lib_null(void) { return 0; }

/* workbench.library's table (wb_lib.sfd 47.1, bias 30), as wblib.c has it */
static const APTR functable[] = {
    (APTR)lib_open, (APTR)lib_close, (APTR)lib_expunge, (APTR)lib_null,
    (APTR)L_UpdateWorkbench,        /*  -30 (needs the desktop: L3) */
    (APTR)L_Zero,                   /*  -36 QuoteWorkbench (private: L2 records it) */
    (APTR)L_Zero,                   /*  -42 StartWorkbench (private: L2 records it) */
    (APTR)L_AddAppWindowA,          /*  -48 */
    (APTR)L_RemoveAppWindow,        /*  -54 */
    (APTR)L_AddAppIconA,            /*  -60 */
    (APTR)L_RemoveAppIcon,          /*  -66 */
    (APTR)L_AddAppMenuItemA,        /*  -72 */
    (APTR)L_RemoveAppMenuItem,      /*  -78 */
    (APTR)L_Zero,                   /*  -84 WBConfig (private: L2 records it) */
    (APTR)L_Zero,                   /*  -90 WBInfo (needs the desktop) */
    (APTR)L_Zero,                   /*  -96 OpenWorkbenchObjectA (needs the desktop) */
    (APTR)L_Zero,                   /* -102 CloseWorkbenchObjectA (needs the desktop) */
    (APTR)L_WorkbenchControlA,      /* -108 the settings */
    (APTR)L_AddAppWindowDropZoneA,  /* -114 */
    (APTR)L_RemoveAppWindowDropZone, /* -120 */
    (APTR)L_Zero,                   /* -126 ChangeWorkbenchSelectionA (needs the desktop) */
    (APTR)L_Zero,                   /* -132 MakeWorkbenchObjectVisibleA (needs the desktop) */
    (APTR)L_Zero,                   /* -138 WhichWorkbenchObjectA (needs the desktop) */
    (APTR)-1
};
typedef char check_count[sizeof functable / sizeof functable[0] == 4 + 19 + 1 ? 1 : -1];

static const APTR inittable[4] = { (APTR)sizeof(struct OWBBase), (APTR)functable, NULL, (APTR)lib_init };

const struct Resident romtag __attribute__((used)) = {
    RTC_MATCHWORD, (struct Resident *)&romtag, (APTR)(&romtag + 1), RTF_AUTOINIT, VERSION, NT_LIBRARY, 0,
    (char *)lib_name, (char *)lib_id, (APTR)inittable
};
