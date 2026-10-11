/* OpenMeters 0.1: a desktop widget for OpenBench (the Team, 11 October 2026: "we want a widget for the desktop
 * that's like this, run as tooltype", with a picture of one). A small panel on the Workbench screen with the
 * date and time, then a bar for Chip RAM, Fast RAM and the graphics card's video RAM, then a bar for each volume:
 * the label on the left, the bar filled as far as it is used, what is free written in it.
 *
 * A tool: started from its icon (or SYS:WBStartup) its tooltypes say how it looks, from a Shell the same words
 * are its arguments:
 *   CORNER=TOPRIGHT       TOPLEFT, TOPRIGHT, BOTTOMLEFT or BOTTOMRIGHT of the screen (default TOPRIGHT)
 *   LEFT=n TOP=n          a place of its own instead (the place it was dragged to is kept: ENVARC:OpenMeters.place)
 *   BARWIDTH=n            the bars' width in pixels (default 200)
 *   REFRESH=n             seconds between readings (default 2)
 *   NODATE NOMEMORY NOVIDEO NOVOLUMES   leave those out
 *   RAM                   show RAM: too
 *   HIDE=Name|Name        volumes not to show
 *   MAKEICON              (Shell only) write OpenMeters.info beside the program with these tooltypes, and stop
 * Drag it anywhere by any part of it. Its menu (right button while it is the active window): Refresh, Quit.
 * Ctrl-C quits too.
 *
 * Video RAM: openrtg.library does not yet say how much of the card's memory is in use, so that bar shows the
 * card's video RAM (the AutoConfig board's size, Dalsin $DA15 product 9 or 15) with no fill.
 *
 * Built so it can join OpenBench's built-ins later (desktop/ob_builtin.h): no C library allocation, every
 * resource freed on the way out. MIT, Copyright (c) 2026 Dalsin Limited. */
#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/rdargs.h>
#include <devices/timer.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <graphics/gfxmacros.h>
#include <graphics/rastport.h>
#include <utility/date.h>
#include <workbench/startup.h>
#include <workbench/workbench.h>
#include <workbench/icon.h>
#include <libraries/configvars.h>
#include <libraries/expansionbase.h>
#include <libraries/gadtools.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/icon.h>
#include <proto/expansion.h>
#include <proto/gadtools.h>
#include <string.h>
#include <stdio.h>

#define OPENMETERS_VERSION "0.1"
static const char version[] __attribute__((used)) = "$VER: OpenMeters " OPENMETERS_VERSION " (11.10.2026) MIT, Copyright (c) 2026 Dalsin Limited";

struct IntuitionBase *IntuitionBase;
struct GfxBase *GfxBase;
struct Library *UtilityBase, *IconBase, *GadToolsBase;
struct ExpansionBase *ExpansionBase;

#define MAX_ROWS 24
#define PAD 8                      /* inside the panel's edge */
#define GAP 8                      /* between a label and its bar */
#define PLACE_ENV "ENV:OpenMeters.place"
#define PLACE_ENVARC "ENVARC:OpenMeters.place"

enum { C_PANEL, C_EDGE, C_TEXT, C_DIM, C_TRACK, C_TRACK_EDGE, C_MEM, C_VOL, C_FULL, C_COUNT };
static const ULONG colours[C_COUNT] = {
    0x26303b, 0x4a5868, 0xe8eef4, 0xb4c0cc, 0x3a4654, 0x1c232b, 0xc84a40, 0x2f9a45, 0xe0662c
};
static LONG pens[C_COUNT];

typedef struct {
    char label[32];
    char value[32];
    ULONG fill;                    /* used, 0 to 1000; -1: no fill */
    int kind;                      /* C_MEM or C_VOL */
} row;

static struct {
    LONG left, top, barwidth, refresh;
    int corner;                    /* 0 TL, 1 TR, 2 BL, 3 BR */
    int nodate, nomemory, novideo, novolumes, ram;
    char hide[200];
} opt = { -1, -1, 200, 2, 1, 0, 0, 0, 0, 0, "" };

static struct Screen *scr;
static struct Window *win;
static struct Menu *menus;
static struct TextFont *font;
static row rows[MAX_ROWS];
static int nrows;
static char date_line[64];
static WORD label_w, row_h;
static struct Gadget drag;

/* ---- settings ------------------------------------------------------------------------------- */

static int corner_of(const char *s)
{
    if (!s) return 1;
    if (!Stricmp((STRPTR)s, (STRPTR)"TOPLEFT")) return 0;
    if (!Stricmp((STRPTR)s, (STRPTR)"BOTTOMLEFT")) return 2;
    if (!Stricmp((STRPTR)s, (STRPTR)"BOTTOMRIGHT")) return 3;
    return 1;
}

static void tooltypes(struct WBStartup *wbs)
{
    struct DiskObject *dob;
    BPTR old;
    char *v;
    if (!IconBase || !wbs || wbs->sm_NumArgs < 1) return;
    old = CurrentDir(wbs->sm_ArgList[0].wa_Lock);
    dob = GetDiskObject((STRPTR)wbs->sm_ArgList[0].wa_Name);
    CurrentDir(old);
    if (!dob) return;
    if ((v = (char *)FindToolType((CONST_STRPTR *)dob->do_ToolTypes, (STRPTR)"LEFT"))) StrToLong((STRPTR)v, &opt.left);
    if ((v = (char *)FindToolType((CONST_STRPTR *)dob->do_ToolTypes, (STRPTR)"TOP"))) StrToLong((STRPTR)v, &opt.top);
    if ((v = (char *)FindToolType((CONST_STRPTR *)dob->do_ToolTypes, (STRPTR)"BARWIDTH"))) StrToLong((STRPTR)v, &opt.barwidth);
    if ((v = (char *)FindToolType((CONST_STRPTR *)dob->do_ToolTypes, (STRPTR)"REFRESH"))) StrToLong((STRPTR)v, &opt.refresh);
    if ((v = (char *)FindToolType((CONST_STRPTR *)dob->do_ToolTypes, (STRPTR)"CORNER"))) opt.corner = corner_of(v);
    if ((v = (char *)FindToolType((CONST_STRPTR *)dob->do_ToolTypes, (STRPTR)"HIDE"))) strncpy(opt.hide, v, sizeof opt.hide - 1);
    opt.nodate = FindToolType((CONST_STRPTR *)dob->do_ToolTypes, (STRPTR)"NODATE") != NULL;
    opt.nomemory = FindToolType((CONST_STRPTR *)dob->do_ToolTypes, (STRPTR)"NOMEMORY") != NULL;
    opt.novideo = FindToolType((CONST_STRPTR *)dob->do_ToolTypes, (STRPTR)"NOVIDEO") != NULL;
    opt.novolumes = FindToolType((CONST_STRPTR *)dob->do_ToolTypes, (STRPTR)"NOVOLUMES") != NULL;
    opt.ram = FindToolType((CONST_STRPTR *)dob->do_ToolTypes, (STRPTR)"RAM") != NULL;
    FreeDiskObject(dob);
}

/* MAKEICON: a tool icon (the system's default one) with the tooltypes, as written out. */
static int make_icon(void)
{
    static STRPTR tt[] = { (STRPTR)"CORNER=TOPRIGHT", (STRPTR)"(LEFT=40)", (STRPTR)"(TOP=40)", (STRPTR)"BARWIDTH=200",
                           (STRPTR)"REFRESH=2", (STRPTR)"(NODATE)", (STRPTR)"(NOMEMORY)", (STRPTR)"(NOVIDEO)",
                           (STRPTR)"(NOVOLUMES)", (STRPTR)"(RAM)", (STRPTR)"(HIDE=Name|Name)", NULL };
    char path[256];
    struct DiskObject *dob;
    int ok = 0;
    if (!IconBase || !GetProgramName((STRPTR)path, sizeof path)) return 0;
    {   /* beside the program: its directory and name */
        BPTR dir = GetProgramDir(), old = CurrentDir(dir);
        if ((dob = GetDefDiskObject(WBTOOL))) {
            dob->do_ToolTypes = tt;
            dob->do_CurrentX = NO_ICON_POSITION;
            dob->do_CurrentY = NO_ICON_POSITION;
            ok = PutDiskObject(FilePart((STRPTR)path), dob) != 0;
            dob->do_ToolTypes = NULL;
            FreeDiskObject(dob);
        }
        CurrentDir(old);
    }
    return ok;
}

static void place_read(void)
{
    char b[32];
    LONG x, y, n;
    if (GetVar((STRPTR)PLACE_ENV, (STRPTR)b, sizeof b, GVF_GLOBAL_ONLY) <= 0) return;
    if ((n = StrToLong((STRPTR)b, &x)) > 0 && StrToLong((STRPTR)b + n + 1, &y) > 0) { opt.left = x; opt.top = y; }
}

static void place_save(void)
{
    char b[32];
    snprintf(b, sizeof b, "%ld %ld", (long)win->LeftEdge, (long)win->TopEdge);
    SetVar((STRPTR)PLACE_ENV, (STRPTR)b, -1, GVF_GLOBAL_ONLY);
    SetVar((STRPTR)PLACE_ENVARC, (STRPTR)b, -1, GVF_GLOBAL_ONLY | GVF_SAVE_VAR);
}

/* ---- readings ------------------------------------------------------------------------------- */

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

static ULONG used_of(unsigned long long used, unsigned long long total)
{
    if (!total) return 0;
    if (used >= total) return 1000;
    return (ULONG)(used * 1000 / total);
}

static row *add_row(const char *label, int kind)
{
    row *r;
    if (nrows >= MAX_ROWS) return NULL;
    r = &rows[nrows++];
    strncpy(r->label, label, sizeof r->label - 1);
    r->label[sizeof r->label - 1] = 0;
    r->kind = kind;
    r->fill = 0;
    r->value[0] = 0;
    return r;
}

static void memory_row(const char *label, ULONG type)
{
    unsigned long long freeb = AvailMem(type), total = AvailMem(type | MEMF_TOTAL);
    char n[24];
    row *r;
    if (!total) return;
    if (!(r = add_row(label, C_MEM))) return;
    thousands(n, sizeof n, freeb);
    snprintf(r->value, sizeof r->value, "%s Bytes", n);
    r->fill = used_of(total - freeb, total);
}

/* The graphics card's video RAM, from its AutoConfig board (ACRTG: 16 MiB, product 9; 64 MiB, product 15; the
 * first and last 64 KB pages are its ROM and its registers). */
static void video_row(void)
{
    struct ConfigDev *cd = NULL;
    unsigned long long total = 0;
    char n[24];
    row *r;
    if (!(ExpansionBase = (struct ExpansionBase *)OpenLibrary((STRPTR)"expansion.library", 36))) return;
    while ((cd = FindConfigDev(cd, 0xDA15, -1)))
        if ((cd->cd_Rom.er_Product == 9 || cd->cd_Rom.er_Product == 15) && cd->cd_BoardSize > 0x20000UL)
            total += cd->cd_BoardSize - 0x20000UL;
    CloseLibrary((struct Library *)ExpansionBase);
    ExpansionBase = NULL;
    if (!total || !(r = add_row("Video RAM", C_MEM))) return;
    thousands(n, sizeof n, total);
    snprintf(r->value, sizeof r->value, "%s Bytes", n);
    r->fill = (ULONG)-1;
}

static int hidden(const char *name)
{
    const char *h = opt.hide;
    size_t n = strlen(name);
    while (*h) {
        const char *e = strchr(h, '|');
        size_t k = e ? (size_t)(e - h) : strlen(h);
        if (k == n && !Strnicmp((STRPTR)h, (STRPTR)name, (LONG)n)) return 1;
        if (!e) break;
        h = e + 1;
    }
    return 0;
}

typedef struct { char name[32]; } vol;

/* The mounted volumes (each with a handler at work), by name, read with Info() with requesters off: a disk that
 * went away is left out, not asked for. The DOS list is only read under its lock; the volumes are locked after. */
static void volume_rows(void)
{
    static vol v[MAX_ROWS];
    int nv = 0, i, j;
    struct DosList *dl;
    struct Process *me = (struct Process *)FindTask(NULL);
    APTR oldwin = me->pr_WindowPtr;
    struct InfoData *id = AllocVec(sizeof *id, MEMF_ANY);
    if (!id) return;
    dl = LockDosList(LDF_VOLUMES | LDF_READ);
    while ((dl = NextDosEntry(dl, LDF_VOLUMES)) && nv < MAX_ROWS) {
        const UBYTE *b = (const UBYTE *)BADDR(dl->dol_Name);
        if (!dl->dol_Task || !b) continue;
        memcpy(v[nv].name, b + 1, b[0] < 31 ? b[0] : 31);
        v[nv].name[b[0] < 31 ? b[0] : 31] = 0;
        nv++;
    }
    UnLockDosList(LDF_VOLUMES | LDF_READ);
    /* by name: the order the volumes are mounted in varies from boot to boot */
    for (i = 1; i < nv; i++)
        for (j = i; j > 0 && Stricmp((STRPTR)v[j - 1].name, (STRPTR)v[j].name) > 0; j--) {
            vol t = v[j]; v[j] = v[j - 1]; v[j - 1] = t;
        }
    me->pr_WindowPtr = (APTR)-1;
    for (i = 0; i < nv; i++) {
        char path[40];
        BPTR l;
        if (hidden(v[i].name)) continue;
        if (!opt.ram && !Stricmp((STRPTR)v[i].name, (STRPTR)"RAM Disk")) continue;
        snprintf(path, sizeof path, "%s:", v[i].name);
        if (!(l = Lock((STRPTR)path, ACCESS_READ))) continue;
        if (Info(l, id) && id->id_NumBlocks > 0 && id->id_BytesPerBlock > 0) {
            unsigned long long bpb = (unsigned long long)id->id_BytesPerBlock;
            unsigned long long total = (unsigned long long)id->id_NumBlocks * bpb;
            unsigned long long used = (unsigned long long)id->id_NumBlocksUsed * bpb;
            unsigned long long freeb = total > used ? total - used : 0;
            row *r = add_row(v[i].name, C_VOL);
            if (r) {
                char n[24];
                if (freeb >= (10ULL << 30)) { thousands(n, sizeof n, (freeb + (1ULL << 29)) >> 30); snprintf(r->value, sizeof r->value, "%s GBytes", n); }
                else { thousands(n, sizeof n, (freeb + (1ULL << 19)) >> 20); snprintf(r->value, sizeof r->value, "%s MBytes", n); }
                r->fill = used_of(used, total);
            }
        }
        UnLock(l);
    }
    me->pr_WindowPtr = oldwin;
    FreeVec(id);
}

static void read_all(void)
{
    static const char *const days[7] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };
    static const char *const months[12] = { "January", "February", "March", "April", "May", "June", "July",
                                            "August", "September", "October", "November", "December" };
    struct DateStamp ds;
    struct ClockData cd;
    nrows = 0;
    DateStamp(&ds);
    Amiga2Date(ds.ds_Days * 86400 + ds.ds_Minute * 60 + ds.ds_Tick / TICKS_PER_SECOND, &cd);
    snprintf(date_line, sizeof date_line, "%s, %d %s %d, %02d:%02d", days[cd.wday % 7], cd.mday,
             months[(cd.month + 11) % 12], cd.year, cd.hour, cd.min);
    if (!opt.nomemory) {
        memory_row("Chip RAM", MEMF_CHIP);
        memory_row("Fast RAM", MEMF_FAST);
    }
    if (!opt.novideo) video_row();
    if (!opt.novolumes) volume_rows();
}

/* ---- drawing -------------------------------------------------------------------------------- */

static void get_pens(void)
{
    int i;
    for (i = 0; i < C_COUNT; i++) {
        ULONG c = colours[i];
        pens[i] = ObtainBestPen(scr->ViewPort.ColorMap, ((c >> 16) & 255) * 0x01010101UL, ((c >> 8) & 255) * 0x01010101UL,
                                (c & 255) * 0x01010101UL, OBP_Precision, PRECISION_EXACT, TAG_DONE);
        if (pens[i] < 0) pens[i] = i == C_PANEL || i == C_TRACK_EDGE ? 1 : 2;
    }
}

static void free_pens(void)
{
    int i;
    for (i = 0; i < C_COUNT; i++) if (pens[i] >= 0 && scr) ReleasePen(scr->ViewPort.ColorMap, pens[i]);
}

static WORD text_w(const char *s)
{
    struct RastPort rp;
    InitRastPort(&rp);
    SetFont(&rp, font);
    return TextLength(&rp, (STRPTR)s, strlen(s));
}

static void measure(WORD *w, WORD *h)
{
    int i;
    WORD dw = opt.nodate ? 0 : text_w(date_line);
    row_h = font->tf_YSize + 6;
    label_w = 0;
    for (i = 0; i < nrows; i++) { WORD t = text_w(rows[i].label); if (t > label_w) label_w = t; }
    *w = PAD + label_w + GAP + opt.barwidth + PAD;
    if (dw + 2 * PAD > *w) *w = dw + 2 * PAD;
    *h = PAD + (opt.nodate ? 0 : row_h + 2) + nrows * row_h + PAD - 2;
}

static void text_at(struct RastPort *rp, LONG x, LONG y, const char *s, LONG pen)
{
    SetAPen(rp, pen);
    SetDrMd(rp, JAM1);
    Move(rp, x, y + font->tf_Baseline);
    Text(rp, (STRPTR)s, strlen(s));
}

static void draw(void)
{
    struct RastPort *rp = win->RPort;
    WORD w = win->Width, h = win->Height, y = PAD, bx = PAD + label_w + GAP;
    int i;
    SetFont(rp, font);
    SetAPen(rp, pens[C_PANEL]);
    RectFill(rp, 1, 1, w - 2, h - 2);
    SetAPen(rp, pens[C_EDGE]);
    Move(rp, 0, 0); Draw(rp, w - 1, 0); Draw(rp, w - 1, h - 1); Draw(rp, 0, h - 1); Draw(rp, 0, 0);
    if (!opt.nodate) {
        text_at(rp, (w - text_w(date_line)) / 2, y + 2, date_line, pens[C_DIM]);
        y += row_h + 2;
    }
    for (i = 0; i < nrows; i++, y += row_h) {
        row *r = &rows[i];
        WORD by = y + 1, bh = row_h - 4, fw;
        text_at(rp, bx - GAP - text_w(r->label), y + 2, r->label, pens[C_TEXT]);
        SetAPen(rp, pens[C_TRACK_EDGE]);
        RectFill(rp, bx, by, bx + opt.barwidth - 1, by + bh - 1);
        SetAPen(rp, pens[C_TRACK]);
        RectFill(rp, bx + 1, by + 1, bx + opt.barwidth - 2, by + bh - 2);
        if (r->fill != (ULONG)-1 && r->fill > 0) {
            fw = (WORD)((opt.barwidth - 2) * r->fill / 1000);
            if (fw < 2) fw = 2;
            SetAPen(rp, pens[r->fill >= 900 ? C_FULL : r->kind]);
            RectFill(rp, bx + 1, by + 1, bx + fw, by + bh - 2);
        }
        text_at(rp, bx + opt.barwidth - 6 - text_w(r->value), y + 2, r->value, pens[C_TEXT]);
    }
}

/* The window's size for the rows; moved back onto the screen if it would go off it. */
static void fit(void)
{
    WORD w, h, x, y;
    measure(&w, &h);
    if (w == win->Width && h == win->Height) return;
    x = win->LeftEdge;
    y = win->TopEdge;
    if (x + w > scr->Width) x = scr->Width - w;
    if (y + h > scr->Height) y = scr->Height - h;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    ChangeWindowBox(win, x, y, w, h);
    drag.Width = w;
    drag.Height = h;
}

/* ---- the program ---------------------------------------------------------------------------- */

static struct NewMenu newmenus[] = {
    { NM_TITLE, (STRPTR)"OpenMeters", NULL, 0, 0, NULL },
    { NM_ITEM, (STRPTR)"Refresh", (STRPTR)"R", 0, 0, (APTR)1 },
    { NM_ITEM, NM_BARLABEL, NULL, 0, 0, NULL },
    { NM_ITEM, (STRPTR)"Quit", (STRPTR)"Q", 0, 0, (APTR)2 },
    { NM_END, NULL, NULL, 0, 0, NULL }
};

static int run(void)
{
    struct MsgPort *tport = NULL;
    struct timerequest *tr = NULL;
    int timer = 0, quit = 0;
    WORD w, h, x, y;
    APTR vi = NULL;
    if (!(scr = LockPubScreen((STRPTR)"Workbench"))) return RETURN_FAIL;
    font = OpenFont(scr->Font);
    if (!font) { UnlockPubScreen(NULL, scr); return RETURN_FAIL; }
    get_pens();
    read_all();
    measure(&w, &h);
    if (opt.left < 0 || opt.top < 0) {
        x = (opt.corner & 1) ? scr->Width - w - 20 : 20;
        y = (opt.corner & 2) ? scr->Height - h - 20 : scr->BarHeight + 20;
    } else {
        x = (WORD)opt.left;
        y = (WORD)opt.top;
    }
    if (x + w > scr->Width) x = scr->Width - w;
    if (y + h > scr->Height) y = scr->Height - h;
    if (x < 0) x = 0;
    if (y < scr->BarHeight + 1) y = scr->BarHeight + 1;
    /* any part of it drags it */
    drag.LeftEdge = 0;
    drag.TopEdge = 0;
    drag.Width = w;
    drag.Height = h;
    drag.Flags = GFLG_GADGHNONE;
    drag.Activation = 0;
    drag.GadgetType = GTYP_WDRAGGING;
    win = OpenWindowTags(NULL, WA_Left, x, WA_Top, y, WA_Width, w, WA_Height, h, WA_PubScreen, (ULONG)scr,
                         WA_Borderless, TRUE, WA_SimpleRefresh, TRUE, WA_Activate, FALSE, WA_RMBTrap, FALSE,
                         WA_NewLookMenus, TRUE, WA_Gadgets, (ULONG)&drag, WA_Title, (ULONG)"OpenMeters",
                         WA_IDCMP, IDCMP_REFRESHWINDOW | IDCMP_MENUPICK | IDCMP_CHANGEWINDOW, TAG_DONE);
    if (!win) goto out;
    WindowToBack(win);
    if ((vi = GetVisualInfoA(scr, NULL)) && (menus = CreateMenusA(newmenus, NULL)) && LayoutMenus(menus, vi, GTMN_NewLookMenus, TRUE, TAG_DONE))
        SetMenuStrip(win, menus);
    draw();
    if ((tport = CreateMsgPort()) && (tr = (struct timerequest *)CreateIORequest(tport, sizeof *tr)) &&
        !OpenDevice((STRPTR)TIMERNAME, UNIT_VBLANK, (struct IORequest *)tr, 0)) {
        timer = 1;
        tr->tr_node.io_Command = TR_ADDREQUEST;
        tr->tr_time.tv_secs = opt.refresh > 0 ? opt.refresh : 2;
        tr->tr_time.tv_micro = 0;
        SendIO((struct IORequest *)tr);
    }
    while (!quit) {
        ULONG got = Wait((1UL << win->UserPort->mp_SigBit) | (timer ? 1UL << tport->mp_SigBit : 0) | SIGBREAKF_CTRL_C);
        struct IntuiMessage *im;
        if (got & SIGBREAKF_CTRL_C) quit = 1;
        if (timer && CheckIO((struct IORequest *)tr)) {
            WaitIO((struct IORequest *)tr);
            read_all();
            fit();
            draw();
            tr->tr_time.tv_secs = opt.refresh > 0 ? opt.refresh : 2;
            tr->tr_time.tv_micro = 0;
            SendIO((struct IORequest *)tr);
        }
        while ((im = (struct IntuiMessage *)GetMsg(win->UserPort))) {
            ULONG cl = im->Class;
            UWORD code = im->Code;
            ReplyMsg((struct Message *)im);
            if (cl == IDCMP_REFRESHWINDOW) { BeginRefresh(win); draw(); EndRefresh(win, TRUE); }
            else if (cl == IDCMP_CHANGEWINDOW) place_save();
            else if (cl == IDCMP_MENUPICK) {
                while (code != MENUNULL) {
                    struct MenuItem *it = ItemAddress(menus, code);
                    if (!it) break;
                    if (GTMENUITEM_USERDATA(it) == (APTR)1) { read_all(); fit(); draw(); }
                    else if (GTMENUITEM_USERDATA(it) == (APTR)2) quit = 1;
                    code = it->NextSelect;
                }
            }
        }
    }
out:
    if (timer) { if (!CheckIO((struct IORequest *)tr)) AbortIO((struct IORequest *)tr); WaitIO((struct IORequest *)tr); CloseDevice((struct IORequest *)tr); }
    if (tr) DeleteIORequest((struct IORequest *)tr);
    if (tport) DeleteMsgPort(tport);
    if (win) { ClearMenuStrip(win); CloseWindow(win); win = NULL; }
    if (menus) { FreeMenus(menus); menus = NULL; }
    if (vi) FreeVisualInfo(vi);
    free_pens();
    CloseFont(font);
    UnlockPubScreen(NULL, scr);
    return RETURN_OK;
}

int main(int argc, char **argv)
{
    struct WBStartup *wbs = argc == 0 ? (struct WBStartup *)argv : NULL;
    struct RDArgs *rda = NULL;
    LONG a[13];
    int rc = RETURN_FAIL;
    IntuitionBase = (struct IntuitionBase *)OpenLibrary((STRPTR)"intuition.library", 39);
    GfxBase = (struct GfxBase *)OpenLibrary((STRPTR)"graphics.library", 39);
    UtilityBase = OpenLibrary((STRPTR)"utility.library", 39);
    GadToolsBase = OpenLibrary((STRPTR)"gadtools.library", 39);
    IconBase = OpenLibrary((STRPTR)"icon.library", 39);
    if (!IntuitionBase || !GfxBase || !UtilityBase || !GadToolsBase) goto done;
    place_read();
    if (wbs) tooltypes(wbs);
    else {
        memset(a, 0, sizeof a);
        if (!(rda = ReadArgs((STRPTR)"CORNER/K,LEFT/K/N,TOP/K/N,BARWIDTH/K/N,REFRESH/K/N,NODATE/S,NOMEMORY/S,NOVIDEO/S,"
                             "NOVOLUMES/S,RAM/S,HIDE/K,MAKEICON/S", a, NULL))) {
            PrintFault(IoErr(), (STRPTR)"OpenMeters");
            goto done;
        }
        if (a[11]) { rc = make_icon() ? RETURN_OK : RETURN_FAIL; PutStr((STRPTR)(rc ? "OpenMeters: no icon written\n" : "OpenMeters: icon written\n")); goto done; }
        if (a[0]) opt.corner = corner_of((const char *)a[0]);
        if (a[1]) opt.left = *(LONG *)a[1];
        if (a[2]) opt.top = *(LONG *)a[2];
        if (a[3]) opt.barwidth = *(LONG *)a[3];
        if (a[4]) opt.refresh = *(LONG *)a[4];
        opt.nodate = a[5] != 0;
        opt.nomemory = a[6] != 0;
        opt.novideo = a[7] != 0;
        opt.novolumes = a[8] != 0;
        opt.ram = a[9] != 0;
        if (a[10]) strncpy(opt.hide, (const char *)a[10], sizeof opt.hide - 1);
    }
    if (opt.barwidth < 60) opt.barwidth = 60;
    if (opt.barwidth > 800) opt.barwidth = 800;
    if (opt.refresh < 1) opt.refresh = 1;
    rc = run();
done:
    if (rda) FreeArgs(rda);
    if (IconBase) CloseLibrary(IconBase);
    if (GadToolsBase) CloseLibrary(GadToolsBase);
    if (UtilityBase) CloseLibrary(UtilityBase);
    if (GfxBase) CloseLibrary((struct Library *)GfxBase);
    if (IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);
    return rc;
}
