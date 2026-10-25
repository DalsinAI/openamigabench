/* ob_vols' host test: the mounted volumes' fingerprint (OpenBench 0.1.40). The same volumes in any order are the same;
 * a volume mounted or dismounted, renamed (a new case too), with another date or in another drive is a change; and on
 * a run of 2-second ticks the desktop is scanned again only on the ticks after the list changed.
 *   test_ob_vols
 * MIT, Copyright (c) 2026 AmigaChrome Team. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ob_vols.h"

static int failures, checks;

#define CHECK(c, ...) do { checks++; if (!(c)) { failures++; printf("FAIL %s:%d: ", __FILE__, __LINE__); \
    printf(__VA_ARGS__); printf("\n"); } } while (0)

/* A volume as the DOS list has it */
typedef struct vol {
    const char *name;
    long days, minute, tick;
    unsigned long drive;                    /* dol_Task, its handler's port */
} vol;

static ob_volfp fp_of(const vol *v, int n)
{
    ob_volfp f;
    int i;
    ob_volfp_init(&f);
    for (i = 0; i < n; i++)
        ob_volfp_add(&f, (const unsigned char *)v[i].name, (unsigned)strlen(v[i].name), v[i].days, v[i].minute,
                     v[i].tick, v[i].drive);
    return f;
}

/* Instance-32's, roughly: the boot disk, Work, the RAM disk and a floppy */
static const vol base[] = {
    { "System", 17800, 600, 120, 0x00201000UL },
    { "Work", 17800, 601, 40, 0x00202000UL },
    { "RAM Disk", 18560, 1100, 7, 0x00203000UL },
    { "Empty", 15000, 3, 0, 0x00204000UL },
};
#define NBASE ((int)(sizeof base / sizeof base[0]))

static int same_as_base(const vol *v, int n)
{
    ob_volfp a = fp_of(base, NBASE), b = fp_of(v, n);
    return ob_volfp_same(&a, &b);
}

static void unchanged(void)
{
    vol v[NBASE + 1];
    ob_volfp a, b;
    ob_volfp_init(&a);
    ob_volfp_init(&b);
    CHECK(ob_volfp_same(&a, &b), "no volumes, twice: the same");
    memcpy(v, base, sizeof base);
    CHECK(same_as_base(v, NBASE), "the same list: the same");
    v[0] = base[3];
    v[3] = base[0];
    v[1] = base[2];
    v[2] = base[1];
    CHECK(same_as_base(v, NBASE), "the same volumes in another order: the same (the DOS list's order isn't looked at)");
    a = fp_of(base, NBASE);
    CHECK(a.n == NBASE, "the count: %u", a.n);
}

static void changed(void)
{
    vol v[NBASE + 1];
    memcpy(v, base, sizeof base);
    v[NBASE].name = "DH2";
    v[NBASE].days = 18900; v[NBASE].minute = 5; v[NBASE].tick = 0; v[NBASE].drive = 0x00205000UL;
    CHECK(!same_as_base(v, NBASE + 1), "Mount DH2: (a volume more): a change");
    CHECK(!same_as_base(base, NBASE - 1), "the floppy out (a volume fewer): a change");
    CHECK(!same_as_base(base + 1, NBASE - 1), "System gone: a change");
    v[NBASE - 1] = v[NBASE];
    CHECK(!same_as_base(v, NBASE), "the floppy out and DH2 in between two ticks (the same count): a change");

    memcpy(v, base, sizeof base);
    v[1].name = "Work2";
    CHECK(!same_as_base(v, NBASE), "Work relabelled Work2: a change");
    v[1].name = "Wor";
    CHECK(!same_as_base(v, NBASE), "Work relabelled Wor: a change");
    v[1].name = "work";
    CHECK(!same_as_base(v, NBASE), "Work relabelled work (only the case): a change, the label shows it");
    v[1].name = "";
    CHECK(!same_as_base(v, NBASE), "Work with no name: a change");

    memcpy(v, base, sizeof base);
    v[3].days++;
    CHECK(!same_as_base(v, NBASE), "another disk called Empty (another day): a change");
    v[3] = base[3];
    v[3].minute++;
    CHECK(!same_as_base(v, NBASE), "another disk called Empty (another minute): a change");
    v[3] = base[3];
    v[3].tick++;
    CHECK(!same_as_base(v, NBASE), "another disk called Empty (another tick): a change");
    v[3] = base[3];
    v[3].drive = 0x00206000UL;
    CHECK(!same_as_base(v, NBASE), "the same floppy in another drive (its icon's kind may differ): a change");

    memcpy(v, base, sizeof base);
    v[0].drive = base[1].drive;
    v[1].drive = base[0].drive;
    CHECK(!same_as_base(v, NBASE), "System and Work in each other's drives: a change");
    memcpy(v, base, sizeof base);
    v[0].name = base[1].name;
    v[1].name = base[0].name;
    v[0].minute = base[1].minute; v[0].tick = base[1].tick;
    v[1].minute = base[0].minute; v[1].tick = base[0].tick;
    CHECK(!same_as_base(v, NBASE), "two volumes' names and dates swapped between drives: a change");
}

/* Many names, each in place of the floppy: no two lists alike, and none like the first */
static int by_fp(const void *a, const void *b)
{
    const ob_volfp *x = a, *y = b;
    if (x->sum != y->sum) return x->sum < y->sum ? -1 : 1;
    if (x->mix != y->mix) return x->mix < y->mix ? -1 : 1;
    return 0;
}

static void apart(void)
{
    static const char *const stem[] = { "DH", "Work", "RAD", "CD", "Empty", "Disk", "Games", "dh", "work", "AC" };
    enum { PER = 1000, NS = (int)(sizeof stem / sizeof stem[0]), N = NS * PER };
    ob_volfp *f = malloc((size_t)(N + 1) * sizeof *f);
    vol v[NBASE];
    char names[64];
    int s, k, i, dups = 0;
    if (!f) { CHECK(0, "no memory"); return; }
    memcpy(v, base, sizeof base);
    for (s = 0; s < NS; s++)
        for (k = 0; k < PER; k++) {
            snprintf(names, sizeof names, "%s%d", stem[s], k);
            v[3].name = names;
            f[s * PER + k] = fp_of(v, NBASE);
        }
    f[N] = fp_of(base, NBASE);
    qsort(f, (size_t)N + 1, sizeof *f, by_fp);
    for (i = 1; i <= N; i++)
        if (!by_fp(&f[i - 1], &f[i])) dups++;
    CHECK(!dups, "%d names in place of the floppy: %d lists alike", N, dups);
    free(f);
}

/* The 2-second ticks as run() has them: a scan keeps the fingerprint, a tick scans again only when it differs */
static int scans;
static ob_volfp seen;

static void scan(const vol *v, int n)
{
    scans++;
    seen = fp_of(v, n);
}

static void tick(const vol *v, int n)
{
    ob_volfp now = fp_of(v, n);
    if (!ob_volfp_same(&now, &seen)) scan(v, n);
}

static void ticks(void)
{
    vol v[NBASE + 1];
    int i;
    memcpy(v, base, sizeof base);
    scans = 0;
    scan(v, NBASE);                                     /* the desktop comes up */
    for (i = 0; i < 100; i++) tick(v, NBASE);
    CHECK(scans == 1, "100 ticks with nothing mounted: no scan again (%d)", scans - 1);
    v[NBASE].name = "RAD";
    v[NBASE].days = 18925; v[NBASE].minute = 280; v[NBASE].tick = 31; v[NBASE].drive = 0x00207000UL;
    tick(v, NBASE + 1);
    CHECK(scans == 2, "Mount RAD: then a tick: scanned again, its icon there (%d)", scans);
    for (i = 0; i < 10; i++) tick(v, NBASE + 1);
    CHECK(scans == 2, "the ticks after: no scan (%d)", scans);
    scan(v, NBASE + 1);                                 /* a disk event, which scans as before */
    tick(v, NBASE + 1);
    CHECK(scans == 3, "a disk event's scan keeps the fingerprint: the next tick doesn't scan again (%d)", scans);
    tick(v, NBASE);
    CHECK(scans == 4, "RAD: dismounted then a tick: scanned again, its icon gone (%d)", scans);
    tick(v, NBASE);
    CHECK(scans == 4, "the tick after: no scan (%d)", scans);
}

int main(void)
{
    unchanged();
    changed();
    apart();
    ticks();
    printf("ob_vols: %d checks, %d failed\n", checks, failures);
    return failures != 0;
}
