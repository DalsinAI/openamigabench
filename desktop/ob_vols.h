/* ob_vols.h: the mounted volumes' fingerprint (OpenBench 0.1.40), how OpenBench sees a volume come or go when no
 * disk event tells it. Plain C with no Amiga headers, tested on the host (tests/host/test_ob_vols.c).
 *
 * Up to 0.1.39 OpenBench read the DOS list's volumes when it started, on Intuition's IDCMP_DISKINSERTED and
 * IDCMP_DISKREMOVED, on Window > Update, and when Workbench's settings or the Icons page changed. Intuition's disk
 * events come only from a handler that posts IECLASS_DISKINSERTED or _DISKREMOVED to input.device. A volume added to
 * the list by one that posts none (a Mount DH2: or a RAD: while running, a drive Cradle adds) got no icon until a
 * disk went in or out somewhere (Salvor's checks, 25 October 2026). DOS tells nobody when its list changes, and no
 * workbench.library call tells of a new volume: UpdateWorkbench names a file or a drawer (and the open library's does
 * nothing yet), and the disk info hook (WBCTRLA_SetDiskInfoHook) is Workbench asking a program about a disk.
 *
 * Since 0.1.40 each scan of the volumes keeps their fingerprint, and OpenBench's 2-second tick takes one again (the
 * list locked for reading only if no one is writing it, else the next tick) and compares the two: the same, nothing
 * more is done; different, the volumes are scanned and drawn again, as after a disk event.
 *
 * The fingerprint: how many volumes are in a drive, and two sums, an added one and an xor'ed one, of a 32-bit hash of
 * each one's name (its bytes, so a new case is a change too), its date (dol_VolumeDate) and its drive (dol_Task). The
 * sums don't depend on the list's order. In use:
 *     ob_volfp_init(&now);
 *     for each volume in a drive: ob_volfp_add(&now, name, len, days, minute, tick, (unsigned long)dol_Task);
 *     if (!ob_volfp_same(&now, &seen)) { scan again; seen = now; }
 *
 * MIT, Copyright (c) 2026 AmigaChrome Team. */
#ifndef OB_VOLS_H
#define OB_VOLS_H

typedef struct ob_volfp {
    unsigned n;                             /* the volumes in a drive */
    unsigned sum;                           /* their hashes added */
    unsigned mix;                           /* their hashes xor'ed */
} ob_volfp;

/* No volumes */
void ob_volfp_init(ob_volfp *f);

/* One volume more: its name (len bytes, not NUL-ended: a BSTR's), its date and its drive */
void ob_volfp_add(ob_volfp *f, const unsigned char *name, unsigned len, long days, long minute, long tick,
                  unsigned long drive);

/* 1 when a and b are the same volumes */
int ob_volfp_same(const ob_volfp *a, const ob_volfp *b);

#endif
