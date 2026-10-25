/* ob_notify.h: the ENV: files OpenBench follows, told by DOS notification (OpenBench 0.1.39), and what a change of
 * each asks for. Plain C with no Amiga headers, tested on the host (tests/host/test_ob_notify.c).
 *
 * Up to 0.1.38 each file had a signal of its own (NRF_SEND_SIGNAL), five in all, and OpenBench's process asked for 17
 * of the 16 signals a task has: the last one asked for, the drawers' thumbnails (opengadtools wb_thumb.c), failed,
 * and thumbnails were quietly off. Since 0.1.39 every notify sends a message (NRF_SEND_MESSAGE) to one port, and the
 * request's nr_UserData names its file (OB_FILE_*): one signal for all six requests.
 *
 * In the main loop, for each NotifyMessage on the port:
 *     changed = ob_notify_add(changed, nm->nm_NReq->nr_UserData);   then ReplyMsg
 * and then ob_notify_todo(changed) says what to do, as each file's own signal did.
 *
 * MIT, Copyright (c) 2026 AmigaChrome Team. */
#ifndef OB_NOTIFY_H
#define OB_NOTIFY_H

/* nr_UserData: which file. 0 and anything past OB_FILE_LAST name none. */
enum {
    OB_FILE_WBPREFS = 1,                    /* ENV:Sys/Workbench.prefs */
    OB_FILE_PATTERN,                        /* ENV:Sys/WBPattern.prefs */
    OB_FILE_LABELS,                         /* ENV:Sys/IconLabels */
    OB_FILE_LAYOUT,                         /* ENV:Sys/IconLayout */
    OB_FILE_FONT,                           /* ENV:Sys/font.prefs */
    OB_FILE_DESKICONS,                      /* ENV:Sys/DesktopIcons (No icons) */
    OB_FILE_LAST = OB_FILE_DESKICONS
};

/* What the files that changed ask for */
enum {
    OB_DO_WBPREFS = 1,                      /* Workbench.prefs read again; the volumes scanned and drawn again */
    OB_DO_PICTURE = 2,                      /* the root picture again */
    OB_DO_FONT = 4,                         /* the labels' font looked at two ticks on (font_check) */
    OB_DO_ICONS = 8                         /* No icons looked at; the icons laid out again only when IconLabels or
                                             * IconLayout really changed (ob_show.h, ob_seen_changed) */
};

/* The files changed so far (0 to begin with), with the one user_data names; one it doesn't name changes nothing.
 * The same file twice counts once. */
unsigned ob_notify_add(unsigned changed, unsigned long user_data);

/* What those files ask for: OB_DO_* bits. IconLabels, IconLayout and DesktopIcons together ask for one look
 * (OB_DO_ICONS), as their two shared signals did up to 0.1.38. */
unsigned ob_notify_todo(unsigned changed);

#endif
