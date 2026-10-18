/* ob_drop: icons let go on the desktop's windows (W67; Dale, 18 October 2026: "drag and drop doesn't seem to
 * work", "dragging icons 'into' windows"), as OS 3.2.3's Workbench does it (measured on lab t5-mouse,
 * LoadWB on the stock ROM, 18 October 2026):
 *
 *   - into another drawer window (its space, or a file's icon in it): moved when it is on the same volume,
 *     copied to another volume; the copy shows its progress with a Stop button;
 *   - onto a drawer's icon (in any window, this one too) or a disk's icon on the desktop: into it, the same
 *     way;
 *   - a disk's icon from the desktop into a drawer window: the disk's contents copied (here into a new drawer
 *     named after the disk, so they never mix with what is there);
 *   - onto a program's AppWindow (or one of its drop zones): one AppMessage, AMTYPE_APPWINDOW (or
 *     AMTYPE_APPWINDOWZONE) with the AppWindow's (or the zone's) id and userdata, the place in the window,
 *     and one WBArg per icon: a drawer or disk as its own lock and an empty name, a file as its drawer's
 *     lock and its name. OpenBench sends it (it is the desktop) and frees it, unlocking, when the program
 *     replies. Which AppWindow is under the pointer, workbench.library says (the Team's OWBCTRLA_AppWindowAt;
 *     amigachrome-guest libraries/openworkbench/owb_team.h);
 *   - onto a tool's icon: nothing is started (3.2.3 puts the icon down there);
 *   - in its own drawer, off a drawer's icon: nothing (OpenBench's drawers keep their icons in order);
 *   - onto the desktop's own space: Leave Out (not yet: Phase B).
 *
 * The copies and moves are OpenFiles' engine's (openamigafilemanager engine/of_ops.c), as OpenBench's header
 * planned: a file goes with its .info, a copy is written under a temporary name until it is whole, a file
 * replaced goes to the trash, a move between volumes is a copy read back before the source goes. Each drop
 * runs in a process of its own, so the desktop never waits for a disk; when it is done, the drawers it
 * touched are read again.
 *
 * MIT, Copyright (c) 2026 AmigaChrome Team. */
#include <stdio.h>
#include <string.h>
#include <exec/memory.h>
#include <exec/ports.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <graphics/layers.h>
#include <graphics/clip.h>
#include <libraries/gadtools.h>
#include <workbench/workbench.h>
#include <workbench/startup.h>
#include <utility/tagitem.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/layers.h>
#include <proto/gadtools.h>
#include <proto/wb.h>

#include "ob_drop.h"
#include "of_ops.h"
#include "of_fs_amiga.h"
#include "of_path.h"

extern struct Library *WorkbenchBase;

/* ---- the Team's WorkbenchControlA tags: a copy of amigachrome-guest libraries/openworkbench/owb_team.h
 * (keep the two the same). OS 3.2.3's workbench.library ignores them. ---- */
#define OWBCTRLA_Dummy          (TAG_USER + 0x4F574200UL)
#define OWBCTRLA_DesktopDrops   (OWBCTRLA_Dummy + 1)
#define OWBCTRLA_AppWindowAt    (OWBCTRLA_Dummy + 2)
struct OWBAppWindowAt {
    struct Window *aa_Window;
    WORD aa_MouseX, aa_MouseY;
    struct MsgPort *aa_Port;
    ULONG aa_ID, aa_UserData;
    LONG aa_Zone;
};

static ob_drop_env env;
static struct MsgPort *done_port;               /* the jobs, back when they end */
static struct MsgPort *app_port;                /* the AppMessages, back when their programs reply */
static volatile int jobs_running;
static int apps_out;
static volatile int stopping;

/* ---- what is where ---- */

struct Window *ob_drop_window_under(WORD sx, WORD sy, struct Window *skip)
{
    struct Screen *s = *env.scr;
    struct Layer *l;
    struct Window *w = NULL;
    if (!s) return NULL;
    LockLayerInfo(&s->LayerInfo);
    for (l = s->LayerInfo.top_layer; l; l = l->back)
        if (sx >= l->bounds.MinX && sx <= l->bounds.MaxX && sy >= l->bounds.MinY && sy <= l->bounds.MaxY &&
            (!skip || (struct Window *)l->Window != skip)) {
            w = (struct Window *)l->Window;
            break;
        }
    UnlockLayerInfo(&s->LayerInfo);
    return w;
}

struct Window *ob_drop_window_at(WORD sx, WORD sy)
{
    return ob_drop_window_under(sx, sy, NULL);
}

static wb_drawer *drawer_of(struct Window *w)
{
    int i;
    for (i = 0; i < *env.ndrawers; i++)
        if (wb_drawer_window(env.drawers[i]) == w) return env.drawers[i];
    return NULL;
}

/* Every open drawer showing path, read again */
static void reread(const char *path)
{
    int i;
    for (i = 0; i < *env.ndrawers; i++)
        if (of_path_equal(wb_drawer_path(env.drawers[i]), path)) wb_drawer_reread(env.drawers[i]);
}

static int is_dir(const char *path)
{
    BPTR l;
    struct FileInfoBlock *fib;
    int dir = 0;
    size_t n = strlen(path);
    if (n && path[n - 1] == ':') return 1;
    if (!(l = Lock((CONST_STRPTR)path, ACCESS_READ))) return 0;
    if ((fib = AllocDosObject(DOS_FIB, NULL))) {
        if (Examine(l, fib)) dir = fib->fib_DirEntryType > 0;
        FreeDosObject(DOS_FIB, fib);
    }
    UnLock(l);
    return dir;
}

/* 1 when a and b are on the same volume (moved), 0 when not (copied) or when one can't be found */
static int same_volume(const char *a, const char *b)
{
    BPTR la = Lock((CONST_STRPTR)a, ACCESS_READ), lb = la ? Lock((CONST_STRPTR)b, ACCESS_READ) : 0;
    LONG r = la && lb ? SameLock(la, lb) : LOCK_DIFFERENT;
    if (lb) UnLock(lb);
    if (la) UnLock(la);
    return r == LOCK_SAME || r == LOCK_SAME_VOLUME;
}

/* ---- the jobs: a copy or a move, in a process of its own ---- */

#define JOB_DIRS 8                              /* drawers to read again afterwards: where it went, where from */
#define WIN_W 440

typedef struct job {
    struct Message msg;                         /* to done_port when it ends */
    struct Screen *scr;                         /* the job's: locked while its window is up */
    int move;                                   /* 1: a move (one volume), 0: a copy */
    int n;
    char **src;
    char dst[OF_PATH_MAX];
    char volume[OF_PATH_MAX];                   /* a disk's contents: into a new drawer in dst named after it */
    int ndirs;
    char dirs[JOB_DIRS][OF_PATH_MAX];
    /* the job's own, in its process */
    struct Window *win;
    struct Gadget *glist;
    APTR vi;
    struct DrawInfo *dri;
    ULONG t0s, t0m, ts, tm;                     /* when it began; when the window was last drawn */
    int tried, stopped, failed, first_err, rc;
    char first_failed[OF_PATH_MAX];
} job;

static void job_free(job *j)
{
    int i;
    for (i = 0; i < j->n; i++) FreeVec(j->src[i]);
    if (j->src) FreeVec(j->src);
    FreeVec(j);
}

static void job_dir(job *j, const char *dir)
{
    int i;
    for (i = 0; i < j->ndirs; i++)
        if (of_path_equal(j->dirs[i], dir)) return;
    if (j->ndirs < JOB_DIRS) of_strlcpy(j->dirs[j->ndirs++], dir, OF_PATH_MAX);
}

static ULONG ms_since(ULONG s0, ULONG m0)
{
    ULONG s, m;
    CurrentTime(&s, &m);
    return (s - s0) * 1000 + m / 1000 - m0 / 1000;
}

/* The window, once a job has taken more than a moment: what it does, the file, a bar, Stop */
static void job_window(job *j)
{
    struct Screen *s;
    struct NewGadget ng;
    struct Gadget *g;
    int fh, top, h;
    /* the Workbench screen locked while the window is up (it may have been reset since the drop) */
    if (!(s = j->scr = LockPubScreen((CONST_STRPTR)"Workbench"))) return;
    fh = s->Font ? s->Font->ta_YSize : 8;
    top = s->WBorTop + fh + 1;
    h = top + 3 * fh + 72;
    if (!(j->vi = GetVisualInfoA(s, NULL)) || !(j->dri = GetScreenDrawInfo(s))) return;
    if (!(g = CreateContext(&j->glist))) return;
    memset(&ng, 0, sizeof ng);
    ng.ng_LeftEdge = (WIN_W - 100) / 2;
    ng.ng_TopEdge = h - fh - 8 - 10;
    ng.ng_Width = 100;
    ng.ng_Height = fh + 8;
    ng.ng_GadgetText = (UBYTE *)"Stop";
    ng.ng_TextAttr = s->Font;
    ng.ng_VisualInfo = j->vi;
    ng.ng_Flags = PLACETEXT_IN;
    if (!CreateGadgetA(BUTTON_KIND, g, &ng, NULL)) return;
    j->win = OpenWindowTags(NULL, WA_Left, (s->Width - WIN_W) / 2, WA_Top, (s->Height - h) / 3, WA_Width, WIN_W,
                            WA_Height, h, WA_Title, (ULONG)(j->move ? "Moving" : "Copying"), WA_DragBar, TRUE,
                            WA_DepthGadget, TRUE, WA_Activate, FALSE, WA_SmartRefresh, TRUE, WA_RMBTrap, TRUE,
                            WA_PubScreen, (ULONG)s, WA_Gadgets, (ULONG)j->glist,
                            WA_IDCMP, BUTTONIDCMP | IDCMP_REFRESHWINDOW, TAG_DONE);
    if (j->win) GT_RefreshWindow(j->win, NULL);
}

static void job_window_close(job *j)
{
    if (j->win) {
        struct IntuiMessage *m;
        while ((m = GT_GetIMsg(j->win->UserPort))) GT_ReplyIMsg(m);
        CloseWindow(j->win);
        j->win = NULL;
    }
    if (j->glist) { FreeGadgets(j->glist); j->glist = NULL; }
    if (j->dri) { FreeScreenDrawInfo(j->scr, j->dri); j->dri = NULL; }
    if (j->vi) { FreeVisualInfo(j->vi); j->vi = NULL; }
    if (j->scr) { UnlockPubScreen(NULL, j->scr); j->scr = NULL; }
}

static void text_at(struct RastPort *rp, int x, int y, int w, const char *s)
{
    struct TextExtent te;
    ULONG n = strlen(s), fit = n ? TextFit(rp, (CONST_STRPTR)s, n, &te, NULL, 1, (WORD)w, 32767) : 0;
    Move(rp, (WORD)x, (WORD)(y + rp->TxBaseline));
    if (fit) Text(rp, (CONST_STRPTR)s, fit);
}

static void job_draw(job *j, const of_progress *p)
{
    struct RastPort *rp = j->win->RPort;
    UWORD *pens = j->dri->dri_Pens;
    int fh = rp->TxHeight, x = j->win->BorderLeft + 12, w = j->win->Width - j->win->BorderLeft - j->win->BorderRight - 24;
    int y = j->win->BorderTop + 8, bar;
    char line[OF_PATH_MAX + 64];
    ULONG done = p->bytes_total ? (ULONG)(p->bytes_done * 1000 / p->bytes_total)
                                : p->files_total ? (ULONG)p->files_done * 1000 / p->files_total : 0;
    SetAPen(rp, pens[BACKGROUNDPEN]);
    RectFill(rp, (WORD)x, (WORD)y, (WORD)(x + w - 1), (WORD)(y + 2 * (fh + 6) + 14));
    SetAPen(rp, pens[TEXTPEN]);
    SetDrMd(rp, JAM1);
    snprintf(line, sizeof line, "%s %lu of %lu to %s", j->move ? "Moving" : "Copying", (unsigned long)p->files_done + 1,
             (unsigned long)(p->files_total ? p->files_total : 1), j->dst);
    if (p->files_done >= p->files_total && p->files_total) line[0] = 0;
    text_at(rp, x, y, w, line);
    text_at(rp, x, y + fh + 6, w, p->path ? of_path_name(p->path) : "");
    y += 2 * (fh + 6);
    bar = (int)((ULONG)w * (done > 1000 ? 1000 : done) / 1000);
    SetAPen(rp, pens[SHADOWPEN]);
    Move(rp, (WORD)x, (WORD)y); Draw(rp, (WORD)(x + w - 1), (WORD)y); Draw(rp, (WORD)(x + w - 1), (WORD)(y + 13));
    Draw(rp, (WORD)x, (WORD)(y + 13)); Draw(rp, (WORD)x, (WORD)y);
    if (bar > 2) {
        SetAPen(rp, pens[FILLPEN]);
        RectFill(rp, (WORD)(x + 1), (WORD)(y + 1), (WORD)(x + bar - 2), (WORD)(y + 12));
    }
}

/* The engine's word as it works: the window after a moment, drawn at most ten times a second; Stop (or
 * OpenBench quitting) stops it */
static int job_progress(void *user, const of_progress *p)
{
    job *j = user;
    if (!j->tried && !j->stopped && ms_since(j->t0s, j->t0m) > 400) {
        j->tried = 1;
        job_window(j);
        j->ts = j->tm = 0;
    }
    if (j->win) {
        struct IntuiMessage *m;
        while ((m = GT_GetIMsg(j->win->UserPort))) {
            ULONG cl = m->Class;
            GT_ReplyIMsg(m);
            if (cl == IDCMP_GADGETUP) j->stopped = 1;
            else if (cl == IDCMP_REFRESHWINDOW) { GT_BeginRefresh(j->win); GT_EndRefresh(j->win, TRUE); j->ts = 0; }
        }
        if (!j->ts || ms_since(j->ts, j->tm) > 100 || !p->path) {
            job_draw(j, p);
            CurrentTime(&j->ts, &j->tm);
        }
    }
    return j->stopped || stopping;
}

/* A name that is taken: asked, as OpenFiles asks */
static int job_ask(void *user, const of_clash *c)
{
    job *j = user;
    struct EasyStruct es;
    char dir[OF_PATH_MAX];
    LONG r;
    of_path_parent(dir, c->dst);
    es.es_StructSize = sizeof es;
    es.es_Flags = 0;
    es.es_Title = (UBYTE *)(j->move ? "Moving" : "Copying");
    es.es_TextFormat = (UBYTE *)"\"%s\" is already in\n%s.";
    es.es_GadgetFormat = (UBYTE *)"Replace|Replace all|Keep both|Skip|Stop";
    r = EasyRequest(j->win, &es, NULL, (ULONG)of_path_name(c->dst), (ULONG)dir);
    switch (r) {
    case 1: return OF_CLASH_REPLACE;
    case 2: return OF_CLASH_REPLACE | OF_CLASH_ALL;
    case 3: return OF_CLASH_KEEP_BOTH;
    case 4: return OF_CLASH_SKIP;
    default: return OF_CLASH_CANCEL;
    }
}

static void job_report(void *user, const char *path, int err)
{
    job *j = user;
    if (!j->failed++) {
        of_strlcpy(j->first_failed, path, OF_PATH_MAX);
        j->first_err = err;
    }
}

static const char *err_text(int e)
{
    switch (e) {
    case OF_ERR_NOTFOUND: return "it isn't there any more";
    case OF_ERR_EXISTS: return "the name is taken";
    case OF_ERR_FULL: return "the disk is full";
    case OF_ERR_VERIFY: return "the copy didn't read back the same";
    case OF_ERR_INVALID: return "a drawer can't go into itself";
    case OF_ERR_NOMEM: return "there isn't enough memory";
    case OF_ERR_PROTECTED: return "it is protected from deleting";
    case OF_ERR_READONLY: return "the disk is write-protected";
    default: return "the disk said no";
    }
}

/* A disk's contents into a new drawer in dst named after the disk ("Work 2" when "Work" is taken) */
typedef struct names { char **v; int n, cap; } names;

static int name_add(void *user, const char *name)
{
    names *l = user;
    char *s;
    if (l->n == l->cap) {
        char **nv = AllocVec((ULONG)(l->cap + 32) * sizeof *nv, MEMF_ANY);
        if (!nv) return 1;
        if (l->v) { memcpy(nv, l->v, (size_t)l->n * sizeof *nv); FreeVec(l->v); }
        l->v = nv;
        l->cap += 32;
    }
    if (!(s = AllocVec(strlen(name) + 1, MEMF_ANY))) return 1;
    strcpy(s, name);
    l->v[l->n++] = s;
    return 0;
}

static int copy_volume(of_ctx *c, job *j)
{
    names l;
    char base[64], into[OF_PATH_MAX], path[OF_PATH_MAX];
    of_stat st;
    int i, k, rc = OF_OK, m = 0;
    char **src;
    memset(&l, 0, sizeof l);
    of_strlcpy(base, j->volume, sizeof base);
    if ((k = (int)strlen(base)) && base[k - 1] == ':') base[k - 1] = 0;
    for (i = 1; i < 100; i++) {
        char name[80];
        if (i == 1) of_strlcpy(name, base, sizeof name);
        else snprintf(name, sizeof name, "%s %d", base, i);
        if (!of_path_join(into, j->dst, name)) return OF_ERR_INVALID;
        if (c->fs->stat(c->fs, into, &st) != OF_OK || st.type == OF_NONE) break;
    }
    if ((rc = of_mkdir(c, into)) != OF_OK) return rc;
    c->fs->list(c->fs, j->volume, name_add, &l);
    if (!(src = AllocVec((ULONG)(l.n + 1) * sizeof *src, MEMF_ANY | MEMF_CLEAR))) rc = OF_ERR_NOMEM;
    for (i = 0; rc == OF_OK && i < l.n; i++) {
        /* the disk's own icon stays: the new drawer has none of its own */
        if (!of_name_cmp(l.v[i], "Disk.info") || !of_path_join(path, j->volume, l.v[i])) continue;
        if (!(src[m] = AllocVec(strlen(path) + 1, MEMF_ANY))) { rc = OF_ERR_NOMEM; break; }
        strcpy(src[m++], path);
    }
    if (rc == OF_OK && m) rc = of_copy(c, (const char *const *)src, m, into);
    for (i = 0; src && i < m; i++) FreeVec(src[i]);
    if (src) FreeVec(src);
    for (i = 0; i < l.n; i++) FreeVec(l.v[i]);
    if (l.v) FreeVec(l.v);
    return rc;
}

/* The job's process */
static void job_entry(void)
{
    job *j = (job *)FindTask(NULL)->tc_UserData;
    of_fs fs;
    of_ctx c;
    of_fs_amiga_init(&fs);
    of_ctx_init(&c, &fs);
    c.user = j;
    c.ask = job_ask;
    c.progress = job_progress;
    c.report = job_report;
    CurrentTime(&j->t0s, &j->t0m);
    if (j->volume[0]) j->rc = copy_volume(&c, j);
    else if (j->move) j->rc = of_move(&c, (const char *const *)j->src, j->n, j->dst);
    else j->rc = of_copy(&c, (const char *const *)j->src, j->n, j->dst);
    job_window_close(j);
    if (!stopping && j->rc != OF_ERR_CANCEL && (j->failed || j->rc != OF_OK)) {
        struct EasyStruct es;
        es.es_StructSize = sizeof es;
        es.es_Flags = 0;
        es.es_Title = (UBYTE *)(j->move ? "Moving" : "Copying");
        es.es_GadgetFormat = (UBYTE *)"OK";
        if (j->failed) {
            es.es_TextFormat = (UBYTE *)"%ld could not be %s:\n%s:\n%s.";
            EasyRequest(NULL, &es, NULL, (LONG)j->failed, (ULONG)(j->move ? "moved" : "copied"), (ULONG)j->first_failed,
                        (ULONG)err_text(j->first_err));
        } else {
            es.es_TextFormat = (UBYTE *)"Nothing could be %s to\n%s:\n%s.";
            EasyRequest(NULL, &es, NULL, (ULONG)(j->move ? "moved" : "copied"), (ULONG)j->dst, (ULONG)err_text(j->rc));
        }
    }
    Forbid();
    jobs_running--;
    PutMsg(done_port, &j->msg);
    /* ends under Forbid: OpenBench, whose code this is, can't unload it before this process is gone */
}

static int job_run(job *j)
{
    struct Process *p;
    j->msg.mn_Length = sizeof *j;
    Forbid();
    p = CreateNewProcTags(NP_Entry, (ULONG)job_entry, NP_StackSize, 32768, NP_Name, (ULONG)"OpenBench copying",
                          TAG_DONE);
    if (p) {
        p->pr_Task.tc_UserData = j;
        jobs_running++;
    }
    Permit();
    return p != NULL;
}

/* A job for these sources, into dst. move: one volume. Drawers to read again: dst, and where each came from. */
static int job_for(char **src, int n, const char *dst, int move, const char *volume)
{
    job *j = AllocVec(sizeof *j, MEMF_PUBLIC | MEMF_CLEAR);
    int i;
    if (!j) return 0;
    j->move = move;
    of_strlcpy(j->dst, dst, OF_PATH_MAX);
    if (volume) of_strlcpy(j->volume, volume, OF_PATH_MAX);
    job_dir(j, dst);
    if (n && !(j->src = AllocVec((ULONG)n * sizeof *j->src, MEMF_ANY | MEMF_CLEAR))) { FreeVec(j); return 0; }
    for (i = 0; i < n; i++) {
        char parent[OF_PATH_MAX];
        if (!(j->src[i] = AllocVec(strlen(src[i]) + 1, MEMF_ANY))) { job_free(j); return 0; }
        strcpy(j->src[i], src[i]);
        j->n = i + 1;
        if (move && of_path_parent(parent, src[i])) job_dir(j, parent);
    }
    if (!job_run(j)) { job_free(j); return 0; }
    return 1;
}

/* Icons into the drawer dst: those on dst's volume moved, the rest copied. An icon already there, or dst
 * itself, is left. 1 when something was started. */
static int transfer(const char *const *paths, int n, const char *dst)
{
    char **mv, **cp, parent[OF_PATH_MAX];
    int nm = 0, nc = 0, i, ok = 0;
    if (n <= 0 || !(mv = AllocVec((ULONG)n * 2 * sizeof *mv, MEMF_ANY))) return 0;
    cp = mv + n;
    for (i = 0; i < n; i++) {
        const char *s = paths[i];
        if (!*of_path_name(s) || of_path_equal(s, dst)) continue;
        if (of_path_parent(parent, s) && of_path_equal(parent, dst)) continue;
        if (same_volume(s, dst)) mv[nm++] = (char *)s;
        else cp[nc++] = (char *)s;
    }
    if (nm) ok |= job_for(mv, nm, dst, 1, NULL);
    if (nc) ok |= job_for(cp, nc, dst, 0, NULL);
    FreeVec(mv);
    return ok;
}

/* ---- AppWindows ---- */

static int appwindow_at(struct Window *w, WORD sx, WORD sy, struct OWBAppWindowAt *at)
{
    struct TagItem t[2];
    memset(at, 0, sizeof *at);
    if (!app_port || !WorkbenchBase || WorkbenchBase->lib_Version < 44) return 0;
    at->aa_Window = w;
    at->aa_MouseX = (WORD)(sx - w->LeftEdge);
    at->aa_MouseY = (WORD)(sy - w->TopEdge);
    t[0].ti_Tag = OWBCTRLA_AppWindowAt;
    t[0].ti_Data = (ULONG)at;
    t[1].ti_Tag = TAG_DONE;
    t[1].ti_Data = 0;
    WorkbenchControlA(NULL, t);
    return at->aa_Port != NULL;
}

static void app_free(struct AppMessage *am)
{
    LONG i;
    for (i = 0; i < am->am_NumArgs; i++)
        if (am->am_ArgList[i].wa_Lock) UnLock(am->am_ArgList[i].wa_Lock);
    FreeVec(am);
}

/* The icons to the program whose AppWindow (or drop zone) is at (sx, sy): one AppMessage. A drawer or a disk
 * is its own lock and an empty name; a file its drawer's lock and its name (OS 3.2.3's, measured). */
static int appwindow_drop(struct Window *w, const char *const *paths, int n, WORD sx, WORD sy)
{
    struct OWBAppWindowAt at;
    struct AppMessage *am;
    struct WBArg *args;
    char *names, parent[OF_PATH_MAX];
    ULONG size = sizeof *am + (ULONG)n * sizeof *args;
    int i, k = 0;
    if (n <= 0 || !appwindow_at(w, sx, sy, &at)) return 0;
    for (i = 0; i < n; i++) size += strlen(of_path_name(paths[i])) + 1;
    if (!(am = AllocVec(size, MEMF_PUBLIC | MEMF_CLEAR))) return 0;
    args = (struct WBArg *)(am + 1);
    names = (char *)(args + n);
    am->am_ArgList = args;
    for (i = 0; i < n; i++) {
        const char *name = is_dir(paths[i]) ? "" : of_path_name(paths[i]);
        BPTR l = *name ? (of_path_parent(parent, paths[i]) ? Lock((CONST_STRPTR)parent, ACCESS_READ) : 0)
                       : Lock((CONST_STRPTR)paths[i], ACCESS_READ);
        if (!l) continue;
        strcpy(names, name);
        args[k].wa_Lock = l;
        args[k].wa_Name = (APTR)names;
        names += strlen(name) + 1;
        am->am_NumArgs = ++k;
    }
    if (!k) { FreeVec(am); return 0; }
    am->am_Message.mn_Node.ln_Type = NT_MESSAGE;
    am->am_Message.mn_ReplyPort = app_port;
    am->am_Message.mn_Length = sizeof *am;
    am->am_Version = AM_VERSION;
    CurrentTime(&am->am_Seconds, &am->am_Micros);
    /* asked again and sent with nothing else running, so the program can't take its AppWindow (and port)
     * away between the two */
    Forbid();
    if (!appwindow_at(w, sx, sy, &at)) {
        Permit();
        app_free(am);
        return 0;
    }
    am->am_Type = at.aa_Zone ? AMTYPE_APPWINDOWZONE : AMTYPE_APPWINDOW;
    am->am_ID = at.aa_ID;
    am->am_UserData = at.aa_UserData;
    am->am_MouseX = at.aa_MouseX;
    am->am_MouseY = at.aa_MouseY;
    PutMsg(at.aa_Port, &am->am_Message);
    apps_out++;
    Permit();
    return 1;
}

/* ---- drops ---- */

int ob_drop(const char *const *paths, int n, WORD sx, WORD sy, UWORD qual, wb_drawer *from)
{
    struct Window *w = ob_drop_window_at(sx, sy);
    wb_drawer *d;
    char dst[OF_PATH_MAX];
    (void)qual;
    if (!w || n <= 0 || stopping) return 0;
    if ((d = drawer_of(w))) {
        int at = wb_drawer_at(d, sx, sy, dst, sizeof dst);
        if (at == WB_AT_NONE || (d == from && at != WB_AT_DRAWER)) return 0;
        if (at == WB_AT_FILE) of_strlcpy(dst, wb_drawer_path(d), sizeof dst);   /* onto a tool: into its drawer */
        return transfer(paths, n, dst);
    }
    if (w == *env.root) {
        if (env.volume_at && env.volume_at(sx, sy, dst, sizeof dst)) return transfer(paths, n, dst);
        return 0;                               /* Leave Out: Phase B */
    }
    return appwindow_drop(w, paths, n, sx, sy);
}

int ob_drop_volume(const char *volume, WORD sx, WORD sy, UWORD qual)
{
    struct Window *w = ob_drop_window_at(sx, sy);
    wb_drawer *d;
    char dst[OF_PATH_MAX];
    (void)qual;
    if (!w || w == *env.root || stopping) return 0;
    if ((d = drawer_of(w))) {
        int at = wb_drawer_at(d, sx, sy, dst, sizeof dst);
        if (at == WB_AT_NONE) return 0;
        if (at == WB_AT_FILE) of_strlcpy(dst, wb_drawer_path(d), sizeof dst);
        if (of_path_inside(dst, volume)) return 0;      /* a disk into a drawer on itself */
        return job_for(NULL, 0, dst, 0, volume);
    }
    return appwindow_drop(w, &volume, 1, sx, sy);
}

/* ---- start, signals, stop ---- */

int ob_drop_start(const ob_drop_env *e)
{
    struct TagItem t[2];
    env = *e;
    stopping = 0;
    if (!(done_port = CreateMsgPort())) return 0;
    if (!(app_port = CreateMsgPort())) {
        DeleteMsgPort(done_port);
        done_port = NULL;
        return 0;
    }
    if (WorkbenchBase && WorkbenchBase->lib_Version >= 44) {
        t[0].ti_Tag = OWBCTRLA_DesktopDrops;
        t[0].ti_Data = TRUE;
        t[1].ti_Tag = TAG_DONE;
        t[1].ti_Data = 0;
        WorkbenchControlA(NULL, t);
    }
    return 1;
}

ULONG ob_drop_sigmask(void)
{
    return (done_port ? 1UL << done_port->mp_SigBit : 0) | (app_port ? 1UL << app_port->mp_SigBit : 0);
}

void ob_drop_handle(void)
{
    struct Message *m;
    while (done_port && (m = GetMsg(done_port))) {
        job *j = (job *)m;
        int i;
        if (!stopping)
            for (i = 0; i < j->ndirs; i++) reread(j->dirs[i]);
        job_free(j);
    }
    while (app_port && (m = GetMsg(app_port))) {
        app_free((struct AppMessage *)m);
        apps_out--;
    }
}

void ob_drop_stop(void)
{
    struct TagItem t[2];
    int i;
    if (!done_port) return;
    if (WorkbenchBase && WorkbenchBase->lib_Version >= 44) {
        t[0].ti_Tag = OWBCTRLA_DesktopDrops;
        t[0].ti_Data = FALSE;
        t[1].ti_Tag = TAG_DONE;
        t[1].ti_Data = 0;
        WorkbenchControlA(NULL, t);
    }
    /* the jobs stop where they are (the engine leaves the place a file was going as it was); the programs
     * get ten seconds to answer */
    stopping = 1;
    for (i = 0; i < 500 && (jobs_running || apps_out); i++) {
        ob_drop_handle();
        if (jobs_running || apps_out) Delay(1);
    }
    while (jobs_running) {                      /* a job can't be left running OpenBench's code (one asking
                                                 * about a name waits for its answer) */
        ob_drop_handle();
        if (jobs_running) Delay(5);
    }
    ob_drop_handle();
    DeleteMsgPort(done_port);
    done_port = NULL;
    if (!apps_out) DeleteMsgPort(app_port);
    else app_port->mp_Flags = PA_IGNORE;        /* left: a late reply goes to a port still there, and signals
                                                 * no one */
    app_port = NULL;
}
