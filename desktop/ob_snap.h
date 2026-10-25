/* ob_snap.h: Snapshot's record (OpenBench 0.1.38; Salvor's OS list, item 53): the icon places OpenBench itself wrote
 * into .info files, so that it keeps those and no others. Plain C with no Amiga headers, tested on the host
 * (tests/host/test_ob_snap.c).
 *
 * Icons > Snapshot writes an icon's place into its .info (do_CurrentX and do_CurrentY, as OS 3.2.3's Snapshot does,
 * so 3.2.3 reads it too), and a line here. A .info's place counts only while this record says OpenBench wrote that
 * very place there: places from before (made for 3.2.3's small icons, 71 pixels apart, which put the labels into
 * the icons under them) and places another Workbench wrote since are passed over, and the icon goes on the grid.
 *
 * The record is one file, ENVARC:OpenBench/Snapshots, read once when OpenBench starts and written when a Snapshot,
 * an Unsnapshot or a Clean Up changes it. One line for each icon:
 *
 *   volume <tab> path on the volume ("" for its Disk.info) <tab> D or W <tab> x y <tab> gx gy
 *
 * D: the icon is on the desktop (a disk, or an icon left out); W: in its drawer's window. x y: the place written into
 * the .info. gx gy: its cell on the grid (wb_gcell's x and y: from the grid's corner on the desktop, from the content's
 * first cell in a drawer), where it goes back to, exactly. Lines starting with # are comments. Names are matched
 * without regard to case, as AmigaDOS does.
 *
 * MIT, Copyright (c) 2026 AmigaChrome Team. */
#ifndef OB_SNAP_H
#define OB_SNAP_H

#define OB_SNAP_NOPOS (-2147483647L - 1)    /* NO_ICON_POSITION (0x80000000) as do_CurrentX holds it */
#define OB_SNAP_VOL 32                      /* a volume's name: 30 characters at most */
#define OB_SNAP_PATH 256                    /* a path on the volume */
#define OB_SNAP_MAX 4096                    /* icons kept at most */

enum { OB_SNAP_DESK = 'D', OB_SNAP_DRAWER = 'W' };

typedef struct ob_snap_ent {
    char vol[OB_SNAP_VOL];
    char path[OB_SNAP_PATH];
    char where;                             /* OB_SNAP_DESK or OB_SNAP_DRAWER */
    long x, y;                              /* what OpenBench wrote into the .info */
    int gx, gy;                             /* its cell's place on the grid */
} ob_snap_ent;

typedef struct ob_snap {
    ob_snap_ent *e;
    int n, cap;
    unsigned long gen;                      /* up by one at each change, for what is kept from it */
} ob_snap;

void ob_snap_init(ob_snap *s);
void ob_snap_free(ob_snap *s);

/* The record's text (len bytes, no NUL needed), in place of what s held: the entries read. Lines it can't read are
 * passed over; a name twice: the later line. */
int ob_snap_parse(ob_snap *s, const char *text, long len);

/* The record's text into out (n bytes, NUL-terminated when n > 0): its length, which may be more than n - 1 (then
 * out holds what fitted, as snprintf). */
long ob_snap_text(const ob_snap *s, char *out, long n);

/* A full path as a disk names it ("Work:Tools/OpenMeters", "Work:") into its volume and the path on it ("Tools/
 * OpenMeters", ""). 0 when it has no volume, or a part is too long. */
int ob_snap_split(const char *full, char *vol, int vn, char *path, int pn);

/* The entry for this icon, or NULL. */
const ob_snap_ent *ob_snap_find(const ob_snap *s, const char *vol, const char *path);

/* An icon snapshotted: its line set. 1 changed, 0 already so, -1 not kept (the record is full, out of memory, or a
 * name has a tab or a line's end in it). */
int ob_snap_set(ob_snap *s, const char *vol, const char *path, int where, long x, long y, int gx, int gy);

/* An icon unsnapshotted: its line taken out. 1 when there was one. */
int ob_snap_forget(ob_snap *s, const char *vol, const char *path);

/* 1 when OpenBench keeps the icon where it was snapshotted: the record has it, in the same kind of window (where), and
 * its .info still says the place OpenBench wrote (info_x, info_y); its cell's place then in *gx, *gy. */
int ob_snap_honour(const ob_snap *s, const char *vol, const char *path, int where, long info_x, long info_y, int *gx,
                   int *gy);

/* How many of the record's icons of this kind (where) are in drawer dir on vol ("" for the volume's top). A drawer
 * with none asks nothing more. */
int ob_snap_count_in(const ob_snap *s, const char *vol, const char *dir, int where);

/* Icons > Clean Up on the desktop: the snapshots of the n icons it lays out (their full paths, a disk's "Work:")
 * taken out, so they go on the grid from now on, as Clean Up leaves them (Snapshot keeps the new places). Only
 * desktop places: an icon's place in its drawer stays. How many went. */
int ob_snap_cleanup(ob_snap *s, const char *const *full, int n);

#endif
