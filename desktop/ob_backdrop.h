/* ob_backdrop.h: a disk's .backdrop, the icons Icons > Leave Out leaves out on the desktop for good (OpenBench 0.1.41).
 * Plain C with no Amiga headers, tested on the host (tests/host/test_ob_backdrop.c).
 *
 * As OS 3.2.3's Workbench keeps it: a text file at the top of the disk (Work:.backdrop), one line for each icon left
 * out from that disk, its path on the disk after a colon (":Tools/OpenMeters"), each line ended by a line feed.
 * Icons > Leave Out adds the icon's line, Icons > Put Away takes it out, and a file with no line left is deleted.
 * OpenBench reads every disk's .backdrop when it scans the disks, and shows those icons on the desktop.
 *
 * Reading: a line that starts with a colon and names something is an icon; a CR before the line feed is taken off;
 * a name twice (in any case: AmigaDOS's names are matched without regard to case) is one icon. Other lines (empty,
 * or something else) aren't icons; when the file is written again they are kept as they were, empty ones left out.
 * No file reads as an empty one.
 *
 * MIT, Copyright (c) 2026 AmigaChrome Team. */
#ifndef OB_BACKDROP_H
#define OB_BACKDROP_H

#define OB_BACKDROP_PATH 256                /* a path on the disk, with its NUL */

/* Each icon in the file's text (len bytes, no NUL needed; NULL or 0: no file): each(ctx, path) with its path on the
 * disk ("Tools/OpenMeters": no colon), once for each name. How many. */
int ob_backdrop_parse(const char *text, long len, void (*each)(void *ctx, const char *path), void *ctx);

/* 1 when the text has a line for path (on the disk, "Tools/OpenMeters"). */
int ob_backdrop_has(const char *text, long len, const char *path);

/* The text with path's line added (add 1: Leave Out, at the end, when it isn't there already) or taken out (add 0:
 * Put Away, every line that names it), into out (n bytes, NUL-terminated when n > 0), as the file is to be written:
 * every line ended by a line feed, its CR taken off, empty lines and the second line for a name left out. Its length
 * is returned, which may be more than n - 1 (out then holds what fitted, as snprintf), and 0 means the file is to be
 * deleted. *changed: 1 when a line was added or taken out (else the file needn't be written). -1 when path can't be
 * a line: empty, too long (OB_BACKDROP_PATH), starting with a colon or with a line's end in it. */
long ob_backdrop_edit(const char *text, long len, const char *path, int add, char *out, long n, int *changed);

#endif
