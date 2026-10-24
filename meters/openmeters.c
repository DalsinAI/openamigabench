/* OpenMeters 0.2: a desktop widget for OpenBench (the Team, 11 October 2026: "we want a widget for the desktop
 * that's like this, run as tooltype", with a picture of one). A small panel on the Workbench screen with the
 * date and time, then a bar for Chip RAM, Fast RAM and the graphics card's video RAM, then a bar for each volume:
 * the label on the left, the bar filled as far as it is used, what is free written in it.
 *
 * 0.2 (the Team, 11 October 2026: its background "needs to respond the same way as the dock does"): the panel is
 * drawn as OpenDock draws its shelf (openamigaprefs Dock/src/opendock.c). It is tinted to the look, dark on a
 * dark look and light on a light one, from the screen's background where that fits. It is see-through by the
 * dock's own settings (ENV:OpenDock/Dock: Glass, Solid or Clear, the opacity, the edge), so the desktop shows
 * through it frosted, with a faint sheen and a lighter rim. Its ends are rounded. The bars are a step from the
 * panel as it shows, as the dock's group lines are. They are filled in the theme's accent (Look prefs' accent
 * when it sets one) and in a warning colour over 90 per cent full. Labels are in the theme's text colour, and
 * the date in its muted one, each made lighter or darker when it would not stand out from the panel. The value
 * in a bar is in the accent's text colour where the fill is under it.
 * Like the dock, it follows changes. When the look, its light or dark mode (auto by the hour), the theme or the
 * dock's settings change, it reads them again and draws itself afresh. When the desktop behind it changes (a new
 * backdrop, a window that was over its place gone), it copies what is behind it again. When Workbench is reset
 * (OpenLook's wbclose.h) it closes and comes back with it. Ctrl-F reads the look again.
 * On a palette screen the panel is the nearest pens, solid, as the dock's is.
 *
 * A tool: started from its icon (or SYS:WBStartup) its tooltypes say how it looks, from a Shell the same words
 * are its arguments:
 *   CORNER=TOPRIGHT       TOPLEFT, TOPRIGHT, BOTTOMLEFT or BOTTOMRIGHT of the screen (default TOPRIGHT)
 *   LEFT=n TOP=n          a place of its own instead (the place it was dragged to is kept: ENVARC:OpenMeters.place)
 *   BARWIDTH=n            the bars' width in pixels at a 13-pixel font (default 200), in proportion to the font
 *                         (0.2.5); never narrower than the numbers in them
 *   REFRESH=n             seconds between readings (default 2)
 *   NODATE NOMEMORY NOVIDEO NOVOLUMES   leave those out
 *   RAM                   show RAM: too
 *   HIDE=Name|Name        volumes not to show
 *   MAKEICON              (Shell only) write OpenMeters.info beside the program with these tooltypes, and stop
 *   DONOTWAIT             (Workbench's, not OpenMeters') OpenMeters runs until it is quit. Started from
 *                         SYS:WBStartup by OS 3.2.3's Workbench (C:LoadWB, as on an 8-bit or AGA screen, where
 *                         OpenBench hands over to it) without this tooltype, Workbench waited for it: no disk icons,
 *                         and after a while "Program 'OpenMeters' has not yet returned" (measured, lab t5-display,
 *                         17 October 2026)
 * Drag it anywhere by any part of it. Its menu (right button while it is the active window): Refresh, Quit.
 * Ctrl-C quits too.
 *
 * 0.2.5 (the user's solid OS list, 24 October 2026: OpenMeters' text much larger than the icon labels): the text
 * is in the icons' font, Font prefs' Workbench icon text font at its size, or with none set the theme's, which
 * OpenBench draws its icon labels in (ogt_open_font), not the screen's, which the open ROM sizes to a tall
 * screen's bar. The rows and bars are sized from it (BARWIDTH, 200 by default and in OpenMeters' icon, is the
 * width at a 13-pixel font) and a bar is never narrower than the number in it, which was cut off ("1,371 MBy"). Each pixel of a number is in
 * whichever of its two colours stands out more from what is under it: white over the bar's light grey track,
 * where a fill ended under a number, was hard to read. A change of Font prefs is followed as one of the look.
 *
 * Video RAM: openrtg.library does not yet say how much of the card's memory is in use, so that bar shows the
 * card's video RAM (the AutoConfig board's size, AmigaChrome Team $DA15 product 9 or 15) with no fill.
 *
 * Built with opengadtools' theme reader (lib/ogt_theme.c) and openamigaprefs' Common/wbclose.h (meters/build.sh).
 * Every resource is freed on the way out. MIT, Copyright (c) 2026 AmigaChrome Team. */
#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/rdargs.h>
#include <devices/timer.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <intuition/screens.h>
#include <graphics/gfx.h>
#include <graphics/gfxmacros.h>
#include <graphics/rastport.h>
#include <utility/date.h>
#include <workbench/startup.h>
#include <workbench/workbench.h>
#include <workbench/icon.h>
#include <libraries/configvars.h>
#include <libraries/expansionbase.h>
#include <libraries/gadtools.h>
#include <cybergraphx/cybergraphics.h>
#include <prefs/font.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/icon.h>
#include <proto/expansion.h>
#include <proto/gadtools.h>
#include <proto/cybergraphics.h>
#include <proto/diskfont.h>
#include <string.h>
#include <stdio.h>

#include "ogt_theme.h"
#include "ogt_font.h"
#include "wbclose.h"

#define OPENMETERS_VERSION "0.2.5"
static const char version[] __attribute__((used)) = "$VER: OpenMeters " OPENMETERS_VERSION " (24.10.2026) MIT, Copyright (c) 2026 AmigaChrome Team";

struct IntuitionBase *IntuitionBase;
struct GfxBase *GfxBase;
struct Library *UtilityBase, *IconBase, *GadToolsBase, *CyberGfxBase, *DiskfontBase;
struct ExpansionBase *ExpansionBase;

#define MAX_ROWS 24
#define PAD 12                     /* inside the panel's edge (0.2: room for its rounded ends) */
#define GAP 8                      /* between a label and its bar */
#define MAX_RADIUS 16              /* the panel's corners */
#define PLACE_ENV "ENV:OpenMeters.place"
#define PLACE_ENVARC "ENVARC:OpenMeters.place"
#define LOOK_ENV "ENV:OpenGadTools/Look"
#define FONT_ENV "ENV:Sys/font.prefs"
#define DOCK_ENV "ENV:OpenDock/Dock"
#define THEME_DIR "SYS:Prefs/Presets/Themes/"

enum { C_MEM = 1, C_VOL = 2 };     /* a row's kind */

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
static APTR vi;
static struct TextFont *font;
static struct TextAttr font_ta;    /* the font asked for */
static char font_name[FONTNAMESIZE];
static WORD bar_w, bar_min;        /* the bars' width; at least that while the panel is open (no jitter) */
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
                           (STRPTR)"(NOVOLUMES)", (STRPTR)"(RAM)", (STRPTR)"(HIDE=Name|Name)", (STRPTR)"DONOTWAIT", NULL };
    char path[256];
    struct DiskObject *dob;
    int ok = 0;
    BPTR dir = GetProgramDir();
    if (!IconBase || !GetProgramName((STRPTR)path, sizeof path)) return 0;
    /* the program's own full name: its directory (the Shell's command may have been given a relative path) */
    if (dir) {
        char name[64];
        strncpy(name, (const char *)FilePart((STRPTR)path), sizeof name - 1);
        name[sizeof name - 1] = 0;
        if (NameFromLock(dir, (STRPTR)path, sizeof path)) AddPart((STRPTR)path, (STRPTR)name, sizeof path);
    }
    dob = IconBase->lib_Version >= 44 ? GetDiskObjectNew((STRPTR)path) : NULL;
    if (!dob) dob = GetDefDiskObject(WBTOOL);
    if (dob) {
        STRPTR *keep = dob->do_ToolTypes;
        dob->do_Type = WBTOOL;
        dob->do_ToolTypes = tt;
        dob->do_CurrentX = NO_ICON_POSITION;
        dob->do_CurrentY = NO_ICON_POSITION;
        ok = PutDiskObject((STRPTR)path, dob) != 0;
        dob->do_ToolTypes = keep;
        FreeDiskObject(dob);
    }
    if (!ok) PrintFault(IoErr(), (STRPTR)path);
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
    strlcpy(r->label, label, sizeof r->label);
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

/* The graphics card's video RAM, from its AutoConfig board (ACRTG: 16 MiB, product 9; 64 MiB, product 15;
 * 256 MiB, product 16; the first and last 64 KB pages are its ROM and its registers). */
static void video_row(void)
{
    struct ConfigDev *cd = NULL;
    unsigned long long total = 0;
    char n[24];
    row *r;
    if (!(ExpansionBase = (struct ExpansionBase *)OpenLibrary((STRPTR)"expansion.library", 36))) return;
    while ((cd = FindConfigDev(cd, 0xDA15, -1)))
        if ((cd->cd_Rom.er_Product == 9 || cd->cd_Rom.er_Product == 15 || cd->cd_Rom.er_Product == 16) && cd->cd_BoardSize > 0x20000UL)
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
        if (snprintf(path, sizeof path, "%s:", v[i].name) >= (int)sizeof path) continue;   /* no volume's name is so long */
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

/* ---- the look: the dock's tint, the theme's colours ------------------------------------------ */

typedef struct { int r, g, b; } colour;
static const colour white = { 255, 255, 255 }, black = { 0, 0, 0 };

static ogt_theme theme;
static int theme_ok;
static int look_mode = -1;         /* as the dock reads it: 0 light, 1 dark, -1 no look */
enum { BG_SOLID, BG_GLASS, BG_CLEAR };
static int dk_bg = BG_GLASS, dk_opacity = 35, dk_border = 1;   /* the dock's settings: its defaults */
static colour tint, bg_c, eff;     /* the dock's tint; the screen's background; the panel as it shows */
static colour c_text, c_dim, c_track, c_fill, c_fill_text, c_full, c_full_text;
static ULONG look_sig;             /* what the look was read from, to see it change */

static int luma(colour c) { return (c.r * 77 + c.g * 150 + c.b * 29) >> 8; }
static int apart(colour a, colour b) { int d = luma(a) - luma(b); return d < 0 ? -d : d; }

static colour mix(colour a, colour b, int t)       /* t of 256 parts b */
{
    colour c;
    c.r = a.r + (b.r - a.r) * t / 256; c.g = a.g + (b.g - a.g) * t / 256; c.b = a.b + (b.b - a.b) * t / 256;
    return c;
}

static colour rgb(ULONG v) { colour c; c.r = (v >> 16) & 255; c.g = (v >> 8) & 255; c.b = v & 255; return c; }

/* A file, with a 0 after it; its length in *n when n is given. */
static char *read_file_n(const char *path, LONG *n)
{
    BPTR fh = Open((STRPTR)path, MODE_OLDFILE);
    char *buf = NULL;
    LONG len;
    if (n) *n = 0;
    if (!fh) return NULL;
    Seek(fh, 0, OFFSET_END);
    len = Seek(fh, 0, OFFSET_BEGINNING);
    if (len >= 0 && len < 64 * 1024 && (buf = AllocVec(len + 1, MEMF_ANY | MEMF_CLEAR)) && Read(fh, buf, len) != len) { FreeVec(buf); buf = NULL; }
    Close(fh);
    if (buf && n) *n = len;
    return buf;
}

static char *read_file(const char *path) { return read_file_n(path, NULL); }

static ULONG hash(ULONG h, const char *t)
{
    if (!t) return h * 33;
    while (*t) h = h * 33 + (UBYTE)*t++;
    return h;
}

static ULONG hash_n(ULONG h, const char *t, LONG n)
{
    if (!t) return h * 33;
    while (n-- > 0) h = h * 33 + (UBYTE)*t++;
    return h;
}

/* The look's mode, as the dock reads it (its look_mode()): the first line of Look prefs, "<theme> light|dark|auto";
 * auto is dark from seven in the evening to seven in the morning. */
static int mode_of(const char *word)
{
    struct DateStamp ds;
    int hour;
    if (!strcmp(word, "dark")) return 1;
    if (strcmp(word, "auto")) return 0;
    DateStamp(&ds);
    hour = (int)(ds.ds_Minute / 60);
    return hour >= 19 || hour < 7;
}

/* What the look depends on, as one number: Look prefs, the dock's settings, the mode now (auto turns with the
 * hour) and the screen's background. It changes when any of them does. */
static ULONG look_signature(void)
{
    LONG fn;
    char *l = read_file(LOOK_ENV), *d = read_file(DOCK_ENV), *f = read_file_n(FONT_ENV, &fn), name[64], mode[8];
    ULONG h = hash_n(hash(hash(5381, l), d), f, fn);    /* 0.2.5: Font prefs too, for the icons' font */
    if (f) FreeVec(f);
    if (l && sscanf(l, "%63s %7s", name, mode) == 2) h = h * 33 + (ULONG)mode_of(mode);
    if (l) FreeVec(l);
    if (d) FreeVec(d);
    if (scr) {
        struct DrawInfo *dri = GetScreenDrawInfo(scr);
        if (dri) {
            ULONG v[3] = { 0, 0, 0 };
            GetRGB32(scr->ViewPort.ColorMap, dri->dri_Pens[BACKGROUNDPEN], 1, v);
            h = h * 33 + (v[0] >> 24) * 65536 + (v[1] >> 24) * 256 + (v[2] >> 24);
            FreeScreenDrawInfo(scr, dri);
        }
    }
    return h;
}

/* The dock's background (Solid, Glass or Clear), its opacity and its edge, from its settings file. */
static void read_dock(void)
{
    char *t = read_file(DOCK_ENV), *line, *next, k[24], v[24];
    dk_bg = BG_GLASS; dk_opacity = 35; dk_border = 1;
    if (!t) return;
    for (line = t; line && *line; line = next) {
        if ((next = strchr(line, '\n'))) *next++ = 0;
        if (line[0] == ';' || sscanf(line, "%23s %23s", k, v) != 2) continue;
        if (!strcmp(k, "background")) dk_bg = !strcmp(v, "solid") ? BG_SOLID : !strcmp(v, "clear") ? BG_CLEAR : BG_GLASS;
        else if (!strcmp(k, "opacity")) { LONG n; if (StrToLong((STRPTR)v, &n) > 0 && n >= 0 && n <= 100) dk_opacity = (int)n; }
        else if (!strcmp(k, "borders")) dk_border = strcmp(v, "off") != 0;
    }
    FreeVec(t);
}

/* Look prefs: the theme, its mode, and the accent the user chose over it; then the theme file. */
static void read_look(void)
{
    char *t = read_file(LOOK_ENV), name[64] = "", mode[8] = "", path[160], err[64], *text, *line, *next;
    ogt_rgb accent;
    int has_accent = 0, first = 1;
    if (theme_ok) { ogt_theme_free(&theme); theme_ok = 0; }
    look_mode = -1;
    if (t) {
        for (line = t; line && *line; line = next) {
            if ((next = strchr(line, '\n'))) *next++ = 0;
            if (first) {
                if (sscanf(line, "%63s %7s", name, mode) == 2) look_mode = mode_of(mode);
                first = 0;
            } else if (!strncmp(line, "accent ", 7) && ogt_parse_colour(line + 7, &accent)) has_accent = 1;
        }
        FreeVec(t);
    }
    if (!name[0]) return;
    snprintf(path, sizeof path, THEME_DIR "%s%s", name, strstr(name, ".theme") ? "" : ".theme");
    if (!(text = read_file(path))) return;
    theme_ok = ogt_theme_parse(&theme, text, err, sizeof err);
    FreeVec(text);
    if (theme_ok && has_accent) ogt_theme_set_accent(&theme, accent);
}

/* A colour of the theme in the mode, or the Open theme's own when the theme hasn't one (or is Classic). */
static colour theme_colour(const char *key, ULONG light, ULONG dark)
{
    ogt_rgb c;
    if (theme_ok && !theme.passthrough && ogt_theme_colour(&theme, look_mode == 1 ? OGT_DARK : OGT_LIGHT, key, &c)) {
        colour r;
        r.r = c.r; r.g = c.g; r.b = c.b;
        return r;
    }
    return rgb(look_mode == 1 ? dark : light);
}

/* 0.2.5: the icons' font (the user's solid OS list, 24 October 2026). Font prefs' Workbench icon text font at its
 * size, from ENV:Sys/font.prefs (a FONT chunk of type FP_WBFONT; the file is the OS's, IFF PREF); with none set,
 * the font OpenBench draws its icon labels in: the theme's, through opengadtools' ogt_open_font, as
 * desktop/openbench.c opens it (Team 3's measurement, 22 October 2026: the screen's font, which the open ROM
 * sizes to a tall screen's bar, was CGTriumvirate 21 on Instance-32, its capitals 13 pixels against the labels'
 * 9). The screen's font only when neither opens. Called with the look read. */
static struct TextFont *icon_font(void)
{
    LONG n, i;
    UBYTE *t = (UBYTE *)read_file_n(FONT_ENV, &n);
    struct TextFont *f = NULL;
    if (t && n >= 12 && !memcmp(t, "FORM", 4) && !memcmp(t + 8, "PREF", 4)) {
        for (i = 12; i + 8 <= n;) {
            ULONG sz = (ULONG)t[i + 4] << 24 | (ULONG)t[i + 5] << 16 | (ULONG)t[i + 6] << 8 | t[i + 7];
            if (sz > (ULONG)(n - i - 8)) break;
            if (!memcmp(t + i, "FONT", 4) && sz >= sizeof(struct FontPrefs)) {
                const struct FontPrefs *fp = (const struct FontPrefs *)(t + i + 8);     /* i is even: aligned */
                if (fp->fp_Type == FP_WBFONT && fp->fp_Name[0] && fp->fp_TextAttr.ta_YSize > 0) {
                    strncpy(font_name, (const char *)fp->fp_Name, sizeof font_name - 1);
                    font_name[sizeof font_name - 1] = 0;
                    font_ta.ta_Name = (STRPTR)font_name;
                    font_ta.ta_YSize = fp->fp_TextAttr.ta_YSize;
                    font_ta.ta_Style = fp->fp_TextAttr.ta_Style;
                    font_ta.ta_Flags = fp->fp_TextAttr.ta_Flags;
                    f = DiskfontBase ? OpenDiskFont(&font_ta) : OpenFont(&font_ta);
                    break;
                }
            }
            i += 8 + sz + (sz & 1);
        }
    }
    if (t) FreeVec(t);
    if (!f) f = ogt_open_font(theme_ok ? &theme : NULL, scr, &font_ta);
    if (!f) f = OpenFont(scr->Font);
    return f;
}

static colour pen_colour(int pen)
{
    ULONG v[3] = { 0, 0, 0 };
    GetRGB32(scr->ViewPort.ColorMap, pen, 1, v);
    return rgb((v[0] >> 24) << 16 | (v[1] >> 24) << 8 | (v[2] >> 24));
}

/* The dock's tint (its choose_colours()): dark for a dark look, light for a light one, from the screen's background
 * where it fits; a neutral grey with no look. */
static void choose_tint(void)
{
    struct DrawInfo *dri = GetScreenDrawInfo(scr);
    bg_c = (colour){ 150, 150, 154 };
    if (dri) { bg_c = pen_colour(dri->dri_Pens[BACKGROUNDPEN]); FreeScreenDrawInfo(scr, dri); }
    if (look_mode == 1) tint = luma(bg_c) < 110 ? mix(bg_c, black, 64) : (colour){ 36, 36, 40 };
    else if (look_mode == 0) tint = luma(bg_c) >= 150 ? mix(bg_c, white, 160) : (colour){ 236, 236, 240 };
    else tint = (colour){ 150, 150, 154 };
}

static int opacity(void) { return dk_bg == BG_SOLID ? 100 : dk_bg == BG_CLEAR ? 0 : dk_opacity; }

/* The text and bar colours: the theme's, each made to stand out from the panel as it shows (eff). */
static void choose_colours(void)
{
    int dark = luma(eff) < 128;
    c_text = theme_colour("text", 0x1c242d, 0xe4e9ee);
    if (apart(c_text, eff) < 100) c_text = dark ? (colour){ 236, 236, 236 } : (colour){ 28, 28, 30 };
    c_dim = theme_colour("muted", 0x4a5663, 0xa9b4bf);
    if (apart(c_dim, eff) < 70) c_dim = mix(c_text, eff, 64);
    /* a bar's track: a step from the panel, as the dock's group line is */
    c_track = dark ? mix(eff, white, 56) : mix(eff, black, 48);
    c_fill = theme_colour("accent", 0x2f6fb3, 0x2f6fb3);
    c_fill_text = theme_colour("accent.text", 0xffffff, 0xffffff);
    if (apart(c_fill_text, c_fill) < 90) c_fill_text = luma(c_fill) < 128 ? white : (colour){ 28, 28, 30 };
    c_full = theme_colour("x-openmeters.warning", 0xd0452e, 0xe0583f);
    c_full_text = luma(c_full) < 150 ? white : (colour){ 28, 28, 30 };
}

/* ---- drawing on a graphics card: everything mixed in ARGB, as the dock's shelf is ------------- */

static int card;                   /* the screen is a graphics card's true colour */
static int W, H;                   /* the panel's size */
static ULONG *shelf_px, *work_px;  /* what is behind made into the panel, once per opening; the panel with the rows */
static UBYTE inside[MAX_RADIUS * MAX_RADIUS], rimcov[MAX_RADIUS * MAX_RADIUS];
static int radius;

static int truecolour(void)
{
    struct BitMap *bm = scr->RastPort.BitMap;
    if (!CyberGfxBase || !GetCyberMapAttr(bm, CYBRMATTR_ISCYBERGFX)) return 0;
    return GetCyberMapAttr(bm, CYBRMATTR_DEPTH) > 8;
}

static ULONG argb(colour c) { return 0xff000000UL | (ULONG)c.r << 16 | (ULONG)c.g << 8 | (ULONG)c.b; }

static ULONG blend(ULONG p, colour c, int a)       /* a of 255 parts c over p */
{
    int r = (p >> 16) & 255, g = (p >> 8) & 255, b = p & 255;
    r += (c.r - r) * a / 255; g += (c.g - g) * a / 255; b += (c.b - b) * a / 255;
    return 0xff000000UL | (ULONG)r << 16 | (ULONG)g << 8 | (ULONG)b;
}

/* The dock's corner (its make_corner()): how much of each pixel is inside the rounded edge, and on its rim. */
static void make_corner(void)
{
    int R = radius, R8 = R * 8, in2 = R8 * R8, rim2 = (R8 - 8) * (R8 - 8);
    for (int j = 0; j < R; j++)
        for (int i = 0; i < R; i++) {
            int a = 0, b = 0;
            for (int v = 0; v < 4; v++)
                for (int u = 0; u < 4; u++) {
                    int dx = R8 - (8 * i + 2 * u + 1), dy = R8 - (8 * j + 2 * v + 1), d2 = dx * dx + dy * dy;
                    if (d2 <= in2) { a++; if (d2 > rim2) b++; }
                }
            inside[j * R + i] = a * 255 / 16;
            rimcov[j * R + i] = b * 255 / 16;
        }
}

static void cover(int x, int y, int w, int h, int *in, int *on_rim)
{
    int i = x < radius ? x : x >= w - radius ? w - 1 - x : -1;
    int j = y < radius ? y : y >= h - radius ? h - 1 - y : -1;
    if (i >= 0 && j >= 0) { *in = inside[j * radius + i]; *on_rim = rimcov[j * radius + i]; return; }
    *in = 255;
    *on_rim = x == 0 || y == 0 || x == w - 1 || y == h - 1 ? 255 : 0;
}

/* The dock's frosted glass: what is behind, blurred (a box of 2 r + 1 pixels, across then down). */
static void frost(ULONG *px, int w, int h, int r)
{
    int n = w > h ? w : h;
    ULONG *line = AllocVec(n * 4, MEMF_ANY);
    if (!line) return;
    for (int pass = 0; pass < 2; pass++) {
        int len = pass ? h : w, lines = pass ? w : h;
        for (int k = 0; k < lines; k++) {
            ULONG *first = pass ? px + k : px + k * w;
            int step = pass ? w : 1, sr = 0, sg = 0, sb = 0, cnt = 2 * r + 1;
            for (int t = 0; t < len; t++) line[t] = first[t * step];
            for (int t = -r; t <= r; t++) {
                ULONG p = line[t < 0 ? 0 : t >= len ? len - 1 : t];
                sr += (p >> 16) & 255; sg += (p >> 8) & 255; sb += p & 255;
            }
            for (int t = 0; t < len; t++) {
                ULONG in, out;
                first[t * step] = 0xff000000UL | (ULONG)(sr / cnt) << 16 | (ULONG)(sg / cnt) << 8 | (ULONG)(sb / cnt);
                in = line[t + r + 1 < len ? t + r + 1 : len - 1];
                out = line[t - r < 0 ? 0 : t - r];
                sr += (int)((in >> 16) & 255) - (int)((out >> 16) & 255);
                sg += (int)((in >> 8) & 255) - (int)((out >> 8) & 255);
                sb += (int)(in & 255) - (int)(out & 255);
            }
        }
    }
    FreeVec(line);
}

/* The panel from what is behind it (in shelf_px): as the dock's shelf (its make_shelf_card()). Sets eff. */
static void make_shelf(void)
{
    int a = opacity() * 255 / 100, ra, n = W * H;
    ULONG sr = 0, sg = 0, sb = 0;
    colour mean;
    for (int k = 0; k < n; k++) { ULONG p = shelf_px[k]; sr += (p >> 16) & 255; sg += (p >> 8) & 255; sb += p & 255; }
    mean.r = sr / n; mean.g = sg / n; mean.b = sb / n;
    eff = dk_bg == BG_CLEAR ? mean : mix(mean, tint, a * 256 / 255);
    ra = luma(tint) < 128 ? 72 : 150;
    if (dk_bg == BG_CLEAR) return;               /* Clear: what is behind, as it is, and nothing else */
    if (dk_bg == BG_GLASS && a < 255) frost(shelf_px, W, H, 6);
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++) {
            int in, on_rim;
            ULONG *p = &shelf_px[y * W + x];
            cover(x, y, W, H, &in, &on_rim);
            if (!in) continue;
            if (a) *p = blend(*p, tint, a * in / 255);
            if (dk_bg == BG_GLASS && y < H / 2) *p = blend(*p, white, (H / 2 - y) * 40 / (H / 2) * in / 255);
            if (dk_border && on_rim) *p = blend(*p, white, ra * on_rim / 255);
        }
}

/* How much of pixel (i, j), counted from a corner, is inside a circle of radius r there: 0 to 16. */
static int corner_cov(int i, int j, int r)
{
    int n = 0, R8 = r * 8;
    for (int v = 0; v < 4; v++)
        for (int u = 0; u < 4; u++) {
            int dx = R8 - (8 * i + 2 * u + 1), dy = R8 - (8 * j + 2 * v + 1);
            if (dx * dx + dy * dy <= R8 * R8) n++;
        }
    return n;
}

/* A box with round ends, smooth, a of 255 parts c over what is there. */
static void round_box(ULONG *px, int x, int y, int w, int h, int r, colour c, int a)
{
    if (w <= 0 || h <= 0) return;
    if (r > w / 2) r = w / 2;
    if (r > h / 2) r = h / 2;
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++) {
            int X = x + i, Y = y + j, ci = i < r ? i : i >= w - r ? w - 1 - i : -1, cj = j < r ? j : j >= h - r ? h - 1 - j : -1, cv = 16;
            if (X < 0 || Y < 0 || X >= W || Y >= H) continue;
            if (ci >= 0 && cj >= 0) cv = corner_cov(ci, cj, r);
            if (cv) px[Y * W + X] = blend(px[Y * W + X], c, a * cv / 16);
        }
}

/* Text in ARGB: drawn in one plane of chip memory, then each set pixel coloured, c left of split and c2 from it. */
static struct BitMap tbm;
static UBYTE *tplane;
static struct RastPort trp;
static int tbm_w;

static void text_free(void) { if (tplane) { WaitBlit(); FreeVec(tplane); tplane = NULL; } }

static int text_open(int w)
{
    text_free();
    tbm_w = (w + 15) & ~15;
    InitBitMap(&tbm, 1, tbm_w, font->tf_YSize);
    if (!(tplane = AllocVec(RASSIZE(tbm_w, font->tf_YSize), MEMF_CHIP | MEMF_CLEAR))) return 0;
    tbm.Planes[0] = tplane;
    InitRastPort(&trp);
    trp.BitMap = &tbm;
    SetFont(&trp, font);
    SetAPen(&trp, 1);
    SetDrMd(&trp, JAM1);
    return 1;
}

static void text_px(int x, int y, const char *s, colour c, int split, colour c2)
{
    int n = strlen(s), fh = font->tf_YSize, bpr = tbm.BytesPerRow, tw;
    if (!tplane || !n) return;
    WaitBlit();
    memset(tplane, 0, bpr * fh);
    Move(&trp, 0, font->tf_Baseline);
    Text(&trp, (STRPTR)s, n);
    WaitBlit();
    tw = TextLength(&trp, (STRPTR)s, n) + 8;
    if (tw > tbm_w) tw = tbm_w;
    for (int r = 0; r < fh; r++) {
        const UBYTE *b = tplane + r * bpr;
        int Y = y + r;
        if (Y < 0 || Y >= H) continue;
        for (int i = 0; i < tw; i++) {
            int X = x + i;
            if (!(b[i >> 3] & (0x80 >> (i & 7))) || X < 0 || X >= W) continue;
            work_px[Y * W + X] = argb(X < split ? c2 : c);
        }
    }
}

/* 0.2.5: a number in its bar, each pixel in whichever of a and b stands out more from what is under it, so a
 * number is read over the fill, the track, and where the fill ends under it. */
static void text_px_contrast(int x, int y, const char *s, colour a, colour b)
{
    int n = strlen(s), fh = font->tf_YSize, bpr = tbm.BytesPerRow, tw;
    if (!tplane || !n) return;
    WaitBlit();
    memset(tplane, 0, bpr * fh);
    Move(&trp, 0, font->tf_Baseline);
    Text(&trp, (STRPTR)s, n);
    WaitBlit();
    tw = TextLength(&trp, (STRPTR)s, n) + 8;
    if (tw > tbm_w) tw = tbm_w;
    for (int r = 0; r < fh; r++) {
        const UBYTE *bits = tplane + r * bpr;
        int Y = y + r;
        if (Y < 0 || Y >= H) continue;
        for (int i = 0; i < tw; i++) {
            int X = x + i;
            colour under;
            if (!(bits[i >> 3] & (0x80 >> (i & 7))) || X < 0 || X >= W) continue;
            under = rgb(work_px[Y * W + X]);
            work_px[Y * W + X] = argb(apart(a, under) >= apart(b, under) ? a : b);
        }
    }
}

/* ---- drawing on a palette screen: the nearest pens, solid, as the dock's are ------------------ */

enum { P_TINT, P_RIM, P_TRACK, P_FILL, P_FULL, P_TEXT, P_DIM, P_FILLTEXT, P_COUNT };
static LONG pens[P_COUNT] = { -1, -1, -1, -1, -1, -1, -1, -1 };
static struct BitMap *behind_bm;   /* what is behind the panel, on a palette screen */

static LONG best_pen(colour c)
{
    return ObtainBestPen(scr->ViewPort.ColorMap, (ULONG)c.r * 0x01010101UL, (ULONG)c.g * 0x01010101UL, (ULONG)c.b * 0x01010101UL,
                         OBP_Precision, PRECISION_IMAGE, TAG_DONE);
}

static void free_pens(void)
{
    for (int i = 0; i < P_COUNT; i++) {
        if (pens[i] >= 0 && scr) ReleasePen(scr->ViewPort.ColorMap, pens[i]);
        pens[i] = -1;
    }
}

static void get_pens(void)
{
    colour c[P_COUNT];
    free_pens();
    c[P_TINT] = tint; c[P_RIM] = mix(tint, white, luma(tint) < 128 ? 72 : 150); c[P_TRACK] = c_track; c[P_FILL] = c_fill;
    c[P_FULL] = c_full; c[P_TEXT] = c_text; c[P_DIM] = c_dim; c[P_FILLTEXT] = c_text;
    for (int i = 0; i < P_COUNT; i++) pens[i] = best_pen(c[i]);
}

static LONG pen(int i) { return pens[i] >= 0 ? pens[i] : i == P_TINT || i == P_TRACK ? 0 : 1; }

/* ---- measuring and drawing ------------------------------------------------------------------- */

static WORD text_w(const char *s)
{
    struct RastPort rp;
    InitRastPort(&rp);
    SetFont(&rp, font);
    return TextLength(&rp, (STRPTR)s, strlen(s));
}

static void measure(WORD *w, WORD *h)
{
    int i, br;
    WORD dw = opt.nodate ? 0 : text_w(date_line), vw = 0;
    row_h = font->tf_YSize + 6;
    br = (row_h - 4) / 2 > 6 ? 6 : (row_h - 4) / 2;
    label_w = 0;
    for (i = 0; i < nrows; i++) {
        WORD t = text_w(rows[i].label), v = text_w(rows[i].value);
        if (t > label_w) label_w = t;
        if (v > vw) vw = v;
    }
    /* 0.2.5: the bars in proportion to the font (BARWIDTH at 13 pixels); never narrower than the widest number and
     * its margins (it was cut off), nor than they were while the panel is open */
    bar_w = (WORD)((opt.barwidth * font->tf_YSize + 6) / 13);
    if (bar_w < vw + 12 + br) bar_w = vw + 12 + br;
    if (bar_w < bar_min) bar_w = bar_min;
    if (bar_w > 800) bar_w = 800;
    bar_min = bar_w;
    *w = PAD + label_w + GAP + bar_w + PAD;
    if (dw + 2 * PAD > *w) *w = dw + 2 * PAD;
    *h = PAD + (opt.nodate ? 0 : row_h + 2) + nrows * row_h + PAD - 2;
}

static void text_at(struct RastPort *rp, LONG x, LONG y, const char *s, LONG p)
{
    SetAPen(rp, p);
    SetDrMd(rp, JAM1);
    Move(rp, x, y + font->tf_Baseline);
    Text(rp, (STRPTR)s, strlen(s));
}

/* The rows over the panel, into work_px (a card) or straight into the window (pens). */
static void compose(void)
{
    WORD y = PAD, bx = PAD + label_w + GAP;
    int i, bh = row_h - 4, br = bh / 2 > 6 ? 6 : bh / 2;
    struct RastPort *rp = win->RPort;
    if (card) memcpy(work_px, shelf_px, W * H * 4);
    else {
        SetFont(rp, font);
        if (behind_bm) BltBitMapRastPort(behind_bm, 0, 0, rp, 0, 0, W, H, 0xc0);
        else { SetAPen(rp, pen(P_TINT)); RectFill(rp, 0, 0, W - 1, H - 1); }
        if (dk_bg != BG_CLEAR) {
            SetAPen(rp, pen(P_TINT));
            for (int yy = 0; yy < H; yy++) {
                int j = yy < radius ? yy : yy >= H - radius ? H - 1 - yy : -1, l = 0;
                if (j >= 0) while (l < radius && inside[j * radius + l] < 128) l++;
                if (l <= W - 1 - l) RectFill(rp, l, yy, W - 1 - l, yy);
            }
        }
    }
    if (!opt.nodate) {
        WORD dx = (W - text_w(date_line)) / 2;
        if (card) text_px(dx, y + 2, date_line, c_dim, 0, c_dim);
        else text_at(rp, dx, y + 2, date_line, pen(P_DIM));
        y += row_h + 2;
    }
    for (i = 0; i < nrows; i++, y += row_h) {
        row *r = &rows[i];
        WORD by = y + 1, fw = 0, vx = bx + bar_w - 6 - text_w(r->value);
        int full = r->fill != (ULONG)-1 && r->fill >= 900;
        if (r->fill != (ULONG)-1 && r->fill > 0) {
            fw = (WORD)(bar_w * r->fill / 1000);
            if (fw < 2 * br) fw = 2 * br;
        }
        if (card) {
            text_px(bx - GAP - text_w(r->label), y + 2, r->label, c_text, 0, c_text);
            round_box(work_px, bx, by, bar_w, bh, br, c_track, 255);
            if (fw) round_box(work_px, bx, by, fw, bh, br, full ? c_full : c_fill, 255);
            text_px_contrast(vx, y + 2, r->value, c_text, full ? c_full_text : c_fill_text);
        } else {
            text_at(rp, bx - GAP - text_w(r->label), y + 2, r->label, pen(P_TEXT));
            SetAPen(rp, pen(P_TRACK));
            RectFill(rp, bx, by, bx + bar_w - 1, by + bh - 1);
            if (fw) { SetAPen(rp, pen(full ? P_FULL : P_FILL)); RectFill(rp, bx, by, bx + fw - 1, by + bh - 1); }
            text_at(rp, vx, y + 2, r->value, pen(P_TEXT));
        }
    }
}

static void show(void)
{
    if (!win) return;
    if (card) WritePixelArray(work_px, 0, 0, W * 4, win->RPort, 0, 0, W, H, RECTFMT_ARGB);
    else compose();
}

static void draw(void)
{
    if (!win) return;
    if (card) compose();
    show();
}

/* ---- seeing through, kept up to date (as the dock does) --------------------------------------- */

static int px0, py0;               /* where the panel is on the screen */
static ULONG *seen_line;           /* the screen beside the panel when what is behind it was copied */
static int line_x, line_y, line_n, line_across;
static int copy_dirty;             /* a window was over the place when it was copied: copy again once it has gone */
static WORD was_x, was_y, was_w, was_h;   /* where the panel was before it opened again; was_w 0: nowhere */

/* True while a window on the screen has its menus up. */
static int menus_up(void)
{
    int up = 0;
    ULONG lock = LockIBase(0);
    for (struct Window *v = scr->FirstWindow; v && !up; v = v->NextWindow) up = (v->Flags & WFLG_MENUSTATE) != 0;
    UnlockIBase(lock);
    return up;
}

/* True when a window other than a backdrop one (or ours) is over the box, or the menus are up: their boxes are
 * layers of no window, and a copy made through them put the menu in the panel and the panel over the menu (0.2.3,
 * the Team, 20 October 2026: "the meter going in front of the screen menu items"). */
static int covered(int x, int y, int w, int h)
{
    int over = 0;
    ULONG lock = LockIBase(0);
    for (struct Window *v = scr->FirstWindow; v && !over; v = v->NextWindow) {
        if (v->Flags & WFLG_MENUSTATE) over = 1;
        if (v == win || (v->Flags & WFLG_BACKDROP)) continue;
        over |= !(v->LeftEdge >= x + w || v->LeftEdge + v->Width <= x || v->TopEdge >= y + h || v->TopEdge + v->Height <= y);
    }
    UnlockIBase(lock);
    return over;
}

/* A line of the desktop beside the panel: above it, or below, or to its side when it is at the screen's edges. */
static void choose_line(void)
{
    line_n = 0;
    if (py0 - 1 > scr->BarHeight) { line_x = px0; line_y = py0 - 1; line_n = W; line_across = 1; }
    else if (py0 + H < scr->Height) { line_x = px0; line_y = py0 + H; line_n = W; line_across = 1; }
    else if (px0 > 0) { line_x = px0 - 1; line_y = py0; line_n = H; line_across = 0; }
    else if (px0 + W < scr->Width) { line_x = px0 + W; line_y = py0; line_n = H; line_across = 0; }
    if (line_across && line_x + line_n > scr->Width) line_n = scr->Width - line_x;
    if (!line_across && line_y + line_n > scr->Height) line_n = scr->Height - line_y;
}

static void read_line(ULONG *to)
{
    ReadPixelArray(to, 0, 0, line_across ? line_n * 4 : 4, &scr->RastPort, line_x, line_y,
                   line_across ? line_n : 1, line_across ? 1 : line_n, RECTFMT_ARGB);
}

/* True when the desktop beside the panel is no longer what it was when the copy was made: the backdrop was
 * drawn or changed since (a theme, a picture decoded), so what shows through is old. */
static int behind_stale(void)
{
    ULONG *now;
    int diff = 0, count = 0;
    if (!card || !seen_line || line_n <= 0) return 0;
    if (covered(line_x, line_y, line_across ? line_n : 1, line_across ? 1 : line_n)) return 0;
    if (!(now = AllocVec(line_n * 4, MEMF_ANY))) return 0;
    read_line(now);
    for (int i = 0; i < line_n; i += 4) {
        ULONG a = now[i], b = seen_line[i];
        int dr = (int)((a >> 16) & 255) - (int)((b >> 16) & 255), dg = (int)((a >> 8) & 255) - (int)((b >> 8) & 255),
            db = (int)(a & 255) - (int)(b & 255);
        diff += (dr < 0 ? -dr : dr) + (dg < 0 ? -dg : dg) + (db < 0 ? -db : db);
        count++;
    }
    FreeVec(now);
    return count && diff / count / 3 > 14;
}

/* ---- opening and closing ---------------------------------------------------------------------- */

static int placed;                 /* a place of its own (LEFT, TOP, or dragged): not the corner's */

static struct NewMenu newmenus[] = {
    { NM_TITLE, (STRPTR)"OpenMeters", NULL, 0, 0, NULL },
    { NM_ITEM, (STRPTR)"Refresh", (STRPTR)"R", 0, 0, (APTR)1 },
    { NM_ITEM, NM_BARLABEL, NULL, 0, 0, NULL },
    { NM_ITEM, (STRPTR)"Quit", (STRPTR)"Q", 0, 0, (APTR)2 },
    { NM_END, NULL, NULL, 0, 0, NULL }
};

static void close_panel(void)
{
    if (win) { was_x = win->LeftEdge; was_y = win->TopEdge; was_w = win->Width; was_h = win->Height; }
    if (win) { ClearMenuStrip(win); CloseWindow(win); win = NULL; }
    if (menus) { FreeMenus(menus); menus = NULL; }
    if (vi) { FreeVisualInfo(vi); vi = NULL; }
    text_free();
    if (shelf_px) { FreeVec(shelf_px); shelf_px = NULL; }
    if (work_px) { FreeVec(work_px); work_px = NULL; }
    if (seen_line) { FreeVec(seen_line); seen_line = NULL; }
    if (behind_bm) { WaitBlit(); FreeBitMap(behind_bm); behind_bm = NULL; }
    if (scr) free_pens();
    if (font) { CloseFont(font); font = NULL; }
    if (scr) { UnlockPubScreen(NULL, scr); scr = NULL; }
}

/* The panel at its place, made from what is behind it now, with the look as it is now. */
static int open_panel(void)
{
    WORD w, h, x, y;
    if (!(scr = LockPubScreen((STRPTR)"Workbench"))) return 0;
    read_look();
    if (!(font = icon_font())) { close_panel(); return 0; }
    bar_min = 0;
    read_dock();
    look_sig = look_signature();
    choose_tint();
    read_all();
    measure(&w, &h);
    if (!placed) {
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
    W = w; H = h; px0 = x; py0 = y;
    radius = h / 4 < MAX_RADIUS ? h / 4 : MAX_RADIUS;
    if (radius < 2) radius = 2;
    make_corner();
    /* what is behind the panel, before the window is there (a window over its place is copied too: then the
     * copy is made again once it has gone) */
    copy_dirty = covered(x, y, w, h);
    /* 0.2.5: in another box than it was (another font's size, a drag): the desktop where the panel was may not
     * be drawn again yet, and the copy kept the old panel's edges; copied again at the next tick */
    if (was_w && (was_x != x || was_y != y || was_w != w || was_h != h)) copy_dirty = 1;
    was_w = 0;
    card = truecolour();
    if (card) {
        shelf_px = AllocVec(W * H * 4, MEMF_ANY);
        work_px = AllocVec(W * H * 4, MEMF_ANY);
        if (!shelf_px || !work_px || !text_open(W)) {
            card = 0;
            if (shelf_px) { FreeVec(shelf_px); shelf_px = NULL; }
            if (work_px) { FreeVec(work_px); work_px = NULL; }
        }
    }
    if (card) {
        ReadPixelArray(shelf_px, 0, 0, W * 4, &scr->RastPort, x, y, W, H, RECTFMT_ARGB);
        choose_line();
        if (line_n > 0 && (seen_line = AllocVec(line_n * 4, MEMF_ANY))) read_line(seen_line);
        make_shelf();
    } else {
        if ((behind_bm = AllocBitMap(W, H, GetBitMapAttr(scr->RastPort.BitMap, BMA_DEPTH), 0, scr->RastPort.BitMap)))
            BltBitMap(scr->RastPort.BitMap, x, y, behind_bm, 0, 0, W, H, 0xc0, 0xff, NULL);
        eff = dk_bg == BG_CLEAR ? bg_c : tint;
    }
    choose_colours();
    if (!card) get_pens();
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
    if (!win) { close_panel(); return 0; }
    WindowToBack(win);                         /* one above the backdrop, as the dock: every other window goes over it */
    if ((vi = GetVisualInfoA(scr, NULL)) && (menus = CreateMenusA(newmenus, NULL)) && LayoutMenus(menus, vi, GTMN_NewLookMenus, TRUE, TAG_DONE))
        SetMenuStrip(win, menus);
    draw();
    return 1;
}

/* Settings, the look, the size or the place changed: the panel opens again, so what is behind it is copied afresh
 * (as the dock's relayout()). */
static int relayout(void)
{
    close_panel();
    Delay(8);                                  /* OpenBench redraws what the panel covered */
    return open_panel();
}

/* ---- the program ---------------------------------------------------------------------------- */

static int run(void)
{
    struct MsgPort *tport = NULL;
    struct timerequest *tr = NULL;
    int timer = 0, quit = 0, ticks = 0, wb_down = 0, stale_wait = 0, held = 0;
    placed = opt.left >= 0 && opt.top >= 0;
    wbc_start();
    if (!open_panel()) { wbc_stop(); return RETURN_FAIL; }
    if ((tport = CreateMsgPort()) && (tr = (struct timerequest *)CreateIORequest(tport, sizeof *tr)) &&
        !OpenDevice((STRPTR)TIMERNAME, UNIT_VBLANK, (struct IORequest *)tr, 0)) {
        timer = 1;
        tr->tr_node.io_Command = TR_ADDREQUEST;
        tr->tr_time.tv_secs = 1;
        tr->tr_time.tv_micro = 0;
        SendIO((struct IORequest *)tr);
    }
    while (!quit && (win || wb_down)) {
        ULONG got = Wait((win ? 1UL << win->UserPort->mp_SigBit : 0) | (timer ? 1UL << tport->mp_SigBit : 0) |
                         SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_F | wbc_sigmask());
        struct IntuiMessage *im;
        int again = 0, redraw = 0;
        if (got & SIGBREAKF_CTRL_C) quit = 1;
        /* Workbench is being reset (Font prefs' Use, a screen mode): no window, no lock on its screen until it is back */
        switch (wbc_take()) {
        case WBC_CLOSE: close_panel(); wb_down = 1; wbc_closed(); break;
        /* 0.2.2: only when the panel is down; a refused CloseWorkBench and the OpenWorkBench after it say
         * "open" twice, and the second opened a second, empty panel over the first */
        case WBC_OPEN: if (wb_down) wb_down = 2; break;
        }
        if (wb_down == 2 && open_panel()) wb_down = 0;
        if (got & SIGBREAKF_CTRL_F) again = 1;  /* told the look changed */
        if (timer && CheckIO((struct IORequest *)tr)) {
            WaitIO((struct IORequest *)tr);
            if (win && !wb_down) {
                ticks++;
                if (stale_wait > 0) stale_wait--;
                /* the look, its mode, the theme or the dock's settings changed: drawn again in the new look */
                if (look_signature() != look_sig) again = 1;
                /* what is behind it changed: a window over its place has gone, or the desktop was drawn again */
                else if (copy_dirty && !covered(px0, py0, W, H)) again = 1;
                else if (stale_wait <= 0 && behind_stale()) { again = 1; stale_wait = 3; }
                if (!again && ticks >= (opt.refresh > 0 ? opt.refresh : 2)) {
                    WORD w, h;
                    ticks = 0;
                    read_all();
                    measure(&w, &h);
                    if (w != W || h != H) again = 1; else redraw = 1;
                }
            }
            tr->tr_time.tv_secs = 1;
            tr->tr_time.tv_micro = 0;
            SendIO((struct IORequest *)tr);
        }
        while (win && (im = (struct IntuiMessage *)GetMsg(win->UserPort))) {
            ULONG cl = im->Class;
            UWORD code = im->Code;
            ReplyMsg((struct Message *)im);
            if (cl == IDCMP_REFRESHWINDOW) { BeginRefresh(win); show(); EndRefresh(win, TRUE); }
            else if (cl == IDCMP_CHANGEWINDOW && (win->LeftEdge != px0 || win->TopEdge != py0)) {
                /* dragged: kept there, and what is behind it there copied */
                opt.left = win->LeftEdge; opt.top = win->TopEdge; placed = 1;
                place_save();
                again = 1;
            } else if (cl == IDCMP_MENUPICK) {
                while (code != MENUNULL) {
                    struct MenuItem *it = ItemAddress(menus, code);
                    if (!it) break;
                    if (GTMENUITEM_USERDATA(it) == (APTR)1) again = 1;
                    else if (GTMENUITEM_USERDATA(it) == (APTR)2) quit = 1;
                    code = it->NextSelect;
                }
            }
        }
        if (quit || wb_down) continue;
        if (again && menus_up()) { held = 1; again = 0; }  /* opened again once the menus are down, not over them */
        else if (held && !menus_up()) { held = 0; again = 1; }
        if (again) { ticks = 0; if (!relayout()) wb_down = 2; }   /* not opened: tried again each second */
        else if (redraw) draw();
    }
    if (timer) { if (!CheckIO((struct IORequest *)tr)) AbortIO((struct IORequest *)tr); WaitIO((struct IORequest *)tr); CloseDevice((struct IORequest *)tr); }
    if (tr) DeleteIORequest((struct IORequest *)tr);
    if (tport) DeleteMsgPort(tport);
    close_panel();
    wbc_stop();
    if (theme_ok) { ogt_theme_free(&theme); theme_ok = 0; }
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
    CyberGfxBase = OpenLibrary((STRPTR)"cybergraphics.library", 41);   /* the panel mixed with the desktop on a card */
    DiskfontBase = OpenLibrary((STRPTR)"diskfont.library", 36);         /* Font prefs' icon font, from disk */
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
    if (CyberGfxBase) CloseLibrary(CyberGfxBase);
    if (DiskfontBase) CloseLibrary(DiskfontBase);
    if (IconBase) CloseLibrary(IconBase);
    if (GadToolsBase) CloseLibrary(GadToolsBase);
    if (UtilityBase) CloseLibrary(UtilityBase);
    if (GfxBase) CloseLibrary((struct Library *)GfxBase);
    if (IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);
    return rc;
}
