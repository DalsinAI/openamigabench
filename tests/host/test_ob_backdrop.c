/* ob_backdrop's host test: a disk's .backdrop (OpenBench 0.1.41), read, with Icons > Leave Out's line added and
 * Icons > Put Away's taken out: the lines, a CR, empty and other lines, a name twice, no file at all, and a path
 * that can't be a line.
 *   test_ob_backdrop
 * MIT, Copyright (c) 2026 AmigaChrome Team. */
#include <stdio.h>
#include <string.h>

#include "ob_backdrop.h"

static int failures, checks;

#define CHECK(c, ...) do { checks++; if (!(c)) { failures++; printf("FAIL %s:%d: ", __FILE__, __LINE__); \
    printf(__VA_ARGS__); printf("\n"); } } while (0)

/* the icons parse finds, "|" between them */
static char got[2048];

static void collect(void *ctx, const char *path)
{
    (void)ctx;
    if (got[0]) strcat(got, "|");
    strcat(got, path);
}

static int parse(const char *t)
{
    got[0] = 0;
    return ob_backdrop_parse(t, t ? (long)strlen(t) : 0, collect, NULL);
}

/* the text after an edit, in out; its length */
static char out[2048];

static long edit(const char *t, const char *path, int add, int *changed)
{
    return ob_backdrop_edit(t, t ? (long)strlen(t) : 0, path, add, out, sizeof out, changed);
}

static void reading(void)
{
    char longline[400];
    CHECK(parse(NULL) == 0 && !got[0], "no file: no icons");
    CHECK(parse("") == 0, "an empty file: no icons");
    CHECK(parse(":Tools/OpenMeters\n:Prefs/Font\n") == 2 && !strcmp(got, "Tools/OpenMeters|Prefs/Font"),
          "two lines, in order: [%s]", got);
    CHECK(parse(":Tools/OpenMeters") == 1 && !strcmp(got, "Tools/OpenMeters"), "no line feed at the end: [%s]", got);
    CHECK(parse(":Tools/OpenMeters\r\n:Prefs/Font\r\n") == 2 && !strcmp(got, "Tools/OpenMeters|Prefs/Font"),
          "CRs taken off: [%s]", got);
    CHECK(parse("\n\n:Shell\n\nnot an icon\n:\n::odd\n:Utilities/Clock\n") == 2 && !strcmp(got, "Shell|Utilities/Clock"),
          "empty lines, other text, a colon alone and two colons aren't icons: [%s]", got);
    CHECK(parse(":Tools/A\n:tools/a\n:Tools/B\n:TOOLS/A\n") == 2 && !strcmp(got, "Tools/A|Tools/B"),
          "a name twice, in any case, is one icon (the first line): [%s]", got);
    CHECK(parse(":\xC4/\xE9\n:\xE4/\xC9\n") == 1, "Latin-1 letters matched without regard to case: [%s]", got);
    memset(longline, 'a', sizeof longline);
    longline[0] = ':';
    memcpy(longline + sizeof longline - 9, "\n:Shell\n", 9);
    CHECK(parse(longline) == 1 && !strcmp(got, "Shell"), "a path too long to hand on is passed over: [%s]", got);
    CHECK(ob_backdrop_has(":Tools/OpenMeters\n", 18, "tools/openmeters"), "has: without regard to case");
    CHECK(!ob_backdrop_has(":Tools/OpenMeters\n", 18, "Tools"), "has: a whole line, not its start");
    CHECK(!ob_backdrop_has(NULL, 0, "Tools"), "has: no file");
}

static void leave_out(void)
{
    int changed;
    long n;
    n = edit(NULL, "Tools/OpenMeters", 1, &changed);
    CHECK(changed && n == 18 && !strcmp(out, ":Tools/OpenMeters\n"), "no file: made with its line: [%s]", out);
    n = edit("", "Tools/OpenMeters", 1, &changed);
    CHECK(changed && !strcmp(out, ":Tools/OpenMeters\n"), "an empty file: its line: [%s]", out);
    n = edit(":Shell\n:Prefs/Font\n", "Tools/OpenMeters", 1, &changed);
    CHECK(changed && n == (long)strlen(out) && !strcmp(out, ":Shell\n:Prefs/Font\n:Tools/OpenMeters\n"),
          "added at the end, the others kept in order: [%s]", out);
    n = edit(":Shell\n:Prefs/Font", "Tools/OpenMeters", 1, &changed);
    CHECK(changed && !strcmp(out, ":Shell\n:Prefs/Font\n:Tools/OpenMeters\n"),
          "the last line's feed put back first: [%s]", out);
    edit(":Shell\n:TOOLS/openmeters\n", "Tools/OpenMeters", 1, &changed);
    CHECK(!changed, "already there in another case: nothing to write");
    edit(":Shell\r\n\r\n\nsomething else\r\n:Shell\n", "Clock", 1, &changed);
    CHECK(changed && !strcmp(out, ":Shell\nsomething else\n:Clock\n"),
          "written with CRs and empty lines taken off, other text kept, a second line for a name gone: [%s]", out);
}

static void put_away(void)
{
    int changed;
    long n;
    n = edit(":Shell\n:Tools/OpenMeters\n:Prefs/Font\n", "tools/openmeters", 0, &changed);
    CHECK(changed && !strcmp(out, ":Shell\n:Prefs/Font\n"), "its line out, without regard to case: [%s]", out);
    n = edit(":Tools/OpenMeters\n:Shell\n:Tools/OpenMeters\r\n", "Tools/OpenMeters", 0, &changed);
    CHECK(changed && !strcmp(out, ":Shell\n"), "every line naming it out: [%s]", out);
    n = edit(":Tools/OpenMeters\n", "Tools/OpenMeters", 0, &changed);
    CHECK(changed && n == 0 && !out[0], "the last line out: nothing left, the file to be deleted (%ld)", n);
    n = edit(":Tools/OpenMeters\n\n\n", "Tools/OpenMeters", 0, &changed);
    CHECK(changed && n == 0, "only empty lines left: deleted too (%ld)", n);
    edit(":Shell\n", "Tools/OpenMeters", 0, &changed);
    CHECK(!changed, "not there: nothing to write");
    n = edit(NULL, "Tools/OpenMeters", 0, &changed);
    CHECK(!changed && n == 0, "no file: nothing to write");
    n = edit(":Tools/OpenMeters/Sub\n:Tools\n", "Tools/OpenMeters", 0, &changed);
    CHECK(!changed && !strcmp(out, ":Tools/OpenMeters/Sub\n:Tools\n"), "only a whole line goes: [%s]", out);
}

/* Leave Out three, Put Away the middle one, Leave Out it again: as a disk's file goes through a run */
static void a_run(void)
{
    char text[256] = "";
    const char *icons[3] = { "Tools/A", "Tools/B", "Prefs/C" };
    int changed, i;
    long n;
    for (i = 0; i < 3; i++) {
        n = ob_backdrop_edit(text, (long)strlen(text), icons[i], 1, out, sizeof out, &changed);
        CHECK(changed && n < (long)sizeof text, "Leave Out %s", icons[i]);
        strcpy(text, out);
    }
    CHECK(parse(text) == 3 && !strcmp(got, "Tools/A|Tools/B|Prefs/C"), "three left out: [%s]", got);
    ob_backdrop_edit(text, (long)strlen(text), "Tools/B", 0, out, sizeof out, &changed);
    strcpy(text, out);
    CHECK(changed && parse(text) == 2 && !strcmp(got, "Tools/A|Prefs/C"), "B put away: [%s]", got);
    ob_backdrop_edit(text, (long)strlen(text), "Tools/B", 1, out, sizeof out, &changed);
    strcpy(text, out);
    CHECK(changed && !strcmp(text, ":Tools/A\n:Prefs/C\n:Tools/B\n"), "B left out again, at the end: [%s]", text);
}

static void edges(void)
{
    char small[8], longpath[300];
    int changed = 7;
    long n;
    CHECK(edit(":Shell\n", "", 1, &changed) == -1 && !changed, "an empty path can't be a line");
    CHECK(edit(":Shell\n", ":Shell", 1, &changed) == -1, "nor one with a colon first");
    CHECK(edit(":Shell\n", "a\nb", 1, &changed) == -1 && edit(":Shell\n", "a\rb", 0, &changed) == -1,
          "nor one with a line's end in it");
    memset(longpath, 'a', sizeof longpath);
    longpath[sizeof longpath - 1] = 0;
    CHECK(edit(NULL, longpath, 1, &changed) == -1, "nor one too long");
    longpath[OB_BACKDROP_PATH] = 0;
    CHECK(edit(NULL, longpath, 1, &changed) == -1, "OB_BACKDROP_PATH characters: too long");
    longpath[OB_BACKDROP_PATH - 1] = 0;
    CHECK(edit(NULL, longpath, 1, &changed) == OB_BACKDROP_PATH + 1, "OB_BACKDROP_PATH - 1 (it and its NUL): a line");
    CHECK(parse(out) == 1 && strlen(got) == OB_BACKDROP_PATH - 1, "and read back");
    /* the length asked first, as snprintf: what fits, NUL-terminated */
    n = ob_backdrop_edit(":Shell\n", 7, "Clock", 1, NULL, 0, &changed);
    CHECK(n == 14 && changed, "the length with no room: %ld", n);
    n = ob_backdrop_edit(":Shell\n", 7, "Clock", 1, small, sizeof small, &changed);
    CHECK(n == 14 && !strcmp(small, ":Shell\n"), "what fits: [%s]", small);
}

int main(void)
{
    reading();
    leave_out();
    put_away();
    a_run();
    edges();
    if (failures) {
        printf("ob_backdrop: %d of %d checks failed\n", failures, checks);
        return 1;
    }
    printf("ob_backdrop: %d checks passed\n", checks);
    return 0;
}
