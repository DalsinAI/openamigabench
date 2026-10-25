/* ob_notify's host test: the ENV: files OpenBench follows on one port (OpenBench 0.1.39), what each request's
 * nr_UserData asks for, that user data naming no file is left out, and that the labels' look still lays the icons
 * out again only when IconLabels or IconLayout changed (0.1.37's rule, with ob_show's ob_seen).
 *   test_ob_notify
 * MIT, Copyright (c) 2026 AmigaChrome Team. */
#include <stdio.h>
#include <string.h>

#include "ob_notify.h"
#include "ob_show.h"

static int failures, checks;

#define CHECK(c, ...) do { checks++; if (!(c)) { failures++; printf("FAIL %s:%d: ", __FILE__, __LINE__); \
    printf(__VA_ARGS__); printf("\n"); } } while (0)

/* What the messages of these files (0-ended) ask for */
static unsigned todo_of(const unsigned long *files)
{
    unsigned changed = 0;
    for (; *files; files++) changed = ob_notify_add(changed, *files);
    return ob_notify_todo(changed);
}

/* Each file on its own: what its own signal did up to 0.1.38 */
static void each(void)
{
    unsigned long f[2] = { 0, 0 };
    f[0] = OB_FILE_WBPREFS;
    CHECK(todo_of(f) == OB_DO_WBPREFS, "Workbench.prefs: read again");
    f[0] = OB_FILE_PATTERN;
    CHECK(todo_of(f) == OB_DO_PICTURE, "WBPattern.prefs: the picture");
    f[0] = OB_FILE_FONT;
    CHECK(todo_of(f) == OB_DO_FONT, "font.prefs: the font looked at later");
    f[0] = OB_FILE_LABELS;
    CHECK(todo_of(f) == OB_DO_ICONS, "IconLabels: the icons' look");
    f[0] = OB_FILE_LAYOUT;
    CHECK(todo_of(f) == OB_DO_ICONS, "IconLayout: the icons' look");
    f[0] = OB_FILE_DESKICONS;
    CHECK(todo_of(f) == OB_DO_ICONS, "DesktopIcons: the same look (it shared IconLabels' signal)");
}

/* Several messages before OpenBench wakes: one look each, whatever their number and order */
static void together(void)
{
    unsigned long all[] = { OB_FILE_DESKICONS, OB_FILE_FONT, OB_FILE_LAYOUT, OB_FILE_PATTERN, OB_FILE_LABELS,
                            OB_FILE_WBPREFS, 0 };
    unsigned long twice[] = { OB_FILE_PATTERN, OB_FILE_PATTERN, OB_FILE_PATTERN, 0 };
    unsigned long icons[] = { OB_FILE_LABELS, OB_FILE_LAYOUT, OB_FILE_DESKICONS, OB_FILE_LABELS, 0 };
    unsigned long none[] = { 0 };
    CHECK(todo_of(all) == (OB_DO_WBPREFS | OB_DO_PICTURE | OB_DO_FONT | OB_DO_ICONS), "all six: all four");
    CHECK(todo_of(twice) == OB_DO_PICTURE, "the same file three times: once");
    CHECK(todo_of(icons) == OB_DO_ICONS, "the icons' three files: one look");
    CHECK(todo_of(none) == 0, "no message: nothing");
    CHECK(ob_notify_add(ob_notify_add(0, OB_FILE_FONT), OB_FILE_FONT) == ob_notify_add(0, OB_FILE_FONT),
          "a file again changes nothing");
}

/* User data naming no file (another request's, or a message that isn't ours) is left out */
static void strangers(void)
{
    unsigned c = ob_notify_add(0, OB_FILE_LAYOUT);
    CHECK(ob_notify_add(0, 0) == 0, "0: no file");
    CHECK(ob_notify_add(0, OB_FILE_LAST + 1) == 0, "past the last: no file");
    CHECK(ob_notify_add(0, 31) == 0 && ob_notify_add(0, 32) == 0 && ob_notify_add(0, 64) == 0,
          "a shift's width or more: no file, and no wrap");
    CHECK(ob_notify_add(0, 0xffffffffUL) == 0, "all ones: no file");
    CHECK(ob_notify_add(c, 0) == c && ob_notify_add(c, 99) == c, "the files seen so far kept");
    CHECK(ob_notify_todo(1u) == 0 && ob_notify_todo(1u << (OB_FILE_LAST + 1)) == 0,
          "bits that are no file ask for nothing");
}

/* 0.1.37's rule kept: the icons are laid out again only when IconLabels or IconLayout changed, not for DesktopIcons
 * alone (the look is ob_seen_changed, as iconprefs_moved calls it) */
static void relayout(void)
{
    ob_seen s;
    unsigned long dic[] = { OB_FILE_DESKICONS, 0 };
    unsigned long lbl[] = { OB_FILE_LABELS, 0 };
    unsigned long lay[] = { OB_FILE_LAYOUT, OB_FILE_DESKICONS, 0 };
    ob_seen_init(&s);
    CHECK(ob_seen_changed(&s, "close", "cell=auto"), "the start's look");
    CHECK((todo_of(dic) & OB_DO_ICONS) && !ob_seen_changed(&s, "close", "cell=auto"),
          "No icons turned on: looked at, nothing laid out");
    CHECK((todo_of(lbl) & OB_DO_ICONS) && ob_seen_changed(&s, "below", "cell=auto"), "IconLabels changed: laid out");
    CHECK((todo_of(lbl) & OB_DO_ICONS) && !ob_seen_changed(&s, "below", "cell=auto"),
          "IconLabels written again the same (Save after Use): nothing laid out");
    CHECK((todo_of(lay) & OB_DO_ICONS) && ob_seen_changed(&s, "below", "cell=48"),
          "IconLayout and DesktopIcons together: laid out once");
}

int main(void)
{
    each();
    together();
    strangers();
    relayout();
    printf("ob_notify: %d checks, %d failed\n", checks, failures);
    return failures ? 1 : 0;
}
