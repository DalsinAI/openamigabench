/* icon_core's host test: reads every icon icon_fixtures.py made, prints
 * what it found in the fixtures' dump format, and checks that each icon
 * written back reads the same (and, where the fixture says so, is the
 * same bytes). Then ic_inflate on deflate.bin's streams, the tool type
 * calls and the run-length coder.
 *   test_icon_core FIXTURE_DIR   (run.sh compares the dumps)
 * MIT, Copyright (c) 2026 Dalsin Limited. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "icon_core.h"

static int failures;

#define CHECK(c, ...) do { if (!(c)) { failures++; printf("FAIL %s:%d: ", __FILE__, __LINE__); \
    printf(__VA_ARGS__); printf("\n"); } } while (0)

static void *t_alloc(struct ic_env *e, uint32_t n) { (void)e; return calloc(1, n ? n : 1); }
static void t_free(struct ic_env *e, void *p) { (void)e; free(p); }
static struct ic_env env = { t_alloc, t_free };

static uint8_t *load(const char *path, uint32_t *len)
{
    FILE *f = fopen(path, "rb");
    uint8_t *buf;
    long n;
    if (!f)
        return NULL;
    fseek(f, 0, SEEK_END);
    n = ftell(f);
    fseek(f, 0, SEEK_SET);
    buf = malloc(n ? (size_t)n : 1);
    if (fread(buf, 1, (size_t)n, f) != (size_t)n) {
        fclose(f);
        free(buf);
        return NULL;
    }
    fclose(f);
    *len = (uint32_t)n;
    return buf;
}

/* A place: IC_NO_POSITION as the fixtures print it, unsigned. */
static long long place(int32_t v)
{
    return (uint32_t)v == IC_NO_POSITION ? (long long)IC_NO_POSITION : (long long)v;
}

/* The dump, as icon_fixtures.py writes its .want files. */
static void dump(FILE *o, const struct ic_icon *ic)
{
    int k;
    char **t;
    fprintf(o, "formats 0x%x\n", (unsigned)ic->formats);
    fprintf(o, "type %d stack %ld x %lld y %lld\n", ic->type, (long)ic->stacksize, place(ic->currentx),
            place(ic->currenty));
    fprintf(o, "gadget %d %d 0x%x %lu\n", ic->gwidth, ic->gheight, ic->gflags, (unsigned long)ic->guserdata);
    fprintf(o, "tool %s\n", ic->defaulttool ? ic->defaulttool : "-");
    if (ic->tooltypes) {
        int n = 0;
        for (t = ic->tooltypes; *t; t++)
            n++;
        fprintf(o, "tooltypes %d\n", n);
        for (t = ic->tooltypes; *t; t++)
            fprintf(o, " tt %s\n", *t);
    } else {
        fprintf(o, "tooltypes -\n");
    }
    fprintf(o, "toolwin %s\n", ic->toolwindow ? ic->toolwindow : "-");
    if (ic->drawer)
        fprintf(o, "drawer %d %d %d %d %ld %ld 0x%lx %d\n", ic->drawer->left, ic->drawer->top, ic->drawer->width,
                ic->drawer->height, (long)ic->drawer->currentx, (long)ic->drawer->currenty,
                (unsigned long)ic->drawer->flags, ic->drawer->viewmodes);
    else
        fprintf(o, "drawer -\n");
    for (k = 0; k < 2; k++) {
        const struct ic_planar *p = ic->planar[k];
        if (p)
            fprintf(o, "planar%d %d %d %d %08lx\n", k, p->width, p->height, p->depth,
                    (unsigned long)ic_crc32(0, p->data, p->size));
    }
    if (ic->has_face && !(ic->formats & IC_FMT_PNG))
        fprintf(o, "face %d %d frameless %d aspect %d\n", ic->pal[0] ? ic->pal[0]->width : 0,
                ic->pal[0] ? ic->pal[0]->height : 0, ic->frameless, ic->aspect);
    for (k = 0; k < 2; k++) {
        const struct ic_palpic *p = ic->pal[k];
        if (!p)
            continue;
        fprintf(o, "pal%d %d %d %d %d %08lx ", k, p->width, p->height, p->ncolours, p->transparent,
                (unsigned long)ic_crc32(0, p->pixels, (uint32_t)p->width * p->height));
        if (p->ncolours)
            fprintf(o, "%08lx\n", (unsigned long)ic_crc32(0, p->palette, p->ncolours * 3u));
        else
            fprintf(o, "-\n");
    }
    for (k = 0; k < 2; k++) {
        const struct ic_argb *a = ic->argb[k];
        if (a)
            fprintf(o, "argb%d %d %d %08lx\n", k, a->width, a->height,
                    (unsigned long)ic_crc32(0, a->argb, (uint32_t)a->width * a->height * 4));
    }
}

static char *dump_string(const struct ic_icon *ic)
{
    char *buf = NULL;
    size_t n = 0;
    FILE *o = open_memstream(&buf, &n);
    dump(o, ic);
    fclose(o);
    return buf;
}

static void test_file(const char *dir, const char *name, int exact)
{
    char path[512];
    uint8_t *buf, *out, *out2;
    uint32_t len, size, size2;
    struct ic_icon *ic = NULL, *ic2 = NULL, *fields = NULL;
    char *d1, *d2;
    FILE *o;
    int r;

    snprintf(path, sizeof path, "%s/%s.info", dir, name);
    buf = load(path, &len);
    CHECK(buf, "%s: can't load", name);
    if (!buf)
        return;
    r = ic_read(&env, buf, len, 0, &ic);
    CHECK(r == IC_OK, "%s: ic_read %d", name, r);
    if (r != IC_OK) {
        free(buf);
        return;
    }
    snprintf(path, sizeof path, "%s/%s.got", dir, name);
    o = fopen(path, "w");
    dump(o, ic);
    fclose(o);

    /* written back, it reads the same */
    size = ic_write(&env, ic, NULL, 0);
    out = malloc(size);
    size2 = ic_write(&env, ic, out, size);
    CHECK(size == size2, "%s: sizes %u %u", name, size, size2);
    r = ic_read(&env, out, size, 0, &ic2);
    CHECK(r == IC_OK, "%s: reading it back: %d", name, r);
    if (r == IC_OK) {
        d1 = dump_string(ic);
        d2 = dump_string(ic2);
        CHECK(strcmp(d1, d2) == 0, "%s: written back, it reads differently:\n%s---\n%s", name, d1, d2);
        free(d1);
        free(d2);
        ic_free(&env, ic2);
    }
    if (exact)
        CHECK(size == len && memcmp(out, buf, len) == 0, "%s: written back, %u bytes, not the same as its %u",
              name, size, len);
    /* too small a buffer: the size is still told, nothing past cap written */
    if (size > 10) {
        uint8_t *small = malloc(size);
        memset(small, 0xA5, size);
        CHECK(ic_write(&env, ic, small, size / 2) == size, "%s: size with a small buffer", name);
        CHECK(small[size / 2] == 0xA5 && small[size - 1] == 0xA5, "%s: wrote past cap", name);
        free(small);
    }
    free(out);

    /* palette pictures written anew (as after IconControlA): FACE and IMAGs
     * from our own coder read back the same pixels and palettes */
    if (ic->pal[0]) {
        struct ic_icon *again = NULL;
        int k;
        ic->pictures_changed = 1;
        size = ic_write(&env, ic, NULL, 0);
        out2 = malloc(size);
        ic_write(&env, ic, out2, size);
        ic->pictures_changed = 0;
        r = ic_read(&env, out2, size, 0, &again);
        CHECK(r == IC_OK && (again->formats & IC_FMT_OS35), "%s: new FORM ICON: %d", name, r);
        for (k = 0; r == IC_OK && k < 2; k++) {
            const struct ic_palpic *a = ic->pal[k], *b = again->pal[k];
            if (!a)
                continue;
            CHECK(b && a->width == b->width && a->height == b->height &&
                  memcmp(a->pixels, b->pixels, (size_t)a->width * a->height) == 0 && a->ncolours == b->ncolours &&
                  (!a->ncolours || memcmp(a->palette, b->palette, a->ncolours * 3u) == 0) &&
                  a->transparent == b->transparent, "%s: new FORM ICON, picture %d differs", name, k);
        }
        if (r == IC_OK)
            ic_free(&env, again);
        free(out2);
    }

    /* fields only: the same fields, no pictures */
    r = ic_read(&env, buf, len, IC_READ_NOPICTURES, &fields);
    CHECK(r == IC_OK, "%s: fields only: %d", name, r);
    if (r == IC_OK) {
        CHECK(!fields->pal[0] && !fields->argb[0], "%s: fields only decoded a picture", name);
        CHECK((fields->defaulttool == NULL) == (ic->defaulttool == NULL), "%s: fields only: tool", name);
        ic_free(&env, fields);
    }

    /* every shorter file is refused or read, never overrun (run with ASan) */
    {
        uint32_t cut;
        for (cut = 0; cut < len; cut += (len > 3000 ? 97 : 7)) {
            uint8_t *part = malloc(cut ? cut : 1);
            struct ic_icon *x = NULL;
            memcpy(part, buf, cut);
            if (ic_read(&env, part, cut, 0, &x) == IC_OK)
                ic_free(&env, x);
            free(part);
        }
    }
    ic_free(&env, ic);
    free(buf);
}

static void test_inflate(const char *dir)
{
    char path[512];
    uint8_t *buf;
    uint32_t len, pos = 0, n = 0;
    snprintf(path, sizeof path, "%s/deflate.bin", dir);
    buf = load(path, &len);
    CHECK(buf, "no deflate.bin");
    if (!buf)
        return;
    while (pos + 12 <= len) {
        uint32_t ulen = (uint32_t)buf[pos] << 24 | buf[pos + 1] << 16 | buf[pos + 2] << 8 | buf[pos + 3];
        uint32_t zlen = (uint32_t)buf[pos + 4] << 24 | buf[pos + 5] << 16 | buf[pos + 6] << 8 | buf[pos + 7];
        uint32_t c = (uint32_t)buf[pos + 8] << 24 | buf[pos + 9] << 16 | buf[pos + 10] << 8 | buf[pos + 11];
        uint8_t *dst = malloc(ulen + 1);
        uint32_t got = 0;
        int r = ic_inflate(buf + pos + 12, zlen, dst, ulen, &got);
        CHECK(r == IC_OK && got == ulen && ic_crc32(0, dst, got) == c, "inflate stream %u: %d, %u of %u", n, r, got, ulen);
        if (ulen > 1) {             /* too small a buffer is an error, not an overrun */
            r = ic_inflate(buf + pos + 12, zlen, dst, ulen - 1, &got);
            CHECK(r != IC_OK, "inflate stream %u fits in one byte less", n);
        }
        if (zlen > 4) {             /* cut short: an error */
            r = ic_inflate(buf + pos + 12, zlen / 2, dst, ulen, &got);
            CHECK(r != IC_OK || got < ulen || ulen == 0, "inflate stream %u cut short", n);
        }
        free(dst);
        pos += 12 + zlen;
        n++;
    }
    CHECK(n == 300, "inflate: %u streams", n);
    printf("inflate: %u streams\n", n);
    free(buf);
}

static void test_tooltypes(void)
{
    char *tt[] = { "DONOTWAIT", "FILE=S:Startup-Sequence", "Tool=OpenEdit", "(OFF)", "OFFX=1", "EMPTY=", NULL };
    char newname[64];
    CHECK(ic_find_tool_type(tt, "DONOTWAIT") && *ic_find_tool_type(tt, "DONOTWAIT") == 0, "a bare tool type");
    CHECK(ic_find_tool_type(tt, "donotwait") != NULL, "case");
    CHECK(strcmp(ic_find_tool_type(tt, "FILE"), "S:Startup-Sequence") == 0, "FILE=");
    CHECK(strcmp(ic_find_tool_type(tt, "TOOL"), "OpenEdit") == 0, "Tool=");
    CHECK(ic_find_tool_type(tt, "OFF") == NULL, "(OFF) is switched off");
    CHECK(ic_find_tool_type(tt, "FIL") == NULL, "a prefix is not a match");
    CHECK(strcmp(ic_find_tool_type(tt, "OFFX"), "1") == 0, "OFFX");
    CHECK(ic_find_tool_type(tt, "EMPTY") && *ic_find_tool_type(tt, "EMPTY") == 0, "EMPTY=");
    CHECK(ic_find_tool_type(NULL, "X") == NULL, "no array");

    CHECK(ic_match_tool_value("text|ascii", "text"), "first");
    CHECK(ic_match_tool_value("text|ascii", "ASCII"), "second, case");
    CHECK(!ic_match_tool_value("text|ascii", "asc"), "a prefix");
    CHECK(!ic_match_tool_value("text|ascii", "ascii2"), "longer");
    CHECK(ic_match_tool_value("a||b", ""), "an empty value");
    CHECK(!ic_match_tool_value("", "a"), "empty string");
    CHECK(ic_match_tool_value("one", "one"), "single");

    /* OS 3.2's answers, from iconprobe at 5c27ff1 */
    {
        static const char *const cases[][2] = {
            { "Disk", "Copy_of_Disk" },
            { "copy_of_Disk", "Copy_2_of_Disk" },
            { "copy_2_of_Disk", "Copy_3_of_Disk" },
            { "copy_99_of_Disk", "Copy_100_of_Disk" },
            { "copy_x_of_Disk", "Copy_2_of_x_of_Disk" },
            { "Copy_Of_Disk", "Copy_2_of_Disk" },
            { "copy_of_", "Copy_2_of_" },
            { "copy_0_of_Disk", "Copy_of_Disk" },
            { "copy_2_of_", "Copy_3_of_" },
            { "A very long name of thirty characters", "Copy_of_A very long name of th" },
            { "123456789012345678901234567890", "Copy_of_1234567890123456789012" },
            { "copy_123456789_of_x", "Copy_123456790_of_x" },
            { "", "Copy_of_" },
            /* iconprobe at d801676 */
            { "copy_ 2_of_x", "Copy_2_of_2_of_x" },
            { "copy_1234567890_of_x", "Copy_1234567891_of_x" },
            { "copy_4294967295_of_x", "Copy_0_of_x" },
        };
        unsigned k;
        for (k = 0; k < sizeof cases / sizeof cases[0]; k++) {
            CHECK(ic_bump_revision(newname, cases[k][0], 30) == 1, "%s", cases[k][0]);
            CHECK(strcmp(newname, cases[k][1]) == 0, "%s: %s", cases[k][0], newname);
        }
    }
    memset(newname, 'U', sizeof newname);
    CHECK(ic_bump_revision(newname, "Disk", 4) == 0 && newname[0] == 'U' && newname[4] == 'U', "untouched at 4");
    CHECK(ic_bump_revision(newname, "copy_of_Disk", 13) == 0 && newname[0] == 'U', "numbered, untouched at 13");
    CHECK(ic_bump_revision(newname, "copy_0_of_Disk", 13) == 1 && strcmp(newname, "Copy_of_Disk") == 0, "%s", newname);
    CHECK(ic_bump_revision(newname, "A very long name of thirty characters", 13) == 1
          && strcmp(newname, "Copy_of_A ver") == 0, "%s", newname);
    CHECK(ic_bump_revision(newname, "copy_99_of_Disk", 22) == 1 && strcmp(newname, "Copy_100_of_Disk") == 0, "%s", newname);

    {
        char *tt2[] = { "  SPACE=1", "a=b=c", "\tTAB=2", " SP AFTER =2", "TRAIL =3", NULL };
        CHECK(ic_find_tool_type(tt2, "SP AFTER") && strcmp(ic_find_tool_type(tt2, "SP AFTER"), "2") == 0, "space before '='");
        CHECK(ic_find_tool_type(tt2, "TRAIL") && strcmp(ic_find_tool_type(tt2, "TRAIL"), "3") == 0, "TRAIL");
        CHECK(ic_find_tool_type(tt2, "TAB") == NULL, "a tab is not skipped");
        CHECK(ic_find_tool_type(tt2, "SPACE") && strcmp(ic_find_tool_type(tt2, "SPACE"), "1") == 0, "leading spaces skipped");
        CHECK(ic_find_tool_type(tt2, "  SPACE") == NULL, "the name is not trimmed");
        CHECK(ic_find_tool_type(tt2, "a=b") && strcmp(ic_find_tool_type(tt2, "a=b"), "c") == 0, "'=' in the name");
        CHECK(ic_find_tool_type(tt2, "a") && strcmp(ic_find_tool_type(tt2, "a"), "b=c") == 0, "a");
    }
}

static void test_rle(void)
{
    unsigned bits;
    int round;
    for (bits = 1; bits <= 8; bits++)
        for (round = 0; round < 50; round++) {
            uint32_t n = (uint32_t)(rand() % 3000), i, packed;
            uint8_t *src = malloc(n + 1), *dst = malloc(n + 1), *z;
            for (i = 0; i < n; i++)
                src[i] = (uint8_t)((round & 1) ? (uint32_t)rand() : (i / (1 + (uint32_t)rand() % 9))) & ((1u << bits) - 1);
            packed = ic_rle_pack(src, n, bits, NULL, 0);
            z = malloc(packed + 1);
            CHECK(ic_rle_pack(src, n, bits, z, packed) == packed, "rle size");
            CHECK(ic_rle_unpack(z, packed, bits, dst, n) == IC_OK && memcmp(src, dst, n) == 0,
                  "rle round trip, %u bits, %u values", bits, n);
            if (n > 10)
                CHECK(ic_rle_unpack(z, packed / 2, bits, dst, n) != IC_OK, "rle cut short");
            free(src);
            free(dst);
            free(z);
        }
}

int main(int argc, char **argv)
{
    char path[512], name[256];
    FILE *m;
    int exact, n = 0;
    if (argc < 2) {
        fprintf(stderr, "test_icon_core FIXTURE_DIR\n");
        return 2;
    }
    snprintf(path, sizeof path, "%s/manifest.txt", argv[1]);
    m = fopen(path, "r");
    if (!m) {
        fprintf(stderr, "no %s\n", path);
        return 2;
    }
    while (fscanf(m, "%255s %d", name, &exact) == 2) {
        test_file(argv[1], name, exact);
        n++;
    }
    fclose(m);
    printf("icons: %d\n", n);
    test_inflate(argv[1]);
    test_tooltypes();
    test_rle();
    {
        struct ic_icon *x;
        static const uint8_t junk[100] = { 0xE3, 0x11 };
        CHECK(ic_read(&env, junk, sizeof junk, 0, &x) == IC_ERR_FORMAT, "a wrong magic");
        CHECK(ic_read(&env, junk, 1, 0, &x) == IC_ERR_FORMAT, "one byte");
    }
    printf(failures ? "%d FAILED\n" : "all passed\n", failures);
    return failures != 0;
}
