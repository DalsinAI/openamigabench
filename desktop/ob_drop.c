/* ob_drop: icons let go on the desktop's windows (W67; Dale, 18 October 2026: "drag and drop doesn't seem to
 * work", "dragging icons 'into' windows"; 24 October 2026: icons dragged between windows "as Workbench does").
 * What OS 3.2.3's Workbench does is the spec: measured on lab t4-dnd (OS 3.2.3, stock ROM 47.115, LoadWB and
 * workbench.library 47.42, 24 October 2026), from the screen, the files and the lists only:
 *
 *   - into another drawer window (its space, or a file's icon in it), or onto a drawer's icon (in any window,
 *     this one too) or a disk's icon on the desktop: moved when it is on the same volume, copied to another;
 *   - the qualifier keys don't change that. Shift, Alt, Ctrl, either Amiga key, Right Alt or Right Shift held as
 *     the icons are let go (pressed on the lab's own keyboard, and as the events' qualifiers) gave the same move
 *     or copy as none. At the press, Shift adds to the selection (so several go), Alt takes the icon out of it
 *     (no drag starts), and Left Amiga is Intuition's screen drag;
 *   - a name already there, on a copy: Workbench's requester, "Error while copying "name" / object already
 *     exists", Replace, Replace all, Skip, Skip all, Cancel; on a move within the volume no question, the icon
 *     stays and the screen's title says "Error while moving name: (203) object already exists";
 *   - onto a trashcan's icon: what is on its volume goes into it; from another volume nothing happens;
 *   - onto the desktop's own space: left out there for this run (gone from its drawer, back in it after a
 *     reboot; .backdrop isn't written: only Icons > Leave Out writes it, ":drawer/name" a line);
 *   - a disk's icon into a drawer: a new drawer named after the disk, with what the disk holds;
 *   - a copy shows its progress ("Copying name", a bar, Stop) after a moment; Stop leaves the files done and
 *     takes away the one being written;
 *   - a full disk: the copy starts, and DOS asks "Volume X is full", Retry or Cancel (room can be made and the
 *     copy goes on); a write-protected disk: DOS's "Volume X is write protected", Retry or Cancel;
 *   - onto a program's AppWindow (or one of its drop zones): one AppMessage, AMTYPE_APPWINDOW (or
 *     AMTYPE_APPWINDOWZONE) with the AppWindow's (or the zone's) id and userdata, the place in the window,
 *     and one WBArg per icon: a drawer or disk as its own lock and an empty name, a file as its drawer's
 *     lock and its name (0.1.26, lab t5-mouse). OpenBench sends it (it is the desktop) and frees it, unlocking,
 *     when the program replies. Which AppWindow is under the pointer, workbench.library says (the Team's
 *     OWBCTRLA_AppWindowAt; amigachrome-guest libraries/openworkbench/owb_team.h);
 *   - onto a tool's icon: nothing is started (it goes into that drawer); in its own drawer, off a drawer's
 *     icon: nothing (OpenBench's drawers keep their icons in order).
 *
 * The copies and moves are OpenFiles' engine's (openamigafilemanager engine/of_ops.c), as OpenBench's design
 * says (5.3), and so they differ from 3.2.3's on purpose where the engine keeps files safe: a file goes with its
 * .info, a copy is written under a temporary name until it is whole (so nothing half written is ever left, and
 * there is no "Remove incomplete object?" to ask), a file replaced goes to the trash, a move between volumes is a
 * copy read back before the source goes; a drawer whose copy is stopped half way keeps the files done and is given
 * its icon, so it shows. Each drop runs in a process of its own, so the desktop never waits for
 * a disk (3.2.3's Workbench waits for its copy); when it is done, the drawers it touched are read again.
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
#include <proto/icon.h>

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

/* ---- the jobs: a copy, a move or a trip to the trash, in a process of its own ---- */

#define JOB_DIRS 8                              /* drawers to read again afterwards: where it went, where from */
#define WIN_W 440

enum { OP_COPY = 0, OP_MOVE, OP_TRASH, OP_DISK };

typedef struct job {
    struct Message msg;                         /* to done_port when it ends */
    struct Screen *scr;                         /* the job's: locked while its window is up */
    int op;                                     /* OP_*: a copy, a move (one volume), to the trash, a disk */
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
    int answered;                               /* the user said Cancel to a full or write-protected disk */
    char first_failed[OF_PATH_MAX];
    char into[OF_PATH_MAX];                     /* a disk's contents: the drawer made for them (OP_DISK) */
    struct of_fs *fs;                           /* the job's file system, for a file tried again */
} job;

static const char *op_doing(const job *j)
{
    return j->op == OP_MOVE ? "Moving" : j->op == OP_TRASH ? "Moving to the trash" : "Copying";
}

static const char *op_done(const job *j)
{
    return j->op == OP_MOVE ? "moved" : j->op == OP_TRASH ? "put in the trash" : "copied";
}

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
                            WA_Height, h, WA_Title, (ULONG)op_doing(j), WA_DragBar, TRUE,
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
    snprintf(line, sizeof line, "%s %lu of %lu to %s", op_doing(j), (unsigned long)p->files_done + 1,
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

/* A move within the volume onto a name that is taken: not asked about (3.2.3 doesn't ask), the end says so */
static void job_report_clash(job *j, const char *src)
{
    if (!j->failed++) {
        of_strlcpy(j->first_failed, src, OF_PATH_MAX);
        j->first_err = OF_ERR_EXISTS;
    }
}

/* A name that is taken. A copy asks as OS 3.2.3's Workbench asks (its words and its five answers; Replace sends
 * the old one to the trash, the engine's rule). A move within the volume isn't asked about, as on 3.2.3: that
 * one stays where it is, and the end says why (job_report). */
static int job_ask(void *user, const of_clash *c)
{
    job *j = user;
    struct EasyStruct es;
    LONG r;
    if (j->op != OP_COPY && j->op != OP_DISK) {
        job_report_clash(j, c->src);
        return OF_CLASH_SKIP;
    }
    es.es_StructSize = sizeof es;
    es.es_Flags = 0;
    es.es_Title = (UBYTE *)"Workbench";
    es.es_TextFormat = (UBYTE *)"Error while copying \"%s\"\nobject already exists";
    es.es_GadgetFormat = (UBYTE *)"Replace|Replace all|Skip|Skip all|Cancel";
    r = EasyRequest(j->win, &es, NULL, (ULONG)of_path_name(c->dst));
    switch (r) {
    case 1: return OF_CLASH_REPLACE;
    case 2: return OF_CLASH_REPLACE | OF_CLASH_ALL;
    case 3: return OF_CLASH_SKIP;
    case 4: return OF_CLASH_SKIP | OF_CLASH_ALL;
    default: return OF_CLASH_CANCEL;
    }
}

/* The drawer in the job's destination that the file path (one the engine was copying) goes into: its drawer's place
 * under the source's drawer, under the destination. 0 when path isn't one of the job's sources' (a drawer the engine
 * was making, say). */
static int dest_dir_of(const job *j, const char *path, char *out)
{
    char base[OF_PATH_MAX], par[OF_PATH_MAX];
    const char *to = j->op == OP_DISK ? j->into : j->dst, *rel;
    size_t n;
    int i;
    if (!of_path_parent(par, path)) return 0;
    if (j->op == OP_DISK) of_strlcpy(base, j->volume, OF_PATH_MAX);
    else {
        for (i = 0; i < j->n && !of_path_inside(path, j->src[i]); i++) ;
        if (i == j->n || !of_path_parent(base, j->src[i])) return 0;
    }
    if (of_path_equal(par, base)) {
        of_strlcpy(out, to, OF_PATH_MAX);
        return 1;
    }
    n = strlen(base);
    if (strlen(par) <= n) return 0;
    for (i = 0; i < (int)n; i++) {
        char a = par[i], b = base[i];
        if (a >= 'A' && a <= 'Z') a += 32;
        if (b >= 'A' && b <= 'Z') b += 32;
        if (a != b) return 0;
    }
    rel = par + n;
    if (*rel == '/') rel++;
    return *rel && of_path_join(out, to, rel);
}

static void retry_report(void *user, const char *path, int err)
{
    (void)path;
    *(int *)user = err;
}

/* The name of the volume dst is on, for the question ("DnDRad"); dst's own volume part when it can't be locked */
static void volume_name(const char *dst, char *out, int size)
{
    BPTR l = Lock((CONST_STRPTR)dst, ACCESS_READ);
    char *c;
    if (!l || !NameFromLock(l, (STRPTR)out, size)) of_strlcpy(out, dst, size);
    if (l) UnLock(l);
    if ((c = strchr(out, ':'))) *c = 0;
}

/* A file that couldn't be written because its disk is full or write-protected: asked about in the job's own
 * requester, Retry or Cancel, as OS 3.2.3's DOS asks (its words: "Volume DnDRad is full"), so room can be made, or
 * the disk unprotected, and the copy goes on (the user, 9 October 2026: "yes to full disk behavour"; the design's
 * 5.11: only this job waits). The file the engine gave up on (its temporary copy already taken away) is copied
 * again into its drawer on Retry, its icon with it. 1 when it went at last; 0 on Cancel (the rest of the job isn't
 * tried, as 3.2.3 stops its copy there). The Team's own file systems don't ask, and the job's process lets no file
 * system ask (pr_WindowPtr -1), so the question is asked once, here, whatever the disk. */
static int retry_write(job *j, const char *path, int err)
{
    char dir[OF_PATH_MAX], vol[64];
    struct EasyStruct es;
    of_ctx c2;
    int got;
    if (j->op != OP_COPY && j->op != OP_DISK) return 0;
    volume_name(j->op == OP_DISK ? j->into : j->dst, vol, sizeof vol);
    for (;;) {
        es.es_StructSize = sizeof es;
        es.es_Flags = 0;
        es.es_Title = (UBYTE *)"System Request";
        es.es_TextFormat = (UBYTE *)(err == OF_ERR_FULL ? "Volume\n%s\nis full" : "Volume\n%s\nis write protected");
        es.es_GadgetFormat = (UBYTE *)"Retry|Cancel";
        if (!EasyRequest(j->win, &es, NULL, (ULONG)vol)) return 0;
        if (!dest_dir_of(j, path, dir)) return 0;
        of_ctx_init(&c2, j->fs);
        got = OF_OK;
        c2.user = &got;
        c2.ask = NULL;
        c2.clash = OF_CLASH_REPLACE;            /* its own temporary copy was taken away: nothing of it is there */
        c2.report = retry_report;
        if (of_copy(&c2, &path, 1, dir) == OF_OK && got == OF_OK) return 1;
        if (got != OF_ERR_FULL && got != OF_ERR_READONLY) return 0;
        err = got;
    }
}

static void job_report(void *user, const char *path, int err)
{
    job *j = user;
    if ((err == OF_ERR_FULL || err == OF_ERR_READONLY) && !j->stopped && !stopping) {
        if (retry_write(j, path, err)) return;
        j->answered = 1;
    }
    if (!j->failed++) {
        of_strlcpy(j->first_failed, path, OF_PATH_MAX);
        j->first_err = err;
    }
    /* a full or write-protected disk the user didn't make room on: the rest isn't tried, as 3.2.3 stops its copy */
    if (err == OF_ERR_FULL || err == OF_ERR_READONLY) j->stopped = 1;
}

static const char *err_text(int e)
{
    switch (e) {
    case OF_ERR_NOTFOUND: return "it isn't there any more";
    case OF_ERR_EXISTS: return "object already exists";
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
    of_strlcpy(j->into, into, OF_PATH_MAX);
    c->fs->list(c->fs, j->volume, name_add, &l);
    if (!(src = AllocVec((ULONG)(l.n + 1) * sizeof *src, MEMF_ANY | MEMF_CLEAR))) rc = OF_ERR_NOMEM;
    for (i = 0; rc == OF_OK && i < l.n; i++) {
        /* the disk's own icon stays on the disk (3.2.3 copies it in, as a file, and the new drawer is left without
         * an icon of its own, so it can't be seen until Show All Files): the new drawer gets a drawer's icon below */
        if (!of_name_cmp(l.v[i], "Disk.info") || !of_path_join(path, j->volume, l.v[i])) continue;
        if (!(src[m] = AllocVec(strlen(path) + 1, MEMF_ANY))) { rc = OF_ERR_NOMEM; break; }
        strcpy(src[m++], path);
    }
    if (rc == OF_OK && m) rc = of_copy(c, (const char *const *)src, m, into);
    /* the new drawer's icon: the default drawer's, placed by the window that shows it (kept even when the copy
     * stopped part-way, as the drawer and what was copied stay) */
    if ((k = (int)strlen(into)) + 6 <= (int)sizeof path) {
        struct DiskObject *o;
        memcpy(path, into, (size_t)k);
        memcpy(path + k, ".info", 6);
        if (c->fs->stat(c->fs, path, &st) != OF_OK || st.type == OF_NONE) {
            if ((o = GetDefDiskObject(WBDRAWER))) {
                o->do_CurrentX = NO_ICON_POSITION;
                o->do_CurrentY = NO_ICON_POSITION;
                PutDiskObject((STRPTR)into, o);
                FreeDiskObject(o);
            }
        }
    }
    for (i = 0; src && i < m; i++) FreeVec(src[i]);
    if (src) FreeVec(src);
    for (i = 0; i < l.n; i++) FreeVec(l.v[i]);
    if (l.v) FreeVec(l.v);
    return rc;
}

/* The space left on a disk, as the engine asks before a copy: not told, so the copy starts and DOS says when the
 * disk is full, with Retry, as on OS 3.2.3 (OpenBench's design 5.11: room can be made and the job goes on). The
 * engine, told nothing, lets the writes say. */
static int no_free_bytes(of_fs *fs, const char *path, uint64_t *out)
{
    (void)fs;
    (void)path;
    *out = 0;
    return OF_ERR_IO;
}

/* The trash drawer the engine keeps on dst's volume ("Work:Trashcan"): of_delete files them there, with a note of
 * where each was, so OpenFiles can put them back */
static int is_engine_trash(of_ctx *c, const char *dst)
{
    char vol[OF_PATH_MAX], t[OF_PATH_MAX];
    return of_path_volume(vol, dst) && of_path_join(t, vol, c->trash) && of_path_equal(t, dst);
}

/* A copy stopped part-way (Stop, or Cancel at a name already there) keeps what was copied, as OS 3.2.3's does; but
 * the engine gives a drawer its icon only once everything in it is done, so a drawer stopped half way would be left
 * without one and couldn't be seen in its window. Each such drawer gets its source's icon (the drawer and the files
 * in it are whole: nothing half written is left) */
static void stopped_icons(job *j)
{
    int i;
    for (i = 0; i < j->n; i++) {
        char to[OF_PATH_MAX], ti[OF_PATH_MAX + 6];
        const char *name = of_path_name(j->src[i]);
        struct DiskObject *o;
        BPTR l;
        size_t k;
        if (!*name || !of_path_join(to, j->dst, name) || (k = strlen(to)) + 6 > sizeof ti) continue;
        if (!is_dir(j->src[i]) || !is_dir(to)) continue;
        memcpy(ti, to, k);
        memcpy(ti + k, ".info", 6);
        if ((l = Lock((CONST_STRPTR)ti, ACCESS_READ))) {
            UnLock(l);
            continue;
        }
        if ((o = GetDiskObject((CONST_STRPTR)j->src[i]))) {
            PutDiskObject((CONST_STRPTR)to, o);
            FreeDiskObject(o);
        }
    }
}

/* The job's process */
static void job_entry(void)
{
    job *j = (job *)FindTask(NULL)->tc_UserData;
    of_fs fs;
    of_ctx c;
    of_fs_amiga_init(&fs);
    fs.free_bytes = no_free_bytes;
    of_ctx_init(&c, &fs);
    c.user = j;
    c.ask = job_ask;
    c.progress = job_progress;
    c.report = job_report;
    j->fs = &fs;
    /* no file system asks in this process: a full or write-protected disk is asked about by the job (retry_write) */
    ((struct Process *)FindTask(NULL))->pr_WindowPtr = (APTR)-1;
    CurrentTime(&j->t0s, &j->t0m);
    if (j->op == OP_DISK) j->rc = copy_volume(&c, j);
    else if (j->op == OP_TRASH && is_engine_trash(&c, j->dst)) j->rc = of_delete(&c, (const char *const *)j->src, j->n);
    else if (j->op == OP_MOVE || j->op == OP_TRASH) j->rc = of_move(&c, (const char *const *)j->src, j->n, j->dst);
    else j->rc = of_copy(&c, (const char *const *)j->src, j->n, j->dst);
    if (j->rc == OF_ERR_IO && j->failed) j->rc = OF_OK;     /* of_delete's word for "some failed": said below */
    if ((j->op == OP_COPY || j->op == OP_MOVE) && (j->stopped || stopping || j->rc == OF_ERR_CANCEL)) stopped_icons(j);
    job_window_close(j);
    /* the summary, unless the user stopped it, or said Cancel to a full or write-protected disk (3.2.3 says no more
     * then either: its screen title has the error) */
    if (!stopping && !j->answered && j->rc != OF_ERR_CANCEL && (j->failed || j->rc != OF_OK)) {
        struct EasyStruct es;
        es.es_StructSize = sizeof es;
        es.es_Flags = 0;
        es.es_Title = (UBYTE *)"Workbench";
        es.es_GadgetFormat = (UBYTE *)"OK";
        if (j->failed == 1) {
            es.es_TextFormat = (UBYTE *)"Error while %s \"%s\"\n%s";
            EasyRequest(NULL, &es, NULL, (ULONG)(j->op == OP_COPY || j->op == OP_DISK ? "copying" : "moving"),
                        (ULONG)of_path_name(j->first_failed), (ULONG)err_text(j->first_err));
        } else if (j->failed) {
            es.es_TextFormat = (UBYTE *)"%ld could not be %s:\n%s:\n%s.";
            EasyRequest(NULL, &es, NULL, (LONG)j->failed, (ULONG)op_done(j), (ULONG)j->first_failed,
                        (ULONG)err_text(j->first_err));
        } else {
            es.es_TextFormat = (UBYTE *)"Nothing could be %s to\n%s:\n%s.";
            EasyRequest(NULL, &es, NULL, (ULONG)op_done(j), (ULONG)j->dst, (ULONG)err_text(j->rc));
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

/* A job for these sources, into dst. Drawers to read again: dst, and where each came from when they go. */
static int job_for(char **src, int n, const char *dst, int op, const char *volume)
{
    job *j = AllocVec(sizeof *j, MEMF_PUBLIC | MEMF_CLEAR);
    int i;
    if (!j) return 0;
    j->op = op;
    of_strlcpy(j->dst, dst, OF_PATH_MAX);
    if (volume) of_strlcpy(j->volume, volume, OF_PATH_MAX);
    job_dir(j, dst);
    if (n && !(j->src = AllocVec((ULONG)n * sizeof *j->src, MEMF_ANY | MEMF_CLEAR))) { FreeVec(j); return 0; }
    for (i = 0; i < n; i++) {
        char parent[OF_PATH_MAX];
        if (!(j->src[i] = AllocVec(strlen(src[i]) + 1, MEMF_ANY))) { job_free(j); return 0; }
        strcpy(j->src[i], src[i]);
        j->n = i + 1;
        if (op != OP_COPY && of_path_parent(parent, src[i])) job_dir(j, parent);
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
    if (nm) ok |= job_for(mv, nm, dst, OP_MOVE, NULL);
    if (nc) ok |= job_for(cp, nc, dst, OP_COPY, NULL);
    FreeVec(mv);
    return ok;
}

/* 1 when the drawer path is a trashcan: its icon is a trashcan's (WBGARBAGE), as Workbench knows one */
static int is_trash(const char *path)
{
    struct DiskObject *o;
    int t = 0;
    if (!*of_path_name(path) || !(o = GetDiskObject((CONST_STRPTR)path))) return 0;
    t = o->do_Type == WBGARBAGE;
    FreeDiskObject(o);
    return t;
}

/* Icons onto a trashcan: those on its volume go into it (OS 3.2.3's); from another volume nothing happens, as
 * on 3.2.3. 1 when something was started. */
static int to_trash(const char *const *paths, int n, const char *trash)
{
    char **tr, parent[OF_PATH_MAX];
    int nt = 0, i, ok = 0;
    if (n <= 0 || !(tr = AllocVec((ULONG)n * sizeof *tr, MEMF_ANY))) return 0;
    for (i = 0; i < n; i++) {
        const char *s = paths[i];
        if (!*of_path_name(s) || of_path_inside(trash, s)) continue;    /* a disk, or the trashcan itself */
        if (of_path_parent(parent, s) && of_path_equal(parent, trash)) continue;
        if (same_volume(s, trash)) tr[nt++] = (char *)s;
    }
    if (nt) ok = job_for(tr, nt, trash, OP_TRASH, NULL);
    FreeVec(tr);
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
    (void)qual;                                 /* no qualifier changes a drop on OS 3.2.3 (measured) */
    if (!w || n <= 0 || stopping) return 0;
    if ((d = drawer_of(w))) {
        int at = wb_drawer_at(d, sx, sy, dst, sizeof dst);
        if (at == WB_AT_NONE || (d == from && at != WB_AT_DRAWER)) return 0;
        if (at == WB_AT_FILE) of_strlcpy(dst, wb_drawer_path(d), sizeof dst);   /* onto a tool: into its drawer */
        if (at == WB_AT_DRAWER && is_trash(dst)) return to_trash(paths, n, dst);
        return transfer(paths, n, dst);
    }
    if (w == *env.root) {
        int k = env.icon_at ? env.icon_at(sx, sy, dst, sizeof dst) : OB_DESK_NONE;
        if (k == OB_DESK_DISK || k == OB_DESK_DRAWER) return transfer(paths, n, dst);
        if (k == OB_DESK_TRASH) return to_trash(paths, n, dst);
        /* the desktop's own space (or a tool left out there): left out, for this run, as on 3.2.3 */
        return from && env.leave_out ? env.leave_out(paths, n, sx, sy) : 0;
    }
    return appwindow_drop(w, paths, n, sx, sy);
}

int ob_drop_volume(const char *volume, WORD sx, WORD sy, UWORD qual)
{
    struct Window *w = ob_drop_window_at(sx, sy);
    wb_drawer *d;
    char dst[OF_PATH_MAX];
    (void)qual;
    if (!w || stopping) return 0;
    if (w == *env.root) {
        /* onto a drawer left out on the desktop (a disk's icon onto another disk's only moves it there) */
        if (env.icon_at && env.icon_at(sx, sy, dst, sizeof dst) == OB_DESK_DRAWER && !of_path_inside(dst, volume))
            return job_for(NULL, 0, dst, OP_DISK, volume);
        return 0;
    }
    if ((d = drawer_of(w))) {
        int at = wb_drawer_at(d, sx, sy, dst, sizeof dst);
        if (at == WB_AT_NONE) return 0;
        if (at == WB_AT_FILE) of_strlcpy(dst, wb_drawer_path(d), sizeof dst);
        if (of_path_inside(dst, volume) || (at == WB_AT_DRAWER && is_trash(dst))) return 0;  /* a disk into itself */
        return job_for(NULL, 0, dst, OP_DISK, volume);
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
    int ended = 0;
    while (done_port && (m = GetMsg(done_port))) {
        job *j = (job *)m;
        int i;
        if (!stopping)
            for (i = 0; i < j->ndirs; i++) reread(j->dirs[i]);
        job_free(j);
        ended = 1;
    }
    if (ended && !stopping && env.changed) env.changed();   /* icons left out may have gone with their files */
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
