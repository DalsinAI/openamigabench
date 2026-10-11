/* ob_info: OpenBench's Information window (the Team, 11 October 2026: "show information doesn't work either":
 * the drawers asked workbench.library's WBInfo(), which does nothing while Workbench isn't running, and OpenBench
 * is the Workbench). One window for each icon asked about, each in a process of its own so the desktop goes on:
 * the icon, its name (renamed, or a disk relabelled, on Save), what it is, its size and date, the protection
 * bits, the comment, a project's default tool and the tool types, which can be edited, added and removed.
 * MIT, Copyright (c) 2026 Dalsin Limited. */
#ifndef OB_INFO_H
#define OB_INFO_H

#include <exec/types.h>
#include <intuition/screens.h>

/* An Information window for path ("Work:", "Work:Tools/OpenMeters") on screen scr. 1 when it was started (or the
 * one for that path was brought to the front), 0 when not. */
int ob_info_open(struct Screen *scr, const char *path);
/* How many are open. */
int ob_info_running(void);
/* Ctrl-C to every one (each closes without saving), then up to 5 s for them to go. */
void ob_info_quit_all(void);

#endif
