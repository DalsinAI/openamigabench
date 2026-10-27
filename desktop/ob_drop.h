/* ob_drop: icons let go on the desktop's windows (W67; Dale, 18 October 2026: "dragging icons 'into'
 * windows"; 24 October 2026: icons dragged between windows, as OS 3.2.3's Workbench does it). See ob_drop.c.
 * MIT, Copyright (c) 2026 AmigaChrome Team. */
#ifndef OB_DROP_H
#define OB_DROP_H

#include <exec/types.h>
#include <intuition/intuition.h>
#include "wb_drawer.h"

/* What a desktop icon is (ob_drop_env's icon_at) */
enum { OB_DESK_NONE = 0,
       OB_DESK_DISK,                            /* a disk: into it */
       OB_DESK_DRAWER,                          /* a drawer left out: into it */
       OB_DESK_TRASH,                           /* a trashcan left out: to the trash */
       OB_DESK_FILE };                          /* a tool or a project left out: the desktop's space */

/* What OpenBench shares with it: its statics, read at each drop (the screen and the root window change when
 * the Workbench screen is reset), and the desktop's icons. */
typedef struct ob_drop_env {
    struct Screen **scr;                        /* the Workbench screen */
    struct Window **root;                       /* the desktop's window */
    wb_drawer **drawers;                        /* the drawer windows open, and how many */
    int *ndrawers;
    /* The desktop's icon at (sx, sy), not the one being dragged: OB_DESK_*, and its path in out ("Work:", or a
     * left-out "Work:Tools/x"). */
    int (*icon_at)(WORD sx, WORD sy, char *out, int n);
    /* Icons let go on the desktop's own space: left out there for this run, as OS 3.2.3 leaves them out (gone
     * from their drawers, not written to .backdrop). 1 when taken. */
    int (*leave_out)(const char *const *paths, int n, WORD sx, WORD sy);
    /* A job has ended: files may have gone from where icons left out on the desktop say they are. */
    void (*changed)(void);
    /* 0.1.44: words for the screen's title, kept there until the next click, as OS 3.2.3 says an error that asks
     * nothing (a move within a volume onto a name that is taken). NULL: not said. */
    void (*say)(const char *text);
} ob_drop_env;

/* Before the dock and WBStartup: tells workbench.library that drops on AppWindows are delivered (the Team's
 * OWBCTRLA_DesktopDrops), so programs' AddAppWindowA is taken. 1, or 0 with no ports. */
int ob_drop_start(const ob_drop_env *env);
/* At the end: the jobs still running are asked to stop and waited for, AppMessages not yet answered too
 * (for a while). */
void ob_drop_stop(void);
ULONG ob_drop_sigmask(void);
/* Its signals came: finished jobs (their drawers read again), answered AppMessages (freed). */
void ob_drop_handle(void);
/* Icons let go at (sx, sy) on the screen: from a drawer window (wb_look's drop; from is the drawer), or left out
 * on the desktop (from NULL). 1 when something was done. */
int ob_drop(const char *const *paths, int n, WORD sx, WORD sy, UWORD qual, wb_drawer *from);
/* A disk's icon from the desktop let go at (sx, sy), over a window that isn't the desktop's: its contents
 * into a drawer (a new drawer named after the disk), or to a program's AppWindow. 1 when taken. */
int ob_drop_volume(const char *volume, WORD sx, WORD sy, UWORD qual);
/* The window whose layer is at (sx, sy) on the screen, or NULL (the screen's bar, say). */
struct Window *ob_drop_window_at(WORD sx, WORD sy);
/* The same, looking through skip (an icon's picture being dragged). */
struct Window *ob_drop_window_under(WORD sx, WORD sy, struct Window *skip);

#endif
