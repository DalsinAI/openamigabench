/* ob_strip's host test: OpenBench's root and strip (OpenBench 0.1.43) on the screens they meet, the strip covering
 * the title bar as OpenTitle's see-through bar needs (see_backed), the root not, and the two meeting with no gap
 * and no overlap.
 *   test_ob_strip
 * MIT, Copyright (c) 2026 AmigaChrome Team. */
#include <stdio.h>

#include "ob_strip.h"

static int failures, checks;

#define CHECK(c, ...) do { checks++; if (!(c)) { failures++; printf("FAIL %s:%d: ", __FILE__, __LINE__); \
    printf(__VA_ARGS__); printf("\n"); } } while (0)

static void screen(const char *name, long w, long h, long bar_height)
{
    ob_box r = ob_root_box(w, h, bar_height), s = ob_strip_box(w, bar_height);
    CHECK(ob_covers_bar(s, w, bar_height), "%s: the strip covers the bar", name);
    CHECK(!ob_covers_bar(r, w, bar_height), "%s: the root doesn't (it starts under the bar, as before)", name);
    CHECK(r.top == bar_height + 1 && r.height == h - bar_height - 1 && r.width == w && r.left == 0,
          "%s: the root where 0.1.42 had it", name);
    CHECK(s.top + s.height == r.top, "%s: the strip ends where the root starts (no gap, no overlap)", name);
    CHECK(s.height + r.height == h, "%s: the two cover the screen's height", name);
    CHECK(s.width == w && s.left == 0, "%s: the strip spans the screen", name);
}

int main(void)
{
    ob_box b;
    /* Instance-32 after #516: 1920x1080, a 24-row bar (BarOpacityTest: "bar 24 rows"); the RTG rule's 1920x1200;
     * PAL high resolution in topaz 8 */
    screen("1920x1080, bar 24 rows", 1920, 1080, 23);
    screen("1920x1200, bar 26 rows", 1920, 1200, 25);
    screen("PAL 640x256, bar 11 rows", 640, 256, 10);
    /* see_backed's own edges */
    b = ob_strip_box(1920, 23);
    b.height--;
    CHECK(!ob_covers_bar(b, 1920, 23), "a strip a row short doesn't cover the bar's last row");
    b = ob_strip_box(1920, 23);
    b.width--;
    CHECK(!ob_covers_bar(b, 1920, 23), "a strip a pixel narrow doesn't cover the bar's right end");
    b = ob_strip_box(1920, 23);
    b.left = 1;
    CHECK(!ob_covers_bar(b, 1920, 23), "a strip starting at x 1 doesn't cover the bar's left end");
    printf("ob_strip: %d checks, %d failed\n", checks, failures);
    return failures != 0;
}
