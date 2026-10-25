/* ob_snap's host test: Snapshot's record (OpenBench 0.1.38), its text read and written, which .info places OpenBench
 * keeps (only those it wrote, and only while the .info still says them), and what Icons > Clean Up takes out.
 *   test_ob_snap
 * MIT, Copyright (c) 2026 AmigaChrome Team. */
#include <stdio.h>
#include <string.h>

#include "ob_snap.h"

static int failures, checks;

#define CHECK(c, ...) do { checks++; if (!(c)) { failures++; printf("FAIL %s:%d: ", __FILE__, __LINE__); \
    printf(__VA_ARGS__); printf("\n"); } } while (0)

static int parse(ob_snap *s, const char *t)
{
    return ob_snap_parse(s, t, (long)strlen(t));
}

static void split(void)
{
    char v[OB_SNAP_VOL], p[OB_SNAP_PATH], longname[400];
    CHECK(ob_snap_split("Work:", v, sizeof v, p, sizeof p) && !strcmp(v, "Work") && !p[0], "a disk: its name, no path");
    CHECK(ob_snap_split("Work:Tools/OpenMeters", v, sizeof v, p, sizeof p) && !strcmp(v, "Work") &&
          !strcmp(p, "Tools/OpenMeters"), "a file in a drawer");
    CHECK(ob_snap_split("System:Tools/", v, sizeof v, p, sizeof p) && !strcmp(p, "Tools"), "a drawer with its slash");
    CHECK(!ob_snap_split("Tools/OpenMeters", v, sizeof v, p, sizeof p), "no volume");
    CHECK(!ob_snap_split(":Tools", v, sizeof v, p, sizeof p), "the current volume's root: no name");
    CHECK(!ob_snap_split("AVolumeNameMuchLongerThanThirtyOneCharacters:x", v, sizeof v, p, sizeof p), "too long a name");
    memset(longname, 'a', sizeof longname);
    memcpy(longname, "Work:", 5);
    longname[sizeof longname - 1] = 0;
    CHECK(!ob_snap_split(longname, v, sizeof v, p, sizeof p), "too long a path");
}

static void set_find_forget(void)
{
    ob_snap s;
    const ob_snap_ent *e;
    int i, full = 0;
    unsigned long g;
    ob_snap_init(&s);
    CHECK(ob_snap_set(&s, "Work", "", OB_SNAP_DESK, 20, 30, 0, 0) == 1, "a disk snapshotted");
    CHECK(ob_snap_set(&s, "Work", "Tools/OpenMeters", OB_SNAP_DRAWER, 130, 60, 126, 0) == 1, "a drawer's icon");
    CHECK(s.n == 2, "two kept");
    CHECK((e = ob_snap_find(&s, "WORK", "tools/openmeters")) && e->x == 130 && e->gx == 126, "found, case aside");
    g = s.gen;
    CHECK(ob_snap_set(&s, "Work", "", OB_SNAP_DESK, 20, 30, 0, 0) == 0 && s.gen == g, "the same again: no change");
    CHECK(ob_snap_set(&s, "Work", "", OB_SNAP_DESK, 20, 130, 0, 100) == 1 && s.n == 2 && s.gen != g, "moved: replaced");
    CHECK(ob_snap_set(&s, "work", "", OB_SNAP_DESK, 20, 130, 0, 100) == 1 && !strcmp(ob_snap_find(&s, "Work", "")->vol, "work"),
          "renamed in case only: as the disk names it now");
    CHECK(ob_snap_set(&s, "Wo\trk", "", OB_SNAP_DESK, 1, 1, 0, 0) == -1, "a tab in a name: not kept");
    CHECK(ob_snap_set(&s, "Work", "a\nb", OB_SNAP_DESK, 1, 1, 0, 0) == -1, "a line's end in a name: not kept");
    CHECK(ob_snap_set(&s, "", "x", OB_SNAP_DESK, 1, 1, 0, 0) == -1, "no volume: not kept");
    CHECK(ob_snap_set(&s, "Work", "x", 'Q', 1, 1, 0, 0) == -1, "neither the desktop nor a drawer: not kept");
    CHECK(ob_snap_set(&s, "Work", "x", OB_SNAP_DESK, OB_SNAP_NOPOS, 1, 0, 0) == -1, "no place: not kept");
    CHECK(ob_snap_forget(&s, "WORK", "") == 1 && s.n == 1 && !ob_snap_find(&s, "Work", ""), "unsnapshotted: gone");
    CHECK(ob_snap_forget(&s, "Work", "") == 0, "again: nothing there");
    CHECK(ob_snap_find(&s, "Work", "Tools/OpenMeters") != NULL, "the other kept");
    for (i = 0; i < OB_SNAP_MAX + 5; i++) {
        char p[32];
        snprintf(p, sizeof p, "Many/%d", i);
        if (ob_snap_set(&s, "DH1", p, OB_SNAP_DRAWER, i, i, i, i) < 0) full++;
    }
    CHECK(s.n == OB_SNAP_MAX && full == 6, "full at %d icons: the rest not kept (n %d, refused %d)", OB_SNAP_MAX, s.n, full);
    CHECK(ob_snap_set(&s, "DH1", "Many/5", OB_SNAP_DRAWER, 9, 9, 9, 9) == 1, "full: one already kept can still move");
    ob_snap_free(&s);
    CHECK(s.n == 0 && !s.e, "freed");
}

/* Which places OpenBench keeps when it starts (Salvor's list, item 53: Disk.info places are kept only when OpenBench
 * put them there) */
static void honour(void)
{
    ob_snap s;
    int gx = -1, gy = -1;
    ob_snap_init(&s);
    parse(&s, "Work\t\tD\t20 30\t0 108\n"
              "System\tTools\tW\t110 22\t126 0\n"
              "System\tTools/OpenMeters\tD\t400 300\t512 216\n");
    CHECK(ob_snap_honour(&s, "Work", "", OB_SNAP_DESK, 20, 30, &gx, &gy) && gx == 0 && gy == 108,
          "a disk OpenBench snapshotted: back on its cell");
    CHECK(ob_snap_honour(&s, "work", "", OB_SNAP_DESK, 20, 30, &gx, &gy), "the disk's name in another case");
    CHECK(!ob_snap_honour(&s, "Work", "", OB_SNAP_DESK, 20, 31, &gx, &gy),
          "its Disk.info says another place (3.2.3 snapshotted it since): passed over");
    CHECK(!ob_snap_honour(&s, "Work", "", OB_SNAP_DESK, 21, 30, &gx, &gy), "another place across only: passed over");
    CHECK(!ob_snap_honour(&s, "Work", "", OB_SNAP_DESK, OB_SNAP_NOPOS, OB_SNAP_NOPOS, &gx, &gy),
          "unsnapshotted by 3.2.3 since: passed over");
    CHECK(!ob_snap_honour(&s, "DH2", "", OB_SNAP_DESK, 9, 74, &gx, &gy), "a 3.2.3 Disk.info place OpenBench never wrote");
    CHECK(!ob_snap_honour(&s, "System", "Prefs", OB_SNAP_DRAWER, 9, 74, &gx, &gy), "a 3.2.3 drawer icon's place");
    CHECK(ob_snap_honour(&s, "System", "Tools", OB_SNAP_DRAWER, 110, 22, &gx, &gy) && gx == 126 && gy == 0,
          "an icon in its drawer OpenBench snapshotted");
    CHECK(!ob_snap_honour(&s, "System", "Tools/OpenMeters", OB_SNAP_DRAWER, 400, 300, &gx, &gy),
          "snapshotted left out on the desktop: not a place in its drawer");
    CHECK(ob_snap_honour(&s, "System", "Tools/OpenMeters", OB_SNAP_DESK, 400, 300, &gx, &gy) && gx == 512,
          "left out on the desktop again: kept there");
    CHECK(!ob_snap_honour(&s, "System", "Tools", OB_SNAP_DESK, 110, 22, &gx, &gy), "a drawer's place isn't the desktop's");
    ob_snap_free(&s);
}

static void parse_text(void)
{
    ob_snap s, t;
    char buf[4096], small[40];
    long n, m;
    const ob_snap_ent *e;
    ob_snap_init(&s);
    ob_snap_init(&t);
    CHECK(parse(&s, "") == 0 && s.n == 0, "empty: nothing");
    CHECK(parse(&s, "# a comment\n\n#Work\t\tD\t1 2\t3 4\n") == 0, "comments and blank lines");
    CHECK(parse(&s, "Work\t\tD\t20 30\t0 108\r\nRAM Disk\t\tD\t-4 +6\t-10 0") == 2, "CRLF, signs, no last line's end");
    CHECK((e = ob_snap_find(&s, "RAM Disk", "")) && e->x == -4 && e->y == 6 && e->gx == -10, "a name with a space; signs");
    CHECK(parse(&s, "Work\t\tX\t1 2\t3 4\n"            /* neither D nor W */
                    "Work\t\tDW\t1 2\t3 4\n"           /* too long a kind */
                    "Work\t\tD\t1\t3 4\n"              /* one number */
                    "Work\t\tD\t1 2\t3\n"              /* one number */
                    "Work\t\tD\t1 2\t3 4 5\n"          /* one too many */
                    "Work\t\tD\t1 2 3 4\n"             /* no tab before the cell */
                    "Work\t\tD\t1 two\t3 4\n"          /* not a number */
                    "Work\t\tD\t99999999999 2\t3 4\n"  /* past a LONG */
                    "Work\t\tD\t1 2\t3 99999\n"        /* past a WORD */
                    "Work\tD\t1 2\t3 4\n"              /* a field missing */
                    "AVolumeNameMuchLongerThanThirtyOneCharacters\t\tD\t1 2\t3 4\n"
                    "\t\tD\t1 2\t3 4\n"                /* no volume */
                    "DH1\t\tD\t1 2\t3 4\n") == 1 && s.n == 1 && ob_snap_find(&s, "DH1", ""),
          "lines it can't read are passed over (%d kept)", s.n);
    CHECK(parse(&s, "Work\t\tD\t1 2\t3 4\nWork\t\tD\t5 6\t7 8\n") == 2 && s.n == 1 && ob_snap_find(&s, "Work", "")->x == 5,
          "a name twice: the later line");

    parse(&s, "Work\t\tD\t20 30\t0 108\nSystem\tTools/OpenMeters\tW\t130 60\t126 0\nRAM Disk\t\tD\t-4 6\t-10 0\n");
    n = ob_snap_text(&s, buf, sizeof buf);
    CHECK(n > 0 && n == (long)strlen(buf), "written whole (%ld)", n);
    CHECK(buf[0] == '#' && strstr(buf, "\nWork\t\tD\t20 30\t0 108\n") && strstr(buf, "\nSystem\tTools/OpenMeters\tW\t130 60\t126 0\n"),
          "its lines as read");
    CHECK(parse(&t, buf) == 3 && t.n == 3, "read back: all three");
    CHECK(ob_snap_find(&t, "RAM Disk", "")->gx == -10 && ob_snap_find(&t, "System", "Tools/OpenMeters")->where == OB_SNAP_DRAWER,
          "the same entries");
    m = ob_snap_text(&s, small, sizeof small);
    CHECK(m == n && strlen(small) == sizeof small - 1, "too small a buffer: the length it needs, what fits, ended");
    CHECK(ob_snap_text(&s, NULL, 0) == n, "the length alone");
    ob_snap_free(&t);
    ob_snap_init(&t);
    CHECK(ob_snap_text(&t, buf, sizeof buf) > 0 && parse(&s, buf) == 0, "an empty record: its header only");
    ob_snap_free(&s);
}

static void count_in(void)
{
    ob_snap s;
    ob_snap_init(&s);
    parse(&s, "System\tTools\tW\t1 1\t0 0\n"
              "System\tPrefs\tW\t1 1\t0 0\n"
              "System\tTools/OpenMeters\tW\t1 1\t0 0\n"
              "System\tTools/Commodities/Exchange\tW\t1 1\t0 0\n"
              "System\tTools/Left\tD\t1 1\t0 0\n"
              "System\t\tD\t1 1\t0 0\n"
              "Work\tTools/Other\tW\t1 1\t0 0\n");
    CHECK(ob_snap_count_in(&s, "System", "", OB_SNAP_DRAWER) == 2, "the volume's top: Tools and Prefs");
    CHECK(ob_snap_count_in(&s, "system", "tools", OB_SNAP_DRAWER) == 1, "Tools: OpenMeters only, case aside");
    CHECK(ob_snap_count_in(&s, "System", "Tools/Commodities", OB_SNAP_DRAWER) == 1, "a drawer in a drawer");
    CHECK(ob_snap_count_in(&s, "System", "Tool", OB_SNAP_DRAWER) == 0, "a name's start isn't the drawer");
    CHECK(ob_snap_count_in(&s, "System", "Fonts", OB_SNAP_DRAWER) == 0, "another drawer of the same length");
    CHECK(ob_snap_count_in(&s, "System", "Tools", OB_SNAP_DESK) == 1, "the desktop's, apart");
    CHECK(ob_snap_count_in(&s, "System", "Utilities", OB_SNAP_DRAWER) == 0, "a drawer with none");
    CHECK(ob_snap_count_in(&s, "Work", "Tools", OB_SNAP_DRAWER) == 1, "another volume's");
    ob_snap_free(&s);
}

/* Icons > Clean Up on the desktop: the desktop's snapshots of the icons it lays out go, and nothing else */
static void cleanup(void)
{
    ob_snap s;
    /* System:Tools is left out on the desktop too, with its place in its drawer's window snapshotted */
    const char *shown[] = { "Work:", "System:", "System:Tools/OpenMeters", "Ram Disk:", "System:Tools", NULL };
    unsigned long g;
    int gx, gy;
    ob_snap_init(&s);
    parse(&s, "Work\t\tD\t20 30\t0 0\n"
              "System\t\tD\t20 130\t0 108\n"
              "System\tTools/OpenMeters\tD\t400 300\t512 216\n"    /* left out on the desktop */
              "System\tTools\tW\t110 22\t126 0\n"                  /* an icon in a drawer's window */
              "DF0\t\tD\t20 230\t0 216\n");                        /* a disk not in a drive now */
    g = s.gen;
    CHECK(ob_snap_cleanup(&s, shown, 6) == 3, "the three on the desktop with snapshots");
    CHECK(s.gen != g, "a change, to be written");
    CHECK(!ob_snap_honour(&s, "Work", "", OB_SNAP_DESK, 20, 30, &gx, &gy), "Work goes on the grid now");
    CHECK(!ob_snap_find(&s, "System", "Tools/OpenMeters"), "the icon left out too");
    CHECK(ob_snap_honour(&s, "System", "Tools", OB_SNAP_DRAWER, 110, 22, &gx, &gy), "a drawer's icon keeps its place");
    CHECK(ob_snap_honour(&s, "DF0", "", OB_SNAP_DESK, 20, 230, &gx, &gy), "a disk Clean Up didn't lay out keeps its");
    g = s.gen;
    CHECK(ob_snap_cleanup(&s, shown, 6) == 0 && s.gen == g, "Clean Up again: nothing to write");
    CHECK(ob_snap_set(&s, "Work", "", OB_SNAP_DESK, 20, 30, 0, 0) == 1, "Snapshot after Clean Up keeps the new place");
    ob_snap_free(&s);
}

static void latin1(void)
{
    ob_snap s;
    ob_snap_init(&s);
    ob_snap_set(&s, "Gr\xd6\xdf""e", "\xc4rger", OB_SNAP_DRAWER, 1, 2, 3, 4);
    CHECK(ob_snap_find(&s, "gr\xf6\xdf""E", "\xe4RGER") != NULL, "Latin-1 letters, case aside");
    CHECK(!ob_snap_find(&s, "gr\xf7\xdf""E", "\xe4RGER"), "the division sign isn't a letter");
    ob_snap_free(&s);
}

int main(void)
{
    split();
    set_find_forget();
    honour();
    parse_text();
    count_in();
    cleanup();
    latin1();
    if (failures) {
        printf("ob_snap: %d of %d checks failed\n", failures, checks);
        return 1;
    }
    printf("ob_snap: all %d checks passed\n", checks);
    return 0;
}
