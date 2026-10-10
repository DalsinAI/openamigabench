/* OpenBench 0.1: the desktop, in place of Workbench (amigachrome docs/design/Design-Workbench-Replacement.md,
 * section 5; the user, 11 October 2026: "surely us replacing it was the ask", "so you just boot openbench").
 *
 * The Startup-Sequence runs OpenBench where it ran LoadWB. OpenBench is then the desktop:
 *   - the root window, a backdrop on the Workbench screen: IPrefs' root picture (ENV:Sys/WBPattern.prefs),
 *     scaled to cover the screen and decoded in a process of its own so the desktop shows at once (the
 *     theme's desktop gradient until it is ready); the mounted volumes on it, down the left, at the places
 *     their Disk.info keep;
 *   - drawers, in the drawer windows approved and built as OpenDrawer (opengadtools workbench/wb_drawer);
 *   - starting things as Workbench starts them: a tool with LoadSeg and a WBStartup message, a project
 *     through its default tool, a drawer or disk in a drawer window; SYS:WBStartup at the start;
 *   - the screen's title, "OpenBench 0.1 - Chip, Fast, GPU = 1, CPU = 1 (m68k)", kept up to date.
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
#include <utility/hooks.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/gadtools.h>
#include <proto/icon.h>
#include <proto/wb.h>
#include <proto/datatypes.h>
#include <proto/expansion.h>
#include <proto/input.h>
#include <inline/macros.h>

#include "obmsg.h"
#include "wb_drawer.h"
#include "wb_dir.h"
#include "ogt_theme.h"
#include "ogt_draw.h"
#include "ogt_font.h"

#define OPENBENCH_VERSION "0.1.2"
static const char version[] __attribute__((used)) = "$VER: OpenBench " OPENBENCH_VERSION " (11.10.2026) MIT, Copyright (c) 2026 Dalsin Limited";

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
#define LOGO_PAD "    "                     /* OpenTitle's logo sits before the title (opentitle.c) */

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
static ULONG gpus, cores;

static void count_boards(void)
{
    struct ConfigDev *cd = NULL;
    gpus = cores = 0;
    if (!(ExpansionBase = (struct ExpansionBase *)OpenLibrary((STRPTR)"expansion.library", 36)))
        return;
    while ((cd = FindConfigDev(cd, 0xDA15, 9)) && gpus < 16) gpus++;
    if ((cd = FindConfigDev(NULL, 0xDA15, 7)) && cd->cd_BoardAddr) {
        volatile ULONG *r = (volatile ULONG *)cd->cd_BoardAddr;
        if (r[0] == 0x41435356UL && r[0x20 / 4] == 2 && r[0x24 / 4] <= 1024) cores = r[0x24 / 4];
    }
    CloseLibrary((struct Library *)ExpansionBase);
    ExpansionBase = NULL;
}

/* 1 when the title changed. */
static int make_title(void)
{
    char chip[16], fast[16], t[sizeof screen_title];
    thousands(chip, sizeof chip, AvailMem(MEMF_CHIP) >> 10);
    thousands(fast, sizeof fast, AvailMem(MEMF_FAST) >> 10);
    if (cores)
        snprintf(t, sizeof t, LOGO_PAD "OpenBench " OPENBENCH_VERSION " - Chip %s KB, Fast %s KB, GPU = %lu, CPU = 1 (m68k) + %lu core%s",
                 chip, fast, (unsigned long)gpus, (unsigned long)cores, cores == 1 ? "" : "s");
    else
        snprintf(t, sizeof t, LOGO_PAD "OpenBench " OPENBENCH_VERSION " - Chip %s KB, Fast %s KB, GPU = %lu, CPU = 1 (m68k)",
                 chip, fast, (unsigned long)gpus);
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

/* The layer's backfill: the backdrop picture where the damage is. The bounds are the screen's; a
 * RastPort that still has its layer draws in the layer's coordinates. */
static ULONG bf_fill(struct RastPort *rp, struct backfill_msg *m)
{
    struct Rectangle *b = &m->bounds;
    LONG x0 = b->MinX, y0 = b->MinY, w = b->MaxX - b->MinX + 1, h = b->MaxY - b->MinY + 1, dx = x0, dy = y0;
    ULONG *bd = backdrop;
    if (rp->Layer) { dx -= rp->Layer->bounds.MinX; dy -= rp->Layer->bounds.MinY; }
    if (bd && CyberGfxBase && x0 >= 0 && y0 >= 0 && x0 + w <= bd_w && y0 + h <= bd_h && w > 0 && h > 0) {
        (void)CGX_WRITE(bd, (UWORD)x0, (UWORD)y0, (UWORD)(bd_w * 4), rp, (UWORD)dx, (UWORD)dy, (UWORD)w, (UWORD)h, RECTFMT_ARGB);
    } else {
        struct RastPort copy = *rp;
        SetAPen(&copy, 0);
        SetDrMd(&copy, JAM1);
        copy.AreaPtrn = NULL;
        RectFill(&copy, dx, dy, dx + w - 1, dy + h - 1);
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

/* The picture decoded and scaled to cover the screen (cut evenly where its shape differs), in a
 * process of its own: decoding a big PNG takes a while on a 68k. */
static struct Task *main_task;
static BYTE picture_sig = -1;
static ULONG *volatile picture_ready;
static volatile int picture_busy;
static char picture_path[256];

static ULONG *scale_picture(const char *path, LONG W, LONG H)
{
    Object *o;
    struct BitMapHeader *bmh = NULL;
    ULONG *src = NULL, *dst = NULL;
    LONG sw, sh, vw, vh, ox, oy, x, y;
    if (!(DataTypesBase = OpenLibrary((STRPTR)"datatypes.library", 44)))
        return NULL;
    o = NewDTObject((APTR)path, DTA_GroupID, GID_PICTURE, PDTA_Remap, FALSE, PDTA_DestMode, PMODE_V43, TAG_DONE);
    if (!o) goto out;
    GetDTAttrs(o, PDTA_BitMapHeader, (ULONG)&bmh, TAG_DONE);
    sw = bmh ? bmh->bmh_Width : 0;
    sh = bmh ? bmh->bmh_Height : 0;
    if (sw > 0 && sh > 0 && sw <= 8192 && sh <= 8192 && (src = AllocVec((ULONG)sw * 4, MEMF_FAST))) {
        struct pdtBlitPixelArray m;
        /* One source row at a time (0.1.1): a read of the whole picture at once wrote its pixels to a
         * wild address on Instance-32 (the JIT's report at boot, 11 October 2026: the backdrop's own
         * grey going to $2C40), as OpenUpTool's row reads never did. */
        if ((dst = AllocVec((ULONG)W * H * 4, MEMF_FAST))) {
            LONG have = -1;
            if (W * sh > H * sw) { vw = sw; vh = H * sw / W; }      /* the screen is wider: the picture's width */
            else { vh = sh; vw = W * sh / H; }
            ox = (sw - vw) / 2;
            oy = (sh - vh) / 2;
            for (y = 0; y < H; y++) {
                ULONG *row = dst + y * W;
                LONG sy = oy + y * vh / H;
                if (sy != have) {
                    m.MethodID = PDTM_READPIXELARRAY;
                    m.pbpa_PixelData = src;
                    m.pbpa_PixelFormat = PBPAFMT_ARGB;
                    m.pbpa_PixelArrayMod = (ULONG)sw * 4;
                    m.pbpa_Left = 0;
                    m.pbpa_Top = (ULONG)sy;
                    m.pbpa_Width = (ULONG)sw;
                    m.pbpa_Height = 1;
                    if (!DoDTMethodA(o, NULL, NULL, (Msg)&m)) { FreeVec(dst); dst = NULL; break; }
                    have = sy;
                }
                for (x = 0; x < W; x++) row[x] = src[ox + x * vw / W];
            }
        }
    }
    DisposeDTObject(o);
out:
    if (src) FreeVec(src);
    CloseLibrary(DataTypesBase);
    DataTypesBase = NULL;
    return dst;
}

static void picture_entry(void)
{
    ULONG *p = scale_picture(picture_path, bd_w, bd_h);
    Forbid();
    picture_ready = p;
    picture_busy = 0;
    Signal(main_task, 1UL << picture_sig);
    /* ends in dos.library under Forbid: OpenBench waits for picture_busy before it goes */
}

/* ---- the desktop's icons --------------------------------------------------------------------- */

typedef struct dicon {
    char name[64];                          /* the volume's name, the label */
    char path[72];                          /* "Work:" */
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

/* An icon cleared to the backdrop and drawn again, and any icon it overlapped. */
static void redraw_around(dicon *ic)
{
    LONG x0, y0, x1, y1;
    int i;
    icon_extent(ic, &x0, &y0, &x1, &y1);
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    EraseRect(root->RPort, x0, y0, x1, y1);
    for (i = 0; i < nicons; i++) {
        LONG a0, b0, a1, b1;
        icon_extent(&icons[i], &a0, &b0, &a1, &b1);
        if (a0 <= x1 && a1 >= x0 && b0 <= y1 && b1 >= y0) draw_icon(&icons[i]);
    }
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

/* The mounted volumes, their Disk.info (a default disk icon where there is none), placed where their
 * icons say, the rest down the left. */
static void scan_volumes(void)
{
    struct DosList *dl;
    LONG next_y = 12;
    int i;
    free_icons();
    dl = LockDosList(LDF_VOLUMES | LDF_READ);
    while ((dl = NextDosEntry(dl, LDF_VOLUMES)) && nicons < MAX_VOLUMES) {
        UBYTE *b = (UBYTE *)BADDR(dl->dol_Name);
        int n;
        if (!dl->dol_Task || !b) continue;          /* not in a drive */
        n = b[0] < 63 ? b[0] : 63;
        memcpy(icons[nicons].name, b + 1, (size_t)n);
        icons[nicons].name[n] = 0;
        nicons++;
    }
    UnLockDosList(LDF_VOLUMES | LDF_READ);
    qsort(icons, (size_t)nicons, sizeof icons[0], by_name);
    for (i = 0; i < nicons; i++) {
        dicon *ic = &icons[i];
        char ip[96];
        struct Rectangle r;
        snprintf(ic->path, sizeof ic->path, "%s:", ic->name);
        snprintf(ip, sizeof ip, "%s:Disk", ic->name);
        ic->selected = 0;
        ic->dobj = GetIconTags((CONST_STRPTR)ip, ICONGETA_FailIfUnavailable, FALSE, ICONGETA_Screen, (ULONG)look.scr, TAG_DONE);
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
    p = CreateNewProcTags(NP_Seglist, (ULONG)seg, NP_FreeSeglist, FALSE, NP_StackSize, (ULONG)stack,
                          NP_Name, (ULONG)L->names[0], NP_CurrentDir, (ULONG)DupLock(L->args[0].wa_Lock),
                          NP_HomeDir, (ULONG)DupLock(L->args[0].wa_Lock), TAG_DONE);
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
static int ob_open_object(const char *path)
{
    struct DiskObject *o = GetIconTags((CONST_STRPTR)path, ICONGETA_FailIfUnavailable, TRUE, TAG_DONE);
    int ok = 0;
    if (!o) {
        BPTR l = Lock((CONST_STRPTR)path, ACCESS_READ);
        struct FileInfoBlock *fib = AllocDosObject(DOS_FIB, NULL);
        int dir = 0;
        if (l && fib && Examine(l, fib)) dir = fib->fib_DirEntryType > 0;
        if (fib) FreeDosObject(DOS_FIB, fib);
        if (l) UnLock(l);
        if (dir) return open_drawer(path);
        return is_program(path) ? start_tool(path, NULL, 8192) : 0;
    }
    switch (o->do_Type) {
    case WBDISK: case WBDRAWER: case WBGARBAGE:
        ok = open_drawer(path);
        break;
    case WBTOOL:
        ok = start_tool(path, NULL, o->do_StackSize);
        break;
    case WBPROJECT: {
        char tool[300];
        if (find_tool(path, (const char *)o->do_DefaultTool, tool, sizeof tool))
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

enum { M_SHELL = 1, M_REDRAW, M_UPDATE, M_ABOUT, M_LOADWB, M_QUIT };

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
            if (nx != ox || ny != oy) {
                redraw_around(pressed_on);           /* where it was: the backdrop again */
                pressed_on->ix += nx - ox;
                pressed_on->iy += ny - oy;
                pressed_on->x = nx;
                pressed_on->y = ny;
                draw_icon(pressed_on);
            }
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
    /* 0.1.2: the picture only when ENV:OpenBench/Picture is there. Both 0.1 boots on Instance-32 had a
     * routine return to a wild address within seconds of the picture's decoding starting (the JIT's
     * reports: 1920 and 1600, the screen's and the picture's widths, in its registers); without it, the
     * theme's gradient, until that is found. */
    if (Lock_exists("ENV:OpenBench/Picture") && root_picture(picture_path, sizeof picture_path)) {
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
        if (picture_ready) {
            ULONG *old;
            Forbid();
            old = backdrop;
            backdrop = picture_ready;
            picture_ready = NULL;
            Permit();
            if (old) FreeVec(old);
            EraseRect(root->RPort, 0, 0, root->Width - 1, root->Height - 1);
            draw_icons();
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
