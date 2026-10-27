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
 *     natively"; Kestrel, Wren and Finch): OpenUp-Setup (and Tata's window, below) run as processes of
 *     OpenBench's when opened from SYS:Prefs (ob_builtin.c, ob_builtin_list.h). The editors OpenPrefs has as
 *     pages (Look, Windows, TitleBar, Menus, OpenDock's editor, OpenDateTime, OpenGamepads, OpenInput, OpenSound,
 *     OpenTypes, OpenSerial) were built in up to 0.1.31; since 0.1.32 their old names open OpenPrefs at their page
 *     (old_prefs_open);
 *   - Tata built in (0.1.15; the Team, 11 October 2026: Tata a built-in feature of OpenBench): before OpenBench
 *     runs a program or opens a project it asks Tata's own inspection, linked in (openamigatata
 *     src/tata_inspect.c), and runs it, asks Tata's question, or refuses; no patch. Tata's window is a built-in,
 *     SYS:Prefs/OpenTata. C:Tata's patches stay for every other program, and the question is asked once;
 *   - Information (0.1.13, ob_info.c): its own window for any icon, a drawer's or the desktop's (Icons >
 *     Information...), as workbench.library's WBInfo() does nothing while Workbench isn't running;
 *   - the screen's title, "OpenBench 0.1 - Chip, Fast, GPU = 1, CPU = 1 (m68k)", kept up to date, with
 *     "+ 10 (m68k/x64)" when the CPU cores board is fitted: 68k code on the host's x86-64 cores (0.1.36, the
 *     user, 25 October 2026: "perhaps m68k/x64", "so lets do that blend"; barlogo_isas), after room for OpenTitle's logo worked out from the logo's width and the bar's font (0.1.12,
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
 *   - the dock (0.1.23; the user, 17 October 2026): no longer built in: OpenDock, a program of its own (C:OpenDock),
 *     is started by name when ENV:OpenDock/Dock is there. (0.1.23 to 0.1.31 also listed ROM:Prefs's editors in
 *     SYS:Prefs's window; the ROM has no ROM: volume since the no-ROM-volume design of 24 October 2026);
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
 *   - labels close to the image (0.1.30; the user's solid OS list, 24 October 2026): with icon.library's setting
 *     on (ENV:Sys/IconLabels "close", OpenLook's Workbench tab) a desktop icon's label goes where icon.library
 *     puts it, 2 rows under the lowest row of its picture that isn't empty, instead of OpenBench's own 4 under the
 *     icon's whole box; when the setting changes the desktop's icons are read again;
 *   - the icons' labels in Font prefs' Workbench icon text font (0.1.31; the user's solid OS list, 24 October
 *     2026): on the desktop and in the drawers (wb_look.label_font), when Font prefs sets one, as OpenMeters
 *     draws in it; with none, in the theme's font as before. A change comes back with the screen IPrefs resets,
 *     or, when it doesn't reset it, with the desktop and its drawers opened again two seconds later;
 *   - the ROM without ROM: (0.1.32; the no-ROM-volume design of 24 October 2026, section 7.3): a tool icon with
 *     no file runs the resident of its name (OpenMonitor's icon, the ROM's OpenMonitor), a user one first, then
 *     a system one, while a disk file of that name still wins (resident_tool); a project's default tool is
 *     looked for in SYS:Preferences too, OpenPrefs's drawer; SYS:Prefs's window no longer lists ROM:Prefs;
 *   - the desktop's grid (0.1.33; the user's solid OS list, 24 October 2026): every desktop icon in a cell of one
 *     grid measured on the icons' boxes and their labels in the labels' font, so a label never runs into the
 *     icon under it; a Disk.info's place is kept as the cell nearest it; an icon let go of on the desktop goes
 *     to the free cell nearest; Icons > Clean Up puts them all down the left in name order (place_icons);
 *   - the column grid (0.1.34; the user, 24 October 2026): the desktop's icons down columns of a standard cell,
 *     an odd-sized icon taking its own height and a wide one widening its column, from the left or the right, in
 *     the Clean Up order (name, kind, size or date); an icon let go of stays the user's for the run, snapped to the
 *     grid, and the others go round it; Disk.info places are no longer used; ENV:Sys/IconLayout and IconLabels
 *     (OpenPrefs' Icons page; labels close by default) followed at once, the drawers too (opengadtools wb_grid.c);
 *     the desktop's labels light on a dark backdrop and dark on a light one, as the picture's .tone says;
 *   - drag and drop as OS 3.2.3's Workbench does it (0.1.35; the user, 24 October 2026: icons dragged between
 *     windows), measured on lab t4-dnd (ob_drop.c has the table): no qualifier key changes a drop; a copy onto a
 *     name already there asks 3.2.3's question (Replace, Replace all, Skip, Skip all, Cancel), a move within a
 *     volume isn't asked about; onto a trashcan, what is on its volume goes into it; onto the desktop's space,
 *     icons are left out for the run (gone from their drawers, Icons > Put Away puts them back), and icons
 *     each disk's .backdrop leaves out (3.2.3's Leave Out) show there too; icons left out can be dragged into
 *     drawers, onto disks and onto the trash, and several selected icons go together; the dragged picture is
 *     the icon itself with its alpha, what is under it showing round it (opengadtools wb_ghost.c); a full or
 *     write-protected disk is DOS's to ask about, with Retry, as on 3.2.3;
 *     Alt held at the press takes an icon out of the selection, and no drag starts (3.2.3's); a disk that comes or
 *     goes, or an icon let go of, never lands on an icon the user put somewhere (the deploy lab, 24 October 2026: a
 *     new DH2 at the top put RAM Disk over System; the columns are now measured at their final widths, opengadtools
 *     wb_grid.c, and a drop takes the next free cell);
 *   - no icons on the desktop (0.1.37; Salvor's OS list, item 59; the user, 24 October 2026, 21:08: "offer a no-icons
 *     mode too... for workbench itself"): with ENV:Sys/DesktopIcons "none" (OpenPrefs' Icons page, or Window > No icons
 *     here; openamigaprefs Common/deskicons.h) the desktop shows no icons at all, disks, icons left out and AppIcons
 *     alike, and they take no clicks; the backdrop, the bar, the dock and the meters stay. Disks still mount (their
 *     icons are kept, unseen), drawers still open from the dock, the bar or a Shell, and an icon let go of on the
 *     desktop is still left out there, unseen. Turned off, or with Icons > Clean Up (which turns it off and lays the
 *     icons out), the icons come back where they were. Followed at once, through the labels' notify signal
 *     (ob_show.c);
 *   - Snapshot (0.1.38; Salvor's OS list, item 53; the user, 25 October 2026, 02:55), as OS 3.2.3's: Icons > Snapshot
 *     writes the selected icons' places into their .info files (do_CurrentX and do_CurrentY, through icon.library's
 *     ICONPUTA_OnlyUpdatePosition, so the rest of each file stays as it was), a disk's into its Disk.info; Icons >
 *     Unsnapshot writes NO_ICON_POSITION; Window > Snapshot > All does every icon, and in a drawer > Window keeps the
 *     window's place in the drawer's icon (opengadtools wb_drawer.c, with the icons through wb_look's snap). OpenBench
 *     keeps only the places it wrote itself: ENVARC:OpenBench/Snapshots, read once at the start, lists each with the
 *     place written (ob_snap.c), and a .info whose place isn't the one listed (made for 3.2.3's small icons, or
 *     written by 3.2.3 since) goes on the grid as before. A snapshotted icon stays on its cell and the others go
 *     round it, as round one let go of; Icons > Clean Up lays every desktop icon out and takes their snapshots out
 *     of the record (Snapshot after it keeps the new places). What can't be written (a write-protected disk) is
 *     named in a requester with DOS's reason, and the rest is still written;
 *   - the drawers' thumbnails on again (0.1.39): a task has 16 signals of its own, and 0.1.38's process asked for 17,
 *     so the last, the thumbnails' (opengadtools wb_thumb.c), failed and they were quietly off (Instance-32). The
 *     ENV: files OpenBench follows (Workbench.prefs, WBPattern.prefs, IconLabels with DesktopIcons, IconLayout,
 *     font.prefs) had a signal each; their notifies now send messages to one port, each request's nr_UserData naming
 *     its file (ob_notify.c), which frees four signals and leaves three for what takes one for a moment (a
 *     requester's window, a Delay on the open ROM's dos). Each change does what its signal did. A signal or a port
 *     run() can't have is said on the Shell, as OpenBench's other lines are;
 *   - a drive mounted while running gets its icon (0.1.40; Salvor's checks, 25 October 2026): a volume added to the DOS
 *     list with no disk event (a Mount DH2:, a RAD:, a drive Cradle adds: their handlers post no IECLASS_DISKINSERTED)
 *     had no icon until a disk went in or out somewhere. Each scan of the volumes keeps their fingerprint (ob_vols.c:
 *     the count, and sums of a hash of each one's name, date and drive), and the 2-second tick takes it again and
 *     compares; when it differs the volumes are scanned and drawn as after a disk event, so a new one's icon comes
 *     and a gone one's goes within a tick. No new signal and no faster timer: every 2 seconds one AttemptLockDosList,
 *     a NextDosEntry for each volume and one more, and an UnLockDosList;
 *   - Leave Out (0.1.41; Team 4's checks, 25 October 2026), as OS 3.2.3's: Icons > Leave Out in a drawer (opengadtools
 *     wb_drawer.c, through wb_look's leave_out) puts the selected icons on the desktop for good, each one's line
 *     (":Tools/OpenMeters") written into its disk's .backdrop, so they are left out at every start and gone from their
 *     drawer; on the desktop it does the same for an icon left out there for the run. Icons > Put Away sends icons
 *     left out back, their lines taken out of .backdrop (the file deleted when none is left). A disk can't be left
 *     out (a beep, as for a drawer with nothing selected). What can't be written (a write-protected disk) is named in
 *     a requester with DOS's reason, and that icon stays as it was. The file's text is read and edited in plain C
 *     (ob_backdrop.c, tested on the host). Icons left out are desktop icons: Snapshot keeps their places, Clean Up
 *     lays them out;
 *   - OpenLook's close notice found again (0.1.42; W128's scan of the GCC 16 builds, 25 October 2026): OpenAmigaGCC
 *     16.2.0-open3's late-combine pass made wbclose.h's strcpy(r->name, "OpenWBNotify") a copy that stored each
 *     byte one place late ("move.b (a1)+,(a1,d0.l)"), so the root semaphore OpenBench made was named "" and
 *     FindSemaphore("OpenWBNotify") (OpenLook's patch, wbn_root here) never found it. desktop/build.sh turns the pass
 *     off on a compiler that has it, until OpenAmigaGCC open4 (openamigagcc #29); no source changed;
 *   - the backdrop under the screen's title bar (0.1.43; Dale, 26 October 2026: "Why isn't the screen title
 *     translucent?"): a second backdrop window, the strip, over the bar's rows, filled by the root window's own
 *     backfill (the gradient and the picture, in screen coordinates), and ShowTitle(screen, TRUE) so the bar stays in
 *     front of it. OpenTitle 0.7.5 on makes the bar see-through only where a layer behind it covers it all
 *     (opentitle.c, see_backed), and the root window starts under the bar: with OpenBench alone only the
 *     compositor's black was behind it, and the bar stayed solid. The strip has no IDCMP and is never active; it is
 *     a Workbench window (WFLG_WBENCHWINDOW), closed and opened with the root (openamigaprefs TitleBar/DESIGN.md, "Not
 *     yet");
 *   - a move within a volume onto a name that is taken says so in the screen's title, as OS 3.2.3 does, not in a
 *     requester (0.1.44; W151's lab t4-dnd, 27 October 2026: 3.2.3's title said "Error while moving x.txt: (203)
 *     object already exists" for 15 seconds and more, until a click in a drawer window; OpenBench 0.1.43 put up a
 *     requester instead). ob_drop hands the words over when the job ends (ob_drop_env.say); say_title keeps them in
 *     the title of every OpenBench window, the 2-second tick leaves them there, and the next click (the left button
 *     pressed in any OpenBench window) gives the usual title back;
 *
 * Next (the user's order): the rest of workbench.library's entries (AppIcons, AppMenuItems) brought to OpenBench, as
 * Directory Opus 5 takes them.
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
#include "ob_show.h"
#include "ob_snap.h"
#include "ob_notify.h"
#include "ob_vols.h"
#include "ob_backdrop.h"
#include "ob_strip.h"
#include "deskicons.h"
#include "wb_dir.h"
#include "wb_fmt.h"
#include "wb_thumb.h"
#include "wb_pic.h"
#include "wb_ghost.h"
#include "wb_grid.h"
#include "ogt_theme.h"
#include "ogt_draw.h"
#include "ogt_font.h"

#define OPENBENCH_VERSION "0.1.44"
static const char version[] __attribute__((used)) = "$VER: OpenBench " OPENBENCH_VERSION " (27.10.2026) MIT, Copyright (c) 2026 AmigaChrome Team";

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
#define MAX_LEFTOUT 64                      /* 0.1.35: icons left out on the desktop */
#define MAX_DESK (MAX_VOLUMES + MAX_LEFTOUT)
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
/* 0.1.31: the icons' labels' font, Font prefs' Workbench icon text font (look.label_font, NULL when Font prefs sets
 * none: the labels in look.font, as before); what Font prefs asked for when it was opened, to see it change */
static struct TextAttr label_ta, label_asked;
static char label_name[128], label_asked_name[128];
static int label_set;
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
    cd = NULL;
    while ((cd = FindConfigDev(cd, 0xDA15, 16)) && gpus < 16) gpus++;   /* the 256 MiB board (24 Oct 2026) */
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
static struct Window *strip;                /* 0.1.43: the backdrop under the screen's title bar (strip_open) */
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

/* 0.1.43: the strip, the backdrop under the screen's title bar: its rows (BarHeight + 1, from the top), with the
 * root's backfill, so the backdrop goes on under the bar as it is behind the root. ShowTitle keeps the bar in
 * front of it (a backdrop window over the bar's rows hides the bar otherwise). OpenTitle's see-through bar shows
 * it then (OpenTitle 0.7.5 on, a composited 32-bit screen with an Intuition that takes ICTRL_BarOpacity); anywhere
 * else the solid bar covers it. No IDCMP: it is never active and takes no input (the bar is in front of all of
 * it). Without it OpenBench is as before. */
static void strip_open(void)
{
    ob_box b;
    if (strip || !look.scr) return;
    b = ob_strip_box(look.scr->Width, look.scr->BarHeight);
    strip = OpenWindowTags(NULL, WA_Flags, WFLG_WBENCHWINDOW, WA_Left, b.left, WA_Top, b.top, WA_Width, b.width,
                           WA_Height, b.height, WA_PubScreen, (ULONG)look.scr, WA_Backdrop, TRUE,
                           WA_Borderless, TRUE, WA_SimpleRefresh, TRUE, WA_NoCareRefresh, TRUE,
                           WA_BackFill, (ULONG)&backfill_hook, WA_ScreenTitle, (ULONG)screen_title, WA_IDCMP, 0, TAG_DONE);
    if (strip) ShowTitle(look.scr, TRUE);
}

static void strip_close(void)
{
    if (strip) { CloseWindow(strip); strip = NULL; }
}

/* drawn again from the backdrop (the picture came, or another one) */
static void strip_redraw(void)
{
    if (strip) EraseRect(strip->RPort, 0, 0, strip->Width - 1, strip->Height - 1);
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
static struct Process *picture_proc;        /* 0.1.34: the picture's process while it decodes, and its ticks */
static int picture_ticks;

/* 0.1.34: the picture's process, still decoding, on at priority 0 (it starts at -1, under everything else). In the lab
 * a backdrop chosen while OpenBench ran never came: its process sat ready at -1 behind Shell commands that never wait
 * at 0, and OpenBench, waiting for it to end before the next picture, stopped too. */
static void picture_hurry(void)
{
    Forbid();
    if (picture_busy && picture_proc) SetTaskPri(&picture_proc->pr_Task, 0);
    if (!picture_busy) picture_proc = NULL;
    Permit();
}
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
    char name[64];                          /* the volume's name, the label (0.1.35: a left-out icon's file's) */
    char path[256];                         /* "Work:" (0.1.35: a left-out icon's file, "Work:Tools/OpenMeters") */
    const char *kind;                       /* its drive's default icon, def_<kind>, when it has no Disk.info */
    struct DiskObject *dobj;
    LONG x, y, w, h;                        /* the picture's box in the root window */
    LONG ix, iy;                            /* where DrawIconState puts it for that box */
    LONG lt;                                /* the label's text, from the box's top */
    ULONG kb;                               /* 0.1.34: its size, for Clean Up by size */
    struct DateStamp date;                  /* 0.1.34: the volume's date, for Clean Up by date */
    int selected;
    int lo;                                 /* 0.1.35: a left-out icon's louts[] entry; -1: a disk */
    int snapped;                            /* 0.1.38: snapshotted by OpenBench, its .info still so: on cell sgx, sgy */
    int sgx, sgy;
} dicon;

static dicon icons[MAX_DESK];
static int nicons;
static struct DrawInfo *dri;
static LONG label_h;
/* 0.1.37: No icons (ob_show.h): while it is on, draw_icon draws nothing, and icon_at and desk_icon_at find nothing. The
 * icons are still read, kept and laid out, so they come back where they were. */
static ob_show show;

#define LABEL_GAP 4

/* 0.1.34: the desktop's column grid (the user, 24 October 2026: "the grid should behave the way the Masonry
 * JavaScript layout engine works", then "the screen is arranged in 'columns', and the grid follows a standard size,
 * until a bigger or smaller icon sits in the column"), with OpenDrawer's engine (opengadtools workbench/wb_grid.c).
 * A standard cell comes from the icons (their usual size, or the size OpenPrefs' Icons page sets): the picture, its
 * label close under it (icon.library's place, 2 rows under the picture's lowest row that shows) and the label's
 * height. The icons go down the columns from the left (or the right, as the Icons page says) in the Clean Up order;
 * each takes the standard height unless its picture is taller or a quarter shorter, and each column is as wide as
 * the standard unless an icon in it (its picture or its label) is wider. An icon the user let go of on the desktop
 * in this run stays there, on the grid's nearest place unless the Icons page turns snapping off, and the others go
 * round it; Icons > Clean Up lets them all flow again. A Disk.info's own place isn't used: made for another screen
 * and other icons, it is what put the labels into the icons under them (0.1.32); 0.1.33 kept the cell nearest it.
 * Places let go of are kept by name for the run; 0.1.38's Snapshot keeps them for good (ob_snap.h): a .info's place
 * is used only when OpenBench wrote it, and the icon goes back on the cell it had, as one let go of there. */
#define GRID_X 16                           /* the grid's corner, from the root window's */
#define GRID_Y 12
static wb_iconprefs ip;                     /* the icons' settings: ENV:Sys/IconLabels, ENV:Sys/IconLayout */
static wb_gspec gspec;
static wb_glayout glay;
static wb_gcell gcells[MAX_DESK];           /* icons[i]'s cell */
static LONG label_max = 112;                /* a label's widest; a longer one ends in "..." */

/* Where the user put an icon in this run, by its volume's name: its cell's corner from the grid's. Kept while its
 * disk is out, so it comes back there. */
typedef struct dplace {
    char name[256];                         /* a disk's name; 0.1.35: a left-out icon's path */
    WORD x, y;
} dplace;
#define MAX_PLACES (2 * MAX_DESK)
static dplace places[MAX_PLACES];
static int nplaces;

/* 0.1.38: Snapshot's record (ob_snap.h): the places OpenBench wrote into .info files, the only ones it keeps. Read
 * from ENVARC:OpenBench/Snapshots once, when OpenBench starts, and written there when it changes. */
#define SNAPS_DIR "ENVARC:OpenBench"
#define SNAPS_FILE SNAPS_DIR "/Snapshots"
static ob_snap snaps;

/* The icons' settings, as OpenPrefs' Icons page writes them. Labels close to the picture by default (0.1.30 had
 * them under the whole box unless ENV:Sys/IconLabels said "close"): icon.library then ends an icon's box gap - 1
 * rows under its picture's lowest row that shows, and the label's text goes where icon.library says
 * (GetIconRectangle with the label, in the labels' font). Read when the desktop's icons are read, as icon.library
 * decides each icon's labels when it is read. */
static void read_iconprefs(void)
{
    char a[40], b[120];
    LONG n = GetVar((CONST_STRPTR)"Sys/IconLabels", (STRPTR)a, sizeof a, GVF_GLOBAL_ONLY);
    LONG m = GetVar((CONST_STRPTR)"Sys/IconLayout", (STRPTR)b, sizeof b, GVF_GLOBAL_ONLY);
    wb_iconprefs_parse(&ip, n >= 0 ? a : NULL, m >= 0 ? b : NULL);
}

static void label_box(dicon *ic, LONG *lx, LONG *ly, LONG *lw)
{
    struct RastPort *rp = root->RPort;
    LONG tw = ogt_text_width(rp, ic->name);
    if (tw > label_max) tw = label_max;
    *lw = tw;
    *lx = ic->x + (ic->w - tw) / 2;
    *ly = ic->y + ic->lt;
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

/* 0.1.34: the desktop's labels in the colour the backdrop asks for (the background pack's lab, 24 October 2026: dark
 * labels on the dark backdrops were hard to read). OpenUp's backdrops have a sidecar beside the picture,
 * <picture>.tone, "dark" or "light" (OpenUpTool RECONCILE reads it for Font prefs' pens, which OpenBench doesn't
 * draw with): on a dark picture the labels are light with a dark shadow, on a light one dark. With no .tone, no
 * picture, or the Icons page's "The theme's" (ENV:Sys/IconLayout labelcolour=theme), the theme's icon.text as before.
 * Read when the desktop comes up and when the root picture changes (ENV:Sys/WBPattern.prefs). */
static int label_tone;                      /* 0 the theme's, 1 light on a dark picture, 2 dark on a light one */

static void read_tone(void)
{
    char path[300], buf[16];
    BPTR f;
    LONG n;
    int k;
    label_tone = 0;
    if (Lock_exists("ENV:OpenBench/NoPicture") || !root_picture(path, (int)sizeof path - 6)) return;
    strcat(path, ".tone");
    if (!(f = Open((CONST_STRPTR)path, MODE_OLDFILE))) return;
    n = Read(f, buf, sizeof buf - 1);
    Close(f);
    buf[n > 0 ? n : 0] = 0;
    for (k = 0; buf[k]; k++)
        if (buf[k] >= 'A' && buf[k] <= 'Z') buf[k] += 32;
    if (!strncmp(buf, "dark", 4)) label_tone = 1;
    else if (!strncmp(buf, "light", 5)) label_tone = 2;
}

static void draw_icon(dicon *ic)
{
    static const ogt_rgb light = { 0xF4, 0xF4, 0xF4 }, shadow = { 0x10, 0x10, 0x14 }, dark = { 0x1A, 0x1A, 0x1E };
    struct RastPort *rp = root->RPort;
    LONG lx, ly, lw;
    int tone = ip.tone ? label_tone : 0;
    if (!ic->dobj || !ob_show_icons(&show)) return;     /* 0.1.37: No icons */
    DrawIconState(rp, ic->dobj, NULL, ic->ix, ic->iy, ic->selected ? IDS_SELECTED : IDS_NORMAL,
                  ICONDRAWA_Frameless, TRUE, ICONDRAWA_Borderless, TRUE, ICONDRAWA_EraseBackground, FALSE,
                  ICONDRAWA_DrawInfo, (ULONG)dri, TAG_DONE);
    label_box(ic, &lx, &ly, &lw);
    if (ic->selected) {
        /* 0.1.34: with labels close, round the letters, not over the picture (the font's rows with no ink) */
        int lead = ip.close ? ogt_font_lead(rp->Font) : 0;
        ogt_box(rp, ogt_pen(&ctx, "accent"), (int)lx - 5, (int)ly + lead - 1, (int)lw + 10, (int)label_h + 2 - lead);
        ogt_text(rp, ogt_pen(&ctx, "accent.text"), (int)lx, (int)ly, ic->name, (int)lw);
    } else if (tone == 1) {                 /* a dark backdrop: light, on a shadow a pixel down and right */
        ogt_text(rp, ogt_pen_rgb(&ctx, shadow), (int)lx + 1, (int)ly + 1, ic->name, (int)lw);
        ogt_text(rp, ogt_pen_rgb(&ctx, light), (int)lx, (int)ly, ic->name, (int)lw);
    } else if (tone == 2)                   /* a light one: dark */
        ogt_text(rp, ogt_pen_rgb(&ctx, dark), (int)lx, (int)ly, ic->name, (int)lw);
    else
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

/* ---- the desktop's column grid (0.1.34) ------------------------------------------------------------ */

static void redraw_all(void);

/* 0.1.35: an icon's key among the places: a disk's name, or a left-out icon's path (no disk's name has a colon) */
static const char *place_key(const dicon *ic)
{
    return ic->lo >= 0 ? ic->path : ic->name;
}

static dplace *place_of(const char *name)
{
    int i;
    for (i = 0; i < nplaces; i++)
        if (!strcmp(places[i].name, name)) return &places[i];
    return NULL;
}

/* A place for the user's icon, a new one if it has none (the one kept longest goes when they are all used). */
static dplace *place_for(const char *name)
{
    dplace *p = place_of(name);
    if (p) return p;
    if (nplaces == MAX_PLACES) {
        memmove(&places[0], &places[1], (MAX_PLACES - 1) * sizeof places[0]);
        nplaces--;
    }
    p = &places[nplaces++];
    snprintf(p->name, sizeof p->name, "%s", name);
    return p;
}

/* An icon's picture in its cell: at the cell's middle and top, its label under it (label_box). */
static void put_in(dicon *ic, const wb_gcell *c)
{
    LONG x = GRID_X + c->x + (c->w - ic->w) / 2, y = GRID_Y + c->y;
    ic->ix += x - ic->x;
    ic->iy += y - ic->y;
    ic->x = x;
    ic->y = y;
}

/* Every icon read into the column grid (above), in icons[]'s order (the Clean Up order). clean: Icons > Clean Up,
 * the user's places forgotten. */
static void place_icons(int clean)
{
    struct RastPort *rp = root->RPort;
    LONG em = ogt_text_width(rp, "n"), off = 0;
    int i;
    if (clean) nplaces = 0;
    if (em < 4) em = 4;
    label_max = 14 * em;
    for (i = 0; i < nicons; i++)
        if (icons[i].lt - icons[i].h > off) off = icons[i].lt - icons[i].h;
    memset(&gspec, 0, sizeof gspec);
    gspec.std_pic = ip.cell;
    gspec.box_extra = ip.close && ip.gap > 1 ? ip.gap - 1 : 0;
    gspec.label_h = label_h;
    gspec.label_off = off;                  /* the label's text under the box: icon.library's, or LABEL_GAP */
    gspec.label_room = 10 * em;             /* a standard column: ten of the labels' font's "n" */
    gspec.label_max = label_max;
    gspec.gap_x = label_h > 16 ? label_h : 16;
    gspec.gap_y = label_h / 2 > 8 ? label_h / 2 : 8;
    for (i = 0; i < nicons; i++) {
        wb_gcell *c = &gcells[i];
        dplace *p = place_of(place_key(&icons[i]));
        memset(c, 0, sizeof *c);
        c->pw = icons[i].w;
        c->ph = icons[i].h;
        c->fh = wb_icon_full_h(icons[i].dobj, (int)icons[i].w);    /* its whole picture: its size */
        c->lw = ogt_text_width(rp, icons[i].name);
        if (p) {
            c->pin = 1;
            c->x = p->x;
            c->y = p->y;
        } else if (icons[i].snapped) {      /* 0.1.38: snapshotted, and not moved in this run: its cell then */
            c->pin = 1;
            c->x = icons[i].sgx;
            c->y = icons[i].sgy;
        }
    }
    wb_grid_columns(&gspec, gcells, nicons, root->Width - 2 * GRID_X, root->Height - 2 * GRID_Y, ip.right, &glay);
    for (i = 0; i < nicons; i++) put_in(&icons[i], &gcells[i]);
}

/* An icon let go of on the desktop: its place the user's for the run, on the grid's nearest place when snapping is
 * on (OpenPrefs' Icons page), and the others round it. */
static void drop_on_desktop(dicon *ic)
{
    int i = (int)(ic - icons), x, y;
    wb_gcell c = gcells[i];
    dplace *p;
    x = (int)(ic->x - (c.w - ic->w) / 2 - GRID_X);
    y = (int)(ic->y - GRID_Y);
    if (ip.snap) wb_grid_snap(&gspec, &glay, &c, &x, &y);
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    /* 0.1.35: on the grid, never onto another icon the user put there: the next free cell down (an icon not put
     * anywhere goes round it, as every icon does round the user's) */
    if (ip.snap) wb_grid_free(&gspec, &glay, gcells, nicons, i, (int)(root->Height - 2 * GRID_Y), &x, &y);
    p = place_for(place_key(ic));
    p->x = (WORD)x;
    p->y = (WORD)y;
    place_icons(0);
    redraw_all();
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

/* 0.1.34: the Clean Up order (OpenPrefs' Icons page): by name, by kind of drive, by size (the smallest first), by
 * date (the oldest first); by name among equals */
static int by_order(const void *a, const void *b)
{
    const dicon *x = (const dicon *)a, *y = (const dicon *)b;
    int c = 0;
    if (ip.order == WB_ORDER_KIND) c = ci_cmp(x->kind, y->kind);
    else if (ip.order == WB_ORDER_SIZE) c = x->kb < y->kb ? -1 : x->kb > y->kb;
    else if (ip.order == WB_ORDER_DATE) {
        const struct DateStamp *p = &x->date, *q = &y->date;
        c = p->ds_Days != q->ds_Days ? (p->ds_Days < q->ds_Days ? -1 : 1) :
            p->ds_Minute != q->ds_Minute ? (p->ds_Minute < q->ds_Minute ? -1 : 1) :
            p->ds_Tick != q->ds_Tick ? (p->ds_Tick < q->ds_Tick ? -1 : 1) : 0;
    }
    return c ? c : ci_cmp(x->name, y->name);
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

/* ---- icons left out on the desktop (0.1.35) -------------------------------------------------------------
 * As OS 3.2.3 has them (measured, lab t4-dnd, 24 October 2026): an icon let go of on the desktop's space is left out
 * there for this run, gone from its drawer's window and back in it after a reboot; Icons > Leave Out (3.2.3's,
 * OpenFiles', or since 0.1.41 OpenBench's own) writes a line ":drawer/name" into the disk's .backdrop, and those are
 * left out at every start; Icons > Put Away puts either kind back, taking its line out of .backdrop (and the file
 * away when no line is left, as 3.2.3 does). A left-out icon opens what it stands for, and dragged into a drawer,
 * onto a disk or onto the trash its file goes there, as from a drawer. */
typedef struct leftout {
    char path[256];                         /* "Work:Tools/OpenMeters" */
    UBYTE permanent;                        /* named in its disk's .backdrop */
} leftout;
static leftout louts[MAX_LEFTOUT];
static int nlouts;
static int reread_pending;                  /* the drawers read again once this round of messages is done */

/* path as its disk names it ("System:DnD-A/two.txt" for "SYS:DnD-A/two.txt"), in out; 0 when it isn't there */
static int full_name(const char *path, char *out, int n)
{
    BPTR l = Lock((CONST_STRPTR)path, ACCESS_READ);
    int ok = l && NameFromLock(l, (STRPTR)out, n);
    if (l) UnLock(l);
    return ok;
}

static int lout_find(const char *path)
{
    int i;
    for (i = 0; i < nlouts; i++)
        if (!ci_cmp(louts[i].path, path)) return i;
    return -1;
}

static void lout_drop(int i)
{
    memmove(&louts[i], &louts[i + 1], (size_t)(nlouts - i - 1) * sizeof louts[0]);
    nlouts--;
}

/* A disk's .backdrop, whole, as AllocVec'd text (NUL-terminated), or NULL when the disk has none or it is empty.
 * *err: 0, or DOS's error when it is there and couldn't be read (then it isn't written over either). No DOS
 * requester while it looks. */
static char *backdrop_text(const char *vol, LONG *len, LONG *err)
{
    struct Process *me = (struct Process *)FindTask(NULL);
    APTR was = me->pr_WindowPtr;
    char p[96];
    BPTR fh;
    char *buf = NULL;
    LONG n = 0;
    snprintf(p, sizeof p, "%s:.backdrop", vol);
    *len = *err = 0;
    me->pr_WindowPtr = (APTR)-1;
    fh = Open((CONST_STRPTR)p, MODE_OLDFILE);
    if (!fh && IoErr() != ERROR_OBJECT_NOT_FOUND) *err = IoErr() ? IoErr() : ERROR_OBJECT_IN_USE;
    me->pr_WindowPtr = was;
    if (!fh) return NULL;
    if (Seek(fh, 0, OFFSET_END) < 0 || (n = Seek(fh, 0, OFFSET_BEGINNING)) < 0)
        *err = IoErr() ? IoErr() : ERROR_SEEK_ERROR;
    else if (n >= 65536)
        *err = ERROR_OBJECT_TOO_LARGE;
    else if (n > 0 && !(buf = AllocVec((ULONG)n + 1, MEMF_ANY)))
        *err = ERROR_NO_FREE_STORE;
    else if (n > 0 && (n = Read(fh, buf, n)) < 0) {
        *err = IoErr() ? IoErr() : ERROR_SEEK_ERROR;
        FreeVec(buf);
        buf = NULL;
    } else if (n > 0) {
        buf[n] = 0;
        *len = n;
    }
    Close(fh);
    return buf;
}

/* ob_backdrop_parse's each: an icon of the disk being read (vol), left out as permanent */
static void backdrop_icon(void *ctx, const char *rest)
{
    char path[256];
    int k;
    if (snprintf(path, sizeof path, "%s:%s", (const char *)ctx, rest) >= (int)sizeof path) return;
    if ((k = lout_find(path)) >= 0) louts[k].permanent = 1;
    else if (nlouts < MAX_LEFTOUT) {
        snprintf(louts[nlouts].path, sizeof louts[0].path, "%s", path);
        louts[nlouts++].permanent = 1;
    }
}

/* Each line of a disk's .backdrop (":drawer/name") left out, as permanent (ob_backdrop.h) */
static void backdrop_read(const char *vol)
{
    LONG len, err;
    char *buf = backdrop_text(vol, &len, &err);
    if (!buf) return;
    ob_backdrop_parse(buf, len, backdrop_icon, (void *)vol);
    FreeVec(buf);
}

/* 0.1.41: path (as its disk names it, "Work:Tools/OpenMeters") added to its disk's .backdrop (add: Icons > Leave
 * Out) or taken out of it (Icons > Put Away), as OS 3.2.3 writes it (ob_backdrop.h): its line at the end, or every
 * line naming it gone, and the file deleted when no line is left. Nothing is written when nothing changes. No DOS
 * requester while it writes (OpenBench's names the icon). 0, or DOS's error. */
static LONG backdrop_change(const char *path, int add)
{
    struct Process *me = (struct Process *)FindTask(NULL);
    APTR was;
    char vol[OB_SNAP_VOL], rest[OB_SNAP_PATH], p[OB_SNAP_VOL + 12], *buf, *out;
    LONG len, need, err = 0;
    int changed;
    if (!ob_snap_split(path, vol, sizeof vol, rest, sizeof rest) || !rest[0]) return ERROR_OBJECT_WRONG_TYPE;
    buf = backdrop_text(vol, &len, &err);
    if (err) return err;                            /* there, and not read: not written over */
    need = ob_backdrop_edit(buf, len, rest, add, NULL, 0, &changed);
    if (need < 0 || !changed) {
        if (buf) FreeVec(buf);
        return need < 0 ? ERROR_INVALID_COMPONENT_NAME : 0;
    }
    if (!(out = AllocVec((ULONG)need + 1, MEMF_ANY))) {
        if (buf) FreeVec(buf);
        return ERROR_NO_FREE_STORE;
    }
    ob_backdrop_edit(buf, len, rest, add, out, need + 1, &changed);
    if (buf) FreeVec(buf);
    snprintf(p, sizeof p, "%s:.backdrop", vol);
    was = me->pr_WindowPtr;
    me->pr_WindowPtr = (APTR)-1;
    if (!need) {
        if (!DeleteFile((CONST_STRPTR)p) && IoErr() != ERROR_OBJECT_NOT_FOUND) err = IoErr();
    } else {
        BPTR fh = Open((CONST_STRPTR)p, MODE_NEWFILE);
        if (!fh)
            err = IoErr() ? IoErr() : ERROR_OBJECT_NOT_FOUND;
        else {
            if (Write(fh, out, need) != need) err = IoErr() ? IoErr() : ERROR_DISK_FULL;
            if (!Close(fh) && !err) err = IoErr();
        }
    }
    me->pr_WindowPtr = was;
    FreeVec(out);
    return err;
}

/* 1 when the disk holding path is among the first nv icons (the disks read) */
static int volume_shown(const char *path, int nv)
{
    const char *c = strchr(path, ':');
    int i, n = c ? (int)(c - path) : -1;
    for (i = 0; n > 0 && i < nv; i++)
        if ((int)strlen(icons[i].name) == n) {
            char a[64];
            memcpy(a, path, (size_t)n);
            a[n] = 0;
            if (!ci_cmp(a, icons[i].name)) return 1;
        }
    return 0;
}

/* After the disks: every disk's .backdrop read, then an icon for each icon left out whose disk is in (one left
 * out for the run whose file has gone is forgotten). */
static void leftouts_add(void)
{
    int nv = nicons, i;
    for (i = 0; i < nv; i++) backdrop_read(icons[i].name);
    for (i = 0; i < nlouts && nicons < MAX_DESK; i++) {
        dicon *ic = &icons[nicons];
        struct Rectangle r;
        BPTR l;
        if (!volume_shown(louts[i].path, nv)) continue;
        if (!(l = Lock((CONST_STRPTR)louts[i].path, ACCESS_READ))) {
            if (!louts[i].permanent) lout_drop(i--);
            continue;
        }
        UnLock(l);
        memset(ic, 0, sizeof *ic);
        snprintf(ic->path, sizeof ic->path, "%s", louts[i].path);
        snprintf(ic->name, sizeof ic->name, "%s", (const char *)FilePart((CONST_STRPTR)louts[i].path));
        ic->kind = "";
        ic->lo = i;
        ic->dobj = GetIconTags((CONST_STRPTR)ic->path, ICONGETA_Screen, (ULONG)look.scr, TAG_DONE);
        if (ic->dobj) {                             /* 0.1.38: snapshotted on the desktop by OpenBench */
            char vol[OB_SNAP_VOL], rest[OB_SNAP_PATH];
            ic->snapped = ob_snap_split(ic->path, vol, sizeof vol, rest, sizeof rest) &&
                          ob_snap_honour(&snaps, vol, rest, OB_SNAP_DESK, ic->dobj->do_CurrentX, ic->dobj->do_CurrentY,
                                         &ic->sgx, &ic->sgy);
        }
        ic->w = 48; ic->h = 40;
        r.MinX = r.MinY = 0;
        if (ic->dobj && GetIconRectangle(root->RPort, ic->dobj, NULL, &r, ICONDRAWA_Borderless, TRUE, TAG_DONE)) {
            ic->w = r.MaxX - r.MinX + 1;
            ic->h = r.MaxY - r.MinY + 1;
        }
        ic->lt = ic->h + LABEL_GAP;
        if (ip.close && ic->dobj) {
            struct Rectangle lr;
            if (GetIconRectangle(root->RPort, ic->dobj, (STRPTR)ic->name, &lr, ICONDRAWA_Borderless, TRUE, TAG_DONE) &&
                lr.MaxY - r.MinY - label_h > 0)
                ic->lt = lr.MaxY - r.MinY - label_h;
        }
        ic->ix = -r.MinX;
        ic->iy = -r.MinY;
        nicons++;
    }
}

/* wb_look's hidden: an icon left out on the desktop isn't shown in its drawer's window. The drawer's path as its
 * disk names it is kept for the next entry (a drawer opened as "SYS:Tools" is "System:Tools") */
static char hid_dir[256], hid_full[256];

static int leftout_hidden(const char *dir, const char *name)
{
    char p[256];
    size_t n;
    if (!nlouts) return 0;
    if (strcmp(dir, hid_dir)) {
        snprintf(hid_dir, sizeof hid_dir, "%s", dir);
        if (!full_name(dir, hid_full, sizeof hid_full)) snprintf(hid_full, sizeof hid_full, "%s", dir);
    }
    n = strlen(hid_full);
    if (n + strlen(name) + 2 > sizeof p) return 0;
    strcpy(p, hid_full);
    if (n && p[n - 1] != ':') strcat(p, "/");
    strcat(p, name);
    return lout_find(p) >= 0;
}

/* 0.1.40: the volumes in a drive when they were last scanned (ob_vols.h), what the 2-second tick compares with */
static ob_volfp vols_seen;

/* A volume, into fingerprint f when it is in a drive; 0 when it isn't. Called with the DOS list locked for volumes. */
static int vol_fp_add(ob_volfp *f, const struct DosList *dl)
{
    const UBYTE *b = (const UBYTE *)BADDR(dl->dol_Name);
    const struct DateStamp *d = &dl->dol_misc.dol_volume.dol_VolumeDate;
    if (!dl->dol_Task || !b) return 0;
    ob_volfp_add(f, b + 1, b[0], d->ds_Days, d->ds_Minute, d->ds_Tick, (unsigned long)dl->dol_Task);
    return 1;
}

/* 0.1.40: the 2-second tick's look: 1 when the volumes in a drive aren't the ones scan_volumes last read, as when a
 * Mount DH2:, a RAD: or a drive Cradle adds brings a volume and no disk event (ob_vols.h). The list is read only when
 * no one is writing it (AttemptLockDosList), so the main loop never waits on it: a list being changed is looked at on
 * the next tick. */
static int volumes_moved(void)
{
    struct DosList *dl;
    ob_volfp now;
    ob_volfp_init(&now);
    dl = AttemptLockDosList(LDF_VOLUMES | LDF_READ);
    if (!dl || dl == (struct DosList *)1) return 0;   /* busy (dos.library's autodoc: old ones answered 1 for it) */
    while ((dl = NextDosEntry(dl, LDF_VOLUMES))) vol_fp_add(&now, dl);
    UnLockDosList(LDF_VOLUMES | LDF_READ);
    return !ob_volfp_same(&now, &vols_seen);
}

/* The mounted volumes, their Disk.info (where there is none, the default icon for their kind of drive), on the
 * column grid in the Clean Up order. 0.1.40: their fingerprint kept, every volume in a drive's, hidden or not. */
static void scan_volumes(void)
{
    struct DosList *dl, *start;
    int i;
    free_icons();
    read_iconprefs();
    ob_volfp_init(&vols_seen);
    dl = start = LockDosList(LDF_VOLUMES | LDF_DEVICES | LDF_READ);
    while ((dl = NextDosEntry(dl, LDF_VOLUMES))) {
        UBYTE *b = (UBYTE *)BADDR(dl->dol_Name);
        int n;
        if (!vol_fp_add(&vols_seen, dl)) continue;  /* not in a drive */
        if (nicons >= MAX_VOLUMES) continue;
        if (volume_hidden(start, dl)) continue;     /* hidden in Workbench's settings */
        n = b[0] < 63 ? b[0] : 63;
        memcpy(icons[nicons].name, b + 1, (size_t)n);
        icons[nicons].name[n] = 0;
        icons[nicons].kind = drive_kind(start, dl);
        icons[nicons].date = dl->dol_misc.dol_volume.dol_VolumeDate;
        icons[nicons].kb = 0;
        icons[nicons].lo = -1;
        nicons++;
    }
    UnLockDosList(LDF_VOLUMES | LDF_DEVICES | LDF_READ);
    if (ip.order == WB_ORDER_SIZE)                  /* the sizes, only for this order: a lock and Info() each */
        for (i = 0; i < nicons; i++) {
            static struct InfoData id __attribute__((aligned(4)));
            char vp[72];
            size_t n = strlen(icons[i].name);
            BPTR l;
            if (n > 63) n = 63;
            memcpy(vp, icons[i].name, n);
            vp[n] = ':';
            vp[n + 1] = 0;
            if ((l = Lock((CONST_STRPTR)vp, SHARED_LOCK))) {
                if (Info(l, &id)) icons[i].kb = (ULONG)id.id_NumBlocks * (ULONG)(id.id_BytesPerBlock >> 9) / 2;
                UnLock(l);
            }
        }
    qsort(icons, (size_t)nicons, sizeof icons[0], by_order);
    for (i = 0; i < nicons; i++) {
        dicon *ic = &icons[i];
        char dpath[96];
        struct Rectangle r;
        snprintf(ic->path, sizeof ic->path, "%s:", ic->name);
        snprintf(dpath, sizeof dpath, "%s:Disk", ic->name);
        ic->selected = 0;
        ic->dobj = GetIconTags((CONST_STRPTR)dpath, ICONGETA_FailIfUnavailable, TRUE, ICONGETA_Screen, (ULONG)look.scr, TAG_DONE);
        /* 0.1.38: a Disk.info's place only when OpenBench snapshotted it there (ob_snap.h); 3.2.3's are passed over */
        ic->snapped = ic->dobj && ob_snap_honour(&snaps, ic->name, "", OB_SNAP_DESK, ic->dobj->do_CurrentX,
                                                 ic->dobj->do_CurrentY, &ic->sgx, &ic->sgy);
        if (!ic->dobj)                              /* no Disk.info: def_<kind>, or the default disk icon */
            ic->dobj = GetIconTags(NULL, ICONGETA_GetDefaultName, (ULONG)ic->kind, ICONGETA_GetDefaultType, WBDISK,
                                   ICONGETA_Screen, (ULONG)look.scr, TAG_DONE);
        ic->w = 48; ic->h = 40;
        r.MinX = r.MinY = 0;
        if (ic->dobj && GetIconRectangle(root->RPort, ic->dobj, NULL, &r, ICONDRAWA_Borderless, TRUE, TAG_DONE)) {
            ic->w = r.MaxX - r.MinX + 1;
            ic->h = r.MaxY - r.MinY + 1;
        }
        ic->lt = ic->h + LABEL_GAP;
        if (ip.close && ic->dobj) {
            struct Rectangle lr;
            /* the label's box ends at MaxY and is the font's height and a row above and below it */
            if (GetIconRectangle(root->RPort, ic->dobj, (STRPTR)ic->name, &lr, ICONDRAWA_Borderless, TRUE, TAG_DONE) &&
                lr.MaxY - r.MinY - label_h > 0)
                ic->lt = lr.MaxY - r.MinY - label_h;
        }
        ic->x = ic->y = 0;                          /* put in its cell by place_icons */
        ic->ix = -r.MinX;
        ic->iy = -r.MinY;
    }
    leftouts_add();                                 /* 0.1.35: the icons left out, after the disks */
    place_icons(0);
}

static dicon *icon_at(LONG x, LONG y)
{
    int i;
    if (!ob_show_icons(&show)) return NULL;         /* 0.1.37: No icons: the desktop's space only */
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
    struct Segment *resident;               /* 0.1.32: the resident it runs (its code is the resident's), or NULL */
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

/* 0.1.32 (the no-ROM-volume design of 24 October 2026, section 7.3): a tool icon with no file runs the resident
 * of its name, as the Shell runs a resident command. OpenMonitor is the open ROM's, a system resident shell.library
 * makes, and only its icon is on the disk (SYS:Utilities/OpenMonitor.info); "Resident NAME file PURE" makes a user
 * one. Only when the tool's file isn't there (LoadSeg's ERROR_OBJECT_NOT_FOUND): a disk file of that name wins.
 * A user resident is looked for first, then a system one, as the Shell does (FindSegment, under Forbid); a user
 * one's count goes up while it runs, so Resident REMOVE leaves it, and back down at the end (resident_done). The
 * seglist is the resident's: the process gets it with NP_FreeSeglist FALSE, and launch_done never unloads it.
 * The seglist, or 0 (*res the resident). */
static BPTR resident_tool(const char *tool, struct Segment **res)
{
    const char *name = (const char *)FilePart((CONST_STRPTR)tool);
    struct Segment *s;
    BPTR seg = 0;
    *res = NULL;
    if (IoErr() != ERROR_OBJECT_NOT_FOUND || !*name) return 0;
    Forbid();
    if (!(s = FindSegment((CONST_STRPTR)name, NULL, FALSE))) s = FindSegment((CONST_STRPTR)name, NULL, TRUE);
    if (s && s->seg_Seg && s->seg_UC >= CMD_SYSTEM) {      /* not a Shell built-in (CMD_INTERNAL), nor one disabled */
        if (s->seg_UC >= 0) s->seg_UC++;                   /* a user resident: in use while it runs */
        seg = s->seg_Seg;
        *res = s;
    }
    Permit();
    return seg;
}

/* A resident's run has ended: a user resident's count back down. */
static void resident_done(struct Segment *s)
{
    Forbid();
    if (s->seg_UC > 0) s->seg_UC--;
    Permit();
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
    if (!(seg = LoadSeg((CONST_STRPTR)tool)) && !(seg = resident_tool(tool, &L->resident))) {
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
        if (L->resident) resident_done(L->resident);
        else UnLoadSeg(seg);
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
    if (L->resident) resident_done(L->resident);   /* a resident's code stays: it is the resident list's */
    else UnLoadSeg(L->wbs.sm_Segment);
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
    /* SYS:Preferences (0.1.32): OpenPrefs's drawer since the no-ROM-volume design (section 8), before SYS:Prefs */
    static const char *const places[] = { "", "C:", "SYS:Utilities/", "SYS:Tools/", "SYS:System/", "SYS:Preferences/",
                                          "SYS:Prefs/", "SYS:Tools/Commodities/", "SYS:Apps/", NULL };
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

/* ---- the old editors' names: OpenPrefs at their pages (0.1.32) ------------------------------------------------
 *
 * Every editor of ours is a page of OpenPrefs, SYS:Preferences/OpenPrefs, with an icon per page by its plain name
 * (the no-ROM-volume design of 24 October 2026, section 8). OpenUp archives the old names' stubs in SYS:Prefs, and
 * OpenBench no longer has those editors built in (0.1.14 to 0.1.31). A dock button, an icon or a program that
 * still asks for one by its old name (SYS:Prefs/OpenSound, or OpenSound) opens OpenPrefs at that page, as a
 * double-click on the page's icon does: OpenPrefs with SYS:Preferences/<page> as its project, whose SECTION tool
 * type (or, with no icon, its name) picks the page. A file of that name in SYS:Prefs still runs itself. */
static const struct { const char *old, *page; } old_prefs[] = {
    { "Look", "Theme" }, { "OpenLook", "Theme" }, { "Windows", "Windows" }, { "TitleBar", "Title Bar" },
    { "Menus", "Menus" }, { "OpenDock", "Dock" }, { "OpenSound", "Sound" }, { "OpenInput", "Input" },
    { "OpenGamepads", "Gamepads" }, { "OpenDateTime", "Date & Time" }, { "OpenSerial", "Serial" },
    { "OpenTypes", "File Types" }, { "OpenSystem", "System" }, { "OpenScreenMode", "Screen Mode" },
    { "OpenPointer", "Pointer" }, { "OpenFonts", "Fonts" }, { "OpenColours", "Colours" },
    { "OpenBackdrops", "Backdrops" }, { "OpenWorkbench", "Workbench" }, { "OpenFileRequesters", "File Requesters" },
    { "OpenBlanker", "Blanker" }, { "OpenIControl", "IControl" },
};
#define OPENPREFS_APP "SYS:Preferences/OpenPrefs"

static int quiet_exists(const char *path);

/* 1 when path's drawer is SYS:Prefs, however it is spelt (SYS:Prefs, System:Prefs, DH0:Prefs), or path is a bare name */
static int in_sys_prefs(const char *path)
{
    char dir[WB_PATH_MAX];
    size_t k = (size_t)((const char *)PathPart((CONST_STRPTR)path) - path);
    struct Process *me = (struct Process *)FindTask(NULL);
    APTR old;
    BPTR a, b;
    int same = 0;
    if (!strchr(path, ':') && !strchr(path, '/')) return 1;
    if (k >= sizeof dir) return 0;
    memcpy(dir, path, k);
    dir[k] = 0;
    old = me->pr_WindowPtr;
    me->pr_WindowPtr = (APTR)-1;
    if ((a = Lock((CONST_STRPTR)dir, ACCESS_READ))) {
        if ((b = Lock((CONST_STRPTR)"SYS:Prefs", ACCESS_READ))) {
            same = SameLock(a, b) == LOCK_SAME;
            UnLock(b);
        }
        UnLock(a);
    }
    me->pr_WindowPtr = old;
    return same;
}

/* An old editor's name with no file of its own: OpenPrefs started at its page. 1 when started. */
static int old_prefs_open(const char *path)
{
    const char *name = (const char *)FilePart((CONST_STRPTR)path);
    char project[WB_PATH_MAX];
    unsigned i;
    for (i = 0; i < sizeof old_prefs / sizeof old_prefs[0]; i++)
        if (!ci_cmp(name, old_prefs[i].old)) break;
    if (i >= sizeof old_prefs / sizeof old_prefs[0] || !in_sys_prefs(path) || quiet_exists(OPENPREFS_APP) == 0)
        return 0;
    if (strchr(path, ':') && quiet_exists(path)) return 0;     /* its own file, put there: it runs itself */
    snprintf(project, sizeof project, "SYS:Preferences/%s", old_prefs[i].page);
    return start_tool(OPENPREFS_APP, project, 32768);
}

static int ob_open_object(const char *path)
{
    struct DiskObject *o;
    int ok = 0;
    if (old_prefs_open(path)) return 1;            /* an old editor's name: OpenPrefs at its page */
    if (ob_builtin_open(path)) return 1;           /* built into OpenBench (ob_builtin_list.h): ours, never asked about */
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

/* 0.1.44: words held in the screen's title until the next click, as OS 3.2.3 shows an error that asks nothing (a
 * move within a volume onto a name that is taken, from ob_drop). While held, the 2-second tick doesn't put the usual
 * title back (it still makes it, so the next click shows it as it is then). */
static char held_title[sizeof screen_title];
static int title_held;

static const char *title_now(void)
{
    return title_held ? held_title : screen_title;
}

static void say_title(const char *text)
{
    char pad[BARLOGO_MAX_SPACES + 1];
    int i;
    barlogo_pad(look.scr, pad);                     /* the bar logo's room, as make_title leaves it */
    snprintf(held_title, sizeof held_title, "%s%s", pad, text);
    title_held = 1;
    if (root) SetWindowTitles(root, (CONST_STRPTR)~0, (CONST_STRPTR)held_title);
    for (i = 0; i < ndrawers; i++)
        SetWindowTitles(wb_drawer_window(drawers[i]), (CONST_STRPTR)~0, (CONST_STRPTR)held_title);
}

static void title_release(void)
{
    if (!title_held) return;
    title_held = 0;
    if (root) titles_everywhere();
}

/* ---- menus ---------------------------------------------------------------------------------- */

enum { M_SHELL = 1, M_REDRAW, M_UPDATE, M_ABOUT, M_LOADWB, M_QUIT, M_ICON_OPEN, M_ICON_INFO, M_DISK_INSERT, M_DISK_EJECT,
       M_RESIZE, M_CLEANUP, M_PUTAWAY, M_NOICONS, M_SNAP_WINDOW, M_SNAP_ALL, M_SNAP, M_UNSNAP, M_LEAVEOUT };

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
    /* 0.1.37: the desktop's own Show, in the Window menu as OS 3.2.3's Show and the drawers' Show all files are: no
     * icons on the desktop at all (ENV:Sys/DesktopIcons, as OpenPrefs' Icons page sets it); checked by desktop_up */
    { NM_TITLE, (STRPTR)"Window", NULL, 0, 0, NULL },
    /* 0.1.38: OS 3.2.3's Snapshot > Window / All, before Show as there. The desktop is a backdrop with no place or
     * size of its own to keep, so Window is greyed here (the drawers' keeps theirs); All snapshots every icon on it */
    { NM_ITEM, (STRPTR)"Snapshot", NULL, 0, 0, NULL },
    { NM_SUB, (STRPTR)"Window", NULL, NM_ITEMDISABLED, 0, (APTR)M_SNAP_WINDOW },
    { NM_SUB, (STRPTR)"All", NULL, 0, 0, (APTR)M_SNAP_ALL },
    { NM_ITEM, (STRPTR)"No icons", NULL, CHECKIT | MENUTOGGLE, 0, (APTR)M_NOICONS },
    { NM_TITLE, (STRPTR)"Icons", NULL, 0, 0, NULL },
    { NM_ITEM, (STRPTR)"Open", (STRPTR)"O", 0, 0, (APTR)M_ICON_OPEN },
    { NM_ITEM, (STRPTR)"Information...", (STRPTR)"I", 0, 0, (APTR)M_ICON_INFO },
    /* 0.1.38: OS 3.2.3's, after Information...: the selected icons' places kept in their .info files, or cleared */
    { NM_ITEM, (STRPTR)"Snapshot", NULL, 0, 0, (APTR)M_SNAP },
    { NM_ITEM, (STRPTR)"Unsnapshot", NULL, 0, 0, (APTR)M_UNSNAP },
    /* 0.1.41: OS 3.2.3's Leave Out, before Put Away as there: an icon left out for the run, for good (.backdrop) */
    { NM_ITEM, (STRPTR)"Leave Out", (STRPTR)"L", 0, 0, (APTR)M_LEAVEOUT },
    { NM_ITEM, (STRPTR)"Put Away", (STRPTR)"P", 0, 0, (APTR)M_PUTAWAY },    /* 0.1.35: icons left out, back */
    { NM_ITEM, NM_BARLABEL, NULL, 0, 0, NULL },
    { NM_ITEM, (STRPTR)"Clean Up", (STRPTR)".", 0, 0, (APTR)M_CLEANUP },    /* 0.1.33: the desktop's icons flow again */
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

/* ---- the dock: OpenDock, C:OpenDock -----------------------------------------------------------------------
 *
 * The user, 17 Oct 2026: the dock "only shows if there's a preferences file". OpenDock runs as a program of its own,
 * started here by name, so the Shell finds C:OpenDock (OpenUp's; the dock left the ROM with the no-ROM-volume
 * design, 24 October 2026, section 7.4). It is started only when ENV:OpenDock/Dock is there (the
 * Startup-sequence has put ENVARC: in ENV: by now) and none runs (its port, "OpenDock"): with no settings file
 * nothing starts. OpenDock 0.6.6 ends at once when one runs already (an old
 * "Run >NIL: C:OpenDock" in User-Startup), and its editor starts it when its settings are saved. It starts
 * things through OpenBench (obmsg.h) and takes OpenLook's close notice, as it did before it was built in. */

/* The dock's process, or NULL. Under Forbid. */
static struct Task *dock_task(void)
{
    struct MsgPort *p = FindPort((CONST_STRPTR)"OpenDock");
    return p ? p->mp_SigTask : NULL;
}

/* 1 when path is there; no requester while we look (a volume the machine hasn't got, say). */
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
static wb_ghost *drag_img;                  /* 0.1.35: its picture (wb_ghost.h), while the pointer is over another window */
static dicon *drag_skip;                    /* 0.1.35: the icon being let go, not what it is let go on */

/* 0.1.35: the desktop's icon at (sx, sy) on the screen, not the one being let go (ob_drop's env): OB_DESK_* and its
 * path */
static int desk_icon_at(WORD sx, WORD sy, char *out, int n)
{
    int i;
    LONG x = root ? sx - root->LeftEdge : 0, y = root ? sy - root->TopEdge : 0;
    /* 0.1.37: No icons: no icon to drop onto, so the desktop's space (an icon from a drawer is left out there, unseen
     * until the icons come back) */
    if (!root || !ob_show_icons(&show)) return OB_DESK_NONE;
    for (i = nicons - 1; i >= 0; i--) {
        dicon *ic = &icons[i];
        LONG x0, y0, x1, y1;
        if (ic == drag_skip) continue;
        icon_extent(ic, &x0, &y0, &x1, &y1);
        if (x < x0 || x > x1 || y < y0 || y > y1) continue;
        snprintf(out, (size_t)n, "%s", ic->path);
        if (ic->lo < 0) return OB_DESK_DISK;
        if (ic->dobj && ic->dobj->do_Type == WBGARBAGE) return OB_DESK_TRASH;
        if (ic->dobj && (ic->dobj->do_Type == WBDRAWER || ic->dobj->do_Type == WBDISK)) return OB_DESK_DRAWER;
        return OB_DESK_FILE;
    }
    return OB_DESK_NONE;
}

static void place_icons(int clean);
static void drop_on_desktop(dicon *ic);

/* 0.1.35: icons from drawers let go on the desktop's space (ob_drop's env): left out for the run where they were
 * let go, side by side, and gone from their drawers (read again once this round of messages is done) */
static int leave_out(const char *const *paths, int n, WORD sx, WORD sy)
{
    static char added_path[MAX_LEFTOUT][256];       /* the ones left out now, to put where they were let go */
    int i, k, added = 0;
    for (i = 0; i < n; i++) {
        char full[256];
        const char *c;
        /* kept as the disk names it, as .backdrop names them, whatever assign the drawer was opened through */
        if (!full_name(paths[i], full, sizeof full) || !(c = strchr(full, ':')) || !c[1]) continue;     /* a disk */
        if (lout_find(full) >= 0 || nlouts == MAX_LEFTOUT) continue;
        snprintf(louts[nlouts].path, sizeof louts[0].path, "%s", full);
        louts[nlouts++].permanent = 0;
        memcpy(added_path[added++], full, sizeof full);
    }
    if (!added || !root) return added > 0;
    scan_volumes();
    for (i = 0, k = 0; i < nicons; i++) {
        dicon *ic = &icons[i];
        LONG nx, ny;
        int j, mine = 0;
        if (ic->lo < 0) continue;
        for (j = 0; j < added && !mine; j++) mine = !ci_cmp(ic->path, added_path[j]);
        if (!mine) continue;
        nx = sx - root->LeftEdge - ic->w / 2 + k++ * (ic->w + 16);
        ny = sy - root->TopEdge - ic->h / 2;
        ic->ix += nx - ic->x;
        ic->iy += ny - ic->y;
        ic->x = nx;
        ic->y = ny;
        drop_on_desktop(ic);                        /* its place the user's, on the grid, the others round it */
    }
    redraw_all();
    reread_pending = 1;
    return 1;
}

/* 0.1.35: a job has ended (ob_drop's env): an icon left out whose file has gone (moved into a drawer, to the trash)
 * goes from the desktop, and from its disk's .backdrop */
static void leftouts_check(void)
{
    int i, gone = 0;
    for (i = 0; i < nlouts; i++) {
        BPTR l = Lock((CONST_STRPTR)louts[i].path, ACCESS_READ);
        if (l) {
            UnLock(l);
            continue;
        }
        if (IoErr() != ERROR_OBJECT_NOT_FOUND) continue;     /* its disk is out, say: it comes back with it */
        if (louts[i].permanent) backdrop_change(louts[i].path, 0);
        lout_drop(i--);
        gone = 1;
    }
    if (gone && root) {
        scan_volumes();
        redraw_all();
        reread_pending = 1;
    }
}

/* ---- Snapshot (0.1.38; Salvor's OS list, item 53) ----------------------------------------------------------------
 * As OS 3.2.3's: Icons > Snapshot writes the selected icons' places into their .info files, Icons > Unsnapshot
 * writes NO_ICON_POSITION there, Window > Snapshot > All does every icon in the window (and in a drawer, > Window
 * its window's place, opengadtools wb_drawer.c). OpenBench keeps only the places it wrote itself, as its record
 * (ob_snap.h) lists them; a .info's place it didn't write (3.2.3's, made for its small icons) goes on the grid. */

/* The record from ENVARC:OpenBench/Snapshots, read once when OpenBench starts (no requester while it looks) */
static void snaps_load(void)
{
    struct Process *me = (struct Process *)FindTask(NULL);
    APTR was = me->pr_WindowPtr;
    BPTR f;
    LONG n;
    char *buf;
    ob_snap_init(&snaps);
    me->pr_WindowPtr = (APTR)-1;
    if ((f = Open((CONST_STRPTR)SNAPS_FILE, MODE_OLDFILE))) {
        if (Seek(f, 0, OFFSET_END) >= 0 && (n = Seek(f, 0, OFFSET_BEGINNING)) > 0 && n < 2097152 &&
            (buf = AllocVec((ULONG)n + 1, MEMF_ANY))) {
            n = Read(f, buf, n);
            if (n > 0) ob_snap_parse(&snaps, buf, n);
            FreeVec(buf);
        }
        Close(f);
    }
    me->pr_WindowPtr = was;
}

/* The record written whole into ENVARC:OpenBench/Snapshots (the drawer made when it isn't there): 0, or DOS's error */
static LONG snaps_save(void)
{
    struct Process *me = (struct Process *)FindTask(NULL);
    APTR was = me->pr_WindowPtr;
    long need = ob_snap_text(&snaps, NULL, 0);
    char *buf = AllocVec((ULONG)need + 1, MEMF_ANY);
    LONG err = 0;
    BPTR f, l;
    if (!buf) return ERROR_NO_FREE_STORE;
    ob_snap_text(&snaps, buf, need + 1);
    me->pr_WindowPtr = (APTR)-1;
    if ((l = Lock((CONST_STRPTR)SNAPS_DIR, ACCESS_READ)) || (l = CreateDir((CONST_STRPTR)SNAPS_DIR))) UnLock(l);
    if (!(f = Open((CONST_STRPTR)SNAPS_FILE, MODE_NEWFILE)))
        err = IoErr() ? IoErr() : ERROR_OBJECT_NOT_FOUND;
    else {
        if (Write(f, buf, (LONG)need) != (LONG)need) err = IoErr() ? IoErr() : ERROR_DISK_FULL;
        if (!Close(f) && !err) err = IoErr();
    }
    me->pr_WindowPtr = was;
    FreeVec(buf);
    return err;
}

/* One icon's place written into its .info (icon: its file without ".info"; a disk's is "Work:Disk"), as 3.2.3's
 * Snapshot writes it: in a .info that is there, only the place changes (icon.library's ICONPUTA_OnlyUpdatePosition:
 * the rest of the file is as it was); an icon with none of its own gets one, the default for it (a disk's for its
 * kind of drive, def_<kind>), at that place. Unsnapshot (x NO_ICON_POSITION) writes nothing where there is no .info.
 * No DOS requester while it writes: OpenBench's names the icon. 0, or the DOS error. */
static LONG snap_write(const char *icon, LONG x, LONG y, const char *defname)
{
    struct Process *me = (struct Process *)FindTask(NULL);
    APTR was = me->pr_WindowPtr;
    struct DiskObject *o;
    LONG err = 0;
    me->pr_WindowPtr = (APTR)-1;
    o = GetIconTags((CONST_STRPTR)icon, ICONGETA_FailIfUnavailable, TRUE, ICONA_ErrorCode, (ULONG)&err, TAG_DONE);
    if (o) {
        o->do_CurrentX = x;
        o->do_CurrentY = y;
        err = 0;
        if (!PutIconTags((CONST_STRPTR)icon, o, ICONPUTA_OnlyUpdatePosition, TRUE, ICONA_ErrorCode, (ULONG)&err, TAG_DONE)
            && !err)
            err = IoErr() ? IoErr() : ERROR_OBJECT_WRONG_TYPE;
        FreeDiskObject(o);
    } else if (!err || err == ERROR_OBJECT_NOT_FOUND) {     /* no .info of its own */
        err = 0;
        if (x != (LONG)NO_ICON_POSITION) {
            o = defname ? GetIconTags(NULL, ICONGETA_GetDefaultName, (ULONG)defname, ICONGETA_GetDefaultType, WBDISK, TAG_DONE)
                        : GetIconTags((CONST_STRPTR)icon, ICONGETA_FailIfUnavailable, FALSE, TAG_DONE);
            if (!o)
                err = IoErr() ? IoErr() : ERROR_OBJECT_NOT_FOUND;
            else {
                o->do_CurrentX = x;
                o->do_CurrentY = y;
                if (!PutIconTags((CONST_STRPTR)icon, o, ICONA_ErrorCode, (ULONG)&err, TAG_DONE) && !err)
                    err = IoErr() ? IoErr() : ERROR_OBJECT_WRONG_TYPE;
                FreeDiskObject(o);
            }
        }
    }
    me->pr_WindowPtr = was;
    return err;
}

/* What a Snapshot, an Unsnapshot or a Clean Up couldn't write, for one requester: each icon with DOS's reason */
typedef struct snapfail {
    char text[640];
    int bad, good;
} snapfail;

static void snapfail_add(snapfail *f, const char *name, LONG err, const char *why)
{
    char reason[100], line[180];
    if (++f->bad > 8) return;                       /* "and more" */
    if (!why) {
        if (!Fault(err, NULL, (STRPTR)reason, sizeof reason)) snprintf(reason, sizeof reason, "DOS error %ld", (long)err);
        why = reason;
    }
    snprintf(line, sizeof line, "  %s: %s\n", name, why);
    if (strlen(f->text) + strlen(line) < sizeof f->text) strcat(f->text, line);
}

/* The record written when it changed; the requester when anything couldn't be (on window w, else the desktop's) */
static void snapfail_end(snapfail *f, int changed, const char *what, struct Window *w)
{
    struct EasyStruct es = { sizeof(struct EasyStruct), 0, (UBYTE *)"OpenBench", (UBYTE *)"%s", (UBYTE *)"OK" };
    char text[800];
    ULONG args[1];
    LONG err;
    if (changed && (err = snaps_save())) snapfail_add(f, SNAPS_FILE, err, NULL);
    if (!f->bad) return;
    snprintf(text, sizeof text, "%s couldn't write %s:\n%s%s%s", what, f->bad == 1 ? "this" : "these", f->text,
             f->bad > 8 ? "  and more\n" : "", f->good ? "\nThe others were written." : "");
    args[0] = (ULONG)text;
    EasyRequestArgs(w ? w : root, &es, NULL, args);
}

/* Icons > Snapshot and Unsnapshot (unsnap) on the desktop, and Window > Snapshot > All (all: every icon on it, else
 * the selected ones). The place written is where icon.library draws the icon in the root window (DrawIconState's,
 * as do_CurrentX and do_CurrentY are on 3.2.3's desktop), and the record keeps its cell. Nothing moves: a snapshotted
 * icon is on its cell already, and one unsnapshotted stays there for this run, as on 3.2.3 (it goes on the grid
 * from the next start, or at Clean Up). */
static void desk_snapshot(int all, int unsnap)
{
    snapfail f;
    int i, changed = 0;
    memset(&f, 0, sizeof f);
    for (i = 0; i < nicons; i++) {
        dicon *ic = &icons[i];
        char vol[OB_SNAP_VOL], rest[OB_SNAP_PATH], file[300];
        LONG err;
        int r;
        if (!all && !ic->selected) continue;
        if (!ob_snap_split(ic->path, vol, sizeof vol, rest, sizeof rest)) {
            snapfail_add(&f, ic->name, 0, "its name is too long to be kept");
            continue;
        }
        /* a disk's Disk.info, or an icon left out: its own; a name too long for the buffer is said, not cut */
        r = ic->lo < 0 ? snprintf(file, sizeof file, "%s:Disk", ic->name) : snprintf(file, sizeof file, "%s", ic->path);
        if (r < 0 || r >= (int)sizeof file) {
            snapfail_add(&f, ic->name, 0, "its name is too long to be kept");
            continue;
        }
        if (unsnap) {
            err = snap_write(file, (LONG)NO_ICON_POSITION, (LONG)NO_ICON_POSITION, NULL);
            changed |= ob_snap_forget(&snaps, vol, rest);
            if (ic->snapped && !place_of(place_key(ic))) {
                dplace *p = place_for(place_key(ic));   /* where it is, for this run */
                p->x = (WORD)gcells[i].x;
                p->y = (WORD)gcells[i].y;
            }
            ic->snapped = 0;
            if (err) snapfail_add(&f, ic->name, err, NULL);
            else f.good++;
            continue;
        }
        if ((err = snap_write(file, ic->ix, ic->iy, ic->lo < 0 ? ic->kind : NULL))) {
            snapfail_add(&f, ic->name, err, NULL);
            continue;
        }
        if ((r = ob_snap_set(&snaps, vol, rest, OB_SNAP_DESK, ic->ix, ic->iy, gcells[i].x, gcells[i].y)) < 0) {
            snapfail_add(&f, ic->name, 0, "OpenBench's record of snapshots is full");
            continue;
        }
        changed |= r > 0;
        ic->snapped = 1;
        ic->sgx = gcells[i].x;
        ic->sgy = gcells[i].y;
        f.good++;
    }
    snapfail_end(&f, changed, unsnap ? "Unsnapshot" : "Snapshot", NULL);
}

/* Icons > Clean Up: every desktop icon goes on the grid, and their snapshots out of the record, so the icons stay
 * where Clean Up puts them (Snapshot keeps those places); their .info files aren't written, as 3.2.3's Clean Up
 * writes none */
static void cleanup_snaps(void)
{
    const char *paths[MAX_DESK];
    snapfail f;
    int i;
    for (i = 0; i < nicons; i++) {
        paths[i] = icons[i].path;
        icons[i].snapped = 0;
    }
    memset(&f, 0, sizeof f);
    snapfail_end(&f, ob_snap_cleanup(&snaps, paths, nicons) > 0, "Clean Up", NULL);
}

/* "Work" + "Tools" + "OpenMeters": the path on the volume of an entry in a drawer; 0 when too long */
static int snap_join(char *out, int n, const char *dir, const char *name)
{
    return snprintf(out, (size_t)n, "%s%s%s", dir, dir[0] ? "/" : "", name) < n;
}

/* wb_look's snapped: the record's place for an icon in a drawer's window. The drawer's path as its disk names it
 * ("SYS:Tools" is "System:Tools") is kept for the next entry, and a drawer the record has none in is answered at
 * once; with no snapshots at all nothing is looked up. */
static char sn_dir[256], sn_vol[OB_SNAP_VOL], sn_rest[OB_SNAP_PATH];
static unsigned long sn_gen;
static int sn_any = -1;

static int drawer_snapped(const char *dir, const char *name, int *gx, int *gy, LONG *x, LONG *y)
{
    char path[OB_SNAP_PATH];
    const ob_snap_ent *e;
    if (!snaps.n) return 0;
    if (sn_any < 0 || sn_gen != snaps.gen || strcmp(dir, sn_dir)) {
        char full[256];
        snprintf(sn_dir, sizeof sn_dir, "%s", dir);
        sn_gen = snaps.gen;
        if (!full_name(dir, full, sizeof full)) snprintf(full, sizeof full, "%s", dir);
        sn_any = ob_snap_split(full, sn_vol, sizeof sn_vol, sn_rest, sizeof sn_rest) &&
                 ob_snap_count_in(&snaps, sn_vol, sn_rest, OB_SNAP_DRAWER) > 0;
    }
    if (!sn_any || !snap_join(path, sizeof path, sn_rest, name) || !(e = ob_snap_find(&snaps, sn_vol, path)) ||
        e->where != OB_SNAP_DRAWER)
        return 0;
    *gx = e->gx;
    *gy = e->gy;
    *x = (LONG)e->x;
    *y = (LONG)e->y;
    return 1;
}

/* wb_look's snap: Icons > Snapshot and Unsnapshot in a drawer, and Window > Snapshot > All's icons */
static int drawer_snap(const char *dir, wb_snapitem *items, int n, int unsnap, wb_drawer *from)
{
    snapfail f;
    char full[256], vol[OB_SNAP_VOL], rest[OB_SNAP_PATH];
    int i, changed = 0, have;
    size_t dl = strlen(dir);
    memset(&f, 0, sizeof f);
    if (!full_name(dir, full, sizeof full)) snprintf(full, sizeof full, "%s", dir);
    have = ob_snap_split(full, vol, sizeof vol, rest, sizeof rest);
    for (i = 0; i < n; i++) {
        char file[WB_PATH_MAX], path[OB_SNAP_PATH];
        int keyed = have && snap_join(path, sizeof path, rest, items[i].name), r;
        LONG err;
        items[i].done = 0;
        if (snprintf(file, sizeof file, "%s%s%s", dir, dl && (dir[dl - 1] == ':' || dir[dl - 1] == '/') ? "" : "/",
                     items[i].name) >= (int)sizeof file || (!keyed && !unsnap)) {
            snapfail_add(&f, items[i].name, 0, "its name is too long to be kept");
            continue;
        }
        if (unsnap) {
            err = snap_write(file, (LONG)NO_ICON_POSITION, (LONG)NO_ICON_POSITION, NULL);
            if (keyed) changed |= ob_snap_forget(&snaps, vol, path);
        } else if (!(err = snap_write(file, items[i].x, items[i].y, NULL))) {
            items[i].done = 1;
            if ((r = ob_snap_set(&snaps, vol, path, OB_SNAP_DRAWER, items[i].x, items[i].y, items[i].gx, items[i].gy)) < 0) {
                snapfail_add(&f, items[i].name, 0, "OpenBench's record of snapshots is full");
                continue;
            }
            changed |= r > 0;
        }
        if (err) snapfail_add(&f, items[i].name, err, NULL);
        else {
            items[i].done = 1;
            f.good++;
        }
    }
    snapfail_end(&f, changed, unsnap ? "Unsnapshot" : "Snapshot", wb_drawer_window(from));
    return !f.bad;
}

/* ---- Leave Out and Put Away (0.1.41) -----------------------------------------------------------------------------
 * As OS 3.2.3's: Icons > Leave Out in a drawer puts the selected icons on the desktop for good, each one's line
 * written into its disk's .backdrop (ob_backdrop.h), so they are left out at every start, and gone from their drawer;
 * Icons > Leave Out on the desktop does the same for an icon left out there for the run (let go of on the desktop's
 * space, 0.1.35); Icons > Put Away sends icons left out back to their drawers, their lines taken out of .backdrop. A
 * disk is on the desktop already, and can't be left out (a beep, as for a drawer window with nothing selected). What
 * couldn't be written (a write-protected disk) is named in a requester with DOS's reason, and that icon stays as it
 * was; the rest is still done. Icons left out are desktop icons like the disks: Snapshot keeps their places
 * (ob_snap.h, "D"), and Clean Up lays them out. */

/* The desktop's icons read again, and the drawers once this round of messages is done (their icons hidden or back) */
static void leftouts_changed(void)
{
    scan_volumes();
    redraw_all();
    reread_pending = 1;
}

/* wb_look's leave_out: Icons > Leave Out in a drawer (dir), for its selected entries (names). How many were left out. */
static int drawer_leave_out(const char *dir, const char *const *names, int n, wb_drawer *from)
{
    snapfail f;
    int i, done = 0;
    size_t dl = strlen(dir);
    memset(&f, 0, sizeof f);
    for (i = 0; i < n; i++) {
        char file[WB_PATH_MAX], full[256];
        const char *c;
        LONG err;
        int k;
        if (snprintf(file, sizeof file, "%s%s%s", dir, dl && (dir[dl - 1] == ':' || dir[dl - 1] == '/') ? "" : "/",
                     names[i]) >= (int)sizeof file) {
            snapfail_add(&f, names[i], 0, "its name is too long to be kept");
            continue;
        }
        /* kept as its disk names it ("SYS:Tools" is "System:Tools"), as .backdrop and the desktop name it */
        if (!full_name(file, full, sizeof full)) {
            snapfail_add(&f, names[i], IoErr() ? IoErr() : ERROR_OBJECT_NOT_FOUND, NULL);
            continue;
        }
        if (!(c = strchr(full, ':')) || !c[1]) continue;    /* a disk: on the desktop already */
        if ((k = lout_find(full)) < 0 && nlouts == MAX_LEFTOUT) {
            snapfail_add(&f, names[i], 0, "OpenBench has no room for more icons left out");
            continue;
        }
        if ((err = backdrop_change(full, 1))) {
            snapfail_add(&f, names[i], err, NULL);
            continue;
        }
        if (k < 0) {
            k = nlouts++;
            snprintf(louts[k].path, sizeof louts[0].path, "%s", full);
        }
        louts[k].permanent = 1;
        done++;
        f.good++;
    }
    if (done && root) leftouts_changed();
    if (!done && !f.bad) DisplayBeep(look.scr);
    snapfail_end(&f, 0, "Leave Out", wb_drawer_window(from));
    return done;
}

/* Icons > Leave Out on the desktop: the selected icons left out for the run, left out for good. A disk, or an icon
 * already in its disk's .backdrop, has nothing to leave out: a beep when no icon had. */
static void desk_leave_out(void)
{
    snapfail f;
    int i, done = 0;
    memset(&f, 0, sizeof f);
    for (i = 0; i < nicons; i++) {
        dicon *ic = &icons[i];
        LONG err;
        int k;
        if (!ic->selected || ic->lo < 0 || (k = lout_find(ic->path)) < 0 || louts[k].permanent) continue;
        if ((err = backdrop_change(ic->path, 1))) {
            snapfail_add(&f, ic->name, err, NULL);
            continue;
        }
        louts[k].permanent = 1;
        done++;
        f.good++;
    }
    if (!done && !f.bad) DisplayBeep(look.scr);
    snapfail_end(&f, 0, "Leave Out", NULL);
}

/* 0.1.35: Icons > Put Away: the selected icons left out go back to their drawers, and (0.1.41) out of .backdrop, or
 * stay, named, when their line couldn't be taken out */
static void put_away(void)
{
    snapfail f;
    int i, any = 0;
    memset(&f, 0, sizeof f);
    for (i = 0; i < nicons; i++) {
        LONG err;
        int k;
        if (!icons[i].selected || icons[i].lo < 0 || (k = lout_find(icons[i].path)) < 0) continue;
        if (louts[k].permanent && (err = backdrop_change(louts[k].path, 0))) {
            snapfail_add(&f, icons[i].name, err, NULL);
            continue;
        }
        lout_drop(k);
        any = 1;
        f.good++;
    }
    if (any) leftouts_changed();
    snapfail_end(&f, 0, "Put Away", NULL);
}

static const ob_drop_env drop_env = { &look.scr, &root, drawers, &ndrawers, desk_icon_at, leave_out, leftouts_check,
                                       say_title };

static void drag_img_close(void)
{
    wb_ghost_free(drag_img);
    drag_img = NULL;
}

/* The dragged icon's picture at (sx, sy) on the screen, above the windows, as a drawer's icons are dragged
 * (opengadtools wb_drawer.c's drag_start): it is made the first time the pointer leaves the desktop for another
 * window. 0.1.35: the icon itself, with its alpha (wb_ghost.h); before, a window with the icon on its own
 * background */
static void drag_img_at(dicon *ic, LONG sx, LONG sy)
{
    if (!drag_img) {
        if (!ic->dobj || !look.scr || !(drag_img = wb_ghost_icon(look.scr, ic->dobj, dri))) return;
    }
    wb_ghost_move(drag_img, (int)sx, (int)sy);
}

/* 0.1.35: the selected desktop icons (and the one dragged) let go over another window, or onto a desktop icon:
 * each disk's contents copied (ob_drop_volume), the icons left out going as from a drawer (ob_drop) */
static void drop_selected(dicon *dragged, WORD sx, WORD sy, UWORD qual)
{
    const char *paths[MAX_LEFTOUT];
    int i, n = 0;
    for (i = 0; i < nicons; i++) {
        dicon *ic = &icons[i];
        if (!ic->selected && ic != dragged) continue;
        if (ic->lo < 0) ob_drop_volume(ic->path, sx, sy, qual);
        else if (n < MAX_LEFTOUT) paths[n++] = ic->path;
    }
    if (n) ob_drop(paths, n, sx, sy, qual, NULL);
}
static ULONG last_secs, last_micros;
static dicon *last_click;
static int quit_to_wb;

/* ---- no icons on the desktop (0.1.37) ---------------------------------------------------------------------------- *
 * ENV:Sys/DesktopIcons (openamigaprefs Common/deskicons.h): "none", no icons on the desktop; "show" or no variable, the
 * icons. Set by OpenPrefs' Icons page, by Window > No icons and by Icons > Clean Up (off), and followed at once. */
static struct Menu *menus;
static struct NotifyRequest dic_nr;
static int dic_notify;

/* 1 when ENV:Sys/DesktopIcons says no icons */
static int deskicons_none(void)
{
    char buf[16];
    LONG n = GetVar((CONST_STRPTR)DESKICONS_VAR, (STRPTR)buf, sizeof buf, GVF_GLOBAL_ONLY);
    return deskicons_parse(n >= 0 ? buf : NULL, n >= 0 ? (long)n : -1L) == DESKICONS_NONE;
}

/* The setting into ENV: and ENVARC:, as the Icons page's Save writes it (SetVar, a few tries); a beep when it can't be */
static void deskicons_write(int none)
{
    int i;
    for (i = 0; i < 5; i++) {
        if (SetVar((CONST_STRPTR)DESKICONS_VAR, (CONST_STRPTR)deskicons_text(none ? DESKICONS_NONE : DESKICONS_SHOW), -1,
                   GVF_GLOBAL_ONLY | GVF_SAVE_VAR))
            return;
        Delay(10);
    }
    if (look.scr) DisplayBeep(look.scr);
}

/* The root window's menu item with this id (GadTools' user data), or NULL; not a bar */
static struct MenuItem *menu_item_of(ULONG id)
{
    struct Menu *m;
    struct MenuItem *it;
    for (m = menus; m; m = m->NextMenu)
        for (it = m->FirstItem; it; it = it->NextItem)
            if ((it->Flags & ITEMTEXT) && (ULONG)GTMENUITEM_USERDATA(it) == id) return it;
    return NULL;
}

/* Window > No icons' checkmark as the setting is (the Icons page, or Clean Up, changed it) */
static void noicons_check(void)
{
    struct MenuItem *it;
    if (!root || !menus || !(it = menu_item_of(M_NOICONS))) return;
    if (!(it->Flags & CHECKED) == !ob_show_checked(&show)) return;
    ClearMenuStrip(root);
    if (ob_show_checked(&show)) it->Flags |= CHECKED;
    else it->Flags &= ~CHECKED;
    ResetMenuStrip(root, menus);
}

/* What a change of the setting asks for (ob_show.h). Hidden, no icon stays selected, pressed or dragged (one being
 * dragged goes back where the drag began), and the desktop is cleared to its backdrop; back, they are drawn where they
 * were. Their places (places[], the grid) are never touched. */
static void noicons_apply(int what)
{
    int i;
    if (what == OB_SHOW_HIDE) {
        drag_img_close();
        if (dragging) {
            dragging->ix += drag_ox - dragging->x;
            dragging->iy += drag_oy - dragging->y;
            dragging->x = drag_ox;
            dragging->y = drag_oy;
        }
        dragging = pressed_on = last_click = NULL;
        for (i = 0; i < nicons; i++) icons[i].selected = 0;
    }
    noicons_check();
    if (what != OB_SHOW_KEEP && root) {
        EraseRect(root->RPort, 0, 0, root->Width - 1, root->Height - 1);
        draw_icons();
    }
}

/* The labels' notify (0.1.37: and ENV:Sys/DesktopIcons'; 0.1.39: messages, OB_DO_ICONS): 1 when ENV:Sys/IconLabels or
 * IconLayout changed since it last looked, as read_iconprefs reads them */
static ob_seen seen;
static int iconprefs_moved(void)
{
    char a[40], b[120];
    LONG n = GetVar((CONST_STRPTR)"Sys/IconLabels", (STRPTR)a, sizeof a, GVF_GLOBAL_ONLY);
    LONG m = GetVar((CONST_STRPTR)"Sys/IconLayout", (STRPTR)b, sizeof b, GVF_GLOBAL_ONLY);
    return ob_seen_changed(&seen, n >= 0 ? a : NULL, m >= 0 ? b : NULL);
}

/* A disk came or went: Intuition's disk event, or (0.1.40) the 2-second tick saw the volumes change. The volumes read
 * and drawn again; a new one's icon placed as at the start, a gone one's gone. */
static void volumes_again(void)
{
    drag_img_close();                           /* 0.1.35: a drag ends with the icons it was of */
    dragging = pressed_on = last_click = NULL;
    scan_volumes();
    EraseRect(root->RPort, 0, 0, root->Width - 1, root->Height - 1);
    draw_icons();
}

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
            if (ic && (qual & IEQUALIFIER_LALT)) {
                /* 0.1.35: Alt held at the press: the icon taken out of the selection, and no drag (OS 3.2.3's,
                 * measured on lab t4-dnd) */
                last_click = NULL;
                pressed_on = NULL;
                if (ic->selected) {
                    ic->selected = 0;
                    redraw_around(ic);
                }
                break;
            }
            if (ic && ic == last_click && DoubleClick(last_secs, last_micros, secs, micros)) {
                last_click = NULL;
                pressed_on = NULL;
                if (ic->lo >= 0) ob_open_object(ic->path);  /* 0.1.35: left out: what it stands for */
                else open_drawer(ic->path);
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
                 * the disk's icon goes back where it was. 0.1.35: every selected icon goes, the icons left out
                 * as from a drawer; and onto a disk, a drawer or the trash on the desktop the same way */
                WORD sx = (WORD)(root->LeftEdge + mx), sy = (WORD)(root->TopEdge + my);
                struct Window *w;
                char on[256];
                int k;
                dicon *dr = dragging;
                drag_img_close();
                dragging = NULL;
                w = ob_drop_window_at(sx, sy);
                drag_skip = dr;
                k = w == root ? desk_icon_at(sx, sy, on, sizeof on) : OB_DESK_NONE;
                if ((w && w != root) || k == OB_DESK_DISK || k == OB_DESK_DRAWER || k == OB_DESK_TRASH) {
                    if (dr->x != drag_ox || dr->y != drag_oy) move_icon(dr, drag_ox, drag_oy);
                    drop_selected(dr, sx, sy, qual);
                } else
                    drop_on_desktop(dr);            /* 0.1.34: the user's place, on the grid */
                drag_skip = NULL;
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
            w = ob_drop_window_under((WORD)(root->LeftEdge + mx), (WORD)(root->TopEdge + my), wb_ghost_window(drag_img));
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
        volumes_again();
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
                        if (id == M_ICON_OPEN && icons[i].lo >= 0) ob_open_object(icons[i].path);
                        else if (id == M_ICON_OPEN) open_drawer(icons[i].path);
                        else if (!ob_info_open(look.scr, icons[i].path)) DisplayBeep(look.scr);
                    }
                break;
            }
            case M_REDRAW: redraw_all(); break;
            case M_PUTAWAY: put_away(); break;      /* 0.1.35 */
            case M_LEAVEOUT: desk_leave_out(); break;       /* 0.1.41 */
            case M_SNAP: desk_snapshot(0, 0); break;        /* 0.1.38 */
            case M_UNSNAP: desk_snapshot(0, 1); break;
            case M_SNAP_ALL: desk_snapshot(1, 0); break;
            case M_CLEANUP: {
                /* 0.1.37: with No icons on, Clean Up turns it off (written as the Icons page's Save), then lays out */
                int w;
                if (ob_show_cleanup(&show, &w) != OB_SHOW_KEEP) {
                    noicons_check();
                    deskicons_write(w);
                }
                cleanup_snaps();                    /* 0.1.38: their snapshots go: Snapshot keeps the new places */
                place_icons(1);
                redraw_all();
                break;
            }
            case M_NOICONS: {                       /* 0.1.37: Window > No icons, its checkmark as Intuition left it */
                int w;
                noicons_apply(ob_show_menu(&show, (it->Flags & CHECKED) != 0, &w));
                deskicons_write(w);
                break;
            }
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
    picture_ticks = 0;
    if (!(picture_proc = CreateNewProcTags(NP_Entry, (ULONG)picture_entry, NP_StackSize, 16384,
                                           NP_Name, (ULONG)"OpenBench backdrop", NP_Priority, -1, TAG_DONE)))
        picture_busy = 0;
}

/* The pictures behind the desktop given up: the decoded one, the datatype's (whose file it keeps open), the
 * gradient. With the root window's layer locked, as its backfill reads them from whichever task draws. */
static void drop_picture(void)
{
    ULONG *old;
    Object *o;
    picture_hurry();                                /* 0.1.34: not held back while we wait for it */
    while (picture_busy) Delay(5);                  /* the picture's process is in our code */
    if (root) LockLayer(0, root->WLayer);
    if (strip) LockLayer(0, strip->WLayer);
    old = backdrop;
    backdrop = NULL;
    o = pic_obj;
    pic_bm = NULL;
    pic_obj = NULL;
    if (strip) UnlockLayer(strip->WLayer);
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
    read_tone();                                /* 0.1.34: the labels' colour from the new picture's .tone */
    grad = gradient_backdrop(bd_w, bd_h);
    LockLayer(0, root->WLayer);
    if (strip) LockLayer(0, strip->WLayer);
    backdrop = grad;
    if (strip) UnlockLayer(strip->WLayer);
    UnlockLayer(root->WLayer);
    EraseRect(root->RPort, 0, 0, root->Width - 1, root->Height - 1);
    strip_redraw();
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
    strip_close();
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
    if (look.label_font) { CloseFont(look.label_font); look.label_font = NULL; }
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
    ob_box rb;
    struct NewMenu nm[sizeof newmenus / sizeof newmenus[0]];
    int i, k;
    if (!look.scr && !(look.scr = LockPubScreen((CONST_STRPTR)"Workbench"))) return 0;
    load_theme(&mode);
    if (!ogt_ctx_init(&ctx, look.scr, &theme, mode) && !ogt_ctx_init(&ctx, look.scr, &theme, OGT_LIGHT)) return 0;
    look.ctx = &ctx;
    look.font = ogt_open_font(&theme, look.scr, &ta);
    look.ta = &ta;
    /* 0.1.31: the labels, on the desktop and in the drawers, in Font prefs' Workbench icon text font when it sets
     * one (the user's solid OS list, 24 October 2026, as OpenMeters 0.2.5 draws in it) */
    label_set = ogt_icon_font_attr(&label_asked, label_asked_name, sizeof label_asked_name);
    look.label_font = label_set ? ogt_open_icon_font(&label_ta, label_name, sizeof label_name) : NULL;
    make_title();

    /* the backdrop: the theme's gradient now, the picture when it is decoded */
    bd_w = look.scr->Width;
    bd_h = look.scr->Height;
    backdrop = gradient_backdrop(bd_w, bd_h);
    backfill_hook.h_Entry = (ULONG (*)())bf_entry;
    /* 0.1.22: a Workbench window (WFLG_WBENCHWINDOW, first: the other tags add to it), as all of OpenBench's
     * are (mark_ours), so CloseWorkBench() asks OpenBench to close it instead of refusing at once */
    rb = ob_root_box(look.scr->Width, look.scr->Height, look.scr->BarHeight);
    root = OpenWindowTags(NULL, WA_Flags, WFLG_WBENCHWINDOW, WA_Left, rb.left, WA_Top, rb.top, WA_Width, rb.width,
                          WA_Height, rb.height, WA_PubScreen, (ULONG)look.scr,
                          WA_Backdrop, TRUE, WA_Borderless, TRUE, WA_SimpleRefresh, TRUE, WA_BackFill, (ULONG)&backfill_hook,
                          WA_ReportMouse, TRUE, WA_NewLookMenus, TRUE, WA_Activate, TRUE,
                          WA_ScreenTitle, (ULONG)screen_title, WA_IDCMP, 0, TAG_DONE);
    if (!root) return 0;
    root->UserPort = idcmp;
    ModifyIDCMP(root, IDCMP_MOUSEBUTTONS | IDCMP_MOUSEMOVE | IDCMP_REFRESHWINDOW | IDCMP_MENUPICK |
                      IDCMP_DISKINSERTED | IDCMP_DISKREMOVED);
    if (look.label_font) SetFont(root->RPort, look.label_font);    /* the root window draws only the labels */
    else if (look.font) SetFont(root->RPort, look.font);
    SetDrMd(root->RPort, JAM1);
    label_h = root->RPort->TxHeight;
    dri = GetScreenDrawInfo(look.scr);
    /* 0.1.37: No icons (ENV:Sys/DesktopIcons), before the menus (its checkmark) and the icons */
    ob_show_init(&show, deskicons_none());
    /* the menus; Resize to Window only on a Native mode, greyed while the size is already the one asked for */
    resize_shown = native_monitor(NULL) > 0;
    for (i = k = 0; i < (int)(sizeof newmenus / sizeof newmenus[0]); i++) {
        if (i == RESIZE_INDEX && !resize_shown) continue;
        nm[k] = newmenus[i];
        if (i == RESIZE_INDEX && !native_differs()) nm[k].nm_Flags |= NM_ITEMDISABLED;
        if (nm[k].nm_UserData == (APTR)M_NOICONS && ob_show_checked(&show)) nm[k].nm_Flags |= CHECKED;
        k++;
    }
    resize_greyed = resize_shown && !native_differs();
    if ((vi = GetVisualInfo(look.scr, TAG_DONE)) && (menus = CreateMenus(nm, TAG_DONE)) &&
        LayoutMenus(menus, vi, GTMN_NewLookMenus, TRUE, TAG_DONE))
        SetMenuStrip(root, menus);
    read_tone();                                /* 0.1.34: the labels' colour from the picture's .tone */
    strip_open();
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

/* 0.1.39: a signal or a port run() asked for and didn't get (AllocSignal, CreateMsgPort), said where OpenBench's other
 * lines go, so it is never quiet again: 0.1.38's process asked for 17 of a task's 16 signals, and the drawers'
 * thumbnails were off with no word. */
static void no_signal(const char *what)
{
    if (!run_shell) return;
    PutStr((CONST_STRPTR)"OpenBench: no free signal for ");
    PutStr((CONST_STRPTR)what);
    PutStr((CONST_STRPTR)"\n");
}

static void aloha_start(void)
{
    struct Task *wb;
    struct Library *o;
    Forbid();
    wb = FindTask((CONST_STRPTR)"Workbench");     /* Hyperion's Workbench has the port: it stays its */
    Permit();
    if (!wb) {
        if ((aloha_port = CreateMsgPort())) {
            AlohaWorkbench((LONG)aloha_port);
            aloha_on = 1;
        } else no_signal("Intuition's Workbench port");
    }
    /* openrtg.library 0.17 or later has ORTG_NativeSize */
    if ((o = OpenLibrary((CONST_STRPTR)"openrtg.library", 0))) {
        if (o->lib_Version > 0 || o->lib_Revision >= 17) OpenRTGBase = o;
        else CloseLibrary(o);
    }
    if ((reset_sig = AllocSignal(-1)) < 0) no_signal("Resize to Window");
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

/* ENV:Sys's files OpenBench follows (0.1.19): Workbench's settings and IPrefs' patterns; and (0.1.30)
 * icon.library's labels close to the image. 0.1.39: every notify sends a message to one port, nfy_port, its
 * nr_UserData naming the file (ob_notify.h), where each had a signal of its own (17 asked for, of a task's 16). */
static struct NotifyRequest wbp_nr, pat_nr, lbl_nr, fnt_nr, lay_nr;
static struct MsgPort *nfy_port;
static int wbp_notify, pat_notify, lbl_notify, fnt_notify, lay_notify;
static int font_check;                      /* 0.1.31: Font prefs changed: look again at the next tick but one */

/* 0.1.31: Font prefs' Workbench icon text font is not the one the labels were opened in. Font prefs' Use resets
 * the Workbench screen (IPrefs), and the desktop comes back in the new font by itself; this is for a change that
 * doesn't reset it. */
static int label_font_changed(void)
{
    struct TextAttr now;
    char name[128];
    int set = ogt_icon_font_attr(&now, name, sizeof name);
    if (set != label_set) return 1;
    return set && (strcmp(name, label_asked_name) || now.ta_YSize != label_asked.ta_YSize ||
                   now.ta_Style != label_asked.ta_Style || now.ta_Flags != label_asked.ta_Flags);
}

/* 0.1.39: a message to nfy_port, user data the file (OB_FILE_*). No NRF_WAIT_REPLY: each message is replied at once
 * (notify_take), and changes while one waits each send their own, as each signal came. */
static int notify_on(struct NotifyRequest *nr, const char *name, ULONG file)
{
    if (!nfy_port) return 0;
    memset(nr, 0, sizeof *nr);
    nr->nr_Name = (STRPTR)name;
    nr->nr_UserData = file;
    nr->nr_Flags = NRF_SEND_MESSAGE;
    nr->nr_stuff.nr_Msg.nr_Port = nfy_port;
    return StartNotify(nr) != 0;
}

/* The files changed since the last look (ob_notify.h), each message replied as it is read */
static unsigned notify_take(void)
{
    struct NotifyMessage *nm;
    unsigned changed = 0;
    while (nfy_port && (nm = (struct NotifyMessage *)GetMsg(nfy_port))) {
        changed = ob_notify_add(changed, nm->nm_NReq ? nm->nm_NReq->nr_UserData : 0);
        ReplyMsg(&nm->nm_ExecMessage);
    }
    return changed;
}

/* The notifies ended, then the port. EndNotify (dos.library's autodoc) takes the request's messages still on the port
 * off it and replies them; the open ROM's (amigachrome-guest opendos, od_EndNotify) only tells the handler, so what
 * is left is replied here before the port goes. The requests are static: a handler that counts its replies
 * (nr_MsgCount, FSKit's notify_back) still finds them. */
static void notify_end(void)
{
    if (wbp_notify) EndNotify(&wbp_nr);
    if (pat_notify) EndNotify(&pat_nr);
    if (lbl_notify) EndNotify(&lbl_nr);
    if (lay_notify) EndNotify(&lay_nr);
    if (fnt_notify) EndNotify(&fnt_nr);
    if (dic_notify) EndNotify(&dic_nr);
    wbp_notify = pat_notify = lbl_notify = fnt_notify = lay_notify = dic_notify = 0;
    if (nfy_port) {
        notify_take();
        DeleteMsgPort(nfy_port);
        nfy_port = NULL;
    }
}

static int run(void)
{
    struct MsgPort *pub = NULL, *tport = NULL;
    struct timerequest *tr = NULL;
    int i, ok = 0, timer = 0;

    main_task = FindTask(NULL);
    if (!(pub = CreateMsgPort())) {
        no_signal("its port");
        return RETURN_FAIL;
    }
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
    if (!(launch_port = CreateMsgPort())) {
        no_signal("the programs it starts");
        goto out;
    }
    if ((picture_sig = AllocSignal(-1)) < 0) {
        no_signal("the root picture");
        goto out;
    }
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
    if (!(idcmp = CreateMsgPort())) {
        no_signal("its windows");
        goto out;
    }
    look.port = idcmp;
    look.open_object = ob_open_object;
    look.info_object = ob_info_object;
    look.screen_title = screen_title;
    look.drop = ob_drop;                           /* 0.1.26: icons dragged out of a drawer */
    look.hidden = leftout_hidden;                  /* 0.1.35: icons left out aren't shown in their drawers */
    look.snapped = drawer_snapped;                 /* 0.1.38: Snapshot in the drawers */
    look.snap = drawer_snap;
    look.leave_out = drawer_leave_out;             /* 0.1.41: Icons > Leave Out in the drawers (.backdrop) */
    snaps_load();                                  /* 0.1.38: the places OpenBench snapshotted, before the icons */
    wb_prefs_load();
    aloha_start();                                 /* 0.1.22: Intuition's Workbench port, openrtg.library 0.17 */
    if (!desktop_up()) goto out;
    ok = 1;

    /* 0.1.39: the files' notifies, on one port (each had a signal up to 0.1.38) */
    if (!(nfy_port = CreateMsgPort())) no_signal("the prefs files' notifies");
    wbp_notify = notify_on(&wbp_nr, "ENV:Sys/Workbench.prefs", OB_FILE_WBPREFS);
    pat_notify = notify_on(&pat_nr, "ENV:Sys/WBPattern.prefs", OB_FILE_PATTERN);
    lbl_notify = notify_on(&lbl_nr, "ENV:Sys/IconLabels", OB_FILE_LABELS);
    /* 0.1.34: the icons' layout (OpenPrefs' Icons page) */
    lay_notify = notify_on(&lay_nr, "ENV:Sys/IconLayout", OB_FILE_LAYOUT);
    /* 0.1.37: No icons' variable, looked at with the labels' (OB_DO_ICONS); without a notify, the 2-second timer
     * looks */
    dic_notify = notify_on(&dic_nr, "ENV:" DESKICONS_VAR, OB_FILE_DESKICONS);
    ob_seen_init(&seen);
    iconprefs_moved();                             /* what the labels' look compares with */
    fnt_notify = notify_on(&fnt_nr, "ENV:Sys/font.prefs", OB_FILE_FONT);
    wbc_start();                                   /* OpenLook's close notice */
    if (wbc_sig < 0 || !wbc_port) no_signal("OpenLook's close notice");
    /* 0.1.26: before the dock and WBStartup add AppWindows */
    if (!ob_drop_start(&drop_env)) no_signal("drag and drop's ports");
    start_dock();
    run_wbstartup();

    /* every two seconds: the title (free memory); 0.1.40: the volumes' fingerprint (volumes_moved) */
    if (!(tport = CreateMsgPort())) no_signal("the 2-second timer");
    else if ((tr = (struct timerequest *)CreateIORequest(tport, sizeof *tr)) &&
             !OpenDevice((CONST_STRPTR)TIMERNAME, UNIT_VBLANK, (struct IORequest *)tr, 0)) {
        timer = 1;
        tr->tr_node.io_Command = TR_ADDREQUEST;
        tr->tr_time.tv_secs = 2;
        tr->tr_time.tv_micro = 0;
        SendIO((struct IORequest *)tr);
    }

    /* the drawers' thumbnails (0.2); none: pictures show their icons. 0.1.39: it fails with no datatypes.library 44
     * too, so a free signal is looked at first */
    if (!wb_thumb_start() && DataTypesBase) {
        BYTE s = AllocSignal(-1);
        if (s < 0) no_signal("the drawers' thumbnails");
        else FreeSignal(s);
    }
    for (;;) {
        ULONG sigs = Wait((1UL << idcmp->mp_SigBit) | (1UL << launch_port->mp_SigBit) | (1UL << picture_sig) | wb_thumb_sigmask() |
                          (1UL << pub->mp_SigBit) | (nfy_port ? 1UL << nfy_port->mp_SigBit : 0) | wbc_sigmask() |
                          (aloha_port ? 1UL << aloha_port->mp_SigBit : 0) | (reset_sig >= 0 ? 1UL << reset_sig : 0) |
                          ob_drop_sigmask() | SIGBREAKF_CTRL_C | (timer ? 1UL << tport->mp_SigBit : 0));
        struct IntuiMessage *im;
        struct Message *lm;
        unsigned todo;
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
        /* 0.1.39: the files that changed, from their messages (ob_notify.h); each does what its signal did */
        todo = ob_notify_todo(notify_take());
        if (todo & OB_DO_WBPREFS) {
            wb_prefs_load();                        /* a new default stack, devices hidden or shown */
            if (root) {
                scan_volumes();
                redraw_all();
            }
        }
        if ((todo & OB_DO_PICTURE) && root) new_picture();
        if (todo & OB_DO_FONT) font_check = 2;     /* after the reset IPrefs may make */
        if (todo & OB_DO_ICONS) {
            /* IconLabels, IconLayout, or (0.1.37) ENV:Sys/DesktopIcons (No icons): that at once */
            noicons_apply(ob_show_set(&show, deskicons_none()));
            /* the labels' place or the icons' layout changed (OpenPrefs' Icons page): the desktop's icons read again,
             * once icon.library (which reads its setting at most once a second) has the new one, and (0.1.34) every
             * drawer's, which are laid out again. 0.1.37: only when one of the two did change, not for No icons */
            if (root && iconprefs_moved()) {
                Delay(60);
                scan_volumes();
                redraw_all();
                for (i = 0; i < ndrawers; i++) wb_drawer_reread(drawers[i]);
            }
        }
        if ((sigs & wb_thumb_sigmask()) && wb_thumb_collect())
            for (i = 0; i < ndrawers; i++) wb_drawer_thumbs(drawers[i]);
        if (sigs & (1UL << picture_sig)) {
            ULONG *old = NULL;
            if (root) LockLayer(0, root->WLayer);
            if (strip) LockLayer(0, strip->WLayer);
            Forbid();
            if (picture_ready) { old = backdrop; backdrop = picture_ready; picture_ready = NULL; }
            Permit();
            if (strip) UnlockLayer(strip->WLayer);
            if (root) UnlockLayer(root->WLayer);
            if (old) FreeVec(old);
            if (root && (backdrop || pic_bm)) {
                EraseRect(root->RPort, 0, 0, root->Width - 1, root->Height - 1);
                strip_redraw();
                draw_icons();
            }
        }
        if (timer && CheckIO((struct IORequest *)tr)) {
            WaitIO((struct IORequest *)tr);
            /* 0.1.34: the picture still decoding two ticks on (4 seconds): at 0 (picture_hurry) */
            if (++picture_ticks == 2) picture_hurry();
            if (root && make_title() && !title_held) titles_everywhere();
            if (root) mark_ours();                  /* 0.1.22: the dock's and Information windows opened since */
            if (!dic_notify) noicons_apply(ob_show_set(&show, deskicons_none()));   /* 0.1.37: no notify for it */
            if (!wb_down) native_tick();            /* 0.1.22: a page asked the Amiga to follow now; the menu item */
            /* 0.1.40: a volume mounted or dismounted with no disk event (ob_vols.h); not while a desktop icon is
             * held, which a scan would end: the next tick */
            if (root && !wb_down && !pressed_on && !dragging && volumes_moved()) volumes_again();
            /* 0.1.31: Font prefs changed and the screen wasn't reset: the desktop and its drawers again, in the
             * icons' new font, as after a reset */
            if (font_check && --font_check == 0 && !wb_down && root && label_font_changed()) {
                go_down();
                if (!go_up()) {
                    desktop_down();
                    wb_down = 2;                    /* tried again each tick */
                }
            }
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
            /* 0.1.44: words held in the title (say_title) go at the next click, in any of OpenBench's windows */
            if (title_held && cl == IDCMP_MOUSEBUTTONS && code == SELECTDOWN) title_release();
            if (root && win == root) {
                if (!root_event(cl, code, qual, mx, my, secs, micros)) quit = 1;
                continue;
            }
            for (i = 0; i < ndrawers; i++)
                if (wb_drawer_window(drawers[i]) == win) {
                    if (!wb_drawer_event(drawers[i], cl, code, qual, ia, mx, my, secs, micros)) close_index(i);
                    else if (cl == IDCMP_ACTIVEWINDOW) SetWindowTitles(win, (CONST_STRPTR)~0, (CONST_STRPTR)title_now());
                    break;
                }
        }
        if (quit) break;
        if (reread_pending) {                       /* 0.1.35: icons left out or put away: their drawers again */
            reread_pending = 0;
            for (i = 0; i < ndrawers; i++) wb_drawer_reread(drawers[i]);
        }
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
    notify_end();                               /* 0.1.39: the notifies, then their port */
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
    ob_snap_free(&snaps);
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
