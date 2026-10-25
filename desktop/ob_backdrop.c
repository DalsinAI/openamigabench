/* ob_backdrop.c: a disk's .backdrop, the icons left out on the desktop for good (ob_backdrop.h).
 * MIT, Copyright (c) 2026 AmigaChrome Team. */
#include <string.h>

#include "ob_backdrop.h"

/* AmigaDOS's case: ASCII and Latin-1 letters */
static int fold(int c)
{
    c &= 0xFF;
    if ((c >= 'a' && c <= 'z') || (c >= 0xE0 && c <= 0xFE && c != 0xF7)) return c - 32;
    return c;
}

/* 1 when a (an bytes) and b (bn bytes) are the same name */
static int same(const char *a, long an, const char *b, long bn)
{
    long i;
    if (an != bn) return 0;
    for (i = 0; i < an; i++)
        if (fold((unsigned char)a[i]) != fold((unsigned char)b[i])) return 0;
    return 1;
}

/* The line at *at: its start in *s and its length without its line feed or CR in *k, *at moved past it. 0 at the
 * end. */
static int next_line(const char *text, long len, long *at, const char **s, long *k)
{
    long e;
    if (!text || *at >= len) return 0;
    for (e = *at; e < len && text[e] != '\n'; e++)
        ;
    *s = text + *at;
    *k = e - *at;
    if (*k && (*s)[*k - 1] == '\r') (*k)--;
    *at = e < len ? e + 1 : len;
    return 1;
}

/* 1 when the line (s, k bytes) is an icon's: a colon, then a path (*p, *pk bytes) */
static int icon_line(const char *s, long k, const char **p, long *pk)
{
    if (k < 2 || s[0] != ':' || s[1] == ':') return 0;
    *p = s + 1;
    *pk = k - 1;
    return 1;
}

/* 1 when a line before end names the icon p (pk bytes) */
static int named_before(const char *text, long end, const char *p, long pk)
{
    long at = 0, k, qk;
    const char *s, *q;
    while (next_line(text, end, &at, &s, &k))
        if (icon_line(s, k, &q, &qk) && same(q, qk, p, pk)) return 1;
    return 0;
}

int ob_backdrop_parse(const char *text, long len, void (*each)(void *ctx, const char *path), void *ctx)
{
    long at = 0, start = 0, k, pk;
    const char *s, *p;
    char buf[OB_BACKDROP_PATH];
    int count = 0;
    for (; next_line(text, len, &at, &s, &k); start = at) {
        if (!icon_line(s, k, &p, &pk) || pk >= (long)sizeof buf || memchr(p, 0, (size_t)pk)) continue;
        if (named_before(text, start, p, pk)) continue;
        memcpy(buf, p, (size_t)pk);
        buf[pk] = 0;
        if (each) each(ctx, buf);
        count++;
    }
    return count;
}

int ob_backdrop_has(const char *text, long len, const char *path)
{
    long at = 0, k, pk, n = path ? (long)strlen(path) : 0;
    const char *s, *p;
    while (next_line(text, len, &at, &s, &k))
        if (icon_line(s, k, &p, &pk) && same(p, pk, path, n)) return 1;
    return 0;
}

/* k bytes of s at out + o, as many as fit in n with a NUL after them; o + k */
static long put(char *out, long n, long o, const char *s, long k)
{
    long i;
    for (i = 0; i < k; i++, o++)
        if (out && o < n - 1) out[o] = s[i];
    if (out && n > 0) out[o < n - 1 ? o : n - 1] = 0;
    return o;
}

long ob_backdrop_edit(const char *text, long len, const char *path, int add, char *out, long n, int *changed)
{
    long at = 0, start = 0, k, pk, o = 0, plen;
    const char *s, *p;
    int found = 0;
    *changed = 0;
    if (out && n > 0) out[0] = 0;
    if (!path || !(plen = (long)strlen(path)) || plen >= OB_BACKDROP_PATH || path[0] == ':' || strchr(path, '\n') ||
        strchr(path, '\r'))
        return -1;
    for (; next_line(text, len, &at, &s, &k); start = at) {
        if (!k) continue;                           /* an empty line */
        if (icon_line(s, k, &p, &pk)) {
            if (same(p, pk, path, plen)) {
                if (!add) {                         /* Put Away: every line naming it goes */
                    *changed = 1;
                    continue;
                }
                if (found) continue;                /* its second line */
                found = 1;
            } else if (named_before(text, start, p, pk)) {
                continue;                           /* another icon's second line */
            }
        }
        o = put(out, n, o, s, k);
        o = put(out, n, o, "\n", 1);
    }
    if (add && !found) {                            /* Leave Out: its line at the end */
        o = put(out, n, o, ":", 1);
        o = put(out, n, o, path, plen);
        o = put(out, n, o, "\n", 1);
        *changed = 1;
    }
    return o;
}
