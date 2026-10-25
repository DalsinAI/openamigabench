/* ob_snap.c: Snapshot's record, the icon places OpenBench wrote (ob_snap.h).
 * MIT, Copyright (c) 2026 AmigaChrome Team. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ob_snap.h"

#define HEADER "# OpenBench's snapshots: the icon places OpenBench wrote into .info files (Icons > Snapshot).\n" \
               "# volume, path on it, D desktop or W drawer, the place in the .info, the cell on the grid\n"

void ob_snap_init(ob_snap *s)
{
    memset(s, 0, sizeof *s);
}

void ob_snap_free(ob_snap *s)
{
    free(s->e);
    s->e = NULL;
    s->n = s->cap = 0;
    s->gen++;
}

/* AmigaDOS's case: ASCII and Latin-1 letters */
static int fold(int c)
{
    c &= 0xFF;
    if ((c >= 'a' && c <= 'z') || (c >= 0xE0 && c <= 0xFE && c != 0xF7)) return c - 32;
    return c;
}

static int same(const char *a, const char *b)
{
    for (; *a && *b; a++, b++)
        if (fold((unsigned char)*a) != fold((unsigned char)*b)) return 0;
    return !*a && !*b;
}

static int index_of(const ob_snap *s, const char *vol, const char *path)
{
    int i;
    for (i = 0; i < s->n; i++)
        if (same(s->e[i].vol, vol) && same(s->e[i].path, path)) return i;
    return -1;
}

/* 1 when s fits in n bytes with its NUL and has no tab or line's end (the record's separators) */
static int fits(const char *s, size_t n)
{
    size_t k;
    for (k = 0; s[k]; k++)
        if (s[k] == '\t' || s[k] == '\r' || s[k] == '\n') return 0;
    return k < n;
}

const ob_snap_ent *ob_snap_find(const ob_snap *s, const char *vol, const char *path)
{
    int i = index_of(s, vol, path);
    return i >= 0 ? &s->e[i] : NULL;
}

int ob_snap_set(ob_snap *s, const char *vol, const char *path, int where, long x, long y, int gx, int gy)
{
    ob_snap_ent *e;
    int i;
    if (!vol[0] || !fits(vol, OB_SNAP_VOL) || !fits(path, OB_SNAP_PATH) ||
        (where != OB_SNAP_DESK && where != OB_SNAP_DRAWER) || x == OB_SNAP_NOPOS || y == OB_SNAP_NOPOS)
        return -1;
    if ((i = index_of(s, vol, path)) >= 0) {
        e = &s->e[i];
        if (e->where == where && e->x == x && e->y == y && e->gx == gx && e->gy == gy && !strcmp(e->vol, vol) &&
            !strcmp(e->path, path))
            return 0;
    } else {
        if (s->n >= OB_SNAP_MAX) return -1;
        if (s->n == s->cap) {
            int cap = s->cap ? s->cap * 2 : 16;
            ob_snap_ent *ne;
            if (cap > OB_SNAP_MAX) cap = OB_SNAP_MAX;
            if (!(ne = realloc(s->e, (size_t)cap * sizeof *ne))) return -1;
            s->e = ne;
            s->cap = cap;
        }
        e = &s->e[s->n++];
    }
    memset(e, 0, sizeof *e);
    strcpy(e->vol, vol);                    /* as the disk names it now */
    strcpy(e->path, path);
    e->where = (char)where;
    e->x = x;
    e->y = y;
    e->gx = gx;
    e->gy = gy;
    s->gen++;
    return 1;
}

int ob_snap_forget(ob_snap *s, const char *vol, const char *path)
{
    int i = index_of(s, vol, path);
    if (i < 0) return 0;
    memmove(&s->e[i], &s->e[i + 1], (size_t)(s->n - i - 1) * sizeof s->e[0]);
    s->n--;
    s->gen++;
    return 1;
}

int ob_snap_honour(const ob_snap *s, const char *vol, const char *path, int where, long info_x, long info_y, int *gx,
                   int *gy)
{
    const ob_snap_ent *e = ob_snap_find(s, vol, path);
    if (!e || e->where != where || info_x == OB_SNAP_NOPOS || info_y == OB_SNAP_NOPOS || e->x != info_x ||
        e->y != info_y)
        return 0;
    *gx = e->gx;
    *gy = e->gy;
    return 1;
}

int ob_snap_count_in(const ob_snap *s, const char *vol, const char *dir, int where)
{
    size_t dl = strlen(dir);
    int i, n = 0;
    for (i = 0; i < s->n; i++) {
        const ob_snap_ent *e = &s->e[i];
        const char *slash = strrchr(e->path, '/');
        size_t pl = slash ? (size_t)(slash - e->path) : 0;   /* its drawer's part of the path */
        size_t k;
        if (e->where != where || !e->path[0] || pl != dl || !same(e->vol, vol)) continue;
        for (k = 0; k < dl && fold((unsigned char)e->path[k]) == fold((unsigned char)dir[k]); k++) ;
        if (k == dl) n++;
    }
    return n;
}

int ob_snap_split(const char *full, char *vol, int vn, char *path, int pn)
{
    const char *c = strchr(full, ':');
    size_t n;
    if (!c || c == full || vn <= 0 || pn <= 0) return 0;
    n = (size_t)(c - full);
    if (n >= (size_t)vn || strlen(c + 1) >= (size_t)pn) return 0;
    memcpy(vol, full, n);
    vol[n] = 0;
    strcpy(path, c + 1);
    n = strlen(path);
    if (n && path[n - 1] == '/') path[n - 1] = 0;   /* "Work:Tools/" is the drawer Tools */
    return 1;
}

int ob_snap_cleanup(ob_snap *s, const char *const *full, int n)
{
    int i, gone = 0;
    for (i = 0; i < n; i++) {
        char vol[OB_SNAP_VOL], path[OB_SNAP_PATH];
        int k;
        if (!full[i] || !ob_snap_split(full[i], vol, sizeof vol, path, sizeof path)) continue;
        if ((k = index_of(s, vol, path)) < 0 || s->e[k].where != OB_SNAP_DESK) continue;
        gone += ob_snap_forget(s, vol, path);
    }
    return gone;
}

/* A number at *p, up to a separator: 1 and *out, the text moved on */
static int number(const char **p, const char *end, long *out)
{
    const char *q = *p;
    unsigned long v = 0;
    int neg = 0, digits = 0;
    while (q < end && *q == ' ') q++;
    if (q < end && (*q == '-' || *q == '+')) neg = *q++ == '-';
    for (; q < end && *q >= '0' && *q <= '9'; q++, digits++) {
        v = v * 10 + (unsigned long)(*q - '0');
        if (v > 2147483647UL) return 0;     /* past a LONG's room */
    }
    if (!digits) return 0;
    *out = neg ? -(long)v : (long)v;
    *p = q;
    return 1;
}

/* A field up to the next tab: its text into out (n bytes), the text moved past the tab */
static int field(const char **p, const char *end, char *out, size_t n)
{
    const char *q = *p, *t = q;
    while (t < end && *t != '\t') t++;
    if (t >= end || (size_t)(t - q) >= n) return 0;
    memcpy(out, q, (size_t)(t - q));
    out[t - q] = 0;
    *p = t + 1;
    return 1;
}

/* Two numbers, "x y" */
static int pair(const char **p, const char *end, long *a, long *b)
{
    return number(p, end, a) && number(p, end, b);
}

int ob_snap_parse(ob_snap *s, const char *text, long len)
{
    const char *p = text, *end = text + (len > 0 ? len : 0);
    int read = 0;
    ob_snap_free(s);
    while (p < end) {
        const char *eol = p, *q = p;
        char vol[OB_SNAP_VOL], path[OB_SNAP_PATH], where[4];
        long x, y, gx, gy;
        while (eol < end && *eol != '\n') eol++;
        if (eol > p && eol[-1] == '\r') eol--;
        if (q < eol && *q != '#' && field(&q, eol, vol, sizeof vol) && field(&q, eol, path, sizeof path) &&
            field(&q, eol, where, sizeof where) && (where[0] == OB_SNAP_DESK || where[0] == OB_SNAP_DRAWER) &&
            !where[1] && pair(&q, eol, &x, &y) && q < eol && *q == '\t') {
            q++;
            if (pair(&q, eol, &gx, &gy)) {
                while (q < eol && *q == ' ') q++;
                if (q == eol && gx >= -32768 && gx <= 32767 && gy >= -32768 && gy <= 32767 &&
                    ob_snap_set(s, vol, path, where[0], x, y, (int)gx, (int)gy) >= 0)
                    read++;
            }
        }
        while (eol < end && *eol != '\n') eol++;
        p = eol < end ? eol + 1 : end;
    }
    return read;
}

long ob_snap_text(const ob_snap *s, char *out, long n)
{
    long k = 0;
    int i;
#define PUT(...) do { char line_[OB_SNAP_VOL + OB_SNAP_PATH + 80]; int m_ = snprintf(line_, sizeof line_, __VA_ARGS__); \
                      if (m_ > 0) { if (k + m_ < n) memcpy(out + k, line_, (size_t)m_); else if (k < n) { \
                      memcpy(out + k, line_, (size_t)(n - 1 - k)); } k += m_; } } while (0)
    PUT("%s", HEADER);
    for (i = 0; i < s->n; i++) {
        const ob_snap_ent *e = &s->e[i];
        PUT("%s\t%s\t%c\t%ld %ld\t%d %d\n", e->vol, e->path, e->where, e->x, e->y, e->gx, e->gy);
    }
#undef PUT
    if (n > 0) out[k < n ? k : n - 1] = 0;
    return k;
}
