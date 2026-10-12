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
 * (the user, 11 October 2026). OpenBench patches nothing.
 *
 * Next (the user's order): workbench.library's entries (OpenWorkbenchObject, AppIcons, AppWindows,
 * AppMenuItems) brought to OpenBench, as Directory Opus 5 takes them; then drag and drop between windows
 * with OpenFiles' engine.
 *
 * MIT, Copyright (c) 2026 Dalsin Limited. AROS's Wanderer and Directory Opus 5 were read for how, and no
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
#include <proto/gadtools.h>
#include <proto/icon.h>
#include <proto/wb.h>
#include <proto/datatypes.h>
#include <clib/alib_protos.h>
#include <proto/expansion.h>
#include <proto/input.h>
#include <inline/macros.h>

#include "obmsg.h"
#include "barlogo.h"
#include "ob_builtin.h"
#include "ob_info.h"
#ifdef OB_HAVE_TATA
#include "tata_inspect.h"
#endif
#include "wb_drawer.h"
#include "wb_dir.h"
#include "ogt_theme.h"
#include "ogt_draw.h"
#include "ogt_font.h"

#define OPENBENCH_VERSION "0.1.17"
static const char version[] __attribute__((used)) = "$VER: OpenBench " OPENBENCH_VERSION " (12.10.2026) MIT, Copyright (c) 2026 Dalsin Limited";

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

/* The boards AmigaChrome presents through AutoConfig (Dalsin $DA15): product 9 a graphics card (ACRTG),
 * product 7 the CPU cores board (ACSV block, CLASS 2, UNITS its cores). As OpenTitle 0.6.10 counts them. */
static ULONG gpus, cores, isas;           /* isas: what the cores run (ISAS, $28; bit 0 m68k always) */

static void count_boards(void)
{
    struct ConfigDev *cd = NULL;
    gpus = cores = 0;
    isas = 1;
    if (!(ExpansionBase = (struct ExpansionBase *)OpenLibrary((STRPTR)"expansion.library", 36)))
        return;
    while ((cd = FindConfigDev(cd, 0xDA15, 9)) && gpus < 16) gpus++;
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
} launch;

static struct MsgPort *launch_port;
static int running;                         /* tools started whose message hasn't come back */

static int open_drawer(const char *path);

/* A lock on the drawer holding path, and the name in it. */
static BPTR parent_and_name(const char *path, char *name, int n)
{
    BPTR l = Lock((CONST_STRPTR)path, ACCESS_READ), p;
    const char *fp = (const char *)FilePart((STRPTR)path);
    if (!l) return 0;
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
    if (stack < 8192) stack = 8192;
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
    PutMsg(&p->pr_MsgPort, &L->wbs.sm_Message);
    running++;
    return 1;
}

static void launch_done(launch *L)
{
    int i;
    UnLoadSeg(L->wbs.sm_Segment);
    for (i = 0; i < L->wbs.sm_NumArgs && i < 2; i++)
        if (L->args[i].wa_Lock) UnLock(L->args[i].wa_Lock);
    FreeVec(L);
    running--;
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
            snprintf(try_, sizeof try_, "%s", project);
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

static int ob_open_object(const char *path)
{
    struct DiskObject *o;
    int ok = 0;
    if (ob_builtin_open(path)) return 1;          /* built into OpenBench (ob_builtin_list.h): ours, never asked about */
    o = GetIconTags((CONST_STRPTR)path, ICONGETA_FailIfUnavailable, TRUE, TAG_DONE);
    if (!o) {
        BPTR l = Lock((CONST_STRPTR)path, ACCESS_READ);
        struct FileInfoBlock *fib = AllocDosObject(DOS_FIB, NULL);
        int dir = 0;
        if (l && fib && Examine(l, fib)) dir = fib->fib_DirEntryType > 0;
        if (fib) FreeDosObject(DOS_FIB, fib);
        if (l) UnLock(l);
        if (dir) return open_drawer(path);
        return is_program(path) && tata_lets(path, 0) ? start_tool(path, NULL, 8192) : 0;
    }
    switch (o->do_Type) {
    case WBDISK: case WBDRAWER: case WBGARBAGE:
        ok = open_drawer(path);
        break;
    case WBTOOL:
        ok = tata_lets(path, 0) && start_tool(path, NULL, o->do_StackSize);
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

static int open_drawer(const char *path)
{
    wb_drawer *d;
    if (ndrawers >= MAX_WINDOWS || !(d = wb_drawer_open(&look, path))) {
        DisplayBeep(look.scr);
        return 0;
    }
    drawers[ndrawers++] = d;
    return 1;
}

static void close_index(int i)
{
    wb_drawer_close(drawers[i]);
    drawers[i] = drawers[--ndrawers];
}

static void titles_everywhere(void)
{
    int i;
    if (root) SetWindowTitles(root, (CONST_STRPTR)~0, (CONST_STRPTR)screen_title);
    for (i = 0; i < ndrawers; i++)
        SetWindowTitles(wb_drawer_window(drawers[i]), (CONST_STRPTR)~0, (CONST_STRPTR)screen_title);
}

/* ---- menus ---------------------------------------------------------------------------------- */

enum { M_SHELL = 1, M_REDRAW, M_UPDATE, M_ABOUT, M_LOADWB, M_QUIT, M_ICON_OPEN, M_ICON_INFO, M_DISK_INSERT, M_DISK_EJECT };

static struct NewMenu newmenus[] = {
    { NM_TITLE, (STRPTR)"OpenBench", NULL, 0, 0, NULL },
    { NM_ITEM, (STRPTR)"New Shell", (STRPTR)"N", 0, 0, (APTR)M_SHELL },
    { NM_ITEM, (STRPTR)"Redraw All", NULL, 0, 0, (APTR)M_REDRAW },
    { NM_ITEM, (STRPTR)"Update", NULL, 0, 0, (APTR)M_UPDATE },
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

/* ---- the dock: OpenDock's code, built in (the user, 11 October 2026: "integrate the dock into
 * OpenWorkbench and not as a separate app") ------------------------------------------------------
 *
 * It runs as a process of OpenBench's own, its window just above the backdrop, and starts things
 * through OpenBench (obmsg.h). An OpenDock started on its own (S:User-Startup) is asked to quit first. */
int opendock_main(void);
static struct Task *volatile dock_task;
static BYTE dock_sig = -1;

static void dock_entry(void)
{
    Forbid();
    dock_task = FindTask(NULL);
    Permit();
    opendock_main();
    Forbid();
    dock_task = NULL;
    Signal(main_task, 1UL << dock_sig);
    /* ends in dos.library under Forbid */
}

static void start_dock(void)
{
    struct MsgPort *p;
    int i;
    for (i = 0; i < 30; i++) {                     /* the separate OpenDock: asked to go, 3 s at most */
        Forbid();
        if ((p = FindPort((CONST_STRPTR)"OpenDock"))) Signal(p->mp_SigTask, SIGBREAKF_CTRL_C);
        Permit();
        if (!p) break;
        Delay(5);
    }
    if (i == 30 || dock_sig < 0) return;
    if (!CreateNewProcTags(NP_Entry, (ULONG)dock_entry, NP_StackSize, 32768, NP_Name, (ULONG)"OpenBench Dock", TAG_DONE))
        return;
    for (i = 0; i < 50 && !dock_task; i++) Delay(1);
}

static void stop_dock(void)
{
    int i;
    Forbid();
    if (dock_task) Signal((struct Task *)dock_task, SIGBREAKF_CTRL_C);
    Permit();
    for (i = 0; i < 250 && dock_task; i++) Delay(2);
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
                dragging = NULL;
                draw_icons();
            }
            pressed_on = NULL;
        }
        break;
    case IDCMP_MOUSEMOVE:
        if (pressed_on && (dragging || mx - press_x > 3 || press_x - mx > 3 || my - press_y > 3 || press_y - my > 3)) {
            LONG nx = mx - grab_dx, ny = my - grab_dy, ox = pressed_on->x, oy = pressed_on->y;
            dragging = pressed_on;
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
        while (num != MENUNULL) {
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
            case M_UPDATE:
                scan_volumes();
                redraw_all();
                break;
            case M_ABOUT:
                ask("About OpenBench", "OpenBench " OPENBENCH_VERSION "\n\nOur own Workbench for AmigaOS 3.2.\n"
                    "MIT, Copyright (c) 2026 Dalsin Limited.", "OK");
                break;
            case M_LOADWB:
            case M_QUIT:
                if (running) {
                    /* their code is unloaded when they end, by us: as Workbench, we can't go before them */
                    ask("OpenBench", "Programs started from OpenBench are still running.\n"
                        "OpenBench can quit when they have ended.", "OK");
                    break;
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

static int run(void)
{
    struct MsgPort *idcmp = NULL, *pub = NULL, *tport = NULL;
    struct timerequest *tr = NULL;
    APTR vi = NULL;
    struct Menu *menus = NULL;
    int mode, i, ok = 0, timer = 0;

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
    if (!IconBase) goto out;
    if (!(launch_port = CreateMsgPort())) goto out;
    if ((picture_sig = AllocSignal(-1)) < 0) goto out;
    dock_sig = AllocSignal(-1);
    if (!(look.scr = LockPubScreen((CONST_STRPTR)"Workbench"))) goto out;
    if (!rtg_here(look.scr)) {
        if (run_shell) PutStr((CONST_STRPTR)"OpenBench: OpenRTG reports no RTG here: Workbench as usual\n");
        goto out;                                  /* ok stays 0: LoadWB */
    }
    load_theme(&mode);
    if (!ogt_ctx_init(&ctx, look.scr, &theme, mode) && !ogt_ctx_init(&ctx, look.scr, &theme, OGT_LIGHT)) goto out;
    look.ctx = &ctx;
    look.font = ogt_open_font(&theme, look.scr, &ta);
    look.ta = &ta;
    if (!(idcmp = CreateMsgPort())) goto out;
    look.port = idcmp;
    look.open_object = ob_open_object;
    look.info_object = ob_info_object;
    count_boards();
    make_title();
    look.screen_title = screen_title;

    /* the backdrop: the theme's gradient now, the picture when it is decoded */
    bd_w = look.scr->Width;
    bd_h = look.scr->Height;
    backdrop = gradient_backdrop(bd_w, bd_h);
    backfill_hook.h_Entry = (ULONG (*)())bf_entry;
    root = OpenWindowTags(NULL, WA_Left, 0, WA_Top, look.scr->BarHeight + 1, WA_Width, look.scr->Width,
                          WA_Height, look.scr->Height - look.scr->BarHeight - 1, WA_PubScreen, (ULONG)look.scr,
                          WA_Backdrop, TRUE, WA_Borderless, TRUE, WA_SimpleRefresh, TRUE, WA_BackFill, (ULONG)&backfill_hook,
                          WA_ReportMouse, TRUE, WA_NewLookMenus, TRUE, WA_Activate, TRUE,
                          WA_ScreenTitle, (ULONG)screen_title, WA_IDCMP, 0, TAG_DONE);
    if (!root) goto out;
    root->UserPort = idcmp;
    ModifyIDCMP(root, IDCMP_MOUSEBUTTONS | IDCMP_MOUSEMOVE | IDCMP_REFRESHWINDOW | IDCMP_MENUPICK |
                      IDCMP_DISKINSERTED | IDCMP_DISKREMOVED);
    if (look.font) SetFont(root->RPort, look.font);
    SetDrMd(root->RPort, JAM1);
    label_h = root->RPort->TxHeight;
    dri = GetScreenDrawInfo(look.scr);
    if ((vi = GetVisualInfo(look.scr, TAG_DONE)) && (menus = CreateMenus(newmenus, TAG_DONE)) &&
        LayoutMenus(menus, vi, GTMN_NewLookMenus, TRUE, TAG_DONE))
        SetMenuStrip(root, menus);
    scan_volumes();
    draw_icons();
    titles_everywhere();
    ok = 1;

    /* the picture, decoded in a process of its own */
    /* the picture: IPrefs' root picture, rendered in a process of its own (0.1.3); ENV:OpenBench/NoPicture: none */
    if (!Lock_exists("ENV:OpenBench/NoPicture") && root_picture(picture_path, sizeof picture_path) &&
        (DataTypesBase = OpenLibrary((CONST_STRPTR)"datatypes.library", 44))) {
        picture_scr = look.scr;
        picture_busy = 1;
        if (!CreateNewProcTags(NP_Entry, (ULONG)picture_entry, NP_StackSize, 16384, NP_Name, (ULONG)"OpenBench backdrop",
                               NP_Priority, -1, TAG_DONE))
            picture_busy = 0;
    }
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

    for (;;) {
        ULONG sigs = Wait((1UL << idcmp->mp_SigBit) | (1UL << launch_port->mp_SigBit) | (1UL << picture_sig) |
                          (1UL << pub->mp_SigBit) | (dock_sig >= 0 ? 1UL << dock_sig : 0) |
                          SIGBREAKF_CTRL_C | (timer ? 1UL << tport->mp_SigBit : 0));
        struct IntuiMessage *im;
        struct Message *lm;
        int quit = 0;
        if (sigs & SIGBREAKF_CTRL_C) break;
        while ((lm = GetMsg(launch_port))) launch_done((launch *)lm);
        while ((lm = GetMsg(pub))) {
            /* something to open, from the dock or another of the Team's programs (obmsg.h) */
            struct ob_msg *om = (struct ob_msg *)lm;
            if (lm->mn_Length >= sizeof *om && om->magic == OB_MSG_MAGIC) {
                om->path[sizeof om->path - 1] = 0;
                om->ok = ob_open_object(om->path);
            }
            ReplyMsg(lm);
        }
        if (sigs & (1UL << picture_sig)) {
            ULONG *old = NULL;
            Forbid();
            if (picture_ready) { old = backdrop; backdrop = picture_ready; picture_ready = NULL; }
            Permit();
            if (old) FreeVec(old);
            if (backdrop || pic_bm) {
                EraseRect(root->RPort, 0, 0, root->Width - 1, root->Height - 1);
                draw_icons();
            }
        }
        if (timer && CheckIO((struct IORequest *)tr)) {
            WaitIO((struct IORequest *)tr);
            if (make_title()) titles_everywhere();
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
            if (win == root) {
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
    }

out:
    ob_builtin_quit_all();                      /* the built-ins first: their code is ours */
#ifdef OB_HAVE_TATA
    tata_inspect_end();
#endif
    ob_info_quit_all();                         /* and the Information windows' (they close without saving) */
    stop_dock();
    if (timer) {
        AbortIO((struct IORequest *)tr);
        WaitIO((struct IORequest *)tr);
        CloseDevice((struct IORequest *)tr);
    }
    if (tr) DeleteIORequest((struct IORequest *)tr);
    if (tport) DeleteMsgPort(tport);
    while (ndrawers) close_index(ndrawers - 1);
    while (picture_busy) Delay(5);                  /* the picture's process is in our code */
    if (picture_ready) { FreeVec(picture_ready); picture_ready = NULL; }
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
    if (menus) FreeMenus(menus);
    if (vi) FreeVisualInfo(vi);
    if (dri) FreeScreenDrawInfo(look.scr, dri);
    free_icons();
    if (backdrop) { FreeVec(backdrop); backdrop = NULL; }
    if (pic_obj) {
        pic_bm = NULL;
        DisposeDTObject(pic_obj);
        pic_obj = NULL;
    }
    if (DataTypesBase) { CloseLibrary(DataTypesBase); DataTypesBase = NULL; }
    if (launch_port) {
        /* tools still running keep their code until they end: wait for them */
        while (running) {
            struct Message *lm;
            WaitPort(launch_port);
            while ((lm = GetMsg(launch_port))) launch_done((launch *)lm);
        }
        DeleteMsgPort(launch_port);
    }
    if (picture_sig >= 0) FreeSignal(picture_sig);
    if (dock_sig >= 0) FreeSignal(dock_sig);
    if (idcmp) DeleteMsgPort(idcmp);
    if (look.font) CloseFont(look.font);
    if (look.ctx) ogt_ctx_free(&ctx);
    ogt_theme_free(&theme);
    if (look.scr) UnlockPubScreen(NULL, look.scr);
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
    if (!ok || quit_to_wb) run_loadwb();            /* never left without a desktop */
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
