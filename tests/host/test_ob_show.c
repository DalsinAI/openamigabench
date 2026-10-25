/* ob_show's host test: the desktop's No icons (OpenBench 0.1.37), what each way of changing it does (the variable's
 * notify, Window > No icons, Icons > Clean Up), that OpenBench's own writes coming back change nothing, and the
 * shared signal's look at IconLabels and IconLayout.
 *   test_ob_show
 * MIT, Copyright (c) 2026 AmigaChrome Team. */
#include <stdio.h>
#include <string.h>

#include "ob_show.h"

static int failures, checks;

#define CHECK(c, ...) do { checks++; if (!(c)) { failures++; printf("FAIL %s:%d: ", __FILE__, __LINE__); \
    printf(__VA_ARGS__); printf("\n"); } } while (0)

static void start(void)
{
    ob_show s;
    ob_show_init(&s, 0);
    CHECK(ob_show_icons(&s) && !ob_show_checked(&s), "no variable: the icons, unchecked");
    ob_show_init(&s, 1);
    CHECK(!ob_show_icons(&s) && ob_show_checked(&s), "\"none\" at the start: no icons, checked");
    ob_show_init(&s, 7);
    CHECK(s.none == 1, "any true value is no icons");
}

/* OpenPrefs' Icons page (Use, Save, Test, Cancel after a Test) through the notify */
static void from_prefs(void)
{
    ob_show s;
    ob_show_init(&s, 0);
    CHECK(ob_show_set(&s, 0) == OB_SHOW_KEEP, "the icons again (Use with no change): nothing");
    CHECK(ob_show_set(&s, 1) == OB_SHOW_HIDE && !ob_show_icons(&s), "none: hidden");
    CHECK(ob_show_checked(&s), "the menu's checkmark follows");
    CHECK(ob_show_set(&s, 1) == OB_SHOW_KEEP, "none again (Save after Use): nothing");
    CHECK(ob_show_set(&s, 0) == OB_SHOW_BACK && ob_show_icons(&s), "show (Cancel after a Test): back");
    CHECK(!ob_show_checked(&s), "unchecked");
    CHECK(ob_show_set(&s, 2) == OB_SHOW_HIDE && s.none == 1, "a true value other than 1 hides");
}

/* Window > No icons; the write comes back through the notify */
static void from_menu(void)
{
    ob_show s;
    int w = -9;
    ob_show_init(&s, 0);
    CHECK(ob_show_menu(&s, 1, &w) == OB_SHOW_HIDE && w == 1, "checked: none written, hidden");
    CHECK(ob_show_set(&s, 1) == OB_SHOW_KEEP, "its own write back: nothing");
    w = -9;
    CHECK(ob_show_menu(&s, 0, &w) == OB_SHOW_BACK && w == 0, "unchecked: show written, back");
    CHECK(ob_show_set(&s, 0) == OB_SHOW_KEEP, "its own write back: nothing");
    w = -9;
    /* the page changed it between (the checkmark was set to match), then the menu again */
    CHECK(ob_show_set(&s, 1) == OB_SHOW_HIDE, "the page: hidden");
    CHECK(ob_show_menu(&s, 0, &w) == OB_SHOW_BACK && w == 0, "the menu after the page: back, show written");
    w = -9;
    CHECK(ob_show_menu(&s, 0, &w) == OB_SHOW_KEEP && w == 0, "unchecked when shown already: written, nothing to do");
}

/* Icons > Clean Up */
static void cleanup(void)
{
    ob_show s;
    int w = -9;
    ob_show_init(&s, 0);
    CHECK(ob_show_cleanup(&s, &w) == OB_SHOW_KEEP && w == -1, "icons shown: nothing written, nothing else");
    CHECK(ob_show_icons(&s), "still shown");
    ob_show_init(&s, 1);
    w = -9;
    CHECK(ob_show_cleanup(&s, &w) == OB_SHOW_BACK && w == 0, "No icons on: show written, back");
    CHECK(ob_show_icons(&s) && !ob_show_checked(&s), "shown, unchecked");
    CHECK(ob_show_set(&s, 0) == OB_SHOW_KEEP, "its own write back: nothing");
    w = -9;
    CHECK(ob_show_cleanup(&s, &w) == OB_SHOW_KEEP && w == -1, "Clean Up again: nothing to write");
}

/* the label signal, shared with ENV:Sys/DesktopIcons */
static void seen(void)
{
    ob_seen s;
    char longer[300];
    ob_seen_init(&s);
    CHECK(ob_seen_changed(&s, NULL, NULL), "the first look is a change, even with no variables");
    CHECK(!ob_seen_changed(&s, NULL, NULL), "still none: no change");
    CHECK(ob_seen_changed(&s, "close", NULL), "IconLabels made");
    CHECK(!ob_seen_changed(&s, "close", NULL), "the same: no change (DesktopIcons' signal)");
    CHECK(ob_seen_changed(&s, "close", "cell=auto snap=on"), "IconLayout made");
    CHECK(!ob_seen_changed(&s, "close", "cell=auto snap=on"), "the same again");
    CHECK(ob_seen_changed(&s, "close 3", "cell=auto snap=on"), "IconLabels changed");
    CHECK(ob_seen_changed(&s, "close 3", "cell=48 snap=on"), "IconLayout changed");
    CHECK(ob_seen_changed(&s, "close 3", ""), "IconLayout emptied");
    CHECK(ob_seen_changed(&s, "close 3", NULL), "an empty variable and none differ");
    CHECK(ob_seen_changed(&s, NULL, NULL), "IconLabels deleted");
    CHECK(ob_seen_changed(&s, "", NULL), "and made empty");
    CHECK(!ob_seen_changed(&s, "", NULL), "empty again: no change");
    /* longer than what is kept: the first OB_SEEN_MAX - 1 characters count, and nothing is overrun */
    memset(longer, 'a', sizeof longer - 1);
    longer[sizeof longer - 1] = 0;
    CHECK(ob_seen_changed(&s, longer, NULL), "a long one");
    CHECK(s.labels[OB_SEEN_MAX - 1] == 0 && strlen(s.labels) == OB_SEEN_MAX - 1, "kept to its size");
    longer[OB_SEEN_MAX + 10] = 'b';
    CHECK(!ob_seen_changed(&s, longer, NULL), "a change past what is kept isn't seen (OpenBench reads less)");
    longer[5] = 'b';
    CHECK(ob_seen_changed(&s, longer, NULL), "a change within it is");
}

int main(void)
{
    start();
    from_prefs();
    from_menu();
    cleanup();
    seen();
    printf("ob_show: %d checks, %d failed\n", checks, failures);
    return failures ? 1 : 0;
}
