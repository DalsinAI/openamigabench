/* OpenBench 0.1: the desktop, in place of Workbench (amigachrome docs/design/Design-Workbench-Replacement.md,
 * section 5; the user, 11 October 2026: "surely us replacing it was the ask", "so you just boot openbench").
 *
 * The Startup-Sequence runs OpenBench where it ran LoadWB. OpenBench is then the desktop:
 *   - the root window, a backdrop on the Workbench screen: IPrefs' root picture (ENV:Sys/WBPattern.prefs),
 *     rendered the way IPrefs renders it for Workbench, in a process of its own so the desktop shows at
 *     once, centred over the theme's desktop gradient; the mounted volumes on it, down the left, at the
 *     places their Disk.info keep (a volume without one gets the default icon for its kind of drive, 0.1.17:
 *     def_Harddisk for a hard disk such as a new instance's Work:, which showed as a floppy before);
 *   - drawers, in the drawer windows approved and built as OpenDrawer (opengadtools workbench/wb_drawer);
 *   - starting things as Workbench starts them: a tool with LoadSeg and a WBStartup message, a project
 *     through its default tool, a drawer or disk in a drawer window; SYS:WBStartup at the start;
 *   - the Open preferences built in (0.1.14; the Team, 11 October 2026: "the open preferences into openbench
 *     natively"; Kestrel, Wren and Finch): Look, Windows, TitleBar, Menus, OpenDock's editor, OpenDateTime,
 *     OpenGamepads, OpenInput, OpenSound, OpenTypes, OpenUp-Setup and OpenSerial run as processes of OpenBench's
 *     when opened from SYS:Prefs (ob_builtin.c, ob_builtin_list.h);
 *   - Tata built in (0.1.15; the Team, 11 October 2026: Tata a built-in feature of OpenBench): before OpenBench
 *     runs a program or opens a project it asks Tata's own inspection, linked in (openamigatata
 *     src/tata_inspect.c), and runs it, asks Tata's question, or refuses; no patch. Tata's window is a built-in,
 *     SYS:Prefs/OpenTata. C:Tata's patches stay for every other program, and the question is asked once;
 *   - Information (0.1.13, ob_info.c): its own window for any icon, a drawer's or the desktop's (Icons >
 *     Information...), as workbench.library's WBInfo() does nothing while Workbench isn't running;
 *   - the screen's title, "OpenBench 0.1 - Chip, Fast, GPU = 1, CPU = 1 (m68k)", kept up to date, with
 *     "+ 8 (m68k)" when the CPU cores board is fitted ("+ 8 (m68k, x86-64)" when its cores run native jobs
 *     too), after room for OpenTitle's logo worked out from the logo's width and the bar's font (0.1.12,
 *     Common/barlogo.h in openamigaprefs, shared with OpenTitle: the bigger logo covered the first letter).
 *   - Workbench's settings (0.1.19): ENV:Sys/Workbench.prefs, Prefs/Workbench's file, read at the start and again
 *     whenever it changes: a tool started from an icon gets at least its default stack, and the devices it
 *     hides have no icon on the desktop. ENV:Sys/WBPattern.prefs is watched too: a new root picture (or
 *     none) shows at once, and the old picture's file is let go;
 *   - IPrefs' reset of the Workbench screen (0.1.19; Font prefs' Use, a screen mode): told through OpenLook's
 *     close notice (Common/wbclose.h, as the dock and OpenMeters are), OpenBench closes its root window, its
 *     drawers and its Information windows and lets go of the screen, then opens them again on the new one;
 *   - quitting (0.1.19): programs started from OpenBench (OpenMeters from SYS:WBStartup, say) are asked to quit
 *     with Ctrl-C, which they answer, as their code is OpenBench's to unload; before, quitting waited for them
 *     for ever;
 *   - picture thumbnails and icon sizes in the drawers (0.1.20, 0.1.21; Dale, 12 October 2026: "using datatypes to
 *     display picture thumbnails and variable size icons", the mockup approved the same evening): OpenDrawer's
 *     (opengadtools workbench/wb_thumb.c), its process started here and stopped before OpenBench ends;
 *   - Intuition's Workbench (0.1.22, the native screen mode's step 4): OpenBench gives Intuition its port
 *     (AlohaWorkbench) and marks its windows as the Workbench's, so CloseWorkBench() asks it to close them
 *     (WBENCHCLOSE) and OpenWorkBench() says the screen is back (WBENCHOPEN), with or without OpenLook's close
 *     notice. Its drawers come back where they were, moved in if they would now fall off the screen, and the
 *     backdrop is made for the new size. ScreenMode prefs' Use takes effect without a reboot;
 *   - Resize to Window (0.1.22): on a Native mode (openrtg.library 0.17), OpenBench > Resize to Window reopens
 *     the Workbench screen at the size the host's window asks for, and the 2-second timer does it when a page
 *     asks the Amiga to follow now (openrtg's ASK). Another program's window blocks it, as it blocks any
 *     reset: a requester names them, with Try Again and Cancel;
 *   - the ROM's prefs editors and dock (0.1.23; Dale, 17 October 2026, rom-blended.md M4): SYS:Prefs's window
 *     lists ROM:Prefs's editors it has no file for, with the ROM's icons, and starting one runs ROM:'s; one in
 *     SYS:Prefs runs itself only when its version is higher. The dock is no longer built in: OpenDock (the
 *     ROM's, or a newer C:OpenDock) is started by name when ENV:OpenDock/Dock is there;
 * Window frames are OpenFrames' (OpenLook), as on every window.
 *
 * RTG first (the user, 11 October 2026): OpenBench is made for true-colour screens, where windows and
 * screens live on the graphics card and chip RAM is hardly used: its own buffers come from fast RAM. The backdrop is a
 * screen-sized ARGB picture written with cybergraphics, icons are drawn with their alpha, colours are the
 * theme's exact ones. A palette screen (AGA) is the fallback: the backdrop is then pen 0 and the rest is
 * drawn with the pens the theme obtains.
 *
 * It never leaves the machine without a desktop: Shift held as it starts, or a start that fails, runs
 * C:LoadWB instead (the design's proxy rule, section 4). A machine whose OpenRTG reports no RTG (no
 * openrtg.library, or a Workbench screen that isn't a true-colour RTG one) gets Workbench as usual
 * (the user, 11 October 2026). Where LoadWB starts no Workbench of its own (0.1.24: the open ROM, whose
 * LoadWB starts OpenBench), OpenBench is the desktop there too, on the palette fallback. OpenBench patches nothing.
 *
 *   - drag and drop (0.1.26, W67; Dale, 18 October 2026: "dragging icons 'into' windows"): icons dragged out of
 *     a drawer window (its own picture under the pointer, several at once when several are selected) into
 *     another drawer window or onto a drawer's or a disk's icon are moved on one volume and copied to
 *     another, with OpenFiles' engine (openamigafilemanager engine/), in a process of its own with a Stop
 *     button; a disk's icon dragged from the desktop into a drawer window has its contents copied; icons
 *     let go over a program's AppWindow reach it as an AppMessage, as from OS 3.2.3's Workbench
 *     (ob_drop.c; workbench.library takes AddAppWindowA while OpenBench says it delivers them).
 *
 * Next (the user's order): Leave Out and Put Away (icons on the desktop, .backdrop); the rest of
 * workbench.library's entries (AppIcons, AppMenuItems) brought to OpenBench, as Directory Opus 5 takes them.
 *
 * MIT, Copyright (c) 2026 AmigaChrome Team. AROS's Wanderer and Directory Opus 5 were read for how, and no
 * code was taken from them or from Hyperion's. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <exec/memory.h>
#include <exec/ports.h>
#include <exec/execbase.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <dos/filehandler.h>
#include <dos/notify.h>
#include <devices/timer.h>
#include <devices/input.h>
#include <devices/inputevent.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <libraries/gadtools.h>
#include <libraries/configvars.h>
#include <graphics/gfxbase.h>
#include <workbench/startup.h>
#include <workbench/workbench.h>
#include <workbench/icon.h>
#include <datatypes/datatypes.h>
#include <datatypes/datatypesclass.h>
#include <datatypes/pictureclass.h>
#include <intuition/gadgetclass.h>
#include <graphics/view.h>
#include <utility/hooks.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/layers.h>
#include <proto/gadtools.h>
#include <proto/icon.h>
#include <proto/wb.h>
#include <proto/datatypes.h>
#include <clib/alib_protos.h>
#include <proto/expansion.h>
#include <proto/input.h>
#include <inline/macros.h>

#include "obmsg.h"
#include "wbclose.h"
#include "barlogo.h"
#include "ob_builtin.h"
#include "ob_info.h"
#ifdef OB_HAVE_TATA
#include "tata_inspect.h"
#endif
#include "wb_drawer.h"
#include "ob_drop.h"
#include "wb_dir.h"
#include "wb_fmt.h"
#include "wb_thumb.h"
#include "wb_pic.h"
#include "ogt_theme.h"
#include "ogt_draw.h"
#include "ogt_font.h"

#define OPENBENCH_VERSION "0.1.27"
static const char version[] __attribute__((used)) = "$VER: OpenBench " OPENBENCH_VERSION " (18.10.2026) MIT, Copyright (c) 2026 AmigaChrome Team";

struct Library *IconBase;
struct Library *WorkbenchBase;
struct Library *DataTypesBase;
struct ExpansionBase *ExpansionBase;   /* the type proto/expansion.h declares */
static struct Library *CyberGfxBase;

#define CGX_WRITE(src, sx, sy, mod, rp, dx, dy, w, h, fmt) \
    LP10(0x7e, ULONG, WritePixelArray, APTR, (src), a0, UWORD, (sx), d0, UWORD, (sy), d1, UWORD, (mod), d2, \
         struct RastPort *, (rp), a1, UWORD, (dx), d3, UWORD, (dy), d4, UWORD, (w), d5, UWORD, (h), d6, UBYTE, (fmt), d7, , CyberGfxBase)
#define RECTFMT_ARGB 2
#define CGX_MAPATTR(bm, a) LP2(0x60, ULONG, GetCyberMapAttr, struct BitMap *, (bm), a0, ULONG, (a), d0, , CyberGfxBase)
#define CYBRMATTR_DEPTH 0x80000007UL
#define CYBRMATTR_ISCYBERGFX 0x80000008UL

/* openrtg.library 0.17 (openamigartg include/openrtg/openrtg.h): the size the host's window asks for, read through
 * OpenRTG (OpenBench never reads a board itself), and the Native modes' ModeIDs (place 0x22 of OpenRTG's table) */
struct ob_native_size {
    UWORD width, height;                    /* asked for; 0 x 0 when nothing is */
    ULONG gen;                              /* up by one at each new size */
    ULONG ask;                              /* up by one each time a page asks the Amiga to follow now */
    UWORD limit_width, limit_height;        /* the board's largest picture */
    ULONG mode_id;
};
#define ORTG_NATIVESIZE(base, monitor, out) \
    LP2(0x7e, BOOL, ORTG_NativeSize, ULONG, (monitor), d0, struct ob_native_size *, (out), a0, , (base))
#define OB_IS_NATIVE(id) ((((id) >> 16) & 0xFFUL) == 0x22 && ((id) & 0x1000UL) && ((id) >> 28) == 6)

/* The windows run on 32 KB, however OpenBench is started (Run in the Startup-Sequence gives the boot
 * Shell's stack): libnix's __stack where its swap works, else a process of our own (as OpenDrawer). */
#define STACK_NEED 32768
unsigned long __stack = STACK_NEED;

#define PORT_NAME "OPENBENCH"
#define MAX_WINDOWS 24
#define MAX_VOLUMES 24
#define THEME_DIR "SYS:Prefs/Presets/Themes/"

static const char fallback_theme[] =
    "name Open\nversion 1\nfont \"DejaVu Sans\" 13\n[light]\n"
    "desktop #a3b5c7 0, #8fa3b6 55, #7a8fa5 100\n"
    "window #eceff2\ntext #1c242d\nlabel #1c242d\nmuted #4a5663\n"
    "button #dde2e8\nbutton.text #1c242d\nbutton.border #8794a2\nbutton.highlight #ffffff\n"
    "string #ffffff\nstring.shadow #8794a2\nstring.shine #ffffff\n"
    "accent #2f6fb3\naccent.text #ffffff\naccent.mark #1c242d\n"
    "selection.inactive #cdd4db\nlist #ffffff\nlist.alternate #f3f5f7\nlist.header #dde2e8\n"
    "group.line #8794a2\nmenu #f3f5f7\nmenu.text #1c242d\nmenu.line #cdd4db\nicon.text #0f1720\n"
    "track #cbd2da\nframe.active #5e6b79\n";

static wb_drawer *drawers[MAX_WINDOWS];
static int ndrawers;
static wb_look look;
static ogt_theme theme;
static ogt_ctx ctx;
static struct TextAttr ta;
static char screen_title[160];

/* ---- the theme, as OpenDrawer reads it ------------------------------------------------------- */

static char *load_text(const char *path)
{
    BPTR f = Open((CONST_STRPTR)path, MODE_OLDFILE);
    struct FileInfoBlock *fib;
    char *buf = NULL;
    if (!f) return NULL;
    if ((fib = AllocDosObject(DOS_FIB, NULL))) {
        if (ExamineFH(f, fib) && fib->fib_Size > 0 && fib->fib_Size < 65536 && (buf = malloc(fib->fib_Size + 1))) {
            LONG n = Read(f, buf, fib->fib_Size);
            buf[n > 0 ? n : 0] = 0;
        }
        FreeDosObject(DOS_FIB, fib);
    }
    Close(f);
    return buf;
}

struct ol_port_view {               /* OpenLook's public port: only the fields read here */
    struct MsgPort mp;
    ULONG magic;
    APTR rectfill, text;
    UBYTE cgx;
    UBYTE mode;
    char theme[48];
};
#define OL_PORT_NAME "OpenLook"
#define OL_MAGIC 0x4f4c3036UL
#define OL_PORT_PRI 5

static int look_mode_now(void)
{
    struct MsgPort *p;
    int mode = -1;
    Forbid();
    if ((p = FindPort((CONST_STRPTR)OL_PORT_NAME)) && p->mp_Node.ln_Pri == OL_PORT_PRI) {
        const struct ol_port_view *o = (const struct ol_port_view *)p;
        if (o->magic == OL_MAGIC && o->theme[0] && (o->mode == OGT_LIGHT || o->mode == OGT_DARK)) mode = o->mode;
    }
    Permit();
    return mode;
}

static int load_theme(int *mode)
{
    char name[80] = "Open", path[200], err[96];
    char *prefs = load_text("ENV:OpenGadTools/Look"), *text;
    ogt_rgb accent;
    int has_accent = 0, now;
    *mode = OGT_LIGHT;
    if (prefs) {
        char *line = prefs, *next;
        int first = 1;
        while (line && *line) {
            if ((next = strchr(line, '\n'))) *next++ = 0;
            if (first) {
                char *sp = strrchr(line, ' ');
                if (sp && !strcmp(sp + 1, "dark")) { *mode = OGT_DARK; *sp = 0; }
                else if (sp && (!strcmp(sp + 1, "auto") || !strcmp(sp + 1, "light"))) *sp = 0;
                if (line[0]) snprintf(name, sizeof name, "%s", line);
                first = 0;
            } else if (!strncmp(line, "accent ", 7) && ogt_parse_colour(line + 7, &accent)) {
                has_accent = 1;
            }
            line = next;
        }
        free(prefs);
    }
    if ((now = look_mode_now()) >= 0) *mode = now;
    snprintf(path, sizeof path, "%s%s%s", THEME_DIR, name, strstr(name, ".theme") ? "" : ".theme");
    if ((text = load_text(path))) {
        int ok = ogt_theme_parse(&theme, text, err, sizeof err);
        free(text);
        if (ok) {
            if (has_accent) ogt_theme_set_accent(&theme, accent);
            return 1;
        }
    }
    *mode = OGT_LIGHT;
    return ogt_theme_parse(&theme, fallback_theme, err, sizeof err);
}

/* ---- the screen's title ---------------------------------------------------------------------- */

static void thousands(char *out, int n, ULONG v)
{
    char tmp[16];
    int len, i, k = 0;
    snprintf(tmp, sizeof tmp, "%lu", (unsigned long)v);
    len = (int)strlen(tmp);
    for (i = 0; i < len && k < n - 1; i++) {
        if (i && (len - i) % 3 == 0 && k < n - 1) out[k++] = ',';
        out[k++] = tmp[i];
    }
    out[k] = 0;
}

/* The boards AmigaChrome presents through AutoConfig (AmigaChrome Team $DA15): product 9 or 15 a graphics card (ACRTG,
 * the 16 MiB board or the 64 MiB one; before 0.1.18 only 9 counted, so a 64 MiB machine showed GPU = 0),
 * product 7 the CPU cores board (ACSV block, CLASS 2, UNITS its cores). As OpenTitle 0.6.15 counts them. */
static ULONG gpus, cores, isas;           /* isas: what the cores run (ISAS, $28; bit 0 m68k always) */

static void count_boards(void)
{
    struct ConfigDev *cd = NULL;
    gpus = cores = 0;
    isas = 1;
    if (!(ExpansionBase = (struct ExpansionBase *)OpenLibrary((STRPTR)"expansion.library", 36)))
        return;
    while ((cd = FindConfigDev(cd, 0xDA15, 9)) && gpus < 16) gpus++;
    cd = NULL;
    while ((cd = FindConfigDev(cd, 0xDA15, 15)) && gpus < 16) gpus++;
    if ((cd = FindConfigDev(NULL, 0xDA15, 7)) && cd->cd_BoardAddr) {
        volatile ULONG *r = (volatile ULONG *)cd->cd_BoardAddr;
        if (r[0] == 0x41435356UL && r[0x20 / 4] == 2 && r[0x24 / 4] <= 1024) { cores = r[0x24 / 4]; isas = r[0x28 / 4] | 1; }
    }
    CloseLibrary((struct Library *)ExpansionBase);
    ExpansionBase = NULL;
}

/* 1 when the title changed. */
static int make_title(void)
{
    char chip[16], fast[16], t[sizeof screen_title], pad[BARLOGO_MAX_SPACES + 1];
    count_boards();                           /* each time: the cores board may give its cores after we start */
    thousands(chip, sizeof chip, AvailMem(MEMF_CHIP) >> 10);
    thousands(fast, sizeof fast, AvailMem(MEMF_FAST) >> 10);
    barlogo_pad(look.scr, pad);               /* made again each time: OpenLook may make the bar taller after we start */
    if (cores)
        snprintf(t, sizeof t, "%sOpenBench " OPENBENCH_VERSION " - Chip %s KB, Fast %s KB, GPU = %lu, CPU = 1 (m68k) + %lu (%s)",
                 pad, chip, fast, (unsigned long)gpus, (unsigned long)cores, barlogo_isas(isas));
    else
        snprintf(t, sizeof t, "%sOpenBench " OPENBENCH_VERSION " - Chip %s KB, Fast %s KB, GPU = %lu, CPU = 1 (m68k)",
                 pad, chip, fast, (unsigned long)gpus);
    if (!strcmp(t, screen_title))
        return 0;
    strcpy(screen_title, t);
    return 1;
}

/* ---- the root window: the backdrop ----------------------------------------------------------- */

static struct Window *root;
static ULONG *volatile backdrop;            /* the screen's size, ARGB, screen coordinates */
static LONG bd_w, bd_h;
static struct Hook backfill_hook;

struct backfill_msg { struct Layer *layer; struct Rectangle bounds; LONG ox, oy; };

/* The backdrop picture (0.1.6): read through datatypes, the Amiga's way, as ARGB pixels (PMODE_V43,
 * PDTM_READPIXELARRAY sent with DoMethodA) and cut and scaled to cover the screen; a PNG comes through
 * openpng.datatype (openamigaimage), our own decoder. 0.1 and 0.1.1 sent that read with DoDTMethodA, which
 * puts a GadgetInfo (NULL, with no window) where the pixel array's address goes: picture.datatype then
 * wrote the picture from address 0 up through Chip RAM, which was the boot crash on Instance-32 (found on
 * 11 October 2026; picture.datatype itself is fine). 0.1.3 to 0.1.5 went round it (IPrefs' render, then
 * ob_png, a reader of OpenBench's own). The IPrefs render (PDTA_Remap, DTM_PROCLAYOUT, a bitmap in the
 * screen's format, blitted centred) stays as the fallback when a datatype can't give its pixels. */
static Object *volatile pic_obj;
static struct BitMap *volatile pic_bm;
static volatile LONG pic_w, pic_h;

/* The layer's backfill, where the damage is: the gradient, then the picture over it. The bounds are
 * the screen's; a RastPort that still has its layer draws in the layer's coordinates. */
static ULONG bf_fill(struct RastPort *rp, struct backfill_msg *m)
{
    struct Rectangle *b = &m->bounds;
    LONG x0 = b->MinX, y0 = b->MinY, w = b->MaxX - b->MinX + 1, h = b->MaxY - b->MinY + 1, lx = 0, ly = 0;
    ULONG *bd = backdrop;
    struct BitMap *pb = pic_bm;
    if (rp->Layer) { lx = rp->Layer->bounds.MinX; ly = rp->Layer->bounds.MinY; }
    if (bd && CyberGfxBase && x0 >= 0 && y0 >= 0 && x0 + w <= bd_w && y0 + h <= bd_h && w > 0 && h > 0) {
        (void)CGX_WRITE(bd, (UWORD)x0, (UWORD)y0, (UWORD)(bd_w * 4), rp, (UWORD)(x0 - lx), (UWORD)(y0 - ly), (UWORD)w, (UWORD)h, RECTFMT_ARGB);
    } else {
        struct RastPort copy = *rp;
        SetAPen(&copy, 0);
        SetDrMd(&copy, JAM1);
        copy.AreaPtrn = NULL;
        RectFill(&copy, x0 - lx, y0 - ly, x0 - lx + w - 1, y0 - ly + h - 1);
    }
    if (pb) {
        /* the picture centred on the screen, where it meets the damage */
        LONG pw = pic_w, ph = pic_h, px = (bd_w - pw) / 2, py = (bd_h - ph) / 2;
        LONG ix0 = x0 > px ? x0 : px, iy0 = y0 > py ? y0 : py;
        LONG ix1 = x0 + w < px + pw ? x0 + w : px + pw, iy1 = y0 + h < py + ph ? y0 + h : py + ph;
        if (ix1 > ix0 && iy1 > iy0)
            BltBitMapRastPort(pb, ix0 - px, iy0 - py, rp, ix0 - lx, iy0 - ly, ix1 - ix0, iy1 - iy0, 0xC0);
    }
    return 0;
}

static ULONG bf_entry(register struct Hook *h __asm("a0"), register struct RastPort *rp __asm("a2"),
                      register struct backfill_msg *m __asm("a1"))
{
    (void)h;
    return bf_fill(rp, m);
}

/* The theme's desktop gradient, top to bottom, until the picture is ready (or without one). */
static ULONG *gradient_backdrop(LONG w, LONG h)
{
    ULONG *b = AllocVec((ULONG)w * h * 4, MEMF_FAST);
    ogt_grad g;
    LONG y, x;
    if (!b) return NULL;
    if (!ogt_theme_grad(&theme, ctx.mode, "desktop", &g) && !ogt_theme_grad(&theme, ctx.mode, "window", &g)) {
        memset(b, 0xaa, (ULONG)w * h * 4);
        return b;
    }
    for (y = 0; y < h; y++) {
        ogt_rgb c = ogt_grad_at10(&g, (int)(h > 1 ? y * 1000 / (h - 1) : 0));
        ULONG p = 0xff000000UL | ((ULONG)c.r << 16) | ((ULONG)c.g << 8) | c.b, *row = b + y * w;
        for (x = 0; x < w; x++) row[x] = p;
    }
    return b;
}

static int Lock_exists(const char *path)
{
    BPTR l = Lock((CONST_STRPTR)path, ACCESS_READ);
    if (l) UnLock(l);
    return l != 0;
}

/* The root picture IPrefs keeps (ENV:Sys/WBPattern.prefs, a PTRN chunk for WBP_ROOT naming a picture). */
static int root_picture(char *out, int n)
{
    BPTR f = Open((CONST_STRPTR)"ENV:Sys/WBPattern.prefs", MODE_OLDFILE);
    UBYTE buf[2048];
    LONG len, i;
    if (!f) return 0;
    len = Read(f, buf, sizeof buf);
    Close(f);
    for (i = 12; i + 8 + 24 <= len; ) {
        ULONG id = ((ULONG)buf[i] << 24) | ((ULONG)buf[i + 1] << 16) | ((ULONG)buf[i + 2] << 8) | buf[i + 3];
        ULONG sz = ((ULONG)buf[i + 4] << 24) | ((ULONG)buf[i + 5] << 16) | ((ULONG)buf[i + 6] << 8) | buf[i + 7];
        UBYTE *d = buf + i + 8;
        if (id == 0x5054524EUL && sz >= 24) {            /* PTRN */
            UWORD which = (UWORD)((d[16] << 8) | d[17]), flags = (UWORD)((d[18] << 8) | d[19]);
            UWORD dlen = (UWORD)((d[22] << 8) | d[23]);
            if (which == 0 && !(flags & 1) && dlen > 1 && 24 + dlen <= sz && i + 8 + 24 + dlen <= len) {
                int k = dlen < n - 1 ? dlen : n - 1;
                memcpy(out, d + 24, (size_t)k);
                out[k] = 0;
                return out[0] != 0;
            }
        }
        i += 8 + sz + (sz & 1);
    }
    return 0;
}

/* Workbench's settings (ENV:Sys/Workbench.prefs, Prefs/Workbench's file, which IPrefs hands to Hyperion's
 * Workbench), the ones the desktop has a use for: the default stack size (the WBNC chunk's first long word),
 * which a tool gets when its icon asks for less, and the hidden devices (a WBHD chunk each, the device's
 * name), whose volumes get no icon. Before 0.1.19 OpenBench read none of it. */
#define MAX_HIDDEN 16
static ULONG wb_stack = 8192;
static char wb_hidden[MAX_HIDDEN][32];
static int wb_nhidden;

static ULONG be32(const UBYTE *p)
{
    return ((ULONG)p[0] << 24) | ((ULONG)p[1] << 16) | ((ULONG)p[2] << 8) | p[3];
}

static void wb_prefs_load(void)
{
    BPTR f = Open((CONST_STRPTR)"ENV:Sys/Workbench.prefs", MODE_OLDFILE);
    UBYTE buf[2048];
    LONG len, i;
    wb_stack = 8192;
    wb_nhidden = 0;
    if (!f) return;
    len = Read(f, buf, sizeof buf);
    Close(f);
    if (len < 12 || be32(buf) != 0x464F524DUL || be32(buf + 8) != 0x50524546UL) return;   /* FORM, PREF */
    for (i = 12; i + 8 <= len; ) {
        ULONG id = be32(buf + i), sz = be32(buf + i + 4);
        const UBYTE *d = buf + i + 8;
        if (sz > (ULONG)(len - i - 8)) break;
        if (id == 0x57424E43UL && sz >= 4) {                /* WBNC: wbp_DefaultStackSize first */
            ULONG st = be32(d);
            if (st > wb_stack && st <= 1024 * 1024) wb_stack = st;
        } else if (id == 0x57424844UL && sz > 1 && wb_nhidden < MAX_HIDDEN) {   /* WBHD: a device's name */
            int k = 0;
            while (k < (int)sz && k < (int)sizeof wb_hidden[0] - 1 && d[k] && d[k] != ':') {
                wb_hidden[wb_nhidden][k] = (char)d[k];
                k++;
            }
            wb_hidden[wb_nhidden][k] = 0;
            if (k) wb_nhidden++;
        }
        i += 8 + sz + (sz & 1);
    }
}

/* 1 when the BCPL name b is one of the hidden devices, case aside. */
static int device_hidden(const UBYTE *b)
{
    int i, k;
    if (!b) return 0;
    for (i = 0; i < wb_nhidden; i++) {
        int n = (int)strlen(wb_hidden[i]);
        if (b[0] != n) continue;
        for (k = 0; k < n; k++)
            if ((b[1 + k] | 0x20) != (wb_hidden[i][k] | 0x20)) break;
        if (k == n) return 1;
    }
    return 0;
}

/* The picture rendered in a process of its own: decoding a big PNG takes a while on a 68k. */
static struct Task *main_task;
static BYTE picture_sig = -1;
static volatile int picture_busy;
static char picture_path[256];
static struct Screen *picture_scr;

/* The picture's screen-sized twin, when OpenUp put one beside it: "...-1600x1200.png" has
 * "...-1920x1080.png" for a 1920 x 1080 screen (OpenUpTool BACKDROP names them so). 1 when there is one. */
static int screen_twin(const char *path, LONG w, LONG h, char *out, int n)
{
    int len = (int)strlen(path), i, j;
    BPTR l;
    if (len < 9 || strcmp(path + len - 4, ".png")) return 0;
    for (i = len - 5; i > 0 && ((path[i] >= '0' && path[i] <= '9') || path[i] == 'x'); i--) ;
    if (path[i] != '-' || i == len - 5) return 0;
    for (j = i + 1; j < len - 4 && path[j] != 'x'; j++) ;
    if (j >= len - 4) return 0;
    snprintf(out, (size_t)n, "%.*s-%ldx%ld.png", i, path, (long)w, (long)h);
    if (!(l = Lock((CONST_STRPTR)out, ACCESS_READ))) return 0;
    UnLock(l);
    return 1;
}

/* The picture cut and scaled to cover W x H (its middle kept where its shape differs), or itself when
 * it is that size already. NULL when there is no memory. */
static ULONG *cover(ULONG *src, LONG sw, LONG sh, LONG W, LONG H)
{
    ULONG *dst;
    LONG vw, vh, ox, oy, x, y;
    if (sw == W && sh == H) return src;
    if (!(dst = AllocVec((ULONG)W * H * 4, MEMF_FAST))) return NULL;
    if (W * sh > H * sw) { vw = sw; vh = H * sw / W; }    /* the screen is wider: the picture's width */
    else { vh = sh; vw = W * sh / H; }
    ox = (sw - vw) / 2;
    oy = (sh - vh) / 2;
    for (y = 0; y < H; y++) {
        ULONG *row = dst + y * W, *s = src + (oy + y * vh / H) * sw + ox;
        for (x = 0; x < W; x++) row[x] = s[x * vw / W];
    }
    return dst;
}

static ULONG *volatile picture_ready;

/* The picture's pixels as ARGB through its datatype, opaque over the gradient where it has alpha. */
static ULONG *picture_pixels(const char *path, LONG *pw, LONG *ph)
{
    struct BitMapHeader *bmh = NULL;
    struct pdtBlitPixelArray m;
    ULONG *px = NULL, n, i;
    Object *o;
    *pw = *ph = 0;
    if (!DataTypesBase || !(o = NewDTObject((APTR)path, DTA_GroupID, GID_PICTURE, PDTA_Remap, FALSE,
                                            PDTA_DestMode, PMODE_V43, TAG_DONE)))
        return NULL;
    if (GetDTAttrs(o, PDTA_BitMapHeader, (ULONG)&bmh, TAG_DONE) && bmh && bmh->bmh_Width > 0 && bmh->bmh_Height > 0
        && (px = AllocVec((ULONG)bmh->bmh_Width * bmh->bmh_Height * 4, MEMF_FAST))) {
        m.MethodID = PDTM_READPIXELARRAY;
        m.pbpa_PixelData = px;
        m.pbpa_PixelFormat = PBPAFMT_ARGB;
        m.pbpa_PixelArrayMod = (ULONG)bmh->bmh_Width * 4;
        m.pbpa_Left = 0;
        m.pbpa_Top = 0;
        m.pbpa_Width = bmh->bmh_Width;
        m.pbpa_Height = bmh->bmh_Height;
        if (DoMethodA(o, (Msg)&m)) {             /* DoMethodA: see above */
            *pw = bmh->bmh_Width;
            *ph = bmh->bmh_Height;
            n = (ULONG)*pw * *ph;
            if (bmh->bmh_Masking != mskHasAlpha)
                for (i = 0; i < n; i++) px[i] |= 0xff000000UL;   /* 24-bit pictures come with alpha 0 */
        } else {
            FreeVec(px);
            px = NULL;
        }
    }
    DisposeDTObject(o);
    return px;
}

/* A picture with alpha over the gradient (both screen-sized). */
static void over_gradient(ULONG *pic, const ULONG *grad, ULONG n)
{
    ULONG i;
    for (i = 0; i < n; i++) {
        ULONG p = pic[i], a = p >> 24, q, r, g, b;
        if (a == 255) continue;
        q = grad ? grad[i] : 0;
        r = (((p >> 16) & 255) * a + ((q >> 16) & 255) * (255 - a) + 127) / 255;
        g = (((p >> 8) & 255) * a + ((q >> 8) & 255) * (255 - a) + 127) / 255;
        b = ((p & 255) * a + (q & 255) * (255 - a) + 127) / 255;
        pic[i] = 0xff000000UL | (r << 16) | (g << 8) | b;
    }
}

static void picture_entry(void)
{
    char twin[300];
    LONG pw = 0, ph = 0;
    ULONG *px = NULL, *done = NULL;
    Object *o = NULL;
    struct BitMap *bm = NULL;
    struct BitMapHeader *bmh = NULL;
    /* the screen-sized twin when there is one, else the picture; its pixels through datatypes */
    if (screen_twin(picture_path, bd_w, bd_h, twin, sizeof twin)) px = picture_pixels(twin, &pw, &ph);
    if (!px) px = picture_pixels(picture_path, &pw, &ph);
    if (px) {
        done = cover(px, pw, ph, bd_w, bd_h);
        if (done != px) FreeVec(px);
        if (done) over_gradient(done, backdrop, (ULONG)bd_w * bd_h);
    } else if (DataTypesBase && (o = NewDTObject((APTR)picture_path, DTA_GroupID, GID_PICTURE, PDTA_Screen, (ULONG)picture_scr,
                                                 PDTA_Remap, TRUE, PDTA_DestMode, PMODE_V43, OBP_Precision, PRECISION_IMAGE,
                                                 TAG_DONE))) {
        /* no pixels to be had: as IPrefs renders it, centred */
        struct gpLayout gl;
        gl.MethodID = DTM_PROCLAYOUT;
        gl.gpl_GInfo = NULL;
        gl.gpl_Initial = TRUE;
        if (DoDTMethodA(o, NULL, NULL, (Msg)&gl))   /* a gpLayout: its GInfo is the second longword, as DoDTMethodA expects */
            GetDTAttrs(o, PDTA_DestBitMap, (ULONG)&bm, PDTA_BitMapHeader, (ULONG)&bmh, TAG_DONE);
        if (!bm || !bmh || bmh->bmh_Width <= 0 || bmh->bmh_Height <= 0) {
            DisposeDTObject(o);
            o = NULL;
            bm = NULL;
        }
    }
    Forbid();
    if (done) picture_ready = done;
    else if (o) {
        pic_w = bmh->bmh_Width;
        pic_h = bmh->bmh_Height;
        pic_obj = o;
        pic_bm = bm;
    }
    picture_busy = 0;
    Signal(main_task, 1UL << picture_sig);
    /* ends in dos.library under Forbid: OpenBench waits for picture_busy before it goes */
}

/* ---- the desktop's icons --------------------------------------------------------------------- */

typedef struct dicon {
    char name[64];                          /* the volume's name, the label */
    char path[72];                          /* "Work:" */
    const char *kind;                       /* its drive's default icon, def_<kind>, when it has no Disk.info */
    struct DiskObject *dobj;
    LONG x, y, w, h;                        /* the picture's box in the root window */
    LONG ix, iy;                            /* where DrawIconState puts it for that box */
    int selected;
} dicon;

static dicon icons[MAX_VOLUMES];
static int nicons;
static struct DrawInfo *dri;
static LONG label_h;

#define LABEL_GAP 4
#define COLUMN_X 24
#define COLUMN_W 96

static void label_box(dicon *ic, LONG *lx, LONG *ly, LONG *lw)
{
    struct RastPort *rp = root->RPort;
    LONG tw = ogt_text_width(rp, ic->name), max = COLUMN_W + 16;
    if (tw > max) tw = max;
    *lw = tw;
    *lx = ic->x + (ic->w - tw) / 2;
    *ly = ic->y + ic->h + LABEL_GAP;
}

/* The box an icon covers with its label, for clearing. */
static void icon_extent(dicon *ic, LONG *x0, LONG *y0, LONG *x1, LONG *y1)
{
    LONG lx, ly, lw;
    label_box(ic, &lx, &ly, &lw);
    *x0 = (lx - 5 < ic->x ? lx - 5 : ic->x) - 1;
    *x1 = (lx + lw + 5 > ic->x + ic->w ? lx + lw + 5 : ic->x + ic->w) + 1;
    *y0 = ic->y - 1;
    *y1 = ly + label_h + 2;
}

static void draw_icon(dicon *ic)
{
    struct RastPort *rp = root->RPort;
    LONG lx, ly, lw;
    if (!ic->dobj) return;
    DrawIconState(rp, ic->dobj, NULL, ic->ix, ic->iy, ic->selected ? IDS_SELECTED : IDS_NORMAL,
                  ICONDRAWA_Frameless, TRUE, ICONDRAWA_Borderless, TRUE, ICONDRAWA_EraseBackground, FALSE,
                  ICONDRAWA_DrawInfo, (ULONG)dri, TAG_DONE);
    label_box(ic, &lx, &ly, &lw);
    if (ic->selected) {
        ogt_box(rp, ogt_pen(&ctx, "accent"), (int)lx - 5, (int)ly - 1, (int)lw + 10, (int)label_h + 2);
        ogt_text(rp, ogt_pen(&ctx, "accent.text"), (int)lx, (int)ly, ic->name, (int)lw);
    } else
        ogt_text(rp, ogt_pen(&ctx, "icon.text"), (int)lx, (int)ly, ic->name, (int)lw);
}

static void draw_icons(void)
{
    int i;
    for (i = 0; i < nicons; i++) draw_icon(&icons[i]);
}

/* A box of the root window cleared to the backdrop, and every icon in it drawn again where it is now. */
static void redraw_box(LONG x0, LONG y0, LONG x1, LONG y1)
{
    int i;
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > root->Width - 1) x1 = root->Width - 1;
    if (y1 > root->Height - 1) y1 = root->Height - 1;
    if (x1 < x0 || y1 < y0) return;
    EraseRect(root->RPort, x0, y0, x1, y1);
    for (i = 0; i < nicons; i++) {
        LONG a0, b0, a1, b1;
        icon_extent(&icons[i], &a0, &b0, &a1, &b1);
        if (a0 <= x1 && a1 >= x0 && b0 <= y1 && b1 >= y0) draw_icon(&icons[i]);
    }
}

/* An icon cleared to the backdrop and drawn again, and any icon it overlapped. */
static void redraw_around(dicon *ic)
{
    LONG x0, y0, x1, y1;
    icon_extent(ic, &x0, &y0, &x1, &y1);
    redraw_box(x0, y0, x1, y1);
}

/* An icon moved to nx, ny (0.1.12): moved first, then the box it left cleared and the icons in it drawn
 * again, itself included where it overlaps its old box, then itself. 0.1.11 cleared the old box and drew
 * the icons in it before moving, the moving one among them at its old place: a copy at every step of a
 * drag (Instance-32, 11 October 2026). */
static void move_icon(dicon *ic, LONG nx, LONG ny)
{
    LONG x0, y0, x1, y1;
    icon_extent(ic, &x0, &y0, &x1, &y1);
    ic->ix += nx - ic->x;
    ic->iy += ny - ic->y;
    ic->x = nx;
    ic->y = ny;
    redraw_box(x0, y0, x1, y1);
    draw_icon(ic);
}

static void free_icons(void)
{
    int i;
    for (i = 0; i < nicons; i++)
        if (icons[i].dobj) FreeDiskObject(icons[i].dobj);
    nicons = 0;
}

static int ci_cmp(const char *a, const char *b)
{
    for (; *a && *b; a++, b++) {
        int x = *a >= 'A' && *a <= 'Z' ? *a + 32 : *a, y = *b >= 'A' && *b <= 'Z' ? *b + 32 : *b;
        if (x != y) return x - y;
    }
    return (unsigned char)*a - (unsigned char)*b;
}

static int by_name(const void *a, const void *b)
{
    return ci_cmp(((const dicon *)a)->name, ((const dicon *)b)->name);
}

/* 1 when the BCPL name b is s, case aside (s in lower case); or starts with s when prefix is set. */
static int bname_is(const UBYTE *b, const char *s, int prefix)
{
    int i, n = (int)strlen(s);
    if (!b || b[0] < n || (!prefix && b[0] != n)) return 0;
    for (i = 0; i < n; i++)
        if ((b[1 + i] | 0x20) != s[i]) return 0;
    return 1;
}

/* The kind of drive a volume is in, for its icon when it has no Disk.info: Workbench's default icon
 * names, def_<kind>. A floppy is "Disk", the RAM disk "RAM", a CD "CDROM", and every other drive a hard
 * disk, "Harddisk". 0.1.17: a volume without a Disk.info was given icon.library's default disk icon, which
 * is a floppy, so a hard disk partition (Work: on a new instance, which has no Disk.info) showed as a
 * floppy. Called with the DOS list locked for volumes and devices; start is what LockDosList gave. */
static const char *drive_kind(struct DosList *start, const struct DosList *vol)
{
    struct DosList *d = start;
    while ((d = NextDosEntry(d, LDF_DEVICES))) {
        const UBYTE *n = (const UBYTE *)BADDR(d->dol_Name);
        ULONG su = (ULONG)d->dol_misc.dol_handler.dol_Startup;
        if (d->dol_Task != vol->dol_Task) continue;
        if (bname_is(n, "ram", 0)) return "RAM";
        if (bname_is(n, "cd", 1)) return "CDROM";
        if (bname_is(n, "df", 1)) return "Disk";
        if (su > 1024 && !(su & 0xC0000000UL)) {     /* a FileSysStartupMsg, not a number some handlers keep there */
            const struct FileSysStartupMsg *fssm = (const struct FileSysStartupMsg *)BADDR(su);
            const UBYTE *dev = (const UBYTE *)BADDR(fssm->fssm_Device);
            if (bname_is(dev, "trackdisk.device", 0) || bname_is(dev, "mfm.device", 0)) return "Disk";
        }
        break;
    }
    return "Harddisk";
}

/* 1 when the volume is in a device Workbench's settings hide (wb_prefs_load). Called with the DOS list
 * locked for volumes and devices; start is what LockDosList gave. */
static int volume_hidden(struct DosList *start, const struct DosList *vol)
{
    struct DosList *d = start;
    if (!wb_nhidden) return 0;
    while ((d = NextDosEntry(d, LDF_DEVICES)))
        if (d->dol_Task == vol->dol_Task && device_hidden((const UBYTE *)BADDR(d->dol_Name))) return 1;
    return 0;
}

/* The mounted volumes, their Disk.info (where there is none, the default icon for their kind of drive),
 * placed where their icons say, the rest down the left. */
static void scan_volumes(void)
{
    struct DosList *dl, *start;
    LONG next_y = 12;
    int i;
    free_icons();
    dl = start = LockDosList(LDF_VOLUMES | LDF_DEVICES | LDF_READ);
    while ((dl = NextDosEntry(dl, LDF_VOLUMES)) && nicons < MAX_VOLUMES) {
        UBYTE *b = (UBYTE *)BADDR(dl->dol_Name);
        int n;
        if (!dl->dol_Task || !b) continue;          /* not in a drive */
        if (volume_hidden(start, dl)) continue;     /* hidden in Workbench's settings */
        n = b[0] < 63 ? b[0] : 63;
        memcpy(icons[nicons].name, b + 1, (size_t)n);
        icons[nicons].name[n] = 0;
        icons[nicons].kind = drive_kind(start, dl);
        nicons++;
    }
    UnLockDosList(LDF_VOLUMES | LDF_DEVICES | LDF_READ);
    qsort(icons, (size_t)nicons, sizeof icons[0], by_name);
    for (i = 0; i < nicons; i++) {
        dicon *ic = &icons[i];
        char ip[96];
        struct Rectangle r;
        snprintf(ic->path, sizeof ic->path, "%s:", ic->name);
        snprintf(ip, sizeof ip, "%s:Disk", ic->name);
        ic->selected = 0;
        ic->dobj = GetIconTags((CONST_STRPTR)ip, ICONGETA_FailIfUnavailable, TRUE, ICONGETA_Screen, (ULONG)look.scr, TAG_DONE);
        if (!ic->dobj)                              /* no Disk.info: def_<kind>, or the default disk icon */
            ic->dobj = GetIconTags(NULL, ICONGETA_GetDefaultName, (ULONG)ic->kind, ICONGETA_GetDefaultType, WBDISK,
                                   ICONGETA_Screen, (ULONG)look.scr, TAG_DONE);
        ic->w = 48; ic->h = 40;
        r.MinX = r.MinY = 0;
        if (ic->dobj && GetIconRectangle(root->RPort, ic->dobj, NULL, &r, ICONDRAWA_Borderless, TRUE, TAG_DONE)) {
            ic->w = r.MaxX - r.MinX + 1;
            ic->h = r.MaxY - r.MinY + 1;
        }
        if (ic->dobj && ic->dobj->do_CurrentX != (LONG)NO_ICON_POSITION && ic->dobj->do_CurrentX >= 0 &&
            ic->dobj->do_CurrentY >= 0 && ic->dobj->do_CurrentX < root->Width - 16 && ic->dobj->do_CurrentY < root->Height - 16) {
            ic->x = ic->dobj->do_CurrentX;
            ic->y = ic->dobj->do_CurrentY;
        } else {
            ic->x = COLUMN_X + (COLUMN_W - ic->w) / 2;
            ic->y = next_y;
        }
        if (ic->y + ic->h + label_h + 24 > next_y) next_y = ic->y + ic->h + label_h + 24;
        ic->ix = ic->x - r.MinX;
        ic->iy = ic->y - r.MinY;
    }
}

static dicon *icon_at(LONG x, LONG y)
{
    int i;
    for (i = nicons - 1; i >= 0; i--) {
        LONG x0, y0, x1, y1;
        icon_extent(&icons[i], &x0, &y0, &x1, &y1);
        if (x >= x0 && x <= x1 && y >= y0 && y <= y1) return &icons[i];
    }
    return NULL;
}

/* ---- starting things, as Workbench starts them ------------------------------------------------ */

/* A tool's run: its WBStartup and its arguments, back on launch_port when it ends. */
typedef struct launch {
    struct WBStartup wbs;
    struct WBArg args[2];
    char names[2][108];
    struct launch *next;                    /* the tools running, for asking them to quit */
} launch;

static struct MsgPort *launch_port;
static int running;                         /* tools started whose message hasn't come back */
static launch *launches;

static int open_drawer(const char *path);

/* A lock on the drawer holding path, and the name in it. */
static BPTR parent_and_name(const char *path, char *name, int n)
{
    BPTR l = Lock((CONST_STRPTR)path, ACCESS_READ), p;
    const char *fp = (const char *)FilePart((STRPTR)path);
    if (!l) {
        /* 0.1.25: a project that is only its icon, as SYS:System/Shell is (Shell.info, default tool
         * SYS:System/CLI, no file Shell): its WBArg is its drawer and its name, as Workbench gives it. Before,
         * no lock meant no start: OpenDock's "wb System:System/Shell" button and a double-click on the Shell
         * icon opened nothing (lab t5-events, W53, Dale's disk: ob_open answered 0) */
        char dir[256];
        int k = (int)((const char *)PathPart((STRPTR)path) - path);
        if (!*fp || k >= (int)sizeof dir) return 0;
        memcpy(dir, path, (size_t)k);
        dir[k] = 0;
        if (!(p = Lock((CONST_STRPTR)dir, ACCESS_READ))) return 0;
        snprintf(name, (size_t)n, "%s", fp);
        return p;
    }
    p = ParentDir(l);
    UnLock(l);
    snprintf(name, (size_t)n, "%s", fp);
    return p;
}

/* A tool started with a WBStartup message: arg 0 the tool, arg 1 the project when there is one. */
static int start_tool(const char *tool, const char *project, LONG stack)
{
    launch *L = AllocVec(sizeof *L, MEMF_PUBLIC | MEMF_CLEAR);
    BPTR seg;
    struct Process *p;
    int n = 1;
    if (!L) return 0;
    if (!(L->args[0].wa_Lock = parent_and_name(tool, L->names[0], sizeof L->names[0]))) { FreeVec(L); return 0; }
    L->args[0].wa_Name = (STRPTR)L->names[0];
    if (project) {
        if (!(L->args[1].wa_Lock = parent_and_name(project, L->names[1], sizeof L->names[1]))) {
            UnLock(L->args[0].wa_Lock);
            FreeVec(L);
            return 0;
        }
        L->args[1].wa_Name = (STRPTR)L->names[1];
        n = 2;
    }
    if (!(seg = LoadSeg((CONST_STRPTR)tool))) {
        UnLock(L->args[0].wa_Lock);
        if (n > 1) UnLock(L->args[1].wa_Lock);
        FreeVec(L);
        return 0;
    }
    if (stack < (LONG)wb_stack) stack = (LONG)wb_stack;   /* Workbench's default stack, 8192 at the least */
    /* As Workbench starts a tool: no input or output (Output() is 0 for it) and no current directory
     * (NP_CurrentDir 0: the tool's own startup makes the WBArg's lock its current directory).
     * 0.1.7: dos's default input and output (NIL:, closed at the end) made every libnix tool fail on
     * quitting with 8700000E, a file closed twice. 0.1.10: with a current directory of its own (a DupLock
     * dos unlocks at the end), a tool that leaves the WBArg's lock as its current directory (Mounter) had
     * that lock unlocked by dos and again by launch_done: 81000005, memory list corrupt. */
    p = CreateNewProcTags(NP_Seglist, (ULONG)seg, NP_FreeSeglist, FALSE, NP_StackSize, (ULONG)stack,
                          NP_Name, (ULONG)L->names[0], NP_CurrentDir, 0,
                          NP_HomeDir, (ULONG)DupLock(L->args[0].wa_Lock), NP_Input, 0, NP_Output, 0,
                          NP_CloseInput, FALSE, NP_CloseOutput, FALSE, TAG_DONE);
    if (!p) {
        UnLoadSeg(seg);
        UnLock(L->args[0].wa_Lock);
        if (n > 1) UnLock(L->args[1].wa_Lock);
        FreeVec(L);
        return 0;
    }
    L->wbs.sm_Message.mn_Node.ln_Type = NT_MESSAGE;
    L->wbs.sm_Message.mn_ReplyPort = launch_port;
    L->wbs.sm_Message.mn_Length = sizeof(struct WBStartup);
    L->wbs.sm_Process = &p->pr_MsgPort;
    L->wbs.sm_Segment = seg;
    L->wbs.sm_NumArgs = n;
    L->wbs.sm_ToolWindow = NULL;
    L->wbs.sm_ArgList = L->args;
    L->next = launches;
    launches = L;
    PutMsg(&p->pr_MsgPort, &L->wbs.sm_Message);
    running++;
    return 1;
}

static void launch_done(launch *L)
{
    launch **pp;
    int i;
    for (pp = &launches; *pp; pp = &(*pp)->next)
        if (*pp == L) { *pp = L->next; break; }
    UnLoadSeg(L->wbs.sm_Segment);
    for (i = 0; i < L->wbs.sm_NumArgs && i < 2; i++)
        if (L->args[i].wa_Lock) UnLock(L->args[i].wa_Lock);
    FreeVec(L);
    running--;
}

/* The tools that have ended, their code unloaded. */
static void launches_reap(void)
{
    struct Message *lm;
    while ((lm = GetMsg(launch_port))) launch_done((launch *)lm);
}

/* The names of the tools still running, one to a line, for a requester. */
static void running_names(char *out, int n)
{
    launch *L;
    int k = 0, shown = 0;
    out[0] = 0;
    for (L = launches; L && k < n - 1; L = L->next) {
        if (++shown > 8) { snprintf(out + k, (size_t)(n - k), "  and %d more\n", running - 8); break; }
        int start = k + 2;
        k += snprintf(out + k, (size_t)(n - k), "  %s\n", L->names[0]);
        if (k >= n) k = n - 1;
        for (; start < k; start++)
            if (out[start] == '%') out[start] = '_';  /* the requester's text is a format */
        if (k >= n - 1) break;
    }
}

/* 0.1.19: the tools started from OpenBench are asked to quit, with Ctrl-C as a Shell's Break asks, and waited
 * for up to secs seconds. Their code is OpenBench's to unload, so it can't go before them: OpenMeters from
 * SYS:WBStartup kept OpenBench's quit waiting for ever. 1 when none is left. */
static int ask_running_to_quit(int secs)
{
    launch *L;
    int i;
    launches_reap();
    Forbid();
    for (L = launches; L; L = L->next)                 /* not yet replied: its process is there */
        if (L->wbs.sm_Message.mn_Node.ln_Type != NT_REPLYMSG && L->wbs.sm_Process)
            Signal(L->wbs.sm_Process->mp_SigTask, SIGBREAKF_CTRL_C);
    Permit();
    for (i = 0; running && i < secs * 10; i++) {
        Delay(5);
        launches_reap();
    }
    return !running;
}

/* 1 when the file is a program (an AmigaDOS load file: HUNK_HEADER). */
static int is_program(const char *path)
{
    BPTR f = Open((CONST_STRPTR)path, MODE_OLDFILE);
    ULONG magic = 0;
    if (!f) return 0;
    if (Read(f, &magic, 4) != 4) magic = 0;
    Close(f);
    return magic == 0x000003F3UL;
}

/* A project's default tool, as a full path: as it is when it names a place, else looked for beside the
 * project, then where Workbench's own tools are. 1 when found. */
static int find_tool(const char *project, const char *tool, char *out, int n)
{
    static const char *const places[] = { "", "C:", "SYS:Utilities/", "SYS:Tools/", "SYS:System/", "SYS:Prefs/",
                                          "SYS:Tools/Commodities/", "SYS:Apps/", NULL };
    char try_[300];
    BPTR l;
    int i;
    if (!tool || !tool[0]) return 0;
    if (strchr(tool, ':') || strchr(tool, '/')) {
        BPTR pd, old;
        char nm[4];
        /* relative to the project's drawer, or a full path */
        if ((pd = parent_and_name(project, nm, sizeof nm))) {
            old = CurrentDir(pd);
            l = Lock((CONST_STRPTR)tool, ACCESS_READ);
            CurrentDir(old);
            UnLock(pd);
        } else
            l = Lock((CONST_STRPTR)tool, ACCESS_READ);
        if (l) {
            int ok = NameFromLock(l, (STRPTR)out, n) != 0;
            UnLock(l);
            return ok;
        }
        return 0;
    }
    for (i = 0; places[i]; i++) {
        if (!places[i][0]) {
            if (snprintf(try_, sizeof try_, "%s", project) >= (int)sizeof try_)
                continue;                                   /* too long a path to try */
            *(char *)PathPart((STRPTR)try_) = 0;
            AddPart((STRPTR)try_, (CONST_STRPTR)tool, sizeof try_);
        } else
            snprintf(try_, sizeof try_, "%s%s", places[i], tool);
        if ((l = Lock((CONST_STRPTR)try_, ACCESS_READ))) {
            int ok = NameFromLock(l, (STRPTR)out, n) != 0;
            UnLock(l);
            if (ok) return 1;
        }
    }
    return 0;
}

/* Opens anything as Workbench would: a disk, drawer or trashcan in a drawer window; a tool runs; a
 * project opens with its default tool; a program with no icon runs. 1 when done. */
static int ob_info_object(const char *path)
{
    return ob_info_open(look.scr, path);
}

/* 1 while SYS:WBStartup is being started: Tata applies "always" and "never" then, and asks nothing */
static int starting_up;

/* Tata, built in (tata_inspect.h): 1 to go ahead with the program (data 0) or the project (data 1) */
static int tata_lets(const char *path, int data)
{
#ifdef OB_HAVE_TATA
    return tata_inspect(path, data, starting_up);
#else
    (void)path; (void)data;
    return 1;
#endif
}

/* ---- ROM:Prefs: the ROM's prefs editors in SYS:Prefs's window (rom-blended.md, M4) ---------------------------
 *
 * ROM: (amigachrome-guest commands/rom/ROMVOL.md) holds our prefs editors in ROM:Prefs, with their icons.
 * SYS:Prefs's drawer window lists each one SYS:Prefs has no file of that name for, with the ROM's icon; a
 * listing is answered from ROM:'s index, so nothing is unpacked until one is started. Starting one runs
 * ROM:'s copy. An editor that is in SYS:Prefs runs itself when its version is higher than ROM:'s; otherwise
 * ROM:'s runs. With no ROM: everything is as before. wb_dir.c (opengadtools) is compiled with its
 * wb_dir_read and wb_entry_icon renamed *_disk (desktop/build.sh); the two below wrap them. */
#define ROM_PREFS "ROM:Prefs"

static int quiet_exists(const char *path);

static int same_name(const char *a, const char *b)
{
    for (; *a && *b; a++, b++) {
        int x = *a >= 'A' && *a <= 'Z' ? *a + 32 : *a, y = *b >= 'A' && *b <= 'Z' ? *b + 32 : *b;
        if (x != y) return 0;
    }
    return *a == *b;
}

/* A file's version, (version << 16) | revision, from its $VER line; 0 when it has none. A file on ROM: is
 * asked of ROM:'s handler (its private packet 0x4F430006, as Version does), so its group stays packed; the
 * handler answers the line without its "$VER: " (commands/rom/romvol_core.cpp). */
static ULONG file_version(const char *path)
{
    static char buf[1024];
    struct Process *me = (struct Process *)FindTask(NULL);
    APTR old = me->pr_WindowPtr;
    struct DevProc *dp;
    struct MsgPort *romport = NULL;
    BPTR fh;
    char *v = NULL;
    const char *t;
    ULONG ver = 0, rev = 0;
    me->pr_WindowPtr = (APTR)-1;
    if ((dp = GetDeviceProc((CONST_STRPTR)"ROM:", NULL))) { romport = dp->dvp_Port; FreeDeviceProc(dp); }
    fh = Open((CONST_STRPTR)path, MODE_OLDFILE);
    me->pr_WindowPtr = old;
    if (!fh) return 0;
    if (romport && ((struct FileHandle *)BADDR(fh))->fh_Type == romport) {
        memset(buf, 0, sizeof buf);
        if (DoPkt(romport, 0x4F430006, ((struct FileHandle *)BADDR(fh))->fh_Arg1, (LONG)buf, sizeof buf - 1, 0, 0))
            v = buf;
    } else {
        LONG n, keep = 0;
        while (!v && (n = Read(fh, buf + keep, sizeof buf - 1 - keep)) > 0) {
            LONG i, len = keep + n;
            for (i = 0; i + 5 <= len; i++)
                if (!memcmp(buf + i, "$VER:", 5)) {
                    /* the line may go on past the buffer: what is here, then the rest */
                    memmove(buf, buf + i, (size_t)(len - i));
                    len -= i;
                    if ((n = Read(fh, buf + len, sizeof buf - 1 - len)) > 0) len += n;
                    buf[len] = 0;
                    v = buf + 5;                   /* after "$VER:" */
                    break;
                }
            if (!v) {                              /* "$VER" may be cut at the end: keep its 4 bytes */
                keep = len < 4 ? len : 4;
                memmove(buf, buf + len - keep, (size_t)keep);
            }
        }
    }
    Close(fh);
    if (!v) return 0;
    /* "name 1.2 (date)": the first number after a space */
    for (t = v; *v && *v != '\n' && *v != '\r'; v++)
        if (v > t && v[-1] == ' ' && *v >= '0' && *v <= '9') {
            for (; *v >= '0' && *v <= '9'; v++) ver = ver * 10 + (ULONG)(*v - '0');
            if (*v == '.') for (v++; *v >= '0' && *v <= '9'; v++) rev = rev * 10 + (ULONG)(*v - '0');
            return (ver & 0xFFFF) << 16 | (rev & 0xFFFF);
        }
    return 0;
}

/* 1 when path names the SYS:Prefs drawer itself, however it is spelt (SYS:Prefs, System:Prefs, DH0:Prefs). */
static int is_sys_prefs(const char *path)
{
    static char last[WB_PATH_MAX];
    static int last_is;
    struct Process *me = (struct Process *)FindTask(NULL);
    APTR old;
    BPTR a, b;
    int is = 0;
    if (!strcmp(path, last)) return last_is;
    old = me->pr_WindowPtr;
    me->pr_WindowPtr = (APTR)-1;
    if ((a = Lock((CONST_STRPTR)path, ACCESS_READ))) {
        if ((b = Lock((CONST_STRPTR)"SYS:Prefs", ACCESS_READ))) {
            is = SameLock(a, b) == LOCK_SAME;
            UnLock(b);
        }
        UnLock(a);
    }
    me->pr_WindowPtr = old;
    strlcpy(last, path, sizeof last);
    last_is = is;
    return is;
}

/* ROM:Prefs/NAME when SYS:Prefs/NAME isn't there and ROM:Prefs/NAME is: an entry the window lists from ROM:. */
static int rom_entry(const wb_dir *d, int i, char *out, int n)
{
    char sys[WB_PATH_MAX];
    if (d->e[i].drawer || !is_sys_prefs(d->path)) return 0;
    wb_entry_path(d, i, sys, sizeof sys);
    snprintf(out, (size_t)n, ROM_PREFS "/%s", d->e[i].name);
    return !quiet_exists(sys) && quiet_exists(out);
}

/* What starting path runs: 1 ROM:'s editor (its path in out), 2 the file in SYS:Prefs itself (its version is
 * higher than ROM:'s), 0 as before (no ROM: copy, or not in SYS:Prefs). */
static int rom_prefs_target(const char *path, char *out, int n)
{
    char dir[WB_PATH_MAX];
    size_t k = (size_t)((const char *)PathPart((CONST_STRPTR)path) - path);
    if (k >= sizeof dir) return 0;
    memcpy(dir, path, k);
    dir[k] = 0;
    if (!is_sys_prefs(dir)) return 0;
    snprintf(out, (size_t)n, ROM_PREFS "/%s", (const char *)FilePart((CONST_STRPTR)path));
    if (!quiet_exists(out)) return 0;
    if (quiet_exists(path) && file_version(path) > file_version(out)) return 2;
    return 1;
}

int wb_dir_read_disk(wb_dir *d, const char *path, int show_all);
struct DiskObject *wb_entry_icon_disk(wb_dir *d, int i, struct Screen *scr);

/* The drawer as wb_dir reads it, and in SYS:Prefs the ROM's editors it has no file for. */
int wb_dir_read(wb_dir *d, const char *path, int show_all)
{
    struct Process *me = (struct Process *)FindTask(NULL);
    APTR old;
    BPTR l;
    struct FileInfoBlock *fib;
    int i, added = 0;
    if (!wb_dir_read_disk(d, path, show_all)) return 0;
    if (!is_sys_prefs(path)) return 1;
    old = me->pr_WindowPtr;
    me->pr_WindowPtr = (APTR)-1;
    l = Lock((CONST_STRPTR)ROM_PREFS, ACCESS_READ);
    me->pr_WindowPtr = old;
    if (!l) return 1;
    if ((fib = AllocDosObject(DOS_FIB, NULL))) {
        if (Examine(l, fib) && fib->fib_DirEntryType > 0)
            while (ExNext(l, fib)) {
                const char *name = (const char *)fib->fib_FileName;
                size_t len = strlen(name);
                char info[WB_NAME_MAX + 16];
                wb_entry *x;
                if (fib->fib_DirEntryType > 0 || (len > 5 && same_name(name + len - 5, ".info"))) continue;
                for (i = 0; i < d->n && !same_name(d->e[i].name, name); i++) ;
                if (i < d->n) continue;            /* SYS:Prefs has one of that name */
                snprintf(info, sizeof info, ROM_PREFS "/%s.info", name);
                if (!show_all && !quiet_exists(info)) continue;   /* as on a disk: only those with icons */
                if (d->n == d->cap) {
                    int nc = d->cap ? d->cap * 2 : 16;
                    wb_entry *ne = realloc(d->e, (size_t)nc * sizeof *ne);
                    if (!ne) break;
                    d->e = ne;
                    d->cap = nc;
                }
                x = &d->e[d->n++];
                memset(x, 0, sizeof *x);
                strlcpy(x->name, name, sizeof x->name);
                x->has_icon = 1;
                x->size = (ULONG)fib->fib_Size;
                x->days = fib->fib_Date.ds_Days;
                x->mins = fib->fib_Date.ds_Minute;
                x->items = -1;
                x->kind = wb_kind(x->name, 0, 0);
                added++;
            }
        FreeDosObject(DOS_FIB, fib);
    }
    UnLock(l);
    if (added) wb_dir_sort(d, d->sort, d->desc);
    return 1;
}

/* An entry listed from ROM: gets ROM:'s icon (ROM:Prefs/NAME.info); the others as wb_dir gives them. */
struct DiskObject *wb_entry_icon(wb_dir *d, int i, struct Screen *scr)
{
    char rp[WB_PATH_MAX], keep[WB_PATH_MAX];
    struct DiskObject *o;
    if (d->e[i].icon || d->e[i].tried || !rom_entry(d, i, rp, sizeof rp))
        return wb_entry_icon_disk(d, i, scr);
    strlcpy(keep, d->path, sizeof keep);
    strlcpy(d->path, ROM_PREFS, sizeof d->path);
    o = wb_entry_icon_disk(d, i, scr);
    strlcpy(d->path, keep, sizeof d->path);
    return o;
}

static int ob_open_object(const char *path)
{
    struct DiskObject *o;
    char rp[WB_PATH_MAX];
    int ok = 0, rom = 0;
    switch (rom_prefs_target(path, rp, sizeof rp)) {
    case 1: path = rp; rom = 1; break;            /* ROM:'s editor: ours, as a built-in, never asked about */
    case 2: break;                                 /* SYS:Prefs's newer file itself */
    default: if (ob_builtin_open(path)) return 1;  /* built into OpenBench (ob_builtin_list.h): ours, never asked about */
    }
    o = GetIconTags((CONST_STRPTR)path, ICONGETA_FailIfUnavailable, TRUE, TAG_DONE);
    if (!o) {
        BPTR l = Lock((CONST_STRPTR)path, ACCESS_READ);
        struct FileInfoBlock *fib = AllocDosObject(DOS_FIB, NULL);
        int dir = 0;
        if (l && fib && Examine(l, fib)) dir = fib->fib_DirEntryType > 0;
        if (fib) FreeDosObject(DOS_FIB, fib);
        if (l) UnLock(l);
        if (dir) return open_drawer(path);
        return is_program(path) && (rom || tata_lets(path, 0)) ? start_tool(path, NULL, 8192) : 0;
    }
    switch (o->do_Type) {
    case WBDISK: case WBDRAWER: case WBGARBAGE:
        ok = open_drawer(path);
        break;
    case WBTOOL:
        ok = (rom || tata_lets(path, 0)) && start_tool(path, NULL, o->do_StackSize);
        break;
    case WBPROJECT: {
        char tool[300];
        if (find_tool(path, (const char *)o->do_DefaultTool, tool, sizeof tool) && tata_lets(tool, 0) && tata_lets(path, 1))
            ok = start_tool(tool, path, o->do_StackSize);
        break;
    }
    }
    FreeDiskObject(o);
    return ok;
}

/* Everything in SYS:WBStartup, started as Workbench starts it. */
static void run_wbstartup(void)
{
    BPTR l = Lock((CONST_STRPTR)"SYS:WBStartup", ACCESS_READ);
    struct FileInfoBlock *fib;
    if (!l) return;
    starting_up = 1;
    if ((fib = AllocDosObject(DOS_FIB, NULL))) {
        if (Examine(l, fib))
            while (ExNext(l, fib)) {
                char path[160];
                int n = (int)strlen((const char *)fib->fib_FileName);
                if (n <= 5 || ci_cmp((const char *)fib->fib_FileName + n - 5, ".info")) continue;
                snprintf(path, sizeof path, "SYS:WBStartup/%.*s", n - 5, (const char *)fib->fib_FileName);
                ob_open_object(path);
            }
        FreeDosObject(DOS_FIB, fib);
    }
    starting_up = 0;
    UnLock(l);
}

/* ---- drawer windows -------------------------------------------------------------------------- */

/* what each drawer window was opened on, to open it again after a reset of the Workbench screen */
static char drawer_paths[MAX_WINDOWS][256];

static int open_drawer(const char *path)
{
    wb_drawer *d;
    if (!root) return 0;                            /* the Workbench screen is being reset */
    if (ndrawers >= MAX_WINDOWS || !(d = wb_drawer_open(&look, path))) {
        DisplayBeep(look.scr);
        return 0;
    }
    snprintf(drawer_paths[ndrawers], sizeof drawer_paths[0], "%s", path);
    drawers[ndrawers++] = d;
    return 1;
}

static void close_index(int i)
{
    wb_drawer_close(drawers[i]);
    drawers[i] = drawers[--ndrawers];
    memcpy(drawer_paths[i], drawer_paths[ndrawers], sizeof drawer_paths[0]);
}

static void titles_everywhere(void)
{
    int i;
    if (root) SetWindowTitles(root, (CONST_STRPTR)~0, (CONST_STRPTR)screen_title);
    for (i = 0; i < ndrawers; i++)
        SetWindowTitles(wb_drawer_window(drawers[i]), (CONST_STRPTR)~0, (CONST_STRPTR)screen_title);
}

/* ---- menus ---------------------------------------------------------------------------------- */

enum { M_SHELL = 1, M_REDRAW, M_UPDATE, M_ABOUT, M_LOADWB, M_QUIT, M_ICON_OPEN, M_ICON_INFO, M_DISK_INSERT, M_DISK_EJECT,
       M_RESIZE };

/* 0.1.22: Resize to Window is the fourth item (newmenus[4]), there only while the Workbench screen is on a Native
 * mode (desktop_up leaves it out otherwise), greyed while the screen already has the size the host asks for. */
#define RESIZE_INDEX 4
#define RESIZE_ITEM 3
static struct NewMenu newmenus[] = {
    { NM_TITLE, (STRPTR)"OpenBench", NULL, 0, 0, NULL },
    { NM_ITEM, (STRPTR)"New Shell", (STRPTR)"N", 0, 0, (APTR)M_SHELL },
    { NM_ITEM, (STRPTR)"Redraw All", NULL, 0, 0, (APTR)M_REDRAW },
    { NM_ITEM, (STRPTR)"Update", NULL, 0, 0, (APTR)M_UPDATE },
    { NM_ITEM, (STRPTR)"Resize to Window", NULL, 0, 0, (APTR)M_RESIZE },
    { NM_ITEM, NM_BARLABEL, NULL, 0, 0, NULL },
    { NM_ITEM, (STRPTR)"About...", (STRPTR)"?", 0, 0, (APTR)M_ABOUT },
    { NM_ITEM, NM_BARLABEL, NULL, 0, 0, NULL },
    { NM_ITEM, (STRPTR)"Quit to Workbench...", NULL, 0, 0, (APTR)M_LOADWB },
    { NM_ITEM, (STRPTR)"Quit OpenBench...", (STRPTR)"Q", 0, 0, (APTR)M_QUIT },
    { NM_TITLE, (STRPTR)"Icons", NULL, 0, 0, NULL },
    { NM_ITEM, (STRPTR)"Open", (STRPTR)"O", 0, 0, (APTR)M_ICON_OPEN },
    { NM_ITEM, (STRPTR)"Information...", (STRPTR)"I", 0, 0, (APTR)M_ICON_INFO },
    /* the floppy drives from the desktop (Dale, 12 Oct 2026: "attach an adf to df0 so it can be booted"):
     * amigachrome-guest's InsertDisk window and ACDisk, through accontrol.device */
    { NM_TITLE, (STRPTR)"Disks", NULL, 0, 0, NULL },
    { NM_ITEM, (STRPTR)"Insert Disk...", (STRPTR)"D", 0, 0, (APTR)M_DISK_INSERT },
    { NM_ITEM, (STRPTR)"Eject DF0", NULL, 0, 0, (APTR)M_DISK_EJECT },
    { NM_END, NULL, NULL, 0, 0, NULL }
};

static void resize_to_window(void);
static int want_resize;                     /* 0.1.22: Resize to Window picked: done once the messages are read */

static int ask(const char *title, const char *text, const char *gadgets)
{
    struct EasyStruct es = { sizeof(struct EasyStruct), 0, (UBYTE *)title, (UBYTE *)text, (UBYTE *)gadgets };
    return EasyRequestArgs(root, &es, NULL, NULL);
}

static void new_shell(void)
{
    BPTR in = Open((CONST_STRPTR)"NIL:", MODE_OLDFILE), out = Open((CONST_STRPTR)"NIL:", MODE_NEWFILE);
    if (SystemTags((CONST_STRPTR)"NewShell", SYS_Input, (ULONG)in, SYS_Output, (ULONG)out, SYS_Asynch, TRUE,
                   NP_StackSize, 16384, TAG_DONE) == -1) {
        Close(in);
        Close(out);
        DisplayBeep(look.scr);
    }
}

/* A command started on its own, with NIL: for its input and output (an asynchronous SystemTags needs real
 * handles); its own failure is told, not OpenBench's. */
static void run_detached(const char *cmd, const char *missing)
{
    BPTR in = Open((CONST_STRPTR)"NIL:", MODE_OLDFILE), out = Open((CONST_STRPTR)"NIL:", MODE_NEWFILE);
    if (!in || !out || SystemTags((CONST_STRPTR)cmd, SYS_Input, (ULONG)in, SYS_Output, (ULONG)out, SYS_Asynch, TRUE,
                                  NP_StackSize, 16384, TAG_DONE) == -1) {
        if (in) Close(in);
        if (out) Close(out);
        ask("OpenBench", missing, "OK");
    }
}

static void run_loadwb(void)
{
    BPTR in = Open((CONST_STRPTR)"NIL:", MODE_OLDFILE), out = Open((CONST_STRPTR)"NIL:", MODE_NEWFILE);
    if (SystemTags((CONST_STRPTR)"C:LoadWB", SYS_Input, (ULONG)in, SYS_Output, (ULONG)out, TAG_DONE) == -1) {
        Close(in);
        Close(out);
    }
}

/* 1 when LoadWB gave a Workbench of its own: OS 3.2.3's runs as a process named "Workbench" (FindTask,
 * measured on the stock ROM, lab t5-display, 17 October 2026). None on the open ROM, where LoadWB starts
 * OpenBench. */
static int loadwb_gave;
static int workbench_there(void)
{
    int i, there = 0;
    for (i = 0; i < 10 && !there; i++) {       /* up to a second for its process to appear */
        Forbid();
        there = FindTask((CONST_STRPTR)"Workbench") != NULL;
        Permit();
        if (!there) Delay(5);
    }
    return there;
}

/* ---- the dock: OpenDock, the ROM's or a newer C:OpenDock -------------------------------------------------
 *
 * Dale, 17 Oct 2026: the dock is in the ROM and "only shows if there's a preferences file". OpenDock runs as a
 * program of its own, started here by name, so the Shell finds the ROM's (ROM:C/OpenDock) or a newer
 * C:OpenDock (the ROM's interceptor, by $VER). It is started only when ENV:OpenDock/Dock is there (the
 * Startup-sequence has put ENVARC: in ENV: by now) and none runs (its port, "OpenDock"): with no settings file
 * nothing starts, and the ROM's copy stays packed. OpenDock 0.6.6 ends at once when one runs already (an old
 * "Run >NIL: C:OpenDock" in User-Startup), and its editor starts it when its settings are saved. It starts
 * things through OpenBench (obmsg.h) and takes OpenLook's close notice, as it did before it was built in. */

/* The dock's process, or NULL. Under Forbid. */
static struct Task *dock_task(void)
{
    struct MsgPort *p = FindPort((CONST_STRPTR)"OpenDock");
    return p ? p->mp_SigTask : NULL;
}

/* 1 when path is there; no requester while we look (ROM: on a ROM without it, say). */
static int quiet_exists(const char *path)
{
    struct Process *me = (struct Process *)FindTask(NULL);
    APTR old = me->pr_WindowPtr;
    BPTR l;
    me->pr_WindowPtr = (APTR)-1;
    l = Lock((CONST_STRPTR)path, ACCESS_READ);
    me->pr_WindowPtr = old;
    if (l) UnLock(l);
    return l != 0;
}

static void start_dock(void)
{
    BPTR in, out;
    struct Task *t;
    if (!quiet_exists("ENV:OpenDock/Dock")) return;
    Forbid();
    t = dock_task();
    Permit();
    if (t) return;
    in = Open((CONST_STRPTR)"NIL:", MODE_OLDFILE);
    out = Open((CONST_STRPTR)"NIL:", MODE_NEWFILE);
    if (in && out && SystemTags((CONST_STRPTR)"OpenDock", SYS_Input, (ULONG)in, SYS_Output, (ULONG)out,
                                SYS_Asynch, TRUE, NP_StackSize, 32768, TAG_DONE) != -1)
        return;                                    /* the handles are the dock's now */
    if (in) Close(in);
    if (out) Close(out);
}

/* OpenBench's quit: the dock goes with the desktop, as the built-in one did (3 s at most). */
static void stop_dock(void)
{
    int i;
    for (i = 0; i < 150; i++) {
        struct Task *t;
        Forbid();
        if ((t = dock_task()) && !i) Signal(t, SIGBREAKF_CTRL_C);
        Permit();
        if (!t) break;
        Delay(1);
    }
}

/* ---- the run --------------------------------------------------------------------------------- */

/* 1 when OpenRTG reports RTG here: openrtg.library is there, and the Workbench screen is one of its
 * true-colour screens (cybergraphics: a CyberGraphX bitmap deeper than 8 bits). */
static int rtg_here(struct Screen *s)
{
    struct Library *ortg = OpenLibrary((CONST_STRPTR)"openrtg.library", 0);
    struct BitMap *bm = s ? s->RastPort.BitMap : NULL;
    if (!ortg) return 0;
    CloseLibrary(ortg);
    return CyberGfxBase && bm && CGX_MAPATTR(bm, CYBRMATTR_ISCYBERGFX) && CGX_MAPATTR(bm, CYBRMATTR_DEPTH) > 8;
}

static void redraw_all(void)
{
    int i;
    EraseRect(root->RPort, 0, 0, root->Width - 1, root->Height - 1);
    draw_icons();
    for (i = 0; i < ndrawers; i++) wb_drawer_redraw(drawers[i]);
}

static void select_only(dicon *keep)
{
    int i;
    for (i = 0; i < nicons; i++)
        if (icons[i].selected != (&icons[i] == keep)) {
            icons[i].selected = &icons[i] == keep;
            redraw_around(&icons[i]);
        }
}

/* The root window's messages: selecting, opening and dragging the desktop's icons, its menus. 0 to quit. */
static dicon *pressed_on, *dragging;
static LONG press_x, press_y, grab_dx, grab_dy;
static LONG drag_ox, drag_oy;               /* 0.1.26: where the dragged disk icon was when the drag began */
static struct Window *drag_img;             /* 0.1.26: its picture, while the pointer is over another window */

/* 0.1.26: the disk whose icon is at (sx, sy) on the screen (ob_drop's env) */
static int volume_at(WORD sx, WORD sy, char *out, int n)
{
    dicon *ic = root ? icon_at(sx - root->LeftEdge, sy - root->TopEdge) : NULL;
    if (!ic) return 0;
    snprintf(out, (size_t)n, "%s", ic->path);
    return 1;
}

static const ob_drop_env drop_env = { &look.scr, &root, drawers, &ndrawers, volume_at };

static void drag_img_close(void)
{
    if (drag_img) {
        CloseWindow(drag_img);
        drag_img = NULL;
    }
}

/* The dragged disk icon's picture at (sx, sy) on the screen, above the windows, as a drawer's icons are
 * dragged (opengadtools wb_drawer.c's drag_start): it is opened the first time the pointer leaves the
 * desktop for another window */
static void drag_img_at(dicon *ic, LONG sx, LONG sy)
{
    if (!drag_img) {
        if (!ic->dobj || !look.scr) return;
        drag_img = OpenWindowTags(NULL, WA_Left, sx, WA_Top, sy, WA_Width, ic->w, WA_Height, ic->h,
                                  WA_Borderless, TRUE, WA_Activate, FALSE, WA_RMBTrap, TRUE, WA_AutoAdjust, TRUE,
                                  WA_SmartRefresh, TRUE, WA_CustomScreen, (ULONG)look.scr, TAG_DONE);
        if (!drag_img) return;
        DrawIconState(drag_img->RPort, ic->dobj, NULL, ic->ix - ic->x, ic->iy - ic->y, IDS_SELECTED,
                      ICONDRAWA_Frameless, TRUE, ICONDRAWA_Borderless, TRUE, ICONDRAWA_EraseBackground, TRUE,
                      ICONDRAWA_DrawInfo, (ULONG)dri, TAG_DONE);
        return;
    }
    if (sx > look.scr->Width - drag_img->Width) sx = look.scr->Width - drag_img->Width;
    if (sy > look.scr->Height - drag_img->Height) sy = look.scr->Height - drag_img->Height;
    if (sx < 0) sx = 0;
    if (sy < 0) sy = 0;
    if (sx != drag_img->LeftEdge || sy != drag_img->TopEdge)
        MoveWindow(drag_img, (WORD)(sx - drag_img->LeftEdge), (WORD)(sy - drag_img->TopEdge));
}
static ULONG last_secs, last_micros;
static dicon *last_click;
static int quit_to_wb;

static int root_event(ULONG cl, UWORD code, UWORD qual, WORD mx, WORD my, ULONG secs, ULONG micros)
{
    switch (cl) {
    case IDCMP_REFRESHWINDOW:
        BeginRefresh(root);
        draw_icons();
        EndRefresh(root, TRUE);
        break;
    case IDCMP_MOUSEBUTTONS:
        if (code == SELECTDOWN) {
            dicon *ic = icon_at(mx, my);
            if (ic && ic == last_click && DoubleClick(last_secs, last_micros, secs, micros)) {
                last_click = NULL;
                pressed_on = NULL;
                open_drawer(ic->path);
                break;
            }
            last_click = ic;
            last_secs = secs;
            last_micros = micros;
            if (ic) {
                if (qual & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT)) {
                    ic->selected = !ic->selected;
                    redraw_around(ic);
                } else if (!ic->selected)
                    select_only(ic);
                pressed_on = ic;
                press_x = mx;
                press_y = my;
                grab_dx = mx - ic->x;
                grab_dy = my - ic->y;
            } else
                select_only(NULL);
        } else if (code == SELECTUP) {
            if (dragging) {
                /* 0.1.26: let go over another window: into that drawer, or to that program's AppWindow;
                 * the disk's icon goes back where it was */
                WORD sx = (WORD)(root->LeftEdge + mx), sy = (WORD)(root->TopEdge + my);
                struct Window *w;
                drag_img_close();
                w = ob_drop_window_at(sx, sy);
                if (w && w != root) {
                    if (dragging->x != drag_ox || dragging->y != drag_oy) move_icon(dragging, drag_ox, drag_oy);
                    ob_drop_volume(dragging->path, sx, sy, qual);
                }
                dragging = NULL;
                draw_icons();
            }
            pressed_on = NULL;
        }
        break;
    case IDCMP_MOUSEMOVE:
        if (pressed_on && (dragging || mx - press_x > 3 || press_x - mx > 3 || my - press_y > 3 || press_y - my > 3)) {
            LONG nx = mx - grab_dx, ny = my - grab_dy, ox = pressed_on->x, oy = pressed_on->y;
            struct Window *w;
            if (!dragging) {
                drag_ox = ox;
                drag_oy = oy;
            }
            dragging = pressed_on;
            /* 0.1.26: over another window, its picture follows the pointer above the windows; the icon
             * stays where it was last on the desktop */
            w = ob_drop_window_under((WORD)(root->LeftEdge + mx), (WORD)(root->TopEdge + my), drag_img);
            if (w && w != root) {
                drag_img_at(dragging, root->LeftEdge + nx, root->TopEdge + ny);
                break;
            }
            drag_img_close();
            if (nx < 0) nx = 0;
            if (ny < 0) ny = 0;
            if (nx > root->Width - pressed_on->w) nx = root->Width - pressed_on->w;
            if (ny > root->Height - pressed_on->h - label_h) ny = root->Height - pressed_on->h - label_h;
            if (nx != ox || ny != oy)
                move_icon(pressed_on, nx, ny);
        }
        break;
    case IDCMP_DISKINSERTED:
    case IDCMP_DISKREMOVED:
        scan_volumes();
        EraseRect(root->RPort, 0, 0, root->Width - 1, root->Height - 1);
        draw_icons();
        break;
    case IDCMP_MENUPICK: {
        UWORD num = code;
        int picked = 0;
        /* 0.1.22: at most as many items as the menus have; an item's NextSelect is only set when it is picked
         * from the menus, and one left at 0 (menu 0, item 0) would loop for ever */
        while (num != MENUNULL && picked++ < 32) {
            struct MenuItem *it = ItemAddress(root->MenuStrip, num);
            ULONG id = it ? (ULONG)GTMENUITEM_USERDATA(it) : 0;
            switch (id) {
            case M_SHELL: new_shell(); break;
            case M_DISK_INSERT:
                run_detached("C:InsertDisk", "Insert Disk couldn't start.\nIt needs C:InsertDisk from AmigaChrome.");
                break;
            case M_DISK_EJECT:
                run_detached("C:ACDisk DRIVE DF0 EJECT QUIET", "DF0 couldn't be ejected.\nIt needs C:ACDisk from AmigaChrome.");
                break;
            case M_ICON_OPEN:
            case M_ICON_INFO: {
                int i;
                for (i = 0; i < nicons; i++)
                    if (icons[i].selected) {
                        if (id == M_ICON_OPEN) open_drawer(icons[i].path);
                        else if (!ob_info_open(look.scr, icons[i].path)) DisplayBeep(look.scr);
                    }
                break;
            }
            case M_REDRAW: redraw_all(); break;
            case M_RESIZE: want_resize = 1; break;   /* not here: it closes the menus being read */
            case M_UPDATE:
                scan_volumes();
                redraw_all();
                break;
            case M_ABOUT:
                ask("About OpenBench", "OpenBench " OPENBENCH_VERSION "\n\nOur own Workbench for AmigaOS 3.2.\n"
                    "MIT, Copyright (c) 2026 AmigaChrome Team.", "OK");
                break;
            case M_LOADWB:
            case M_QUIT:
                if (running) {
                    /* their code is unloaded when they end, by us: as Workbench, we can't go before them.
                     * 0.1.19: they can be asked to quit (OpenMeters from SYS:WBStartup never ends by itself) */
                    char names[400], text[600];
                    launches_reap();
                    running_names(names, sizeof names);
                    if (running) {
                        snprintf(text, sizeof text, "These programs started from OpenBench are still running:\n%s\n"
                                 "OpenBench can quit when they have ended.", names);
                        if (!ask("OpenBench", text, "Ask them to quit|Cancel")) break;
                        if (!ask_running_to_quit(5)) {
                            running_names(names, sizeof names);
                            snprintf(text, sizeof text, "These programs haven't quit:\n%s\n"
                                     "Quit them yourself, then quit OpenBench again.", names);
                            ask("OpenBench", text, "OK");
                            break;
                        }
                        if (id == M_LOADWB) quit_to_wb = 1;
                        return 0;
                    }
                }
                if (id == M_QUIT) {
                    if (ask("OpenBench", "Quit OpenBench?\nThere will be no desktop until it is started again\n"
                            "(or LoadWB is run from a Shell).", "Quit|Cancel"))
                        return 0;
                    break;
                }
                if (ask("OpenBench", "Close OpenBench and start Hyperion's Workbench?", "Workbench|Cancel")) {
                    quit_to_wb = 1;
                    return 0;
                }
                break;
            }
            num = it ? it->NextSelect : MENUNULL;
        }
        break;
    }
    }
    return 1;
}

static int run_shell;

/* ---- the desktop on the Workbench screen, and its reset ---------------------------------------- *
 * 0.1.19: IPrefs resets the Workbench screen (CloseWorkBench, then OpenWorkBench) when Font prefs' Use or a
 * new screen mode changes it. OpenLook's close notice (Common/wbclose.h) tells OpenBench first, as it tells
 * the dock and OpenMeters: OpenBench closes everything it has on the screen and lets go of it, answers, and
 * opens it all again when the screen is back. Before, its root window never answered, and Intuition's
 * requester asked for "<No title>" to be closed. */
static struct MsgPort *idcmp;
static APTR vi;
static struct Menu *menus;
static int resize_shown, resize_greyed;     /* 0.1.22: Resize to Window in the menu, and greyed */

/* The picture, decoded in a process of its own (0.1.3): IPrefs' root picture; ENV:OpenBench/NoPicture, none. */
static void start_picture(void)
{
    if (picture_busy || Lock_exists("ENV:OpenBench/NoPicture") || !root_picture(picture_path, sizeof picture_path))
        return;
    if (!DataTypesBase && !(DataTypesBase = OpenLibrary((CONST_STRPTR)"datatypes.library", 44)))
        return;
    picture_scr = look.scr;
    picture_busy = 1;
    if (!CreateNewProcTags(NP_Entry, (ULONG)picture_entry, NP_StackSize, 16384, NP_Name, (ULONG)"OpenBench backdrop",
                           NP_Priority, -1, TAG_DONE))
        picture_busy = 0;
}

/* The pictures behind the desktop given up: the decoded one, the datatype's (whose file it keeps open), the
 * gradient. With the root window's layer locked, as its backfill reads them from whichever task draws. */
static void drop_picture(void)
{
    ULONG *old;
    Object *o;
    while (picture_busy) Delay(5);                  /* the picture's process is in our code */
    if (root) LockLayer(0, root->WLayer);
    old = backdrop;
    backdrop = NULL;
    o = pic_obj;
    pic_bm = NULL;
    pic_obj = NULL;
    if (root) UnlockLayer(root->WLayer);
    if (picture_ready) { FreeVec(picture_ready); picture_ready = NULL; }
    if (old) FreeVec(old);
    if (o) DisposeDTObject(o);
}

/* 0.1.19: ENV:Sys/WBPattern.prefs changed: the new root picture shows, or the gradient alone, and the old
 * picture's file is let go (OpenUpTool UNINSTALL couldn't delete a backdrop picture OpenBench held open). */
static void new_picture(void)
{
    ULONG *grad;
    drop_picture();
    grad = gradient_backdrop(bd_w, bd_h);
    LockLayer(0, root->WLayer);
    backdrop = grad;
    UnlockLayer(root->WLayer);
    EraseRect(root->RPort, 0, 0, root->Width - 1, root->Height - 1);
    draw_icons();
    start_picture();
}

/* Everything OpenBench has on the Workbench screen given up, the screen too: before a reset, and at the end.
 * The drawers' paths are kept, to open them again. */
static void desktop_down(void)
{
    drag_img_close();                           /* 0.1.26: a disk icon's drag ends with the desktop */
    dragging = pressed_on = NULL;
    ob_info_quit_all();                         /* the Information windows (they close without saving) */
    while (ndrawers) close_index(ndrawers - 1); /* close_index keeps drawer_paths[] beyond ndrawers */
    drop_picture();
    if (root) {
        ClearMenuStrip(root);
        {
            struct IntuiMessage *m;
            Forbid();
            while ((m = (struct IntuiMessage *)GetMsg(root->UserPort))) ReplyMsg(&m->ExecMessage);
            root->UserPort = NULL;
            ModifyIDCMP(root, 0);
            Permit();
        }
        CloseWindow(root);
        root = NULL;
    }
    if (menus) { FreeMenus(menus); menus = NULL; }
    if (vi) { FreeVisualInfo(vi); vi = NULL; }
    if (dri) { FreeScreenDrawInfo(look.scr, dri); dri = NULL; }
    free_icons();
    if (look.font) { CloseFont(look.font); look.font = NULL; }
    if (look.ctx) { ogt_ctx_free(&ctx); look.ctx = NULL; }
    ogt_theme_free(&theme);
    if (look.scr) { UnlockPubScreen(NULL, look.scr); look.scr = NULL; }
}

static int native_monitor(ULONG *mode_id);
static int native_differs(void);
static void mark_ours(void);

/* The desktop on the Workbench screen: the theme, the root window with its menus and icons, the picture.
 * 1 when it is up; 0 with whatever was opened still to be given up with desktop_down. */
static int desktop_up(void)
{
    int mode;
    struct NewMenu nm[sizeof newmenus / sizeof newmenus[0]];
    int i, k;
    if (!look.scr && !(look.scr = LockPubScreen((CONST_STRPTR)"Workbench"))) return 0;
    load_theme(&mode);
    if (!ogt_ctx_init(&ctx, look.scr, &theme, mode) && !ogt_ctx_init(&ctx, look.scr, &theme, OGT_LIGHT)) return 0;
    look.ctx = &ctx;
    look.font = ogt_open_font(&theme, look.scr, &ta);
    look.ta = &ta;
    make_title();

    /* the backdrop: the theme's gradient now, the picture when it is decoded */
    bd_w = look.scr->Width;
    bd_h = look.scr->Height;
    backdrop = gradient_backdrop(bd_w, bd_h);
    backfill_hook.h_Entry = (ULONG (*)())bf_entry;
    /* 0.1.22: a Workbench window (WFLG_WBENCHWINDOW, first: the other tags add to it), as all of OpenBench's
     * are (mark_ours), so CloseWorkBench() asks OpenBench to close it instead of refusing at once */
    root = OpenWindowTags(NULL, WA_Flags, WFLG_WBENCHWINDOW, WA_Left, 0, WA_Top, look.scr->BarHeight + 1, WA_Width, look.scr->Width,
                          WA_Height, look.scr->Height - look.scr->BarHeight - 1, WA_PubScreen, (ULONG)look.scr,
                          WA_Backdrop, TRUE, WA_Borderless, TRUE, WA_SimpleRefresh, TRUE, WA_BackFill, (ULONG)&backfill_hook,
                          WA_ReportMouse, TRUE, WA_NewLookMenus, TRUE, WA_Activate, TRUE,
                          WA_ScreenTitle, (ULONG)screen_title, WA_IDCMP, 0, TAG_DONE);
    if (!root) return 0;
    root->UserPort = idcmp;
    ModifyIDCMP(root, IDCMP_MOUSEBUTTONS | IDCMP_MOUSEMOVE | IDCMP_REFRESHWINDOW | IDCMP_MENUPICK |
                      IDCMP_DISKINSERTED | IDCMP_DISKREMOVED);
    if (look.font) SetFont(root->RPort, look.font);
    SetDrMd(root->RPort, JAM1);
    label_h = root->RPort->TxHeight;
    dri = GetScreenDrawInfo(look.scr);
    /* the menus; Resize to Window only on a Native mode, greyed while the size is already the one asked for */
    resize_shown = native_monitor(NULL) > 0;
    for (i = k = 0; i < (int)(sizeof newmenus / sizeof newmenus[0]); i++) {
        if (i == RESIZE_INDEX && !resize_shown) continue;
        nm[k] = newmenus[i];
        if (i == RESIZE_INDEX && !native_differs()) nm[k].nm_Flags |= NM_ITEMDISABLED;
        k++;
    }
    resize_greyed = resize_shown && !native_differs();
    if ((vi = GetVisualInfo(look.scr, TAG_DONE)) && (menus = CreateMenus(nm, TAG_DONE)) &&
        LayoutMenus(menus, vi, GTMN_NewLookMenus, TRUE, TAG_DONE))
        SetMenuStrip(root, menus);
    scan_volumes();
    draw_icons();
    titles_everywhere();
    start_picture();
    mark_ours();
    return 1;
}

/* ---- Intuition's Workbench, and Resize to Window (0.1.22, the native screen mode's step 4) -------------- *
 * Measured on OS 3.2.3 (lab backlog, 12 October 2026, tools/alohaprobe): CloseWorkBench() refuses at once,
 * telling the Workbench nothing, while the screen has a window that isn't the Workbench's (one without
 * WFLG_WBENCHWINDOW). Otherwise it sends the registered Workbench port WBENCHCLOSE, waits for the answer and
 * closes the screen; when that fails (a program still holds a lock on the screen), it sends WBENCHOPEN. So
 * OpenBench registers its port (AlohaWorkbench), marks its own windows as the Workbench's (the root window, the
 * drawers, the dock, the Information windows: mark_ours), closes them on WBENCHCLOSE and opens them again on
 * WBENCHOPEN. A reset by another program (IPrefs, for ScreenMode prefs' Use) then needs no OpenLook; with
 * OpenLook its close notice comes first, as in 0.1.19, and the two agree. CloseWorkBench() and OpenWorkBench()
 * are called from a process of OpenBench's own (reset_entry): Intuition waits for OpenBench's answer to
 * WBENCHCLOSE, which OpenBench's own process could not give while inside the call. */
/* 0: the desktop is up; 1: down, the screen being reset; 2: to come up again (tried each time round) */
static int wb_down;
static struct MsgPort *aloha_port;          /* Intuition's Workbench messages */
static int aloha_on;
static struct Library *OpenRTGBase;         /* openrtg.library 0.17 or later, for the Native size; else NULL */
static ULONG last_ask;                      /* ASK when last seen: a new value is a page asking to follow now */
static int ask_seen;

/* The drawers open when the screen closed: where they were, to put them back (moved in at a smaller size). */
static struct { WORD x, y, w, h; } drawer_box[MAX_WINDOWS];
static int nreopen;

/* The monitor (1-4) whose Native mode the Workbench screen is on, else 0; its ModeID in *mode_id. */
static int native_monitor(ULONG *mode_id)
{
    ULONG id;
    if (!look.scr || !OpenRTGBase) return 0;
    id = GetVPModeID(&look.scr->ViewPort);
    if (mode_id) *mode_id = id;
    return OB_IS_NATIVE(id) ? (int)((id >> 24) & 0x0F) : 0;
}

/* 1 when the Workbench screen is on a Native mode and the host asks for another size. */
static int native_differs(void)
{
    struct ob_native_size ns;
    int n = native_monitor(NULL);
    if (!n || !ORTG_NATIVESIZE(OpenRTGBase, (ULONG)n, &ns) || !ns.width || !ns.height) return 0;
    return ns.width != look.scr->Width || ns.height != look.scr->Height;
}

/* OpenLook's close notice's list (Common/wbclose.h): the programs that close their windows when told. */
static struct WBNRoot *wbn_root(void)
{
    struct WBNRoot *r = (struct WBNRoot *)FindSemaphore((STRPTR)WBN_SEM_NAME);   /* under Forbid */
    return r && r->magic == WBN_MAGIC ? r : NULL;
}

/* 1 when OpenLook runs, so its patch tells every registered program to close before the screen does. */
static int openlook_running(void)
{
    struct MsgPort *p;
    int on;
    Forbid();
    p = FindPort((CONST_STRPTR)OL_PORT_NAME);
    on = p && p->mp_Node.ln_Pri == OL_PORT_PRI && ((const struct ol_port_view *)p)->magic == OL_MAGIC;
    Permit();
    return on;
}

/* 1 when task t is OpenBench's: its own process, the dock, an Information window's. Under Forbid. */
static int task_ours(struct Task *t)
{
    return t && (t == main_task || t == dock_task() ||
                 (t->tc_Node.ln_Name && !strcmp(t->tc_Node.ln_Name, "OpenBench Information")));
}

/* 1 when task t has registered for OpenLook's close notice. Under Forbid. */
static int task_told(struct WBNRoot *r, struct Task *t)
{
    struct MinNode *n;
    if (!r || !t) return 0;
    for (n = r->clients.mlh_Head; n->mln_Succ; n = n->mln_Succ)
        if (((struct WBNClient *)n)->task == t) return 1;
    return 0;
}

/* OpenBench's windows marked as the Workbench's (WFLG_WBENCHWINDOW), as Workbench's own are, so Intuition asks
 * OpenBench to close them instead of refusing. The drawers (opengadtools' wb_drawer), the dock (OpenDock's code)
 * and the Information windows (processes of their own) open theirs without it: the flag is set after, here,
 * each time the desktop comes up and every two seconds. */
static void mark_ours(void)
{
    struct Window *w;
    if (!look.scr) return;
    Forbid();
    for (w = look.scr->FirstWindow; w; w = w->NextWindow)
        if (w->UserPort && task_ours(w->UserPort->mp_SigTask)) w->Flags |= WFLG_WBENCHWINDOW;
    Permit();
}

/* The windows on the Workbench screen that would stop it closing: not OpenBench's, and not of a program
 * OpenLook's notice will close (when OpenLook runs). Their titles (else their program's name), one to a line,
 * at most eight, into out; how many there are. scr NULL: the Workbench screen, found by its flag. */
static int blocking_windows(struct Screen *scr, char *out, int n)
{
    struct Window *w;
    struct WBNRoot *r;
    int count = 0, k = 0, told = openlook_running();
    out[0] = 0;
    Forbid();
    if (!scr)
        for (scr = IntuitionBase->FirstScreen; scr; scr = scr->NextScreen)
            if ((scr->Flags & SCREENTYPE) == WBENCHSCREEN) break;
    r = told ? wbn_root() : NULL;
    for (w = scr ? scr->FirstWindow : NULL; w; w = w->NextWindow) {
        struct Task *t = w->UserPort ? w->UserPort->mp_SigTask : NULL;
        const char *name = (const char *)w->Title;
        char line[60];
        int j;
        if (task_ours(t) || (told && task_told(r, t))) continue;
        if (!name || !name[0]) name = t && t->tc_Node.ln_Name ? t->tc_Node.ln_Name : "a window with no title";
        snprintf(line, sizeof line, "  %s\n", name);
        for (j = 0; line[j]; j++) if (line[j] == '%') line[j] = '_';   /* the requester's text is a format */
        if (strstr(out, line)) continue;                             /* named once */
        if (++count <= 8 && k + (int)strlen(line) < n) { strcpy(out + k, line); k += (int)strlen(line); }
    }
    Permit();
    if (count > 8 && k + 20 < n) snprintf(out + k, (size_t)(n - k), "  and %d more\n", count - 8);
    return count;
}

/* The dock told to close (WBC_CLOSE) or open (WBC_OPEN) through its own close notice record, as OpenLook tells
 * it; closing waits up to 3 s for it to say it has. OpenLook may tell it too: the dock takes either. */
static void tell_dock(LONG what)
{
    struct WBNRoot *r;
    struct MinNode *n;
    struct WBNClient *c = NULL;
    struct Task *dt;
    int i;
    Forbid();
    if ((dt = dock_task()) && (r = wbn_root()))
        for (n = r->clients.mlh_Head; n->mln_Succ; n = n->mln_Succ)
            if (((struct WBNClient *)n)->task == dt) { c = (struct WBNClient *)n; break; }
    if (c) {
        if (what == WBC_CLOSE) c->ack = 0;
        c->want = what;
        Signal(c->task, c->sigmask);
    }
    Permit();
    for (i = 0; c && what == WBC_CLOSE && i < 150; i++) {
        int done;
        Forbid();
        done = !dock_task() || c->ack;             /* the record goes with the dock */
        Permit();
        if (done) break;
        Delay(1);
    }
}

/* Everything OpenBench has on the screen closed, the drawers' places kept: before the screen closes. */
static void go_down(void)
{
    int i;
    nreopen = ndrawers;
    for (i = 0; i < ndrawers; i++) {
        struct Window *w = wb_drawer_window(drawers[i]);
        drawer_box[i].x = w->LeftEdge; drawer_box[i].y = w->TopEdge;
        drawer_box[i].w = w->Width; drawer_box[i].h = w->Height;
    }
    desktop_down();
    tell_dock(WBC_CLOSE);
}

/* The desktop on the screen again, the drawers where they were, moved in (and made smaller) where they would
 * now fall off the screen. 1 when it is up. */
static int go_up(void)
{
    int i;
    if (!desktop_up()) return 0;
    for (i = 0; i < nreopen; i++) {
        char path[sizeof drawer_paths[0]];
        WORD x = drawer_box[i].x, y = drawer_box[i].y, w = drawer_box[i].w, h = drawer_box[i].h;
        WORD sw = look.scr->Width, sh = look.scr->Height, top = look.scr->BarHeight + 1;
        memcpy(path, drawer_paths[i], sizeof path);   /* open_drawer writes drawer_paths[ndrawers] */
        if (!open_drawer(path)) continue;
        if (w > sw) w = sw;
        if (h > sh - top) h = sh - top;
        if (x + w > sw) x = sw - w;
        if (y + h > sh) y = sh - h;
        if (x < 0) x = 0;
        if (y < top) y = top;
        ChangeWindowBox(wb_drawer_window(drawers[ndrawers - 1]), x, y, w, h);
    }
    nreopen = 0;
    tell_dock(WBC_OPEN);
    mark_ours();
    return 1;
}

/* The reset's process: CloseWorkBench(), then OpenWorkBench() when the screen closed. */
static volatile int reset_busy;
static volatile LONG reset_closed, reset_opened;
static BYTE reset_sig = -1;
static char reset_blockers[480];
static int reset_refused;                   /* the last reset was refused: say so once the desktop is back */

static void reset_entry(void)
{
    LONG closed = CloseWorkBench();
    reset_closed = closed;
    reset_opened = 0;
    if (closed) reset_opened = OpenWorkBench() != 0;
    else blocking_windows(NULL, reset_blockers, sizeof reset_blockers);
    Forbid();
    reset_busy = 0;
    Signal(main_task, 1UL << reset_sig);
    /* ends in dos.library under Forbid */
}

/* Resize to Window: the Workbench screen closed and opened again, so it opens at the size the host's window
 * asks for. Another program's window would stop the close, so they are named first, with Try Again; nothing
 * of OpenBench's is closed while one is open. */
static int start_reset(void)
{
    char names[480], text[600];
    if (reset_busy || reset_sig < 0 || !root) return 0;
    for (;;) {
        if (!blocking_windows(look.scr, names, sizeof names)) break;
        snprintf(text, sizeof text, "The Workbench screen can't be made the window's size\n"
                 "while these windows are open on it:\n%s\nClose them, then Try Again.", names);
        if (!ask("OpenBench", text, "Try Again|Cancel")) return 0;
        if (!root) return 0;                    /* the screen was reset meanwhile */
    }
    go_down();
    wb_down = 1;                                /* OpenLook's notice and Intuition's WBENCHCLOSE find it down */
    reset_busy = 1;
    reset_blockers[0] = 0;
    if (!CreateNewProcTags(NP_Entry, (ULONG)reset_entry, NP_StackSize, 8192, NP_Name, (ULONG)"OpenBench reset",
                           TAG_DONE)) {
        reset_busy = 0;
        wb_down = 2;                            /* up again next time round */
        return 0;
    }
    return 1;
}

static void resize_to_window(void)
{
    if (!native_differs()) {
        if (look.scr) DisplayBeep(look.scr);
        return;
    }
    start_reset();
}

/* The 2-second timer (0.1.22): a page asked the Amiga to follow now (ASK moved): Resize to Window. And the
 * menu item greyed or not as the sizes match or not. */
static void native_tick(void)
{
    struct ob_native_size ns;
    int n = native_monitor(NULL), differs;
    if (!n) n = 1;
    if (!OpenRTGBase || !ORTG_NATIVESIZE(OpenRTGBase, (ULONG)n, &ns)) return;
    if (!ask_seen) { last_ask = ns.ask; ask_seen = 1; }
    if (!root) return;
    differs = native_differs();
    if (resize_shown && differs == resize_greyed) {
        resize_greyed = !differs;
        if (differs) OnMenu(root, FULLMENUNUM(0, RESIZE_ITEM, NOSUB));
        else OffMenu(root, FULLMENUNUM(0, RESIZE_ITEM, NOSUB));
    }
    if (ns.ask != last_ask) {
        last_ask = ns.ask;
        if (differs && !reset_busy) start_reset();
    }
}

static void aloha_start(void)
{
    struct Task *wb;
    struct Library *o;
    Forbid();
    wb = FindTask((CONST_STRPTR)"Workbench");     /* Hyperion's Workbench has the port: it stays its */
    Permit();
    if (!wb && (aloha_port = CreateMsgPort())) {
        AlohaWorkbench((LONG)aloha_port);
        aloha_on = 1;
    }
    /* openrtg.library 0.17 or later has ORTG_NativeSize */
    if ((o = OpenLibrary((CONST_STRPTR)"openrtg.library", 0))) {
        if (o->lib_Version > 0 || o->lib_Revision >= 17) OpenRTGBase = o;
        else CloseLibrary(o);
    }
    reset_sig = AllocSignal(-1);
}

static void aloha_stop(void)
{
    struct Message *m;
    /* a reset still running has our code, and may be waiting for our answer to WBENCHCLOSE */
    while (reset_busy) {
        while (aloha_port && (m = GetMsg(aloha_port))) ReplyMsg(m);
        Delay(2);
    }
    if (aloha_on) {
        AlohaWorkbench(0);
        aloha_on = 0;
    }
    while (aloha_port && (m = GetMsg(aloha_port))) ReplyMsg(m);
    if (aloha_port) { DeleteMsgPort(aloha_port); aloha_port = NULL; }
    if (OpenRTGBase) { CloseLibrary(OpenRTGBase); OpenRTGBase = NULL; }
    if (reset_sig >= 0) { FreeSignal(reset_sig); reset_sig = -1; }
}

/* ENV:Sys's two files OpenBench follows (0.1.19): Workbench's settings and IPrefs' patterns. */
static struct NotifyRequest wbp_nr, pat_nr;
static BYTE wbp_sig = -1, pat_sig = -1;
static int wbp_notify, pat_notify;

static int notify_on(struct NotifyRequest *nr, const char *name, BYTE sig)
{
    if (sig < 0) return 0;
    memset(nr, 0, sizeof *nr);
    nr->nr_Name = (STRPTR)name;
    nr->nr_Flags = NRF_SEND_SIGNAL;
    nr->nr_stuff.nr_Signal.nr_Task = main_task;
    nr->nr_stuff.nr_Signal.nr_SignalNum = (UBYTE)sig;
    return StartNotify(nr) != 0;
}

static int run(void)
{
    struct MsgPort *pub = NULL, *tport = NULL;
    struct timerequest *tr = NULL;
    int i, ok = 0, timer = 0;

    main_task = FindTask(NULL);
    if (!(pub = CreateMsgPort())) return RETURN_FAIL;
    pub->mp_Node.ln_Name = (char *)PORT_NAME;
    pub->mp_Node.ln_Pri = 0;
    Forbid();
    if (FindPort((CONST_STRPTR)PORT_NAME)) {
        Permit();
        DeleteMsgPort(pub);
        if (run_shell) PutStr((CONST_STRPTR)"OpenBench is already running\n");
        return RETURN_WARN;
    }
    AddPort(pub);
    Permit();

    IconBase = OpenLibrary((CONST_STRPTR)"icon.library", 44);
    WorkbenchBase = OpenLibrary((CONST_STRPTR)"workbench.library", 44);
    CyberGfxBase = OpenLibrary((CONST_STRPTR)"cybergraphics.library", 40);
    /* OpenDrawer's pictures (the list's small icons, sizes, thumbnails) are written with cybergraphics,
     * which wb_pic opens for itself: 0.1.20, which never opened it, drew them all as icons (Instance-32) */
    wb_pic_open();
    if (!IconBase) goto out;
    if (!(launch_port = CreateMsgPort())) goto out;
    if ((picture_sig = AllocSignal(-1)) < 0) goto out;
    if (!(look.scr = LockPubScreen((CONST_STRPTR)"Workbench"))) goto out;
    if (!rtg_here(look.scr)) {
        if (run_shell) PutStr((CONST_STRPTR)"OpenBench: OpenRTG reports no RTG here: Workbench as usual\n");
        UnlockPubScreen(NULL, look.scr);
        look.scr = NULL;
        run_loadwb();
        if (workbench_there()) {
            loadwb_gave = 1;
            goto out;                              /* ok stays 0, and LoadWB is not run again */
        }
        /* No Workbench came: on the open ROM, workbench.library's StartWorkbench starts OpenBench and refuses
         * while an OpenBench process runs, so on an 8-bit RTG or an AGA screen no desktop opened at all
         * (W33, 16 October 2026; lab t5-display, 17 October). OpenBench is then the desktop, on its palette
         * fallback. */
        if (run_shell) PutStr((CONST_STRPTR)"OpenBench: LoadWB started no Workbench: OpenBench on this screen\n");
        if (!(look.scr = LockPubScreen((CONST_STRPTR)"Workbench"))) goto out;
    }
    if (!(idcmp = CreateMsgPort())) goto out;
    look.port = idcmp;
    look.open_object = ob_open_object;
    look.info_object = ob_info_object;
    look.screen_title = screen_title;
    look.drop = ob_drop;                           /* 0.1.26: icons dragged out of a drawer */
    wb_prefs_load();
    aloha_start();                                 /* 0.1.22: Intuition's Workbench port, openrtg.library 0.17 */
    if (!desktop_up()) goto out;
    ok = 1;

    wbp_sig = AllocSignal(-1);
    pat_sig = AllocSignal(-1);
    wbp_notify = notify_on(&wbp_nr, "ENV:Sys/Workbench.prefs", wbp_sig);
    pat_notify = notify_on(&pat_nr, "ENV:Sys/WBPattern.prefs", pat_sig);
    wbc_start();                                   /* OpenLook's close notice */
    ob_drop_start(&drop_env);                      /* 0.1.26: before the dock and WBStartup add AppWindows */
    start_dock();
    run_wbstartup();

    /* every two seconds: the title (free memory) */
    if ((tport = CreateMsgPort()) && (tr = (struct timerequest *)CreateIORequest(tport, sizeof *tr)) &&
        !OpenDevice((CONST_STRPTR)TIMERNAME, UNIT_VBLANK, (struct IORequest *)tr, 0)) {
        timer = 1;
        tr->tr_node.io_Command = TR_ADDREQUEST;
        tr->tr_time.tv_secs = 2;
        tr->tr_time.tv_micro = 0;
        SendIO((struct IORequest *)tr);
    }

    wb_thumb_start();                           /* the drawers' thumbnails (0.2); none: pictures show their icons */
    for (;;) {
        ULONG sigs = Wait((1UL << idcmp->mp_SigBit) | (1UL << launch_port->mp_SigBit) | (1UL << picture_sig) | wb_thumb_sigmask() |
                          (1UL << pub->mp_SigBit) |
                          (wbp_sig >= 0 ? 1UL << wbp_sig : 0) | (pat_sig >= 0 ? 1UL << pat_sig : 0) | wbc_sigmask() |
                          (aloha_port ? 1UL << aloha_port->mp_SigBit : 0) | (reset_sig >= 0 ? 1UL << reset_sig : 0) |
                          ob_drop_sigmask() | SIGBREAKF_CTRL_C | (timer ? 1UL << tport->mp_SigBit : 0));
        struct IntuiMessage *im;
        struct Message *lm;
        int quit = 0;
        if (sigs & SIGBREAKF_CTRL_C) break;
        /* the Workbench screen is being reset: nothing of ours on it, and no lock, until it is back */
        switch (wbc_take()) {
        case WBC_CLOSE:
            if (!wb_down) {
                go_down();
                wb_down = 1;
            }
            wbc_closed();
            break;
        case WBC_OPEN:
            if (wb_down) wb_down = 2;               /* a refused reset says "open" too: only when we are down */
            break;
        }
        /* 0.1.22: Intuition's own word, as the Workbench (AlohaWorkbench): WBENCHCLOSE before the screen closes
         * (everything closed, then answered), WBENCHOPEN when it is back or the close was refused (answered,
         * then opened again) */
        if (aloha_port) {
            struct IntuiMessage *am;
            while ((am = (struct IntuiMessage *)GetMsg(aloha_port))) {
                ULONG cl = am->Class;
                UWORD code = am->Code;
                if (cl == IDCMP_WBENCHMESSAGE && code == WBENCHCLOSE && !wb_down) {
                    go_down();
                    wb_down = 1;
                }
                ReplyMsg(&am->ExecMessage);
                if (cl == IDCMP_WBENCHMESSAGE && code == WBENCHOPEN && wb_down) wb_down = 2;
            }
        }
        /* 0.1.22: Resize to Window's process is back: a refusal at once sends no WBENCHOPEN, so up again here */
        if (reset_sig >= 0 && (sigs & (1UL << reset_sig)) && !reset_busy) {
            if (wb_down) wb_down = 2;
            if (!reset_closed) reset_refused = 1;
        }
        if (wb_down == 2) {
            if (go_up())
                wb_down = 0;
            else {
                desktop_down();                     /* the screen isn't there yet: tried again each tick */
                wb_down = 2;
            }
        }
        if (reset_refused && root && !reset_busy) {
            char text[640];
            reset_refused = 0;
            if (reset_blockers[0])
                snprintf(text, sizeof text, "The Workbench screen couldn't be made the window's size\n"
                         "while these windows are open on it:\n%s\nClose them, then Try Again.", reset_blockers);
            else
                snprintf(text, sizeof text, "The Workbench screen couldn't be made the window's size:\n"
                         "a program is still using the screen.\nQuit the programs you started on it, then Try Again.");
            if (ask("OpenBench", text, "Try Again|Cancel")) resize_to_window();
        }
        launches_reap();
        if (sigs & ob_drop_sigmask()) ob_drop_handle();   /* 0.1.26: copies done, AppMessages answered */
        while ((lm = GetMsg(pub))) {
            /* something to open, from the dock or another of the Team's programs (obmsg.h) */
            struct ob_msg *om = (struct ob_msg *)lm;
            if (lm->mn_Length >= sizeof *om && om->magic == OB_MSG_MAGIC) {
                om->path[sizeof om->path - 1] = 0;
                om->ok = ob_open_object(om->path);
            }
            ReplyMsg(lm);
        }
        if (wbp_sig >= 0 && (sigs & (1UL << wbp_sig))) {
            wb_prefs_load();                        /* a new default stack, devices hidden or shown */
            if (root) {
                scan_volumes();
                redraw_all();
            }
        }
        if (pat_sig >= 0 && (sigs & (1UL << pat_sig)) && root) new_picture();
        if ((sigs & wb_thumb_sigmask()) && wb_thumb_collect())
            for (i = 0; i < ndrawers; i++) wb_drawer_thumbs(drawers[i]);
        if (sigs & (1UL << picture_sig)) {
            ULONG *old = NULL;
            if (root) LockLayer(0, root->WLayer);
            Forbid();
            if (picture_ready) { old = backdrop; backdrop = picture_ready; picture_ready = NULL; }
            Permit();
            if (root) UnlockLayer(root->WLayer);
            if (old) FreeVec(old);
            if (root && (backdrop || pic_bm)) {
                EraseRect(root->RPort, 0, 0, root->Width - 1, root->Height - 1);
                draw_icons();
            }
        }
        if (timer && CheckIO((struct IORequest *)tr)) {
            WaitIO((struct IORequest *)tr);
            if (root && make_title()) titles_everywhere();
            if (root) mark_ours();                  /* 0.1.22: the dock's and Information windows opened since */
            if (!wb_down) native_tick();            /* 0.1.22: a page asked the Amiga to follow now; the menu item */
            tr->tr_time.tv_secs = 2;
            tr->tr_time.tv_micro = 0;
            SendIO((struct IORequest *)tr);
        }
        while ((im = GT_GetIMsg(idcmp))) {
            struct Window *win = im->IDCMPWindow;
            ULONG cl = im->Class, secs = im->Seconds, micros = im->Micros;
            UWORD code = im->Code, qual = im->Qualifier;
            APTR ia = im->IAddress;
            WORD mx = im->MouseX, my = im->MouseY;
            GT_ReplyIMsg(im);
            if (root && win == root) {
                if (!root_event(cl, code, qual, mx, my, secs, micros)) quit = 1;
                continue;
            }
            for (i = 0; i < ndrawers; i++)
                if (wb_drawer_window(drawers[i]) == win) {
                    if (!wb_drawer_event(drawers[i], cl, code, qual, ia, mx, my, secs, micros)) close_index(i);
                    else if (cl == IDCMP_ACTIVEWINDOW) SetWindowTitles(win, (CONST_STRPTR)~0, (CONST_STRPTR)screen_title);
                    break;
                }
        }
        if (quit) break;
        if (want_resize) {
            want_resize = 0;
            if (root && !reset_busy) resize_to_window();
        }
    }

out:
    aloha_stop();                               /* 0.1.22: Intuition's Workbench port given back first */
    ob_builtin_quit_all();                      /* the built-ins first: their code is ours */
#ifdef OB_HAVE_TATA
    tata_inspect_end();
#endif
    stop_dock();
    ob_drop_stop();                             /* 0.1.26: its copies run our code; AppMessages answered */
    if (timer) {
        AbortIO((struct IORequest *)tr);
        WaitIO((struct IORequest *)tr);
        CloseDevice((struct IORequest *)tr);
    }
    if (tr) DeleteIORequest((struct IORequest *)tr);
    if (tport) DeleteMsgPort(tport);
    if (wbp_notify) EndNotify(&wbp_nr);
    if (pat_notify) EndNotify(&pat_nr);
    wbp_notify = pat_notify = 0;
    if (wbp_sig >= 0) { FreeSignal(wbp_sig); wbp_sig = -1; }
    if (pat_sig >= 0) { FreeSignal(pat_sig); pat_sig = -1; }
    desktop_down();                             /* the Information windows, the drawers, the root, the screen */
    wbc_stop();
    wb_thumb_stop();                            /* its process runs our code, and uses datatypes.library */
    wb_pic_close();
    if (DataTypesBase) { CloseLibrary(DataTypesBase); DataTypesBase = NULL; }
    if (launch_port) {
        /* tools still running keep their code until they end: they are asked to quit, then waited for */
        if (running && !ask_running_to_quit(5) && run_shell)
            PutStr((CONST_STRPTR)"OpenBench: waiting for the programs it started to end\n");
        while (running) {
            WaitPort(launch_port);
            launches_reap();
        }
        DeleteMsgPort(launch_port);
    }
    if (picture_sig >= 0) FreeSignal(picture_sig);
    if (idcmp) DeleteMsgPort(idcmp);
    if (CyberGfxBase) CloseLibrary(CyberGfxBase);
    if (WorkbenchBase) CloseLibrary(WorkbenchBase);
    if (IconBase) CloseLibrary(IconBase);
    CyberGfxBase = WorkbenchBase = IconBase = NULL;
    {
        struct Message *lm;
        Forbid();
        RemPort(pub);
        Permit();
        while ((lm = GetMsg(pub))) ReplyMsg(lm);   /* ok stays 0: not opened */
        DeleteMsgPort(pub);
    }
    if ((!ok && !loadwb_gave) || quit_to_wb) run_loadwb();   /* never left without a desktop */
    return ok ? RETURN_OK : RETURN_FAIL;
}

/* ---- the run in a process of its own, when the stack given is small (as OpenDrawer) ---------- */

static struct Task *volatile run_task;
static struct Task *volatile parent_task;
static volatile ULONG done_mask;
static volatile int run_rc = RETURN_FAIL;

static LONG run_entry(void)
{
    int rc;
    Forbid();
    run_task = FindTask(NULL);
    Permit();
    rc = run();
    Forbid();
    run_rc = rc;
    run_task = NULL;
    Signal((struct Task *)parent_task, done_mask);
    return 0;
}

static int run_in_process(void)
{
    BYTE sig = AllocSignal(-1);
    if (sig < 0) return RETURN_FAIL;
    parent_task = FindTask(NULL);
    done_mask = 1UL << sig;
    if (!CreateNewProcTags(NP_Entry, (ULONG)run_entry, NP_StackSize, STACK_NEED, NP_Name, (ULONG)"OpenBench", TAG_DONE)) {
        FreeSignal(sig);
        return RETURN_FAIL;
    }
    for (;;) {
        ULONG sigs = Wait(done_mask | SIGBREAKF_CTRL_C);
        if (sigs & SIGBREAKF_CTRL_C) {
            Forbid();
            if (run_task) Signal((struct Task *)run_task, SIGBREAKF_CTRL_C);
            Permit();
        }
        if (sigs & done_mask) break;
    }
    FreeSignal(sig);
    return run_rc;
}

static int shift_held(void)
{
    struct IOStdReq io;
    int held = 0;
    memset(&io, 0, sizeof io);
    if (!OpenDevice((CONST_STRPTR)"input.device", 0, (struct IORequest *)&io, 0)) {
        struct Library *InputBase = (struct Library *)io.io_Device;
        held = (PeekQualifier() & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT)) != 0;
        CloseDevice((struct IORequest *)&io);
    }
    return held;
}

int main(int argc, char **argv)
{
    struct Task *me = FindTask(NULL);
    (void)argv;
    (void)version;
    run_shell = argc != 0;
    if (shift_held()) {
        /* safe start: Hyperion's Workbench */
        run_loadwb();
        return RETURN_OK;
    }
    if ((ULONG)me->tc_SPUpper - (ULONG)me->tc_SPLower >= STACK_NEED)
        return run();
    return run_in_process();
}
