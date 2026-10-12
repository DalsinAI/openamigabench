/* openicon.library: our icon.library's own calls, at icon.library's LVOs,
 * under a test name (step R2, amigachrome
 * docs/design/Design-Workbench-Replacement.md, sections 3 and 8). It is
 * opened beside Hyperion's icon.library by tools/iconprobe, which makes
 * every call through both and lists each difference. Nothing opens it by
 * the name icon.library, and it forwards nothing.
 *
 * The file work is icon_core.c's (portable C). Here: the library, DOS,
 * the DiskObjects programs see, the FreeList calls and the tags.
 *
 * Done here (R2): the tool type calls, the FreeList calls, reading,
 * writing, deleting and freeing every format, default icons,
 * GetIconTagList, PutIconTagList, DupDiskObjectA, and IconControlA's
 * global settings and its questions about an icon's pictures.
 * Step R3: drawing (DrawIconStateA), GetIconRectangleA, LayoutIconA,
 * ChangeToSelectedIconColor, IconControlA's setters, the identify hook
 * being called, masks. Until then those fail as an empty call would.
 *
 * Every DiskObject we hand out is one of ours (a struct OIObject), listed
 * under the library's semaphore, so a call is never made on memory we
 * didn't give: a DiskObject a program built itself is only read.
 *
 * MIT, Copyright (c) 2026 Dalsin Limited. */
#include <stddef.h>
#include <exec/types.h>
#include <exec/resident.h>
#include <exec/libraries.h>
#include <exec/memory.h>
#include <exec/semaphores.h>
#include <exec/lists.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <intuition/intuition.h>
#include <graphics/view.h>
#include <workbench/workbench.h>
#include <workbench/icon.h>
#include <utility/tagitem.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/wb.h>

#include "icon_core.h"

#define REG(r, decl) register decl __asm(#r)

#ifndef LIBNAME
#define LIBNAME "openicon.library"
#endif
#define VERSION 47
#define REVISION 5

/* First in the file: run as a program, the library returns at once. */
__asm("\t.text\n\t.globl _oi_start\n_oi_start:\n\tmoveq #-1,d0\n\trts\n");

static const char lib_name[] = LIBNAME;
static const char lib_id[] = "icon.library 47.5 (12.10.2026) OpenBench R2 (our own, for the side-by-side test)\r\n";
static const char lib_ver[] __attribute__((used)) =
    "$VER: openicon.library 47.5 (12.10.2026) OpenBench R2, MIT, Copyright (c) 2026 Dalsin Limited";

/* The layouts the file format and the programs share. */
typedef char check_diskobject[sizeof(struct DiskObject) == 78 ? 1 : -1];
typedef char check_newwindow[sizeof(struct NewWindow) == 48 ? 1 : -1];
typedef char check_image[sizeof(struct Image) == 20 ? 1 : -1];

struct ExecBase *SysBase;
struct DosLibrary *DOSBase;
struct Library *UtilityBase;

struct OIBase {
    struct Library lib;
    UWORD pad;
    BPTR seglist;
    struct SignalSemaphore lock;    /* the object list and the global settings */
    struct MinList objects;         /* every OIObject handed out */
    /* IconControlA's global settings */
    struct Screen *screen;
    LONG precision;
    struct Rectangle emboss;
    LONG frameless;
    LONG newicons;
    LONG coloricons;
    struct Hook *identify;
    LONG maxnamelen;
};

static struct OIBase *Base;

/* One DiskObject we handed out. The DiskObject is first, so its address is
 * the object's. */
#define OI_MAGIC 0x4F49434FUL       /* "OICO" */
struct OIObject {
    struct DiskObject dobj;
    UWORD pad;
    struct MinNode node;
    ULONG magic;
    struct ic_icon *icon;           /* what was read; owns strings and pictures */
    struct Image image[2];          /* the classic pictures, as handed out */
    APTR given[2];                  /* GadgetRender and SelectRender as handed out */
    UBYTE *chip[2];                 /* their planes, in Chip RAM */
    ULONG chipsize[2];
    struct DrawerData drawer;
    char *label;                    /* ICONGETA_Label's copy, if any */
    struct ic_palpic selected;      /* no second picture: the first, its palette darkened */
};

/* ---- memory for the core ---- */

static void *env_alloc(struct ic_env *e, uint32_t size)
{
    (void)e;
    return AllocVec(size, MEMF_ANY | MEMF_CLEAR);
}

static void env_free(struct ic_env *e, void *p)
{
    (void)e;
    FreeVec(p);
}

static struct ic_env env = { env_alloc, env_free };

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

static ULONG slen(const char *s)
{
    ULONG n = 0;
    while (s && s[n]) n++;
    return n;
}

/* ---- the object list ---- */

static int is_ours(struct DiskObject *d)
{
    struct MinNode *n;
    int found = 0;
    if (!d)
        return 0;
    ObtainSemaphoreShared(&Base->lock);
    for (n = Base->objects.mlh_Head; n->mln_Succ; n = n->mln_Succ)
        if ((struct DiskObject *)((UBYTE *)n - __builtin_offsetof(struct OIObject, node)) == d) {
            found = 1;
            break;
        }
    ReleaseSemaphore(&Base->lock);
    return found;
}

static void free_object(struct OIObject *o)
{
    int k;
    for (k = 0; k < 2; k++)
        if (o->chip[k])
            FreeMem(o->chip[k], o->chipsize[k]);
    if (o->label)
        FreeVec(o->label);
    ic_free(&env, o->icon);
    FreeVec(o);
}

/* ---- PNG and ARGB icons get a classic picture too ---- */

/* The 4 Workbench colours of 3.x: grey, black, white, blue. */
static const UBYTE wb_colours[4][3] = { { 0x95, 0x95, 0x95 }, { 0, 0, 0 }, { 0xFF, 0xFF, 0xFF }, { 0x3B, 0x67, 0xA2 } };

/* A 2-plane picture from ARGB, each pixel the nearest Workbench colour,
 * pen 0 where it is more than half see-through. */
static struct ic_planar *planar_from_argb(struct ic_icon *icon, const struct ic_argb *a)
{
    ULONG row = ((ULONG)(a->width + 15) >> 4) << 1, x, y;
    struct ic_planar *p = ic_own(&env, icon, sizeof *p + row * a->height * 2);
    if (!p)
        return NULL;
    p->width = a->width;
    p->height = a->height;
    p->depth = 2;
    p->planepick = 3;
    p->data = (uint8_t *)(p + 1);
    p->size = row * a->height * 2;
    for (y = 0; y < a->height; y++)
        for (x = 0; x < a->width; x++) {
            const UBYTE *px = a->argb + (y * a->width + x) * 4;
            ULONG best = 0, bestd = 0xFFFFFFFFUL, c;
            if (px[0] >= 128)
                for (c = 0; c < 4; c++) {
                    LONG dr = (LONG)px[1] - wb_colours[c][0];
                    LONG dg = (LONG)px[2] - wb_colours[c][1];
                    LONG db = (LONG)px[3] - wb_colours[c][2];
                    ULONG d = (ULONG)(dr * dr * 3 + dg * dg * 4 + db * db * 2);
                    if (d < bestd) {
                        bestd = d;
                        best = c;
                    }
                }
            if (best & 1)
                p->data[y * row + (x >> 3)] |= (UBYTE)(0x80 >> (x & 7));
            if (best & 2)
                p->data[row * a->height + y * row + (x >> 3)] |= (UBYTE)(0x80 >> (x & 7));
        }
    return p;
}

/* ---- a DiskObject from an icon ---- */

static struct OIObject *make_object(struct ic_icon *icon)
{
    struct OIObject *o = AllocVec(sizeof *o, MEMF_PUBLIC | MEMF_CLEAR);
    struct DiskObject *d;
    int k;
    if (!o)
        return NULL;
    o->magic = OI_MAGIC;
    o->icon = icon;
    d = &o->dobj;
    if (!icon->planar[0] && icon->argb[0]) {
        for (k = 0; k < 2; k++)
            if (icon->argb[k])
                icon->planar[k] = planar_from_argb(icon, icon->argb[k]);
        if (icon->gwidth == 0) {
            icon->gwidth = icon->argb[0]->width;
            icon->gheight = icon->argb[0]->height;
        }
    }
    /* a colour icon with no second picture: 3.2 answers for the second
     * with the first, its palette darkened as ChangeToSelectedIconColor
     * does (iconprobe at d801676 and 0000c5c) */
    if (icon->pal[0] && !icon->pal[1] && icon->pal[0]->ncolours) {
        struct ic_palpic *p = icon->pal[0];
        UBYTE *dark = ic_own(&env, icon, (ULONG)p->ncolours * 3);
        ULONG i;
        if (!dark) {
            FreeVec(o);
            return NULL;
        }
        for (i = 0; i < (ULONG)p->ncolours * 3; i++)
            dark[i] = (UBYTE)(p->palette[i] > 40 ? p->palette[i] - 40 : 0);
        o->selected = *p;
        o->selected.palette = dark;
    }
    d->do_Magic = WB_DISKMAGIC;
    d->do_Version = WB_DISKVERSION;
    d->do_Gadget.LeftEdge = icon->gleft;
    d->do_Gadget.TopEdge = icon->gtop;
    d->do_Gadget.Width = icon->gwidth;
    d->do_Gadget.Height = icon->gheight;
    d->do_Gadget.Flags = icon->gflags;
    d->do_Gadget.Activation = icon->gactivation;
    d->do_Gadget.GadgetType = icon->gtype;
    d->do_Gadget.UserData = (APTR)icon->guserdata;
    for (k = 0; k < 2; k++) {
        struct ic_planar *p = icon->planar[k];
        struct Image *im = &o->image[k];
        if (!p)
            continue;
        im->LeftEdge = p->left;
        im->TopEdge = p->top;
        im->Width = p->width;
        im->Height = p->height;
        im->Depth = p->depth;
        im->PlanePick = p->planepick;
        im->PlaneOnOff = p->planeonoff;
        if (p->size && p->data) {
            o->chip[k] = AllocMem(p->size, MEMF_CHIP);
            if (!o->chip[k]) {
                o->icon = NULL;     /* the caller frees it */
                free_object(o);
                return NULL;
            }
            o->chipsize[k] = p->size;
            CopyMem(p->data, o->chip[k], p->size);
            im->ImageData = (UWORD *)o->chip[k];
        }
        o->given[k] = im;
    }
    d->do_Gadget.GadgetRender = o->given[0];
    d->do_Gadget.SelectRender = o->given[1];
    d->do_Type = icon->type;
    d->do_DefaultTool = (STRPTR)icon->defaulttool;
    d->do_ToolTypes = (STRPTR *)icon->tooltypes;
    d->do_CurrentX = icon->currentx;
    d->do_CurrentY = icon->currenty;
    d->do_ToolWindow = (STRPTR)icon->toolwindow;
    d->do_StackSize = icon->stacksize;
    if (icon->drawer) {
        CopyMem(icon->drawer->newwindow, &o->drawer.dd_NewWindow, 48);
        o->drawer.dd_NewWindow.LeftEdge = icon->drawer->left;
        o->drawer.dd_NewWindow.TopEdge = icon->drawer->top;
        o->drawer.dd_NewWindow.Width = icon->drawer->width;
        o->drawer.dd_NewWindow.Height = icon->drawer->height;
        o->drawer.dd_CurrentX = icon->drawer->currentx;
        o->drawer.dd_CurrentY = icon->drawer->currenty;
        o->drawer.dd_Flags = icon->drawer->flags;
        o->drawer.dd_ViewModes = icon->drawer->viewmodes;
        d->do_DrawerData = &o->drawer;
    }
    ObtainSemaphore(&Base->lock);
    AddTail((struct List *)&Base->objects, (struct Node *)&o->node);
    ReleaseSemaphore(&Base->lock);
    return o;
}

/* ---- an icon from a DiskObject, to write it ---- */

/* The fields as the DiskObject has them now. Pictures: ours as read, while
 * the program hasn't replaced them; otherwise its Images. Owned memory is
 * only the icon itself and the planar descriptions. */
static struct ic_icon *icon_from_object(struct DiskObject *d, ULONG drop)
{
    struct ic_icon *icon = AllocVec(sizeof *icon, MEMF_ANY | MEMF_CLEAR);
    struct OIObject *o = is_ours(d) ? (struct OIObject *)d : NULL;
    int k, same = 1;
    if (!icon)
        return NULL;
    icon->gleft = d->do_Gadget.LeftEdge;
    icon->gtop = d->do_Gadget.TopEdge;
    icon->gwidth = d->do_Gadget.Width;
    icon->gheight = d->do_Gadget.Height;
    icon->gflags = d->do_Gadget.Flags;
    icon->gactivation = d->do_Gadget.Activation;
    icon->gtype = d->do_Gadget.GadgetType;
    icon->guserdata = (ULONG)d->do_Gadget.UserData;
    icon->type = d->do_Type;
    icon->defaulttool = (char *)d->do_DefaultTool;
    icon->tooltypes = (char **)d->do_ToolTypes;
    icon->currentx = d->do_CurrentX;
    icon->currenty = d->do_CurrentY;
    icon->toolwindow = (char *)d->do_ToolWindow;
    icon->stacksize = d->do_StackSize;
    if (d->do_DrawerData) {
        struct DrawerData *dd = d->do_DrawerData;
        struct ic_drawer *x = ic_own(&env, icon, sizeof *x);
        if (!x)
            goto fail;
        CopyMem(&dd->dd_NewWindow, x->newwindow, 48);
        x->left = dd->dd_NewWindow.LeftEdge;
        x->top = dd->dd_NewWindow.TopEdge;
        x->width = dd->dd_NewWindow.Width;
        x->height = dd->dd_NewWindow.Height;
        x->currentx = dd->dd_CurrentX;
        x->currenty = dd->dd_CurrentY;
        /* NewDrawerData is only there when the revision says so */
        if (icon->guserdata & 0xFF) {
            x->flags = dd->dd_Flags;
            x->viewmodes = dd->dd_ViewModes;
        }
        icon->drawer = x;
    }
    if (o)
        for (k = 0; k < 2; k++)
            if ((APTR)(k ? d->do_Gadget.SelectRender : d->do_Gadget.GadgetRender) != o->given[k])
                same = 0;
    for (k = 0; k < 2; k++) {
        struct Image *im = (struct Image *)(k ? d->do_Gadget.SelectRender : d->do_Gadget.GadgetRender);
        struct ic_planar *p;
        if (!im || (drop & IC_FMT_CLASSIC))
            continue;
        /* a SelectRender is written whatever the highlight says, as 3.2 does */
        p = ic_own(&env, icon, sizeof *p);
        if (!p)
            goto fail;
        p->left = im->LeftEdge;
        p->top = im->TopEdge;
        p->width = (uint16_t)im->Width;
        p->height = (uint16_t)im->Height;
        p->depth = (uint16_t)im->Depth;
        p->planepick = im->PlanePick;
        p->planeonoff = im->PlaneOnOff;
        p->data = (uint8_t *)im->ImageData;
        p->size = p->data ? (((ULONG)p->width + 15) >> 4) * 2 * p->height * p->depth : 0;
        icon->planar[k] = p;
    }
    if (o && same) {
        struct ic_icon *r = o->icon;
        icon->has_face = r->has_face;
        icon->frameless = r->frameless;
        icon->aspect = r->aspect;
        if (!(drop & (IC_FMT_OS35 | IC_FMT_ARGB))) {
            icon->form = r->form;
            icon->formlen = r->formlen;
        }
        if ((r->formats & IC_FMT_PNG) && !(drop & IC_FMT_PNG)) {
            icon->formats = IC_FMT_PNG;
            icon->png[0] = r->png[0];
            icon->png[1] = r->png[1];
            icon->pnglen[0] = r->pnglen[0];
            icon->pnglen[1] = r->pnglen[1];
        }
        /* a PNG icon's classic picture is ours, made from it: not written,
         * unless the PNG is dropped (then it's all there is) */
        if ((r->formats & IC_FMT_PNG) && !(drop & IC_FMT_PNG))
            icon->planar[0] = icon->planar[1] = NULL;
    }
    /* ICONPUTA_DropNewIconToolTypes: the IM1=/IM2= lines and their marker
     * left out */
    if ((drop & IC_FMT_NEWICONS) && icon->tooltypes) {
        char **tt = icon->tooltypes, **keep;
        ULONG n = 0, k = 0;
        while (tt[n])
            n++;
        keep = ic_own(&env, icon, (n + 1) * sizeof *keep);
        if (!keep)
            goto fail;
        for (n = 0; tt[n]; n++) {
            const char *t = tt[n];
            if ((t[0] == 'I' && t[1] == 'M' && (t[2] == '1' || t[2] == '2') && t[3] == '=') ||
                (t[0] == '*' && t[1] == '*' && t[2] == '*' && t[3] == ' '))
                continue;
            keep[k++] = tt[n];
        }
        keep[k] = NULL;
        icon->tooltypes = keep;
    }
    return icon;
fail:
    ic_free(&env, icon);
    return NULL;
}

/* ---- files ---- */

/* name + ".info", in a new AllocVec'd string. */
static char *info_name(CONST_STRPTR name)
{
    ULONG n = slen((const char *)name);
    char *s = AllocVec(n + 6, MEMF_ANY);
    if (!s)
        return NULL;
    CopyMem((APTR)name, s, n);
    CopyMem(".info", s + n, 6);
    return s;
}

/* The whole file; *len its size. NULL with IoErr set when it can't be had. */
static UBYTE *read_file(const char *path, ULONG *len)
{
    BPTR fh = Open((CONST_STRPTR)path, MODE_OLDFILE);
    LONG size;
    UBYTE *buf;
    if (!fh)
        return NULL;
    Seek(fh, 0, OFFSET_END);
    size = Seek(fh, 0, OFFSET_BEGINNING);
    if (size < 0 || size > 4L * 1024 * 1024) {
        Close(fh);
        SetIoErr(ERROR_OBJECT_WRONG_TYPE);
        return NULL;
    }
    buf = AllocVec((ULONG)size + 1, MEMF_ANY);
    if (!buf) {
        Close(fh);
        SetIoErr(ERROR_NO_FREE_STORE);
        return NULL;
    }
    if (Read(fh, buf, size) != size) {
        LONG e = IoErr();
        Close(fh);
        FreeVec(buf);
        SetIoErr(e ? e : ERROR_OBJECT_WRONG_TYPE);
        return NULL;
    }
    Close(fh);
    *len = (ULONG)size;
    return buf;
}

static LONG core_error(int r)
{
    return r == IC_ERR_NOMEM ? ERROR_NO_FREE_STORE : ERROR_OBJECT_WRONG_TYPE;
}

/* An icon file into a DiskObject; NULL with *err set. */
static struct DiskObject *load(const char *path, ULONG flags, LONG *err)
{
    ULONG len = 0;
    UBYTE *buf = read_file(path, &len);
    struct ic_icon *icon = NULL;
    struct OIObject *o;
    int r;
    if (!buf) {
        *err = IoErr();
        return NULL;
    }
    if (!Base->newicons)
        flags |= IC_READ_NONEWICONS;
    r = ic_read(&env, buf, len, flags, &icon);
    FreeVec(buf);
    if (r != IC_OK) {
        *err = core_error(r);
        return NULL;
    }
    /* colour icons switched off (ICONCTRLA_SetGlobalColorIconSupport):
     * only the classic pictures are handed out; a PNG icon keeps its own,
     * having no other */
    if (!Base->coloricons && !(icon->formats & IC_FMT_PNG)) {
        icon->pal[0] = icon->pal[1] = NULL;
        icon->argb[0] = icon->argb[1] = NULL;
    }
    o = make_object(icon);
    if (!o) {
        ic_free(&env, icon);
        *err = ERROR_NO_FREE_STORE;
        return NULL;
    }
    return &o->dobj;
}

static BOOL save(const char *path, struct DiskObject *d, ULONG drop, LONG *err)
{
    struct ic_icon *icon = icon_from_object(d, drop);
    ULONG size;
    UBYTE *buf;
    BPTR fh;
    LONG wrote;
    if (!icon) {
        *err = ERROR_NO_FREE_STORE;
        return FALSE;
    }
    /* What OS 3.2's PutDiskObject writes in place of what it was given
     * (iconprobe at 5c27ff1 and 0000c5c): a BOOLGADGET with RELVERIFY
     * only, highlighted by its selected picture when it has one; a stack
     * under 4096 as 4096; an empty tool type array as none; the revision
     * byte as the disk revision (an OS 1.3 icon's new drawer fields zero).
     * An icon whose picture is empty (NewDiskObject's) is refused. */
    if ((!icon->planar[0] || !icon->planar[0]->width || !icon->planar[0]->height || !icon->planar[0]->size)
        && !icon->form && !(icon->formats & IC_FMT_PNG)) {
        ic_free(&env, icon);
        *err = ERROR_OBJECT_WRONG_TYPE;
        return FALSE;
    }
    icon->gactivation = GACT_RELVERIFY;
    icon->gtype = GTYP_BOOLGADGET;
    if (icon->planar[1])
        icon->gflags = (uint16_t)((icon->gflags & ~GFLG_GADGHIGHBITS) | GFLG_GADGIMAGE | GFLG_GADGHIMAGE);
    else
        icon->gflags |= GFLG_GADGIMAGE;
    if (icon->stacksize < 4096)
        icon->stacksize = 4096;
    if (icon->tooltypes && !icon->tooltypes[0])
        icon->tooltypes = NULL;
    if ((icon->guserdata & 0xFF) != WB_DISKREVISION)
        icon->guserdata = (icon->guserdata & ~0xFFUL) | WB_DISKREVISION;
    size = ic_write(&env, icon, NULL, 0);
    buf = AllocVec(size ? size : 1, MEMF_ANY);
    if (!buf) {
        ic_free(&env, icon);
        *err = ERROR_NO_FREE_STORE;
        return FALSE;
    }
    ic_write(&env, icon, buf, size);
    ic_free(&env, icon);
    fh = Open((CONST_STRPTR)path, MODE_NEWFILE);
    if (!fh) {
        *err = IoErr();
        FreeVec(buf);
        return FALSE;
    }
    wrote = Write(fh, buf, (LONG)size);
    if (wrote != (LONG)size)
        *err = IoErr();
    if (!Close(fh) && wrote == (LONG)size) {
        *err = IoErr();
        wrote = -1;
    }
    FreeVec(buf);
    return wrote == (LONG)size;
}

/* ---- default icons ---- */

/* ENV:Sys/def_<name>.info: a type's default, or a named one (Harddisk...) */
static const char *const def_names[] = {
    NULL, "disk", "drawer", "tool", "project", "trashcan", "device", "kick", "appicon"
};

/* "ENV:Sys/def_" name ".info", in a new AllocVec'd string. */
static char *def_path(const char *n)
{
    ULONG k = slen(n);
    char *path = AllocVec(k + 18, MEMF_ANY);
    if (path) {
        CopyMem("ENV:Sys/def_", path, 12);
        CopyMem((APTR)n, path + 12, k);
        CopyMem(".info", path + 12 + k, 6);
    }
    return path;
}

/* A built-in picture: a framed box in the 4 Workbench colours, its kind
 * drawn inside (a drawer's handle, a tool's bar, a page's lines). Our own
 * drawing, at the size 3.2's built-in ones have (35x17, iconprobe at
 * d801676). */
static struct ic_planar *builtin_picture(struct ic_icon *icon, LONG type, int selected)
{
    enum { W = 35, H = 17, ROW = 6 };
    struct ic_planar *p = ic_own(&env, icon, sizeof *p + ROW * H * 2);
    int x, y;
    if (!p)
        return NULL;
    p->width = W;
    p->height = H;
    p->depth = 2;
    p->planepick = 3;
    p->data = (uint8_t *)(p + 1);
    p->size = ROW * H * 2;
    for (y = 0; y < H; y++)
        for (x = 0; x < W; x++) {
            int pen = 0;
            int edge = x == 2 || x == W - 3 || y == 1 || y == H - 2;
            int inside = x > 2 && x < W - 3 && y > 1 && y < H - 2;
            if (edge)
                pen = 1;
            else if (inside) {
                pen = selected ? 3 : 2;
                if (type == WBDRAWER && y == 5 && x > 13 && x < 22)
                    pen = 1;
                if (type == WBTOOL && y > 3 && y < 7)
                    pen = 3;
                if (type == WBPROJECT && (y % 4) == 0 && x > 5 && x < W - 6)
                    pen = 1;
                if ((type == WBDISK || type == WBKICK) && x > 10 && x < 24 && y < 7)
                    pen = 0;
                if (type == WBGARBAGE && (x % 5) == 0)
                    pen = 1;
            }
            if (pen & 1)
                p->data[y * ROW + (x >> 3)] |= (UBYTE)(0x80 >> (x & 7));
            if (pen & 2)
                p->data[ROW * H + y * ROW + (x >> 3)] |= (UBYTE)(0x80 >> (x & 7));
        }
    return p;
}

/* The highlight 3.2 gives each type's new and built-in icons (iconprobe
 * at 5c27ff1 and d801676): a selected picture for a drawer and the
 * trashcan, complement for a tool, a box for the rest. */
static const UBYTE new_highlight[] = {
    0, GFLG_GADGHBOX, GFLG_GADGHIMAGE, GFLG_GADGHCOMP, GFLG_GADGHBOX, GFLG_GADGHIMAGE, GFLG_GADGHBOX, GFLG_GADGHBOX
};

static struct DiskObject *builtin(LONG type)
{
    struct ic_icon *icon = AllocVec(sizeof *icon, MEMF_ANY | MEMF_CLEAR);
    struct OIObject *o;
    if (!icon)
        return NULL;
    icon->formats = IC_FMT_CLASSIC;
    icon->type = (uint8_t)type;
    icon->currentx = icon->currenty = (int32_t)NO_ICON_POSITION;
    icon->stacksize = (type == WBTOOL || type == WBPROJECT) ? 4096 : 0;
    icon->guserdata = WB_DISKREVISION;
    icon->planar[0] = builtin_picture(icon, type, 0);
    if (new_highlight[type] == GFLG_GADGHIMAGE)
        icon->planar[1] = builtin_picture(icon, type, 1);
    if (!icon->planar[0] || (new_highlight[type] == GFLG_GADGHIMAGE && !icon->planar[1])) {
        ic_free(&env, icon);
        return NULL;
    }
    icon->gwidth = icon->planar[0]->width;
    icon->gheight = (uint16_t)(icon->planar[0]->height + 1);
    icon->gflags = GFLG_GADGIMAGE | new_highlight[type];
    icon->gactivation = GACT_RELVERIFY | GACT_IMMEDIATE;
    icon->gtype = GTYP_BOOLGADGET;
    if (type == WBDISK || type == WBDRAWER || type == WBGARBAGE) {
        struct ic_drawer *d = ic_own(&env, icon, sizeof *d);
        if (!d) {
            ic_free(&env, icon);
            return NULL;
        }
        d->left = 50;
        d->top = 50;
        d->width = 400;
        d->height = 100;
        icon->drawer = d;
    }
    o = make_object(icon);
    if (!o) {
        ic_free(&env, icon);
        return NULL;
    }
    return &o->dobj;
}

/* ENV:Sys/def_<name>.info, else the built-in one. */
static struct DiskObject *default_icon(LONG type, CONST_STRPTR name, LONG *err)
{
    struct DiskObject *d = NULL;
    char *path;
    /* a named default first (ICONGETA_GetDefaultName); missing, the type's */
    if (name) {
        path = def_path((const char *)name);
        if (!path) {
            *err = ERROR_NO_FREE_STORE;
            return NULL;
        }
        d = load(path, 0, err);
        FreeVec(path);
        if (d || !type)
            goto done;
    }
    if (type < WBDISK || type > WBAPPICON) {
        *err = ERROR_REQUIRED_ARG_MISSING;
        return NULL;
    }
    if (type == WBAPPICON) {            /* 3.2 has no default AppIcon */
        *err = ERROR_OBJECT_NOT_FOUND;
        return NULL;
    }
    path = def_path(def_names[type]);
    if (!path) {
        *err = ERROR_NO_FREE_STORE;
        return NULL;
    }
    d = load(path, 0, err);
    FreeVec(path);
    if (!d) {
        d = builtin(type);
        if (!d)
            *err = ERROR_NO_FREE_STORE;
        return d;
    }
done:
    if (d)
        d->do_CurrentX = d->do_CurrentY = (LONG)NO_ICON_POSITION;   /* as 3.2's */
    return d;
}

/* What kind of object name is: a disk, a drawer, a tool or a project. */
static LONG kind_of(CONST_STRPTR name, LONG *err)
{
    BPTR lock = Lock(name, SHARED_LOCK);
    struct FileInfoBlock *fib;
    LONG type = 0;
    if (!lock) {
        *err = IoErr();
        return 0;
    }
    fib = AllocDosObject(DOS_FIB, NULL);
    if (fib && Examine(lock, fib)) {
        if (fib->fib_DirEntryType > 0) {
            BPTR parent = ParentDir(lock);
            if (parent) {
                UnLock(parent);
                type = WBDRAWER;
            } else {
                type = WBDISK;
            }
        } else {
            BPTR fh = Open(name, MODE_OLDFILE);
            ULONG first = 0;
            if (fh) {
                if (Read(fh, &first, 4) != 4)
                    first = 0;
                Close(fh);
            }
            type = first == 0x000003F3UL ? WBTOOL : WBPROJECT;
        }
    } else {
        *err = IoErr();
    }
    if (fib)
        FreeDosObject(DOS_FIB, fib);
    UnLock(lock);
    return type;
}

/* ---- the calls ---- */

/* GetIconTagList: the heart of reading. */
static struct DiskObject *get_icon(CONST_STRPTR name, struct TagItem *tags)
{
    LONG err = 0, deftype, *errp, *isdef, was_default = FALSE;
    STRPTR defname;
    BOOL fail_if, palette;
    struct DiskObject *d = NULL;
    char *path;
    ULONG flags = 0;

    deftype = (LONG)GetTagData(ICONGETA_GetDefaultType, 0, tags);
    defname = (STRPTR)GetTagData(ICONGETA_GetDefaultName, 0, tags);
    fail_if = (BOOL)GetTagData(ICONGETA_FailIfUnavailable, TRUE, tags);
    palette = (BOOL)GetTagData(ICONGETA_GetPaletteMappedIcon, TRUE, tags);
    errp = (LONG *)GetTagData(ICONA_ErrorCode, 0, tags);
    isdef = (LONG *)GetTagData(ICONGETA_IsDefaultIcon, 0, tags);
    if (!palette)
        flags |= IC_READ_NOPICTURES;
    /* *isdef is only written when an icon comes back (3.2 leaves it) */
    if (deftype || defname) {
        d = default_icon(deftype, defname, &err);
        if (d)
            was_default = TRUE;
    } else if (name) {
        path = info_name(name);
        if (!path) {
            err = ERROR_NO_FREE_STORE;
        } else {
            d = load(path, flags, &err);
            FreeVec(path);
        }
        if (!d && !fail_if && err == ERROR_OBJECT_NOT_FOUND) {
            LONG kerr = 0, type = kind_of(name, &kerr);
            if (type) {
                d = default_icon(type, NULL, &err);
                if (d)
                    was_default = TRUE;
            } else {
                err = kerr;
            }
        }
    } else {
        err = ERROR_REQUIRED_ARG_MISSING;
    }
    if (d && isdef)
        *isdef = was_default;
    if (d) {
        STRPTR label = (STRPTR)GetTagData(ICONGETA_Label, 0, tags);
        if (label) {
            struct OIObject *o = (struct OIObject *)d;
            ULONG n = slen((const char *)label) + 1;
            o->label = AllocVec(n, MEMF_ANY);
            if (o->label)
                CopyMem(label, o->label, n);
        }
        err = 0;
    }
    if (errp)
        *errp = err;
    SetIoErr(err);
    return d;
}

/* Tell a running Workbench that name's icon (at path) was written:
 * UpdateWorkbench, as PutDiskObject does. Nothing when there's no
 * workbench.library 44. */
static void notify_workbench(const char *path, CONST_STRPTR name)
{
    struct Library *WorkbenchBase = OpenLibrary((CONST_STRPTR)"workbench.library", 44);
    BPTR lock, parent;
    if (!WorkbenchBase)
        return;
    lock = Lock((CONST_STRPTR)path, SHARED_LOCK);
    if (lock) {
        parent = ParentDir(lock);
        UnLock(lock);
        if (parent) {
            UpdateWorkbench(FilePart(name), parent, UPDATEWB_ObjectAdded);
            UnLock(parent);
        }
    }
    CloseLibrary(WorkbenchBase);
}

static BOOL put_icon(CONST_STRPTR name, struct DiskObject *d, struct TagItem *tags)
{
    LONG err = 0, *errp = (LONG *)GetTagData(ICONA_ErrorCode, 0, tags);
    LONG deftype = (LONG)GetTagData(ICONPUTA_PutDefaultType, 0, tags);
    STRPTR defname = (STRPTR)GetTagData(ICONPUTA_PutDefaultName, 0, tags);
    ULONG drop = 0;
    BOOL ok = FALSE;
    char *path = NULL;

    if (GetTagData(ICONPUTA_DropPlanarIconImage, FALSE, tags))
        drop |= IC_FMT_CLASSIC;
    if (GetTagData(ICONPUTA_DropChunkyIconImage, FALSE, tags))
        drop |= IC_FMT_OS35 | IC_FMT_ARGB | IC_FMT_PNG;
    if (GetTagData(ICONPUTA_DropNewIconToolTypes, FALSE, tags))
        drop |= IC_FMT_NEWICONS;
    if (!d) {
        err = ERROR_REQUIRED_ARG_MISSING;
    } else if (deftype || defname) {
        const char *n = defname ? (const char *)defname
                        : (deftype >= WBDISK && deftype <= WBAPPICON ? def_names[deftype] : NULL);
        if (n)
            path = def_path(n);
        else {
            err = ERROR_REQUIRED_ARG_MISSING;
        }
    } else if (name) {
        /* a name too long for its .info: success, nothing written, and
         * ERROR_TOO_MANY_LEVELS, so no file is overwritten by a cut name */
        if (slen((const char *)FilePart(name)) > (ULONG)Base->maxnamelen) {
            if (errp)
                *errp = ERROR_TOO_MANY_LEVELS;
            return TRUE;
        }
        path = info_name(name);
    } else {
        err = ERROR_REQUIRED_ARG_MISSING;
    }
    if (path) {
        if (GetTagData(ICONPUTA_OnlyUpdatePosition, FALSE, tags)) {
            /* the file as it is, with only the place changed */
            LONG e2 = 0;
            struct DiskObject *old = load(path, 0, &e2);
            if (old) {
                old->do_CurrentX = d->do_CurrentX;
                old->do_CurrentY = d->do_CurrentY;
                if (old->do_DrawerData && d->do_DrawerData) {
                    old->do_DrawerData->dd_CurrentX = d->do_DrawerData->dd_CurrentX;
                    old->do_DrawerData->dd_CurrentY = d->do_DrawerData->dd_CurrentY;
                }
                ok = save(path, old, 0, &err);
                ObtainSemaphore(&Base->lock);
                Remove((struct Node *)&((struct OIObject *)old)->node);
                ReleaseSemaphore(&Base->lock);
                free_object((struct OIObject *)old);
            } else if (e2 == ERROR_OBJECT_NOT_FOUND) {
                ok = save(path, d, drop, &err);     /* no icon yet: the whole one */
            } else {
                err = e2;                           /* never replace one we can't read */
            }
        } else {
            ok = save(path, d, drop, &err);
        }
        if (ok && name && !(deftype || defname) && GetTagData(ICONPUTA_NotifyWorkbench, FALSE, tags))
            notify_workbench(path, name);
        FreeVec(path);
    } else if (!err) {
        err = ERROR_NO_FREE_STORE;
    }
    if (errp)
        *errp = ok ? 0 : err;
    SetIoErr(ok ? 0 : err);
    return ok;
}

static void L_FreeDiskObject(REG(a0, struct DiskObject *d))
{
    struct OIObject *o = (struct OIObject *)d;
    if (!is_ours(d))
        return;
    ObtainSemaphore(&Base->lock);
    Remove((struct Node *)&o->node);
    ReleaseSemaphore(&Base->lock);
    free_object(o);
}

static struct DiskObject *L_GetDiskObject(REG(a0, CONST_STRPTR name))
{
    return get_icon(name, NULL);
}

static struct DiskObject *L_GetDiskObjectNew(REG(a0, CONST_STRPTR name))
{
    struct TagItem t[2];
    if (!name)                      /* "allocated for you", for AddAppIcon: a project's default */
        return default_icon(WBPROJECT, NULL, &(LONG){ 0 });
    t[0].ti_Tag = ICONGETA_FailIfUnavailable;
    t[0].ti_Data = FALSE;
    t[1].ti_Tag = TAG_DONE;
    return get_icon(name, t);
}

static BOOL L_PutDiskObject(REG(a0, CONST_STRPTR name), REG(a1, struct DiskObject *d))
{
    struct TagItem t[2];
    t[0].ti_Tag = ICONPUTA_NotifyWorkbench;     /* PutDiskObject tells Workbench */
    t[0].ti_Data = TRUE;
    t[1].ti_Tag = TAG_DONE;
    return put_icon(name, d, t);
}

static BOOL L_DeleteDiskObject(REG(a0, CONST_STRPTR name))
{
    char *path = info_name(name);
    BOOL ok;
    if (!path) {
        SetIoErr(ERROR_NO_FREE_STORE);
        return FALSE;
    }
    ok = DeleteFile((CONST_STRPTR)path);
    FreeVec(path);
    return ok;
}

static struct DiskObject *L_GetDefDiskObject(REG(d0, LONG type))
{
    struct TagItem t[2];
    t[0].ti_Tag = ICONGETA_GetDefaultType;
    t[0].ti_Data = (ULONG)type;
    t[1].ti_Tag = TAG_DONE;
    return get_icon(NULL, t);
}

static BOOL L_PutDefDiskObject(REG(a0, struct DiskObject *d))
{
    struct TagItem t[2];
    if (!d)
        return FALSE;
    t[0].ti_Tag = ICONPUTA_PutDefaultType;
    t[0].ti_Data = d->do_Type;
    t[1].ti_Tag = TAG_DONE;
    return put_icon(NULL, d, t);
}

/* NewDiskObject, as OS 3.2's (iconprobe at 5c27ff1 and d801676): a
 * BOOLGADGET with RELVERIFY, an empty Image (no size, no planes) for the
 * picture and, for a drawer and the trashcan, the selected one, a drawer
 * window at 50,50 400x100 for a disk, a drawer and the trashcan; no
 * AppIcon. */
static struct DiskObject *L_NewDiskObject(REG(d0, LONG type))
{
    struct ic_icon *icon;
    struct OIObject *o;
    if (type < WBDISK || type > WBKICK)
        return NULL;
    icon = AllocVec(sizeof *icon, MEMF_ANY | MEMF_CLEAR);
    if (!icon)
        return NULL;
    icon->formats = IC_FMT_CLASSIC;
    icon->type = (uint8_t)type;
    icon->currentx = icon->currenty = (int32_t)NO_ICON_POSITION;
    icon->guserdata = WB_DISKREVISION;
    icon->gflags = GFLG_GADGIMAGE | new_highlight[type];
    icon->gactivation = GACT_RELVERIFY;
    icon->gtype = GTYP_BOOLGADGET;
    icon->planar[0] = ic_own(&env, icon, sizeof(struct ic_planar));
    if (!icon->planar[0])
        goto fail;
    if (new_highlight[type] == GFLG_GADGHIMAGE) {
        icon->planar[1] = ic_own(&env, icon, sizeof(struct ic_planar));
        if (!icon->planar[1])
            goto fail;
    }
    if (type == WBDISK || type == WBDRAWER || type == WBGARBAGE) {
        struct ic_drawer *d = ic_own(&env, icon, sizeof *d);
        if (!d)
            goto fail;
        d->left = 50;
        d->top = 50;
        d->width = 400;
        d->height = 100;
        icon->drawer = d;
    }
    o = make_object(icon);
    if (o)
        return &o->dobj;
fail:
    ic_free(&env, icon);
    return NULL;
}

static struct DiskObject *L_GetIconTagList(REG(a0, CONST_STRPTR name), REG(a1, struct TagItem *tags))
{
    return get_icon(name, tags);
}

static BOOL L_PutIconTagList(REG(a0, CONST_STRPTR name), REG(a1, struct DiskObject *d), REG(a2, struct TagItem *tags))
{
    return put_icon(name, d, tags);
}

/* DupDiskObjectA: a copy as our own object, from the fields as they are. */
static struct DiskObject *L_DupDiskObjectA(REG(a0, struct DiskObject *d), REG(a1, struct TagItem *tags))
{
    struct ic_icon *src, *copy = NULL;
    ULONG size;
    UBYTE *buf;
    struct OIObject *o;
    LONG *errp = (LONG *)GetTagData(ICONA_ErrorCode, 0, tags);
    int r;
    ULONG drop = 0;
    if (!d) {
        if (errp)
            *errp = ERROR_REQUIRED_ARG_MISSING;
        return NULL;
    }
    if (!GetTagData(ICONDUPA_DuplicateImages, TRUE, tags))
        drop |= IC_FMT_CLASSIC | IC_FMT_OS35 | IC_FMT_ARGB | IC_FMT_PNG;
    /* through the file format: one way to copy every kind of icon */
    src = icon_from_object(d, drop);
    if (!src) {
        if (errp)
            *errp = ERROR_NO_FREE_STORE;
        return NULL;
    }
    size = ic_write(&env, src, NULL, 0);
    buf = AllocVec(size ? size : 1, MEMF_ANY);
    if (buf)
        ic_write(&env, src, buf, size);
    ic_free(&env, src);
    if (!buf) {
        if (errp)
            *errp = ERROR_NO_FREE_STORE;
        return NULL;
    }
    r = ic_read(&env, buf, size, 0, &copy);
    FreeVec(buf);
    if (r != IC_OK) {
        if (errp)
            *errp = core_error(r);
        return NULL;
    }
    o = make_object(copy);
    if (!o) {
        ic_free(&env, copy);
        if (errp)
            *errp = ERROR_NO_FREE_STORE;
        return NULL;
    }
    /* what wasn't asked for is NULL (DupDiskObjectA's tags, all TRUE
     * by default) */
    if (!GetTagData(ICONDUPA_DuplicateDrawerData, TRUE, tags))
        o->dobj.do_DrawerData = NULL;
    if (!GetTagData(ICONDUPA_DuplicateDefaultTool, TRUE, tags))
        o->dobj.do_DefaultTool = NULL;
    if (!GetTagData(ICONDUPA_DuplicateToolTypes, TRUE, tags))
        o->dobj.do_ToolTypes = NULL;
    if (!GetTagData(ICONDUPA_DuplicateToolWindow, TRUE, tags))
        o->dobj.do_ToolWindow = NULL;
    if (!GetTagData(ICONDUPA_DuplicateImageData, TRUE, tags))
        o->image[0].ImageData = o->image[1].ImageData = NULL;
    if (errp)
        *errp = 0;
    return &o->dobj;
}

/* IconControlA: the global settings, and the questions about an icon's
 * pictures. The setters for an icon's pictures are step R3. */
/* The picture IconControlA answers about for image k, and its palette. */
static struct ic_palpic *picture_for(struct ic_icon *ic, int k)
{
    return ic->pal[k] ? ic->pal[k] : ic->pal[0];
}

static struct ic_palpic *palette_for(struct OIObject *o, struct ic_icon *ic, int k)
{
    struct ic_palpic *p;
    if (k && !ic->pal[1] && o->selected.palette)
        return &o->selected;
    p = picture_for(ic, k);
    if (p && !p->ncolours)
        p = ic->pal[0];
    return p && p->ncolours ? p : NULL;
}

static ULONG L_IconControlA(REG(a0, struct DiskObject *d), REG(a1, struct TagItem *tags))
{
    struct TagItem *state = tags, *t;
    struct OIObject *o = is_ours(d) ? (struct OIObject *)d : NULL;
    struct ic_icon *ic = o ? o->icon : NULL;
    LONG *errp = (LONG *)GetTagData(ICONA_ErrorCode, 0, tags);
    struct TagItem **errtag = (struct TagItem **)GetTagData(ICONA_ErrorTagItem, 0, tags);
    ULONG done = 0;
    LONG err = 0;

    ObtainSemaphore(&Base->lock);
    while ((t = NextTagItem(&state)) != NULL) {
        LONG *lp = (LONG *)t->ti_Data;
        BOOL ok = TRUE;
        switch (t->ti_Tag) {
        case ICONA_ErrorCode:
        case ICONA_ErrorTagItem:
            continue;
        case ICONCTRLA_SetGlobalScreen: Base->screen = (struct Screen *)t->ti_Data; break;
        case ICONCTRLA_GetGlobalScreen: *(struct Screen **)t->ti_Data = Base->screen; break;
        case ICONCTRLA_SetGlobalPrecision: Base->precision = (LONG)t->ti_Data; break;
        case ICONCTRLA_GetGlobalPrecision: *lp = Base->precision; break;
        case ICONCTRLA_SetGlobalEmbossRect:
            if (t->ti_Data)
                CopyMem((APTR)t->ti_Data, &Base->emboss, sizeof Base->emboss);
            break;
        case ICONCTRLA_GetGlobalEmbossRect:
            CopyMem(&Base->emboss, (APTR)t->ti_Data, sizeof Base->emboss);
            break;
        case ICONCTRLA_SetGlobalFrameless: Base->frameless = (LONG)t->ti_Data; break;
        case ICONCTRLA_GetGlobalFrameless: *lp = Base->frameless; break;
        case ICONCTRLA_SetGlobalNewIconsSupport: Base->newicons = (LONG)t->ti_Data; break;
        case ICONCTRLA_GetGlobalNewIconsSupport: *lp = Base->newicons; break;
        case ICONCTRLA_SetGlobalColorIconSupport: Base->coloricons = (LONG)t->ti_Data; break;
        case ICONCTRLA_GetGlobalColorIconSupport: *lp = Base->coloricons; break;
        case ICONCTRLA_SetGlobalIdentifyHook: Base->identify = (struct Hook *)t->ti_Data; break;
        case ICONCTRLA_GetGlobalIdentifyHook: *(struct Hook **)t->ti_Data = Base->identify; break;
        case ICONCTRLA_SetGlobalMaxNameLength: Base->maxnamelen = (LONG)t->ti_Data; break;
        case ICONCTRLA_GetGlobalMaxNameLength: *lp = Base->maxnamelen; break;
        default:
            if (!ic) {
                ok = FALSE;
                break;
            }
            /* As OS 3.2 answers (iconprobe at 5c27ff1): with no second
             * picture the first stands in for it, a second picture with no
             * palette of its own uses the first's, and an icon with no
             * colour picture says colour 0 is transparent. */
            switch (t->ti_Tag) {
            case ICONCTRLA_GetImageData1:
            case ICONCTRLA_GetImageData2: {
                struct ic_palpic *p = picture_for(ic, t->ti_Tag == ICONCTRLA_GetImageData2);
                *(UBYTE **)t->ti_Data = p ? p->pixels : NULL;
                break;
            }
            case ICONCTRLA_GetPalette1:
            case ICONCTRLA_GetPalette2: {
                struct ic_palpic *p = palette_for(o, ic, t->ti_Tag == ICONCTRLA_GetPalette2);
                *(struct ColorRegister **)t->ti_Data = p ? (struct ColorRegister *)p->palette : NULL;
                break;
            }
            case ICONCTRLA_GetPaletteSize1:
            case ICONCTRLA_GetPaletteSize2: {
                struct ic_palpic *p = palette_for(o, ic, t->ti_Tag == ICONCTRLA_GetPaletteSize2);
                *lp = p ? p->ncolours : 0;
                break;
            }
            case ICONCTRLA_GetTransparentColor1:
            case ICONCTRLA_GetTransparentColor2: {
                struct ic_palpic *p = picture_for(ic, t->ti_Tag == ICONCTRLA_GetTransparentColor2);
                *lp = p ? p->transparent : 0;
                break;
            }
            case ICONCTRLA_GetWidth: *lp = ic->pal[0] ? ic->pal[0]->width : 0; break;
            case ICONCTRLA_GetHeight: *lp = ic->pal[0] ? ic->pal[0]->height : 0; break;
            case ICONCTRLA_GetFrameless: *lp = ic->frameless; break;
            case ICONCTRLA_GetAspectRatio: *(UBYTE *)t->ti_Data = ic->aspect; break;   /* a UBYTE * */
            case ICONCTRLA_IsPaletteMapped: *lp = ic->pal[0] != NULL; break;
            case ICONCTRLA_IsNewIcon: *lp = (ic->formats & IC_FMT_NEWICONS) != 0; break;
            case ICONCTRLA_HasRealImage2: *lp = ic->pal[1] != NULL; break;
            case ICONCTRLA_IsNativeIcon: *lp = TRUE; break;    /* one of ours (is_ours); the probe checks 3.2 */
            case ICONCTRLA_GetScreen: *(struct Screen **)t->ti_Data = NULL; break;
            default:
                ok = FALSE;         /* step R3 */
                break;
            }
        }
        if (ok) {
            done++;
        } else if (!err) {
            err = ERROR_BAD_NUMBER;
            if (errtag)
                *errtag = t;
        }
    }
    ReleaseSemaphore(&Base->lock);
    if (errp)
        *errp = err;
    return done;
}

static BOOL L_DrawIconStateA(REG(a0, struct RastPort *rp), REG(a1, struct DiskObject *d), REG(a2, STRPTR label),
                             REG(d0, LONG x), REG(d1, LONG y), REG(d2, ULONG state), REG(a3, struct TagItem *tags))
{
    return FALSE;                   /* step R3 */
}

static BOOL L_GetIconRectangleA(REG(a0, struct RastPort *rp), REG(a1, struct DiskObject *d), REG(a2, STRPTR label),
                                REG(a3, struct Rectangle *rect), REG(a4, struct TagItem *tags))
{
    return FALSE;                   /* step R3 */
}

static BOOL L_LayoutIconA(REG(a0, struct DiskObject *d), REG(a1, struct Screen *s), REG(a2, struct TagItem *tags))
{
    return FALSE;                   /* step R3 */
}

/* OS 3.2's sum (iconprobe at 0000c5c): each gun 40 darker, down to 0. */
static UBYTE selected_gun(UBYTE c)
{
    return (UBYTE)(c > 40 ? c - 40 : 0);
}

static void L_ChangeToSelectedIconColor(REG(a0, struct ColorRegister *cr))
{
    if (!cr)
        return;
    cr->red = selected_gun(cr->red);
    cr->green = selected_gun(cr->green);
    cr->blue = selected_gun(cr->blue);
}

/* ---- the tool type calls ---- */

static UBYTE *L_FindToolType(REG(a0, STRPTR *tt), REG(a1, CONST_STRPTR name))
{
    return (UBYTE *)ic_find_tool_type((char **)tt, (const char *)name);
}

static BOOL L_MatchToolValue(REG(a0, UBYTE *s), REG(a1, UBYTE *v))
{
    return (BOOL)ic_match_tool_value((const char *)s, (const char *)v);
}

static UBYTE *L_BumpRevision(REG(a0, UBYTE *newname), REG(a1, UBYTE *oldname))
{
    ic_bump_revision((char *)newname, (const char *)oldname, 30);
    return newname;
}

static UBYTE *L_BumpRevisionLength(REG(a0, UBYTE *newname), REG(a1, UBYTE *oldname), REG(d0, ULONG maxlen))
{
    /* too short: left as it was, and NULL, as 3.2 does */
    return ic_bump_revision((char *)newname, (const char *)oldname, maxlen) ? newname : NULL;
}

/* ---- the FreeList calls ---- */

#define FL_PER 10                   /* entries in each MemList we add */
#define FL_SIZE (sizeof(struct MemList) + (FL_PER - 1) * sizeof(struct MemEntry))

/* A MemList's entries, past the one its declaration has. */
static struct MemEntry *entries(struct MemList *ml)
{
    return (struct MemEntry *)((UBYTE *)ml + __builtin_offsetof(struct MemList, ml_ME));
}

static BOOL L_AddFreeList(REG(a0, struct FreeList *fl), REG(a1, APTR mem), REG(a2, ULONG size))
{
    struct MemList *ml;
    if (fl->fl_NumFree <= 0) {
        ml = AllocMem(FL_SIZE, MEMF_PUBLIC | MEMF_CLEAR);
        if (!ml)
            return FALSE;
        ml->ml_NumEntries = 0;
        AddHead(&fl->fl_MemList, &ml->ml_Node);
        fl->fl_NumFree = FL_PER;
    }
    ml = (struct MemList *)fl->fl_MemList.lh_Head;
    entries(ml)[ml->ml_NumEntries].me_Addr = mem;
    entries(ml)[ml->ml_NumEntries].me_Length = size;
    ml->ml_NumEntries++;
    fl->fl_NumFree--;
    return TRUE;
}

static void L_FreeFreeList(REG(a0, struct FreeList *fl))
{
    struct MemList *ml;
    struct List all;
    if (!fl)
        return;
    /* the FreeList may itself be one of its allocations (GetIcon's), so
     * every MemList is moved off it before anything is freed */
    all.lh_Head = (struct Node *)&all.lh_Tail;
    all.lh_Tail = NULL;
    all.lh_TailPred = (struct Node *)&all.lh_Head;
    while ((ml = (struct MemList *)RemHead(&fl->fl_MemList)) != NULL)
        AddTail(&all, (struct Node *)ml);
    while ((ml = (struct MemList *)RemHead(&all)) != NULL) {
        UWORD i;
        struct MemEntry *me = entries(ml);
        for (i = 0; i < ml->ml_NumEntries; i++)
            if (me[i].me_Addr)
                FreeMem(me[i].me_Addr, me[i].me_Length);
        FreeMem(ml, FL_SIZE);
    }
}

static APTR L_FreeAlloc(REG(a0, struct FreeList *fl), REG(a1, ULONG len), REG(a2, ULONG type))
{
    APTR mem;
    if (!len)
        return NULL;
    mem = AllocMem(len, type);
    if (mem && !L_AddFreeList(fl, mem, len)) {
        FreeMem(mem, len);
        mem = NULL;
    }
    return mem;
}

static ULONG L_FreeFree(REG(a0, struct FreeList *fl), REG(a1, APTR address))
{
    struct Node *n;
    for (n = fl->fl_MemList.lh_Head; n->ln_Succ; n = n->ln_Succ) {
        struct MemList *ml = (struct MemList *)n;
        UWORD i;
        struct MemEntry *me = entries(ml);
        for (i = 0; i < ml->ml_NumEntries; i++)
            if (me[i].me_Addr == address) {
                FreeMem(address, me[i].me_Length);
                me[i].me_Addr = NULL;
                me[i].me_Length = 0;
                return TRUE;
            }
    }
    return FALSE;
}

/* The private, pre-2.0 calls. GetIcon fills the caller's DiskObject; what
 * it allocates goes on the caller's FreeList. Step R3 (Workbench 1.x
 * programs); for now they fail as 3.2's obsolete entries do. */
static ULONG L_Obsolete(void) { return 0; }

static BOOL L_GetIcon(REG(a0, CONST_STRPTR name), REG(a1, struct DiskObject *icon), REG(a2, struct FreeList *fl))
{
    return FALSE;                   /* step R3 */
}

static BOOL L_PutIcon(REG(a0, CONST_STRPTR name), REG(a1, struct DiskObject *icon))
{
    return put_icon(name, icon, NULL);
}

/* ---- the library ---- */

static struct Library *lib_init(REG(d0, struct OIBase *b), REG(a0, BPTR seg), REG(a6, struct ExecBase *sys))
{
    SysBase = sys;
    b->seglist = seg;
    b->lib.lib_Node.ln_Type = NT_LIBRARY;
    b->lib.lib_Node.ln_Name = (char *)lib_name;
    b->lib.lib_Flags = LIBF_SUMUSED | LIBF_CHANGED;
    b->lib.lib_Version = VERSION;
    b->lib.lib_Revision = REVISION;
    b->lib.lib_IdString = (APTR)lib_id;
    DOSBase = (struct DosLibrary *)OpenLibrary((CONST_STRPTR)"dos.library", 39);
    UtilityBase = OpenLibrary((CONST_STRPTR)"utility.library", 39);
    if (!DOSBase || !UtilityBase) {
        if (DOSBase)
            CloseLibrary((struct Library *)DOSBase);
        if (UtilityBase)
            CloseLibrary(UtilityBase);
        FreeMem((UBYTE *)b - b->lib.lib_NegSize, b->lib.lib_NegSize + b->lib.lib_PosSize);
        return NULL;
    }
    InitSemaphore(&b->lock);
    b->objects.mlh_Head = (struct MinNode *)&b->objects.mlh_Tail;   /* an empty list */
    b->objects.mlh_Tail = NULL;
    b->objects.mlh_TailPred = (struct MinNode *)&b->objects.mlh_Head;
    b->precision = PRECISION_ICON;
    b->frameless = TRUE;            /* both on, as 3.2 has them (iconprobe at d801676) */
    b->newicons = TRUE;             /* 3.2 says so but shows no NewIcon colour picture; we do (ours only) */
    b->coloricons = TRUE;
    b->maxnamelen = 25;
    Base = b;
    return &b->lib;
}

static struct Library *lib_open(REG(a6, struct OIBase *b))
{
    b->lib.lib_OpenCnt++;
    b->lib.lib_Flags &= ~LIBF_DELEXP;
    return &b->lib;
}

static BPTR lib_expunge(REG(a6, struct OIBase *b))
{
    BPTR seg;
    if (b->lib.lib_OpenCnt || b->objects.mlh_Head->mln_Succ) {
        b->lib.lib_Flags |= LIBF_DELEXP;
        return 0;
    }
    seg = b->seglist;
    Remove(&b->lib.lib_Node);
    CloseLibrary(UtilityBase);
    CloseLibrary((struct Library *)DOSBase);
    FreeMem((UBYTE *)b - b->lib.lib_NegSize, b->lib.lib_NegSize + b->lib.lib_PosSize);
    return seg;
}

static BPTR lib_close(REG(a6, struct OIBase *b))
{
    if (b->lib.lib_OpenCnt)
        b->lib.lib_OpenCnt--;
    if (!b->lib.lib_OpenCnt && (b->lib.lib_Flags & LIBF_DELEXP))
        return lib_expunge(b);
    return 0;
}

static ULONG lib_null(void) { return 0; }

/* icon.library's table (icon_lib.sfd 47.1, bias 30), as iconlib.c has it */
static const APTR functable[] = {
    (APTR)lib_open, (APTR)lib_close, (APTR)lib_expunge, (APTR)lib_null,
    (APTR)L_Obsolete,               /*  -30 OBSOLETEGetWBObject */
    (APTR)L_Obsolete,               /*  -36 OBSOLETEPutWBObject */
    (APTR)L_GetIcon,                /*  -42 */
    (APTR)L_PutIcon,                /*  -48 */
    (APTR)L_FreeFreeList,           /*  -54 */
    (APTR)L_Obsolete,               /*  -60 OBSOLETEFreeWBObject */
    (APTR)L_Obsolete,               /*  -66 OBSOLETEAllocWBObject */
    (APTR)L_AddFreeList,            /*  -72 */
    (APTR)L_GetDiskObject,          /*  -78 */
    (APTR)L_PutDiskObject,          /*  -84 */
    (APTR)L_FreeDiskObject,         /*  -90 */
    (APTR)L_FindToolType,           /*  -96 */
    (APTR)L_MatchToolValue,         /* -102 */
    (APTR)L_BumpRevision,           /* -108 */
    (APTR)L_FreeAlloc,              /* -114 */
    (APTR)L_GetDefDiskObject,       /* -120 */
    (APTR)L_PutDefDiskObject,       /* -126 */
    (APTR)L_GetDiskObjectNew,       /* -132 */
    (APTR)L_DeleteDiskObject,       /* -138 */
    (APTR)L_FreeFree,               /* -144 */
    (APTR)L_DupDiskObjectA,         /* -150 */
    (APTR)L_IconControlA,           /* -156 */
    (APTR)L_DrawIconStateA,         /* -162 */
    (APTR)L_GetIconRectangleA,      /* -168 */
    (APTR)L_NewDiskObject,          /* -174 */
    (APTR)L_GetIconTagList,         /* -180 */
    (APTR)L_PutIconTagList,         /* -186 */
    (APTR)L_LayoutIconA,            /* -192 */
    (APTR)L_ChangeToSelectedIconColor, /* -198 */
    (APTR)L_BumpRevisionLength,     /* -204 */
    (APTR)-1
};
typedef char check_count[sizeof functable / sizeof functable[0] == 4 + 30 + 1 ? 1 : -1];

static const APTR inittable[4] = { (APTR)sizeof(struct OIBase), (APTR)functable, NULL, (APTR)lib_init };

const struct Resident romtag __attribute__((used)) = {
    RTC_MATCHWORD, (struct Resident *)&romtag, (APTR)(&romtag + 1), RTF_AUTOINIT, VERSION, NT_LIBRARY, 0,
    (char *)lib_name, (char *)lib_id, (APTR)inittable
};
