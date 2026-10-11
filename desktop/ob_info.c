/* ob_info: OpenBench's Information window (ob_info.h). Each window runs in a process of its own, started with
 * the path in a message, and frees everything before it ends; OpenBench waits for them before it quits (their
 * code is OpenBench's). GadTools gadgets, the screen's font; the window is opened first and the gadgets laid out
 * inside its real borders (OpenFrames draws its own). Workbench 3.x's Information window, read for what it holds
 * (no code taken): name, type, size, date; Script, Archived, Readable, Writable, Executable, Deletable; comment;
 * a project's (or a disk's) default tool; a tool's or a project's tool types; Save and Cancel.
 * MIT, Copyright (c) 2026 Dalsin Limited. */
#include <exec/types.h>
#include <exec/memory.h>
#include <exec/lists.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/datetime.h>
#include <dos/dostags.h>
#include <intuition/intuition.h>
#include <intuition/gadgetclass.h>
#include <libraries/gadtools.h>
#include <workbench/workbench.h>
#include <workbench/icon.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/gadtools.h>
#include <proto/icon.h>
#include <string.h>
#include <stdio.h>

#include "ob_info.h"

#define MAX_INFO 8
#define MAX_TT 64
#define TT_LEN 128
#define PATH_LEN 256
#define ICON_BOX 84

struct info_start {
    struct Message msg;
    struct Screen *scr;
    int slot;
    char path[PATH_LEN];
};

static struct Task *volatile info_task[MAX_INFO];
static struct Window *volatile info_win[MAX_INFO];
static char info_for[MAX_INFO][PATH_LEN];

enum { G_NAME = 1, G_TYPE, G_SIZE, G_DATE, G_PS, G_PA, G_PR, G_PW, G_PE, G_PD, G_COMMENT, G_TOOL, G_TTLIST, G_TTSTR,
       G_TTNEW, G_TTDEL, G_SAVE, G_CANCEL, G_COUNT };

typedef struct {
    struct Screen *scr;
    char path[PATH_LEN];
    char iconpath[PATH_LEN];        /* the icon's name without .info: "Work:Disk" for a disk */
    char name[108];
    int disk, drawer, had_icon;
    struct DiskObject *dob;
    LONG prot;
    char comment[80];
    char tool[PATH_LEN];
    struct List ttlist;
    struct Node ttnode[MAX_TT];
    char tt[MAX_TT][TT_LEN];
    int ntt, ttsel, tt_changed;
    char type[24], size[100], date[64];
    struct Gadget *glist, *g[G_COUNT];
    struct Window *w;
    APTR vi;
    WORD bx, by, bw, bh;            /* the icon's box */
} info;

/* ---- reading -------------------------------------------------------------------------------- */

static void thousands(char *out, int n, unsigned long long v)
{
    char tmp[32];
    int len, i, k = 0;
    snprintf(tmp, sizeof tmp, "%llu", v);
    len = (int)strlen(tmp);
    for (i = 0; i < len && k < n - 1; i++) {
        if (i && (len - i) % 3 == 0 && k < n - 1) out[k++] = ',';
        out[k++] = tmp[i];
    }
    out[k] = 0;
}

static void date_text(char *out, int n, const struct DateStamp *ds)
{
    char day[LEN_DATSTRING], date[LEN_DATSTRING], time[LEN_DATSTRING];
    struct DateTime dt;
    memset(&dt, 0, sizeof dt);
    dt.dat_Stamp = *ds;
    dt.dat_Format = FORMAT_DOS;
    dt.dat_StrDay = (STRPTR)day;
    dt.dat_StrDate = (STRPTR)date;
    dt.dat_StrTime = (STRPTR)time;
    day[0] = date[0] = time[0] = 0;
    DateToStr(&dt);
    snprintf(out, n, "%s %s %s", day, date, time);
}

static int exists(const char *path)
{
    BPTR l = Lock((CONST_STRPTR)path, ACCESS_READ);
    if (l) UnLock(l);
    return l != 0;
}

static void tt_relist(info *in)
{
    int i;
    NewList(&in->ttlist);
    for (i = 0; i < in->ntt; i++) {
        in->ttnode[i].ln_Name = (char *)in->tt[i];
        in->ttnode[i].ln_Type = 0;
        in->ttnode[i].ln_Pri = 0;
        AddTail(&in->ttlist, &in->ttnode[i]);
    }
}

static int read_info(info *in)
{
    size_t n = strlen(in->path);
    struct FileInfoBlock *fib;
    BPTR l;
    int ok = 0;
    in->disk = n > 0 && in->path[n - 1] == ':';
    if (!(l = Lock((CONST_STRPTR)in->path, ACCESS_READ))) return 0;
    if (!(fib = AllocDosObject(DOS_FIB, NULL))) { UnLock(l); return 0; }
    if (Examine(l, fib)) {
        ok = 1;
        if (in->disk) {
            struct InfoData *id = AllocVec(sizeof *id, MEMF_ANY);
            snprintf(in->iconpath, sizeof in->iconpath, "%sDisk", in->path);
            strncpy(in->name, in->path, sizeof in->name - 1);
            in->name[strlen(in->name) - 1] = 0;            /* without the colon */
            if (id && Info(l, id)) {
                unsigned long long bpb = id->id_BytesPerBlock, total = (unsigned long long)id->id_NumBlocks * bpb,
                                   used = (unsigned long long)id->id_NumBlocksUsed * bpb;
                char t[24], u[24], f[24];
                thousands(t, sizeof t, total >> 20);
                thousands(u, sizeof u, used >> 20);
                thousands(f, sizeof f, (total - used) >> 20);
                snprintf(in->size, sizeof in->size, "%s MB: %s used, %s free (%s)", t, u, f,
                         id->id_DiskState == ID_WRITE_PROTECTED ? "read only" : "read/write");
                if (id->id_VolumeNode) {
                    struct DosList *vn = (struct DosList *)BADDR(id->id_VolumeNode);
                    date_text(in->date, sizeof in->date, &vn->dol_misc.dol_volume.dol_VolumeDate);
                }
            }
            if (id) FreeVec(id);
        } else {
            strncpy(in->iconpath, in->path, sizeof in->iconpath - 1);
            strncpy(in->name, (const char *)FilePart((STRPTR)in->path), sizeof in->name - 1);
            in->drawer = fib->fib_DirEntryType > 0;
            in->prot = fib->fib_Protection;
            strncpy(in->comment, (const char *)fib->fib_Comment, sizeof in->comment - 1);
            if (!in->drawer) {
                char b[24];
                thousands(b, sizeof b, (unsigned long long)fib->fib_Size);
                snprintf(in->size, sizeof in->size, "%s bytes, %ld blocks", b, (long)fib->fib_NumBlocks);
            }
            date_text(in->date, sizeof in->date, &fib->fib_Date);
        }
    }
    FreeDosObject(DOS_FIB, fib);
    UnLock(l);
    if (!ok) return 0;
    {
        char ip[PATH_LEN + 6];
        snprintf(ip, sizeof ip, "%s.info", in->iconpath);
        in->had_icon = exists(ip);
    }
    in->dob = IconBase->lib_Version >= 44 ? GetDiskObjectNew((STRPTR)in->iconpath) : GetDiskObject((STRPTR)in->iconpath);
    if (!in->dob) in->dob = GetDefDiskObject(in->disk ? WBDISK : in->drawer ? WBDRAWER : WBTOOL);
    if (in->dob) {
        static const char *const types[] = { "", "Disk", "Drawer", "Tool", "Project", "Trashcan", "Device", "Kickstart", "AppIcon" };
        STRPTR *t = in->dob->do_ToolTypes;
        strncpy(in->type, in->dob->do_Type >= 1 && in->dob->do_Type <= 8 ? types[in->dob->do_Type] : "", sizeof in->type - 1);
        if (in->dob->do_DefaultTool) strncpy(in->tool, (const char *)in->dob->do_DefaultTool, sizeof in->tool - 1);
        for (; t && *t && in->ntt < MAX_TT; t++) {
            strncpy(in->tt[in->ntt], (const char *)*t, TT_LEN - 1);
            in->ntt++;
        }
    } else
        strcpy(in->type, in->disk ? "Disk" : in->drawer ? "Drawer" : "File");
    in->ttsel = -1;
    tt_relist(in);
    return 1;
}

/* ---- the window ----------------------------------------------------------------------------- */

static int has_tool(info *in) { return in->dob && (in->dob->do_Type == WBPROJECT || in->dob->do_Type == WBDISK); }
static int has_tt(info *in) { return in->dob && (in->dob->do_Type == WBTOOL || in->dob->do_Type == WBPROJECT); }

static WORD text_len(struct Screen *s, const char *t)
{
    struct RastPort rp;
    struct TextFont *f = OpenFont(s->Font);
    WORD w;
    InitRastPort(&rp);
    if (f) SetFont(&rp, f);
    w = TextLength(&rp, (STRPTR)t, strlen(t));
    if (f) CloseFont(f);
    return w;
}

static struct Gadget *gad(info *in, struct Gadget *prev, ULONG kind, int id, WORD x, WORD y, WORD w, WORD h,
                          const char *label, ULONG flags, Tag tag1, ...)
{
    struct NewGadget ng;
    memset(&ng, 0, sizeof ng);
    ng.ng_LeftEdge = x;
    ng.ng_TopEdge = y;
    ng.ng_Width = w;
    ng.ng_Height = h;
    ng.ng_GadgetText = (STRPTR)label;
    ng.ng_TextAttr = in->scr->Font;
    ng.ng_GadgetID = id;
    ng.ng_Flags = flags;
    ng.ng_VisualInfo = in->vi;
    prev = CreateGadgetA(kind, prev, &ng, (struct TagItem *)&tag1);
    if (prev) in->g[id] = prev;
    return prev;
}

/* The window's inner size for its gadgets; *lay 1: create them too, at the window's borders. */
static void layout(info *in, WORD *iw, WORD *ih, int make)
{
    static const char *const labels[] = { "Name", "Type", "Size", "Changed", "Comment", "Default tool", "Tool types", NULL };
    static const char *const prot[6] = { "Script", "Archived", "Readable", "Writable", "Executable", "Deletable" };
    struct Gadget *p = NULL;
    WORD fh = in->scr->Font->ta_YSize, rh = fh + 6, m = 8, lw = 0, vw = 340, x0, y, i, ox = 0, oy = 0, W;
    for (i = 0; labels[i]; i++) { WORD t = text_len(in->scr, labels[i]); if (t > lw) lw = t; }
    lw += 12;
    if (make) { ox = in->w->BorderLeft; oy = in->w->BorderTop; p = CreateContext(&in->glist); }
    W = m + ICON_BOX + m + lw + vw + m;
    x0 = m + ICON_BOX + m + lw;
    y = m;
    in->bx = ox + m; in->by = oy + m; in->bw = ICON_BOX; in->bh = 4 * (rh + 4) - 4;
    if (make) {
        p = gad(in, p, STRING_KIND, G_NAME, ox + x0, oy + y, vw, rh, "Name", PLACETEXT_LEFT, GTST_String, (ULONG)in->name, GTST_MaxChars, 100, TAG_DONE);
        p = gad(in, p, TEXT_KIND, G_TYPE, ox + x0, oy + y + (rh + 4), vw, rh, "Type", PLACETEXT_LEFT, GTTX_Text, (ULONG)in->type, GTTX_Border, FALSE, TAG_DONE);
        p = gad(in, p, TEXT_KIND, G_SIZE, ox + x0, oy + y + 2 * (rh + 4), vw, rh, "Size", PLACETEXT_LEFT, GTTX_Text, (ULONG)in->size, TAG_DONE);
        p = gad(in, p, TEXT_KIND, G_DATE, ox + x0, oy + y + 3 * (rh + 4), vw, rh, in->disk ? "Created" : "Changed", PLACETEXT_LEFT, GTTX_Text, (ULONG)in->date, TAG_DONE);
    }
    y += 4 * (rh + 4) + 4;
    if (!in->disk) {
        WORD cw = (W - 2 * m) / 3;
        for (i = 0; i < 6; i++) {
            WORD cx = m + (i % 3) * cw, cy = y + (i / 3) * (rh + 2);
            LONG on;
            switch (i) {
            case 0: on = (in->prot & FIBF_SCRIPT) != 0; break;
            case 1: on = (in->prot & FIBF_ARCHIVE) != 0; break;
            case 2: on = !(in->prot & FIBF_READ); break;
            case 3: on = !(in->prot & FIBF_WRITE); break;
            case 4: on = !(in->prot & FIBF_EXECUTE); break;
            default: on = !(in->prot & FIBF_DELETE); break;
            }
            if (make) p = gad(in, p, CHECKBOX_KIND, G_PS + i, ox + cx, oy + cy, 26, rh, prot[i], PLACETEXT_RIGHT, GTCB_Checked, on, GTCB_Scaled, TRUE, TAG_DONE);
        }
        y += 2 * (rh + 2) + 6;
        if (make) p = gad(in, p, STRING_KIND, G_COMMENT, ox + m + lw, oy + y, W - 2 * m - lw, rh, "Comment", PLACETEXT_LEFT, GTST_String, (ULONG)in->comment, GTST_MaxChars, 79, TAG_DONE);
        y += rh + 4;
    }
    if (has_tool(in)) {
        if (make) p = gad(in, p, STRING_KIND, G_TOOL, ox + m + lw, oy + y, W - 2 * m - lw, rh, "Default tool", PLACETEXT_LEFT, GTST_String, (ULONG)in->tool, GTST_MaxChars, PATH_LEN - 1, TAG_DONE);
        y += rh + 4;
    }
    if (has_tt(in)) {
        WORD lh = 5 * (fh + 2) + 4, bw2 = 64;
        if (make) {
            p = gad(in, p, LISTVIEW_KIND, G_TTLIST, ox + m + lw, oy + y, W - 2 * m - lw, lh, "Tool types", PLACETEXT_LEFT,
                    GTLV_Labels, (ULONG)&in->ttlist, GTLV_ShowSelected, 0, TAG_DONE);
            p = gad(in, p, STRING_KIND, G_TTSTR, ox + m + lw, oy + y + lh + 2, W - 2 * m - lw - 2 * (bw2 + 4), rh, NULL, 0,
                    GTST_MaxChars, TT_LEN - 1, GA_Disabled, TRUE, TAG_DONE);
            p = gad(in, p, BUTTON_KIND, G_TTNEW, ox + W - m - 2 * bw2 - 4, oy + y + lh + 2, bw2, rh, "New", PLACETEXT_IN, TAG_DONE);
            p = gad(in, p, BUTTON_KIND, G_TTDEL, ox + W - m - bw2, oy + y + lh + 2, bw2, rh, "Del", PLACETEXT_IN, TAG_DONE);
        }
        y += lh + 2 + rh + 4;
    }
    y += 6;
    if (make) {
        p = gad(in, p, BUTTON_KIND, G_SAVE, ox + m, oy + y, 100, rh + 2, "Save", PLACETEXT_IN, TAG_DONE);
        p = gad(in, p, BUTTON_KIND, G_CANCEL, ox + W - m - 100, oy + y, 100, rh + 2, "Cancel", PLACETEXT_IN, TAG_DONE);
    }
    y += rh + 2 + m;
    *iw = W;
    *ih = y;
}

static void draw_icon_box(info *in)
{
    struct RastPort *rp = in->w->RPort;
    struct Rectangle r;
    WORD iw = 32, ih = 16, x, y;
    DrawBevelBox(rp, in->bx, in->by, in->bw, in->bh, GT_VisualInfo, (ULONG)in->vi, GTBB_Recessed, TRUE, TAG_DONE);
    if (!in->dob) return;
    if (GetIconRectangleA(rp, in->dob, NULL, &r, NULL)) { iw = r.MaxX - r.MinX + 1; ih = r.MaxY - r.MinY + 1; }
    x = in->bx + (in->bw - iw) / 2;
    y = in->by + (in->bh - ih) / 2;
    DrawIconState(rp, in->dob, NULL, x, y, IDS_NORMAL, ICONDRAWA_Borderless, TRUE, ICONDRAWA_Frameless, TRUE,
                  ICONDRAWA_EraseBackground, TRUE, TAG_DONE);
}

static const char *str_of(info *in, int id)
{
    return in->g[id] ? (const char *)((struct StringInfo *)in->g[id]->SpecialInfo)->Buffer : "";
}

static LONG checked(info *in, int id)
{
    LONG v = 0;
    if (in->g[id]) GT_GetGadgetAttrs(in->g[id], in->w, NULL, GTCB_Checked, (ULONG)&v, TAG_DONE);
    return v;
}

static void tt_show(info *in)
{
    if (!in->g[G_TTLIST]) return;
    GT_SetGadgetAttrs(in->g[G_TTLIST], in->w, NULL, GTLV_Labels, (ULONG)&in->ttlist, GTLV_Selected, in->ttsel < 0 ? ~0UL : (ULONG)in->ttsel,
                      GTLV_MakeVisible, in->ttsel < 0 ? 0 : in->ttsel, TAG_DONE);
    GT_SetGadgetAttrs(in->g[G_TTSTR], in->w, NULL, GTST_String, (ULONG)(in->ttsel < 0 ? "" : in->tt[in->ttsel]),
                      GA_Disabled, in->ttsel < 0, TAG_DONE);
}

static void tt_detach(info *in)
{
    if (in->g[G_TTLIST]) GT_SetGadgetAttrs(in->g[G_TTLIST], in->w, NULL, GTLV_Labels, ~0UL, TAG_DONE);
}

/* ---- saving --------------------------------------------------------------------------------- */

static void say(info *in, const char *what)
{
    struct EasyStruct es = { sizeof(struct EasyStruct), 0, (UBYTE *)"Information", (UBYTE *)"%s\n%s", (UBYTE *)"OK" };
    char err[80];
    Fault(IoErr(), NULL, (STRPTR)err, sizeof err);
    EasyRequest(in->w, &es, NULL, (ULONG)what, (ULONG)err);
}

static int save(info *in)
{
    const char *newname = str_of(in, G_NAME);
    if (!in->disk) {
        LONG p = in->prot & ~(FIBF_SCRIPT | FIBF_ARCHIVE | FIBF_READ | FIBF_WRITE | FIBF_EXECUTE | FIBF_DELETE);
        if (checked(in, G_PS)) p |= FIBF_SCRIPT;
        if (checked(in, G_PA)) p |= FIBF_ARCHIVE;
        if (!checked(in, G_PR)) p |= FIBF_READ;
        if (!checked(in, G_PW)) p |= FIBF_WRITE;
        if (!checked(in, G_PE)) p |= FIBF_EXECUTE;
        if (!checked(in, G_PD)) p |= FIBF_DELETE;
        if (p != in->prot && !SetProtection((STRPTR)in->path, p)) { say(in, "The protection could not be set."); return 0; }
        if (strcmp(str_of(in, G_COMMENT), in->comment) && !SetComment((STRPTR)in->path, (STRPTR)str_of(in, G_COMMENT))) {
            say(in, "The comment could not be set.");
            return 0;
        }
    }
    if (in->dob && (in->tt_changed || (in->g[G_TOOL] && strcmp(str_of(in, G_TOOL), in->tool)))) {
        STRPTR arr[MAX_TT + 1], *keep_tt = in->dob->do_ToolTypes, keep_tool = in->dob->do_DefaultTool;
        int i;
        for (i = 0; i < in->ntt; i++) arr[i] = (STRPTR)in->tt[i];
        arr[in->ntt] = NULL;
        if (has_tt(in)) in->dob->do_ToolTypes = arr;
        if (in->g[G_TOOL]) in->dob->do_DefaultTool = (STRPTR)str_of(in, G_TOOL);
        i = PutDiskObject((STRPTR)in->iconpath, in->dob);
        in->dob->do_ToolTypes = keep_tt;
        in->dob->do_DefaultTool = keep_tool;
        if (!i) { say(in, "The icon could not be saved."); return 0; }
    }
    if (newname[0] && strcmp(newname, in->name)) {
        if (in->disk) {
            if (!Relabel((STRPTR)in->path, (STRPTR)newname)) { say(in, "The disk could not be renamed."); return 0; }
        } else {
            char to[PATH_LEN], from_i[PATH_LEN + 6], to_i[PATH_LEN + 6];
            size_t k = (size_t)((const char *)FilePart((STRPTR)in->path) - in->path);
            if (k >= sizeof to) return 0;
            memcpy(to, in->path, k);
            to[k] = 0;
            if (!AddPart((STRPTR)to, (STRPTR)newname, sizeof to)) return 0;
            if (!Rename((STRPTR)in->path, (STRPTR)to)) { say(in, "It could not be renamed."); return 0; }
            snprintf(from_i, sizeof from_i, "%s.info", in->path);
            snprintf(to_i, sizeof to_i, "%s.info", to);
            if (in->had_icon) Rename((STRPTR)from_i, (STRPTR)to_i);
        }
    }
    return 1;
}

/* ---- the process ---------------------------------------------------------------------------- */

static void run(info *in, int slot)
{
    WORD iw, ih;
    int done = 0;
    char title[140];
    layout(in, &iw, &ih, 0);
    snprintf(title, sizeof title, "Information: %s", in->disk ? in->path : in->name);
    in->w = OpenWindowTags(NULL, WA_PubScreen, (ULONG)in->scr, WA_InnerWidth, iw, WA_InnerHeight, ih, WA_Title, (ULONG)title,
                           WA_Left, (in->scr->Width - iw) / 2, WA_Top, (in->scr->Height - ih) / 3,
                           WA_DragBar, TRUE, WA_DepthGadget, TRUE, WA_CloseGadget, TRUE, WA_Activate, TRUE,
                           WA_SimpleRefresh, TRUE, WA_NewLookMenus, TRUE, WA_AutoAdjust, TRUE,
                           WA_IDCMP, BUTTONIDCMP | STRINGIDCMP | LISTVIEWIDCMP | CHECKBOXIDCMP | IDCMP_CLOSEWINDOW |
                                     IDCMP_REFRESHWINDOW | IDCMP_VANILLAKEY, TAG_DONE);
    if (!in->w) return;
    info_win[slot] = in->w;
    layout(in, &iw, &ih, 1);
    if (in->glist) {
        AddGList(in->w, in->glist, (UWORD)~0, -1, NULL);
        RefreshGList(in->glist, in->w, NULL, -1);
        GT_RefreshWindow(in->w, NULL);
    }
    draw_icon_box(in);
    while (!done) {
        ULONG got = Wait((1UL << in->w->UserPort->mp_SigBit) | SIGBREAKF_CTRL_C);
        struct IntuiMessage *im;
        if (got & SIGBREAKF_CTRL_C) break;
        while (!done && (im = GT_GetIMsg(in->w->UserPort))) {
            ULONG cl = im->Class;
            UWORD code = im->Code;
            struct Gadget *g = (struct Gadget *)im->IAddress;
            GT_ReplyIMsg(im);
            switch (cl) {
            case IDCMP_REFRESHWINDOW:
                GT_BeginRefresh(in->w);
                draw_icon_box(in);
                GT_EndRefresh(in->w, TRUE);
                break;
            case IDCMP_CLOSEWINDOW: done = 1; break;
            case IDCMP_VANILLAKEY: if (code == 27) done = 1; break;
            case IDCMP_GADGETUP:
                switch (g->GadgetID) {
                case G_SAVE: if (save(in)) done = 1; break;
                case G_CANCEL: done = 1; break;
                case G_TTLIST:
                    in->ttsel = code < in->ntt ? code : -1;
                    tt_show(in);
                    if (in->ttsel >= 0) ActivateGadget(in->g[G_TTSTR], in->w, NULL);
                    break;
                case G_TTSTR:
                    if (in->ttsel >= 0) {
                        tt_detach(in);
                        strncpy(in->tt[in->ttsel], str_of(in, G_TTSTR), TT_LEN - 1);
                        in->tt[in->ttsel][TT_LEN - 1] = 0;
                        in->tt_changed = 1;
                        tt_show(in);
                    }
                    break;
                case G_TTNEW:
                    if (in->ntt < MAX_TT) {
                        tt_detach(in);
                        in->tt[in->ntt][0] = 0;
                        in->ttsel = in->ntt++;
                        tt_relist(in);
                        in->tt_changed = 1;
                        tt_show(in);
                        ActivateGadget(in->g[G_TTSTR], in->w, NULL);
                    }
                    break;
                case G_TTDEL:
                    if (in->ttsel >= 0 && in->ttsel < in->ntt) {
                        int i;
                        tt_detach(in);
                        for (i = in->ttsel; i < in->ntt - 1; i++) memcpy(in->tt[i], in->tt[i + 1], TT_LEN);
                        in->ntt--;
                        in->ttsel = -1;
                        tt_relist(in);
                        in->tt_changed = 1;
                        tt_show(in);
                    }
                    break;
                }
                break;
            }
        }
    }
    info_win[slot] = NULL;
    if (in->glist) RemoveGList(in->w, in->glist, -1);
    CloseWindow(in->w);
    in->w = NULL;
}

static void info_entry(void)
{
    struct Process *me = (struct Process *)FindTask(NULL);
    struct info_start *st;
    info *in;
    int slot;
    WaitPort(&me->pr_MsgPort);
    st = (struct info_start *)GetMsg(&me->pr_MsgPort);
    slot = st->slot;
    if ((in = AllocVec(sizeof *in, MEMF_ANY | MEMF_CLEAR))) {
        in->scr = st->scr;
        strncpy(in->path, st->path, sizeof in->path - 1);
        if (read_info(in) && (in->vi = GetVisualInfoA(in->scr, NULL)))
            run(in, slot);
        else
            DisplayBeep(in->scr);
        if (in->glist) FreeGadgets(in->glist);
        if (in->vi) FreeVisualInfo(in->vi);
        if (in->dob) FreeDiskObject(in->dob);
        FreeVec(in);
    }
    FreeVec(st);
    Forbid();                         /* ends in dos.library under Forbid: OpenBench waits for info_task to clear */
    info_task[slot] = NULL;
    info_win[slot] = NULL;
}

int ob_info_open(struct Screen *scr, const char *path)
{
    struct info_start *st;
    struct Process *p;
    int i, slot = -1;
    if (!scr || !path || !path[0] || !IconBase) return 0;
    Forbid();
    for (i = 0; i < MAX_INFO; i++)
        if (info_task[i] && !strcmp(info_for[i], path)) {
            struct Window *w = info_win[i];
            if (w) { WindowToFront(w); ActivateWindow(w); }
            Permit();
            return 1;
        }
    for (i = 0; i < MAX_INFO && slot < 0; i++)
        if (!info_task[i]) slot = i;
    if (slot >= 0) info_task[slot] = (struct Task *)1;          /* taken */
    Permit();
    if (slot < 0) return 0;
    strncpy(info_for[slot], path, PATH_LEN - 1);
    info_for[slot][PATH_LEN - 1] = 0;
    if (!(st = AllocVec(sizeof *st, MEMF_PUBLIC | MEMF_CLEAR))) { info_task[slot] = NULL; return 0; }
    st->scr = scr;
    st->slot = slot;
    strncpy(st->path, path, PATH_LEN - 1);
    st->msg.mn_Length = sizeof *st;
    p = CreateNewProcTags(NP_Entry, (ULONG)info_entry, NP_Name, (ULONG)"OpenBench Information", NP_StackSize, 16384, TAG_DONE);
    if (!p) { FreeVec(st); info_task[slot] = NULL; return 0; }
    info_task[slot] = (struct Task *)p;
    PutMsg(&p->pr_MsgPort, &st->msg);
    return 1;
}

int ob_info_running(void)
{
    int i, n = 0;
    for (i = 0; i < MAX_INFO; i++) if (info_task[i]) n++;
    return n;
}

void ob_info_quit_all(void)
{
    int i, tries = 250;
    Forbid();
    for (i = 0; i < MAX_INFO; i++)
        if (info_task[i] && info_task[i] != (struct Task *)1) Signal(info_task[i], SIGBREAKF_CTRL_C);
    Permit();
    while (ob_info_running() && tries--) Delay(1);
}
