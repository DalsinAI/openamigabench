/* alohaprobe: what Intuition tells a Workbench, and when (the native screen mode's step 4, 12 October 2026).
 * For the lab only. The design asked, before OpenBench took Intuition's Workbench messages: does Intuition
 * send WBENCHCLOSE before it looks for other programs' windows, and WBENCHOPEN again when the close fails?
 * Is that the same on OS 3.2.3?
 *
 *   alohaprobe INFO       the Workbench screen (size, ModeID), the windows on it, the pointer and free memory
 *   alohaprobe PROBE      registers as the Workbench (AlohaWorkbench), has a process of its own call
 *                         CloseWorkBench() and then OpenWorkBench(), and lists in order every message Intuition
 *                         sends, each call's result, and whether the screen was there at each step; then gives
 *                         Intuition's Workbench port back (AlohaWorkbench(NULL)). Only where no Workbench is
 *                         registered: not with LoadWB's Workbench, nor an OpenBench from 0.1.22 on
 *   alohaprobe RESET      CloseWorkBench() and then OpenWorkBench() from the Shell, as IPrefs does for a new
 *                         screen mode, and their results
 *   ... WAIT n            seconds PROBE waits for each call (20): a call stuck in a requester is reported
 *   ... WINDOW            PROBE with a window of its own on the Workbench screen, as another program's, open
 *                         through the close
 *   ... WBWINDOW          PROBE with a window of its own marked as the Workbench's (WFLG_WBENCHWINDOW), closed
 *                         when Intuition says WBENCHCLOSE, as a Workbench closes its drawers
 *   alohaprobe MENU       the Workbench screen's backdrop window's menus (the desktop's), each item enabled or not
 *   alohaprobe PICK m/i   item i of menu m of that window picked, as the mouse would (an IDCMP_MENUPICK sent to
 *                         the window's port): the lab's input can click but not hold the menu button
 *   alohaprobe OPEN path  OpenBench asked to open path (a drawer opens in a drawer window), as the dock asks it
 *                         (openamigaprefs Common/obmsg.h)
 *
 * MIT, Copyright (c) 2026 AmigaChrome Team. */
#include <string.h>
#include <exec/types.h>
#include <exec/memory.h>
#include <exec/ports.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <dos/rdargs.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <intuition/screens.h>
#include <graphics/modeid.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>

static const char version[] __attribute__((used)) =
    "$VER: alohaprobe 0.1 (12.10.2026) OpenBench, MIT, Copyright (c) 2026 AmigaChrome Team";

#define TEMPLATE "INFO/S,PROBE/S,RESET/S,WAIT/K/N,WINDOW/S,WBWINDOW/S,MENU/S,PICK/K,OPEN/K"
enum { A_INFO, A_PROBE, A_RESET, A_WAIT, A_WINDOW, A_WBWINDOW, A_MENU, A_PICK, A_OPEN, A_COUNT };

/* Printf with a plain string for its format */
#define say(fmt, ...) Printf((CONST_STRPTR)(fmt), ##__VA_ARGS__)

struct IntuitionBase *IntuitionBase;
struct GfxBase *GfxBase;

/* The Workbench screen, found without a lock (a lock would keep it open) and without LockIBase (Intuition
 * may hold it while it waits for our answer): under Forbid, by its flag. */
static struct Screen *wb_screen(ULONG *w, ULONG *h)
{
    struct Screen *s;
    Forbid();
    for (s = IntuitionBase->FirstScreen; s; s = s->NextScreen)
        if ((s->Flags & SCREENTYPE) == WBENCHSCREEN) break;
    if (s && w) { *w = (ULONG)s->Width; *h = (ULONG)s->Height; }
    Permit();
    return s;
}

static void info(void)
{
    struct Screen *s = LockPubScreen((CONST_STRPTR)"Workbench");
    if (!s) { say("No Workbench screen.\n"); return; }
    {
        ULONG id = GetVPModeID(&s->ViewPort), lock;
        struct Window *w;
        int n = 0;
        say("Workbench screen: %ldx%ld, ModeID $%08lx, depth %ld; pointer at %ld,%ld\n", (LONG)s->Width,
               (LONG)s->Height, id, (LONG)s->RastPort.BitMap->Depth, (LONG)s->MouseX, (LONG)s->MouseY);
        lock = LockIBase(0);
        for (w = s->FirstWindow; w; w = w->NextWindow, n++)
            say("  window %ld: \"%s\" %ldx%ld at %ld,%ld%s\n", (LONG)n, w->Title ? (LONG)w->Title : (LONG)"",
                   (LONG)w->Width, (LONG)w->Height, (LONG)w->LeftEdge, (LONG)w->TopEdge,
                   (w->Flags & WFLG_BACKDROP) ? (LONG)" (backdrop)" : (LONG)"");
        UnlockIBase(lock);
    }
    UnlockPubScreen(NULL, s);
    say("Free: chip %lu, fast %lu, largest %lu\n", AvailMem(MEMF_CHIP), AvailMem(MEMF_FAST), AvailMem(MEMF_LARGEST | MEMF_ANY));
}

/* ---- PROBE: an event log, filled by the main process and the caller's process, printed at the end ---- */

#define EVENTS 32
static struct { struct DateStamp at; char what[72]; } events[EVENTS];
static volatile int nevents;

static void note(const char *what, LONG v)
{
    int i;
    Forbid();
    i = nevents < EVENTS ? nevents++ : -1;
    Permit();
    if (i < 0) return;
    DateStamp(&events[i].at);
    {
        char *d = events[i].what;
        int k = 0;
        while (*what && k < 56) d[k++] = *what++;
        if (v != -1) {
            char num[12];
            int j = 0;
            ULONG u = (ULONG)(v < 0 ? -v : v);
            do { num[j++] = (char)('0' + u % 10); u /= 10; } while (u && j < 11);
            d[k++] = ' ';
            if (v < 0) d[k++] = '-';
            while (j) d[k++] = num[--j];
        }
        d[k] = 0;
    }
}

static struct Task *main_task;
static struct Window *own_window;           /* WINDOW or WBWINDOW */
static int own_is_wb;
static volatile ULONG done_sig, call_which;
static volatile LONG call_result;

/* The windows on the Workbench screen and the locks on it, noted (under Forbid: Intuition may be busy). */
static void note_windows(const char *who)
{
    struct Screen *s = wb_screen(NULL, NULL);
    struct Window *w;
    char line[64];
    int n = 0;
    struct List *l;
    struct PubScreenNode *p;
    LONG visitors = -1;
    if (!s) return;
    l = LockPubScreenList();
    for (p = (struct PubScreenNode *)l->lh_Head; p->psn_Node.ln_Succ; p = (struct PubScreenNode *)p->psn_Node.ln_Succ)
        if (p->psn_Screen == s) visitors = p->psn_VisitorCount;
    UnlockPubScreenList();
    strcpy(line, who);
    strcat(line, ": locks on the screen:");
    note(line, visitors);
    Forbid();
    for (w = s->FirstWindow; w && n < 8; w = w->NextWindow, n++) {
        int k = 0;
        const char *t = w->Title ? (const char *)w->Title : "(no title)";
        strcpy(line, who);
        strcat(line, ": window ");
        k = (int)strlen(line);
        while (*t && k < 50) line[k++] = *t++;
        line[k] = 0;
        Permit();
        note(line, (LONG)(w->Flags & WFLG_WBENCHWINDOW ? 1 : 0));
        Forbid();
    }
    Permit();
}

static void caller_entry(void)
{
    ULONG w = 0, h = 0;
    if (call_which == 0) {
        note("caller: CloseWorkBench() called; screen there:", wb_screen(NULL, NULL) != NULL);
        call_result = CloseWorkBench();
        note("caller: CloseWorkBench() returned", call_result);
        if (!call_result) note_windows("caller");
    } else {
        note("caller: OpenWorkBench() called; screen there:", wb_screen(NULL, NULL) != NULL);
        call_result = (LONG)OpenWorkBench();
        note("caller: OpenWorkBench() returned non-zero:", call_result != 0);
    }
    if (wb_screen(&w, &h)) { note("caller: the screen now, width", (LONG)w); note("caller: and height", (LONG)h); }
    else note("caller: no Workbench screen now", -1);
    Forbid();
    Signal(main_task, done_sig);
    /* ends in dos.library under Forbid */
}

/* One call made by a process of its own while this one answers Intuition's messages. 1 when it ended. */
static int call_and_answer(struct MsgPort *port, ULONG which, LONG secs)
{
    ULONG ticks = 0, limit = (ULONG)secs * 50;
    int ended = 0;
    call_which = which;
    SetSignal(0, done_sig);
    if (!CreateNewProcTags(NP_Entry, (ULONG)caller_entry, NP_StackSize, 8192, NP_Name, (ULONG)"alohaprobe caller", TAG_DONE)) {
        note("could not start the caller", -1);
        return 0;
    }
    /* the call's messages, then a second more for any that come after it */
    for (;;) {
        struct IntuiMessage *im;
        ULONG sigs = SetSignal(0, 0);
        while ((im = (struct IntuiMessage *)GetMsg(port))) {
            if (im->Class == IDCMP_WBENCHMESSAGE && im->Code == WBENCHCLOSE && own_window && own_is_wb) {
                CloseWindow(own_window);    /* as a Workbench closes its drawers */
                own_window = NULL;
                note("our Workbench window closed before the answer", -1);
            }
            if (im->Class == IDCMP_WBENCHMESSAGE)
                note(im->Code == WBENCHOPEN ? "Intuition: WBENCHOPEN; screen there:" :
                     im->Code == WBENCHCLOSE ? "Intuition: WBENCHCLOSE; screen there:" : "Intuition: a Workbench message, code",
                     im->Code == WBENCHOPEN || im->Code == WBENCHCLOSE ? (LONG)(wb_screen(NULL, NULL) != NULL) : (LONG)im->Code);
            else
                note("Intuition: another message, class", (LONG)im->Class);
            ReplyMsg(&im->ExecMessage);
        }
        if (sigs & done_sig) { SetSignal(0, done_sig); ended = 1; limit = ticks + 50; }
        if (ticks >= limit) break;
        Delay(1);
        ticks++;
    }
    if (!ended) note("the call hasn't returned after seconds:", secs);
    return ended;
}

static int probe(LONG secs, int window, int wbwindow)
{
    struct MsgPort *port = CreateMsgPort();
    BYTE sig = AllocSignal(-1);
    int i;
    if (!port || sig < 0) { if (port) DeleteMsgPort(port); if (sig >= 0) FreeSignal(sig); return RETURN_FAIL; }
    main_task = FindTask(NULL);
    done_sig = 1UL << sig;
    AlohaWorkbench((LONG)port);
    note("registered as the Workbench (AlohaWorkbench)", -1);
    note_windows("before");
    if (window || wbwindow) {
        own_is_wb = wbwindow;
        own_window = OpenWindowTags(NULL, WA_PubScreenName, (ULONG)"Workbench", WA_Left, 200, WA_Top, 200, WA_Width, 300,
                                    WA_Height, 100, WA_Title, (ULONG)(wbwindow ? "alohaprobe: a Workbench window" :
                                    "alohaprobe: another program's window"), WA_DragBar, TRUE, WA_DepthGadget, TRUE,
                                    WA_Flags, wbwindow ? WFLG_WBENCHWINDOW : 0, TAG_DONE);
        note(own_window ? (wbwindow ? "our window open, WFLG_WBENCHWINDOW; its Flags bit set:" : "our window open, a plain one")
                        : "our window could not be opened", own_window && wbwindow ? (LONG)((own_window->Flags & WFLG_WBENCHWINDOW) != 0) : -1);
    }
    if (call_and_answer(port, 0, secs)) {
        if (!wb_screen(NULL, NULL) || call_result) call_and_answer(port, 1, secs);
        else note("the close failed and the screen is there: OpenWorkBench() not called", -1);
    }
    if (own_window) { CloseWindow(own_window); own_window = NULL; note("our window closed", -1); }
    AlohaWorkbench(0);
    note("Intuition's Workbench port given back (AlohaWorkbench(NULL))", -1);
    {   /* anything left (none expected) is answered */
        struct Message *m;
        while ((m = GetMsg(port))) ReplyMsg(m);
    }
    for (i = 0; i < nevents; i++)
        say("%3ld.%02ld  %s\n", (LONG)(events[i].at.ds_Minute % 60 * 60 + events[i].at.ds_Tick / 50) % 1000,
               (LONG)(events[i].at.ds_Tick % 50 * 2), (LONG)events[i].what);
    /* a caller still in a requester has our code: we can't go before it */
    while (!(SetSignal(0, 0) & done_sig) && nevents && strstr(events[nevents - 1].what, "hasn't returned")) {
        say("Waiting for the caller to return (answer its requester)...\n");
        Wait(done_sig);
    }
    FreeSignal(sig);
    DeleteMsgPort(port);
    Flush(Output());
    return RETURN_OK;
}

/* The Workbench screen's backdrop window (the desktop's root), locked screen in *scr. */
static struct Window *desktop_window(struct Screen **scr)
{
    struct Window *w;
    *scr = LockPubScreen((CONST_STRPTR)"Workbench");
    if (!*scr) return NULL;
    for (w = (*scr)->FirstWindow; w; w = w->NextWindow)
        if ((w->Flags & WFLG_BACKDROP) && w->MenuStrip) return w;
    return NULL;
}

static void menus(void)
{
    struct Screen *s;
    struct Window *w = desktop_window(&s);
    struct Menu *m;
    int mi = 0;
    if (!w) { say("No backdrop window with menus on the Workbench screen.\n"); if (s) UnlockPubScreen(NULL, s); return; }
    for (m = w->MenuStrip; m; m = m->NextMenu, mi++) {
        struct MenuItem *it;
        int ii = 0;
        say("menu %ld: %s\n", (LONG)mi, (LONG)m->MenuName);
        for (it = m->FirstItem; it; it = it->NextItem, ii++)
            if (it->Flags & ITEMTEXT)
                say("  %ld/%ld: %s%s\n", (LONG)mi, (LONG)ii, (LONG)((struct IntuiText *)it->ItemFill)->IText,
                    (it->Flags & ITEMENABLED) ? (LONG)"" : (LONG)"  (greyed)");
    }
    UnlockPubScreen(NULL, s);
}

static int pick(const char *what)
{
    struct Screen *s;
    struct Window *w = desktop_window(&s);
    struct MsgPort *reply = CreateMsgPort();
    struct IntuiMessage im;
    LONG m = 0, i = 0;
    int ok = 0;
    const char *slash = strchr(what, '/');
    if (slash) { StrToLong((CONST_STRPTR)what, &m); StrToLong((CONST_STRPTR)slash + 1, &i); }
    if (w && w->UserPort && reply && slash && ItemAddress(w->MenuStrip, (UWORD)FULLMENUNUM(m, i, NOSUB))) {
        /* the end of the chain of items picked, as Intuition sets it (the window reads NextSelect after it) */
        Forbid();
        ItemAddress(w->MenuStrip, (UWORD)FULLMENUNUM(m, i, NOSUB))->NextSelect = MENUNULL;
        Permit();
        memset(&im, 0, sizeof im);
        im.ExecMessage.mn_Node.ln_Type = NT_MESSAGE;
        im.ExecMessage.mn_ReplyPort = reply;
        im.ExecMessage.mn_Length = sizeof im;
        im.Class = IDCMP_MENUPICK;
        im.Code = (UWORD)FULLMENUNUM(m, i, NOSUB);
        im.IDCMPWindow = w;
        PutMsg(w->UserPort, &im.ExecMessage);
        UnlockPubScreen(NULL, s);
        s = NULL;
        WaitPort(reply);
        GetMsg(reply);
        say("Picked %ld/%ld.\n", m, i);
        ok = 1;
    } else say("Nothing picked (MENU lists them; give PICK menu/item).\n");
    if (s) UnlockPubScreen(NULL, s);
    if (reply) DeleteMsgPort(reply);
    return ok ? RETURN_OK : RETURN_WARN;
}

/* OpenBench's message (openamigaprefs Common/obmsg.h: the layout is OpenBench's) */
struct ob_msg { struct Message m; ULONG magic; LONG ok; char path[256]; };

static int ob_open(const char *path)
{
    struct MsgPort *reply = CreateMsgPort(), *ob;
    struct ob_msg *m = AllocVec(sizeof *m, MEMF_PUBLIC | MEMF_CLEAR);
    int ok = -1;
    if (reply && m) {
        m->m.mn_ReplyPort = reply;
        m->m.mn_Length = sizeof *m;
        m->magic = 0x4F424D31UL;
        strncpy(m->path, path, sizeof m->path - 1);
        Forbid();
        if ((ob = FindPort((CONST_STRPTR)"OPENBENCH"))) PutMsg(ob, &m->m);
        Permit();
        if (ob) { WaitPort(reply); GetMsg(reply); ok = m->ok ? 1 : 0; }
    }
    if (m) FreeVec(m);
    if (reply) DeleteMsgPort(reply);
    say("OpenBench %s %s\n", ok < 0 ? (LONG)"isn't there to open" : ok ? (LONG)"opened" : (LONG)"couldn't open", (LONG)path);
    return ok > 0 ? RETURN_OK : RETURN_WARN;
}

int main(void)
{
    LONG args[A_COUNT] = { 0, 0, 0, 0 };
    struct RDArgs *rd = ReadArgs((CONST_STRPTR)TEMPLATE, args, NULL);
    int rc = RETURN_OK;
    (void)version;
    if (!rd) { PrintFault(IoErr(), (CONST_STRPTR)"alohaprobe"); return RETURN_FAIL; }
    IntuitionBase = (struct IntuitionBase *)OpenLibrary((CONST_STRPTR)"intuition.library", 39);
    GfxBase = (struct GfxBase *)OpenLibrary((CONST_STRPTR)"graphics.library", 39);
    if (!IntuitionBase || !GfxBase) rc = RETURN_FAIL;
    else if (args[A_PROBE]) rc = probe(args[A_WAIT] ? *(LONG *)args[A_WAIT] : 20, args[A_WINDOW] != 0, args[A_WBWINDOW] != 0);
    else if (args[A_MENU]) menus();
    else if (args[A_OPEN]) rc = ob_open((const char *)args[A_OPEN]);
    else if (args[A_PICK]) rc = pick((const char *)args[A_PICK]);
    else if (args[A_RESET]) {
        LONG closed = CloseWorkBench();
        say("CloseWorkBench() returned %ld\n", closed);
        say("OpenWorkBench() returned %s\n", OpenWorkBench() ? (LONG)"a screen" : (LONG)"0");
        info();
    } else info();
    if (GfxBase) CloseLibrary((struct Library *)GfxBase);
    if (IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);
    FreeArgs(rd);
    return rc;
}
