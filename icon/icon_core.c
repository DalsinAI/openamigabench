/* icon_core: reading and writing icons, in portable C. See icon_core.h.
 *
 * MIT, Copyright (c) 2026 Dalsin Limited. */
#include "icon_core.h"

/* ---- small helpers ---------------------------------------------------- */

static void ic_copy(void *d, const void *s, uint32_t n)
{
    uint8_t *dp = d;
    const uint8_t *sp = s;
    while (n--)
        *dp++ = *sp++;
}

static uint32_t ic_strlen(const char *s)
{
    uint32_t n = 0;
    while (s[n])
        n++;
    return n;
}

static uint16_t be16(const uint8_t *p) { return (uint16_t)((p[0] << 8) | p[1]); }
static uint32_t be32(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3];
}

static int ic_toupper(int c)
{
    /* ISO-8859-1, as utility.library's ToUpper: a-z and the accented
     * letters E0-FE, except F7 (division sign). */
    if ((c >= 'a' && c <= 'z') || (c >= 0xE0 && c <= 0xFE && c != 0xF7))
        return c - 0x20;
    return c;
}

/* ---- memory an icon owns ---------------------------------------------- */

struct ic_block {
    struct ic_block *next;
    uint32_t pad;                   /* keeps what follows 8-byte aligned on the host */
};

void *ic_own(struct ic_env *env, struct ic_icon *icon, uint32_t size)
{
    struct ic_block *b = env->alloc(env, (uint32_t)sizeof *b + size);
    if (!b)
        return NULL;
    b->next = icon->blocks;
    icon->blocks = b;
    return b + 1;
}

char *ic_own_string(struct ic_env *env, struct ic_icon *icon, const char *s)
{
    uint32_t n = ic_strlen(s) + 1;
    char *d = ic_own(env, icon, n);
    if (d)
        ic_copy(d, s, n);
    return d;
}

void ic_free(struct ic_env *env, struct ic_icon *icon)
{
    struct ic_block *b, *next;
    if (!icon)
        return;
    for (b = icon->blocks; b; b = next) {
        next = b->next;
        env->free(env, b);
    }
    env->free(env, icon);
}

/* ---- the tool type calls ---------------------------------------------- */

/* FindToolType: the tool type whose name (up to '=' or its end) is name,
 * compared without regard to case. As OS 3.2's (measured, iconprobe at
 * 5c27ff1): spaces before a tool type's name are skipped, name is not
 * trimmed, and name is compared whole, so "a=b" finds "a=b=c" and gives
 * "c". The value after the '=', or the empty string at the end when there
 * is none. */
char *ic_find_tool_type(char **tooltypes, const char *name)
{
    if (!tooltypes || !name)
        return NULL;
    for (; *tooltypes; tooltypes++) {
        char *t = *tooltypes;
        const char *n = name;
        while (*t == ' ')
            t++;
        while (*n && *t && ic_toupper((uint8_t)*t) == ic_toupper((uint8_t)*n)) {
            t++;
            n++;
        }
        if (*n == 0 && (*t == '=' || *t == 0))
            return *t == '=' ? t + 1 : t;
    }
    return NULL;
}

/* MatchToolValue: is value one of typestring's values, which are separated
 * by '|'? Without regard to case. */
int ic_match_tool_value(const char *typestring, const char *value)
{
    const char *s = typestring;
    if (!typestring || !value)
        return 0;
    for (;;) {
        const char *v = value;
        while (*s && *s != '|' && *v && ic_toupper((uint8_t)*s) == ic_toupper((uint8_t)*v)) {
            s++;
            v++;
        }
        if (*v == 0 && (*s == 0 || *s == '|'))
            return 1;
        while (*s && *s != '|')
            s++;
        if (*s == 0)
            return 0;
        s++;
    }
}

static int prefix(const char *s, const char *p)
{
    while (*p)
        if (ic_toupper((uint8_t)*s++) != ic_toupper((uint8_t)*p++))
            return 0;
    return 1;
}

/* BumpRevision, as OS 3.2's (measured, iconprobe at 5c27ff1): "name"
 * becomes "Copy_of_name". A name starting "copy_" (any case) is a copy:
 * the number after it (1 when there are no digits), then "_" and "of_"
 * when they are there, are taken off, and the rest gets "Copy_N+1_of_",
 * or "Copy_of_" when N+1 is 1. So "copy_x_of_a" becomes "Copy_2_of_x_of_a"
 * and "copy_0_of_a" becomes "Copy_of_a".
 * At most maxlen characters and a NUL. When maxlen is under IC_BUMP_MIN
 * (IC_BUMP_MIN_NUMBERED for a numbered copy) newname is left as it was
 * and 0 is returned, as 3.2's BumpRevisionLength does. */
int ic_bump_revision(char *newname, const char *oldname, uint32_t maxlen)
{
    char head[24];
    const char *rest = oldname;
    uint32_t n = 0, i;
    unsigned long num = 1;

    if (prefix(oldname, "copy_")) {
        const char *p = oldname + 5;
        unsigned long v = 0;
        int digits = 0;
        while (*p >= '0' && *p <= '9' && digits < 9) {
            v = v * 10 + (unsigned long)(*p++ - '0');
            digits++;
        }
        if (!digits)
            v = 1;
        else if (*p == '_')
            p++;
        if (prefix(p, "of_"))
            p += 3;
        num = v + 1;
        rest = p;
    }
    if (maxlen < (num == 1 ? IC_BUMP_MIN : IC_BUMP_MIN_NUMBERED))
        return 0;
    /* head: "Copy_of_" or "Copy_<num>_of_" */
    {
        const char *c = "Copy_";
        while (*c)
            head[n++] = *c++;
        if (num != 1) {
            char d[12];
            int k = 0;
            while (num) {
                d[k++] = (char)('0' + num % 10);
                num /= 10;
            }
            while (k)
                head[n++] = d[--k];
            head[n++] = '_';
        }
        c = "of_";
        while (*c)
            head[n++] = *c++;
    }
    i = 0;
    while (i < n && i < maxlen) {
        newname[i] = head[i];
        i++;
    }
    while (*rest && i < maxlen)
        newname[i++] = *rest++;
    newname[i] = 0;
    return 1;
}

/* ---- inflate (RFC 1951) ----------------------------------------------- */

struct inbits {
    const uint8_t *src;
    uint32_t len, pos;
    uint32_t acc;
    int n;
    int err;
};

static uint32_t getbits(struct inbits *b, int need)
{
    uint32_t v;
    while (b->n < need) {
        if (b->pos >= b->len) {
            b->err = 1;
            return 0;
        }
        b->acc |= (uint32_t)b->src[b->pos++] << b->n;
        b->n += 8;
    }
    v = b->acc & ((1UL << need) - 1);
    b->acc >>= need;
    b->n -= need;
    return v;
}

struct huff {
    uint16_t count[16];             /* codes of each length */
    uint16_t sym[288];              /* symbols, by code */
};

/* A canonical code from code lengths: 0 if usable. */
static int huff_build(struct huff *h, const uint8_t *lengths, int n)
{
    uint16_t offs[16];
    int i, left = 1;
    for (i = 0; i < 16; i++)
        h->count[i] = 0;
    for (i = 0; i < n; i++)
        h->count[lengths[i]]++;
    if (h->count[0] == n)
        return 0;                   /* no codes: usable if never read */
    for (i = 1; i < 16; i++) {
        left <<= 1;
        left -= h->count[i];
        if (left < 0)
            return -1;              /* too many codes */
    }
    offs[1] = 0;
    for (i = 1; i < 15; i++)
        offs[i + 1] = offs[i] + h->count[i];
    for (i = 0; i < n; i++)
        if (lengths[i])
            h->sym[offs[lengths[i]]++] = (uint16_t)i;
    return 0;
}

static int huff_decode(struct inbits *b, const struct huff *h)
{
    int code = 0, first = 0, index = 0, len;
    for (len = 1; len < 16; len++) {
        int count;
        code |= (int)getbits(b, 1);
        if (b->err)
            return -1;
        count = h->count[len];
        if (code - first < count)
            return h->sym[index + code - first];
        index += count;
        first = (first + count) << 1;
        code <<= 1;
    }
    return -1;
}

static const uint16_t len_base[29] = {
    3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
    35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258 };
static const uint8_t len_extra[29] = {
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
    3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0 };
static const uint16_t dist_base[30] = {
    1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193,
    257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577 };
static const uint8_t dist_extra[30] = {
    0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6,
    7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13 };
static const uint8_t cl_order[19] = {
    16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15 };

struct inflate_state {
    struct inbits b;
    uint8_t *dst;
    uint32_t dstlen, out;
    struct huff lit, dist;
    uint8_t lengths[320];
};

static int inflate_codes(struct inflate_state *s)
{
    for (;;) {
        int sym = huff_decode(&s->b, &s->lit);
        if (sym < 0)
            return IC_ERR_DATA;
        if (sym < 256) {
            if (s->out >= s->dstlen)
                return IC_ERR_DATA;
            s->dst[s->out++] = (uint8_t)sym;
        } else if (sym == 256) {
            return IC_OK;
        } else {
            uint32_t len, d;
            sym -= 257;
            if (sym >= 29)
                return IC_ERR_DATA;
            len = len_base[sym] + getbits(&s->b, len_extra[sym]);
            sym = huff_decode(&s->b, &s->dist);
            if (sym < 0 || sym >= 30)
                return IC_ERR_DATA;
            d = dist_base[sym] + getbits(&s->b, dist_extra[sym]);
            if (s->b.err || d > s->out || len > s->dstlen - s->out)
                return IC_ERR_DATA;
            while (len--) {
                s->dst[s->out] = s->dst[s->out - d];
                s->out++;
            }
        }
    }
}

static int inflate_fixed(struct inflate_state *s)
{
    int i;
    for (i = 0; i < 144; i++) s->lengths[i] = 8;
    for (; i < 256; i++) s->lengths[i] = 9;
    for (; i < 280; i++) s->lengths[i] = 7;
    for (; i < 288; i++) s->lengths[i] = 8;
    huff_build(&s->lit, s->lengths, 288);
    for (i = 0; i < 30; i++) s->lengths[i] = 5;
    huff_build(&s->dist, s->lengths, 30);
    return inflate_codes(s);
}

static int inflate_dynamic(struct inflate_state *s)
{
    uint32_t nlen, ndist, ncode, i;
    nlen = getbits(&s->b, 5) + 257;
    ndist = getbits(&s->b, 5) + 1;
    ncode = getbits(&s->b, 4) + 4;
    if (s->b.err || nlen > 286 || ndist > 30)
        return IC_ERR_DATA;
    for (i = 0; i < 19; i++)
        s->lengths[cl_order[i]] = i < ncode ? (uint8_t)getbits(&s->b, 3) : 0;
    if (s->b.err || huff_build(&s->lit, s->lengths, 19) != 0)
        return IC_ERR_DATA;
    i = 0;
    while (i < nlen + ndist) {
        int sym = huff_decode(&s->b, &s->lit);
        uint32_t rep;
        uint8_t v = 0;
        if (sym < 0)
            return IC_ERR_DATA;
        if (sym < 16) {
            s->lengths[i++] = (uint8_t)sym;
            continue;
        }
        if (sym == 16) {
            if (i == 0)
                return IC_ERR_DATA;
            v = s->lengths[i - 1];
            rep = 3 + getbits(&s->b, 2);
        } else if (sym == 17) {
            rep = 3 + getbits(&s->b, 3);
        } else {
            rep = 11 + getbits(&s->b, 7);
        }
        if (s->b.err || i + rep > nlen + ndist)
            return IC_ERR_DATA;
        while (rep--)
            s->lengths[i++] = v;
    }
    if (s->lengths[256] == 0)
        return IC_ERR_DATA;         /* no end-of-block code */
    if (huff_build(&s->lit, s->lengths, (int)nlen) != 0 ||
        huff_build(&s->dist, s->lengths + nlen, (int)ndist) != 0)
        return IC_ERR_DATA;
    return inflate_codes(s);
}

/* A raw deflate stream into dst. */
static int inflate_raw(struct inflate_state *s)
{
    int last;
    do {
        uint32_t type;
        last = (int)getbits(&s->b, 1);
        type = getbits(&s->b, 2);
        if (s->b.err)
            return IC_ERR_DATA;
        if (type == 0) {
            uint32_t len, nlen;
            s->b.acc = 0;           /* to the byte boundary */
            s->b.n = 0;
            if (s->b.pos + 4 > s->b.len)
                return IC_ERR_DATA;
            len = be16(s->b.src + s->b.pos);
            len = ((len & 0xFF) << 8) | (len >> 8);
            nlen = be16(s->b.src + s->b.pos + 2);
            nlen = ((nlen & 0xFF) << 8) | (nlen >> 8);
            s->b.pos += 4;
            if ((len ^ 0xFFFF) != nlen || s->b.pos + len > s->b.len || len > s->dstlen - s->out)
                return IC_ERR_DATA;
            ic_copy(s->dst + s->out, s->b.src + s->b.pos, len);
            s->out += len;
            s->b.pos += len;
        } else if (type == 1) {
            if (inflate_fixed(s) != IC_OK)
                return IC_ERR_DATA;
        } else if (type == 2) {
            if (inflate_dynamic(s) != IC_OK)
                return IC_ERR_DATA;
        } else {
            return IC_ERR_DATA;
        }
    } while (!last);
    return IC_OK;
}

/* The state is about 1.3 KB, too much for a Workbench tool's 4 KB stack,
 * so the library's callers take it from the env (new_state); only the
 * plain entries for the tests keep it on the stack. */
static int run_inflate(struct inflate_state *s, const uint8_t *src, uint32_t srclen,
                       uint8_t *dst, uint32_t dstlen, uint32_t *outlen)
{
    int r;
    s->b.src = src;
    s->b.len = srclen;
    s->b.pos = 0;
    s->b.acc = 0;
    s->b.n = 0;
    s->b.err = 0;
    s->dst = dst;
    s->dstlen = dstlen;
    s->out = 0;
    r = inflate_raw(s);
    if (outlen)
        *outlen = s->out;
    return r;
}

static uint32_t adler32(const uint8_t *p, uint32_t n)
{
    uint32_t a = 1, b = 0;
    while (n) {
        uint32_t k = n < 3800 ? n : 3800;
        n -= k;
        while (k--) {
            a += *p++;
            b += a;
        }
        a %= 65521;
        b %= 65521;
    }
    return (b << 16) | a;
}

/* A zlib stream (RFC 1950): its header, the deflate data, its Adler-32. */
static int run_zlib(struct inflate_state *s, const uint8_t *src, uint32_t srclen,
                    uint8_t *dst, uint32_t dstlen, uint32_t *outlen)
{
    int r;
    uint32_t used;
    if (srclen < 6 || (src[0] & 0x0F) != 8 || (src[0] >> 4) > 7 ||
        ((uint32_t)src[0] << 8 | src[1]) % 31 != 0 || (src[1] & 0x20))
        return IC_ERR_DATA;
    r = run_inflate(s, src + 2, srclen - 2, dst, dstlen, outlen);
    if (r != IC_OK)
        return r;
    used = 2 + s->b.pos;
    if (used + 4 <= srclen && be32(src + used) != adler32(dst, s->out))
        return IC_ERR_DATA;
    return IC_OK;
}

static struct inflate_state *new_state(struct ic_env *env)
{
    return env->alloc(env, (uint32_t)sizeof(struct inflate_state));
}

/* The plain entries, for the tests: the state on the stack. */
int ic_inflate(const uint8_t *src, uint32_t srclen, uint8_t *dst, uint32_t dstlen, uint32_t *outlen)
{
    struct inflate_state s;
    return run_inflate(&s, src, srclen, dst, dstlen, outlen);
}

int ic_zlib_inflate(const uint8_t *src, uint32_t srclen, uint8_t *dst, uint32_t dstlen, uint32_t *outlen)
{
    struct inflate_state s;
    return run_zlib(&s, src, srclen, dst, dstlen, outlen);
}

/* ---- CRC-32 (PNG chunks) ---------------------------------------------- */

uint32_t ic_crc32(uint32_t crc, const uint8_t *p, uint32_t n)
{
    crc = ~crc;
    while (n--) {
        int k;
        crc ^= *p++;
        for (k = 0; k < 8; k++)
            crc = (crc >> 1) ^ (0xEDB88320UL & (0UL - (crc & 1)));
    }
    return ~crc;
}

/* ---- the OS 3.5 run-length code --------------------------------------- */

/* A bit stream, first bit first: 8-bit controls and bits-wide values. */
int ic_rle_unpack(const uint8_t *src, uint32_t srclen, unsigned bits, uint8_t *dst, uint32_t count)
{
    uint32_t acc = 0, pos = 0, out = 0;
    int n = 0;
#define TAKE(w, v) do { while (n < (int)(w)) { if (pos >= srclen) return IC_ERR_DATA; \
        acc = (acc << 8) | src[pos++]; n += 8; } n -= (int)(w); \
        (v) = (acc >> n) & ((1UL << (w)) - 1); } while (0)
    if (bits < 1 || bits > 8)
        return IC_ERR_DATA;
    while (out < count) {
        uint32_t c, v;
        TAKE(8, c);
        if (c <= 127) {
            c++;
            while (c--) {
                TAKE(bits, v);
                if (out < count)
                    dst[out++] = (uint8_t)v;
            }
        } else if (c > 128) {
            c = 257 - c;
            TAKE(bits, v);
            while (c-- && out < count)
                dst[out++] = (uint8_t)v;
        }
    }
#undef TAKE
    return IC_OK;
}

struct outbits {
    uint8_t *dst;
    uint32_t cap, pos;
    uint32_t acc;
    int n;
};

static void putbits(struct outbits *o, uint32_t v, int w)
{
    o->acc = (o->acc << w) | (v & ((1UL << w) - 1));
    o->n += w;
    while (o->n >= 8) {
        o->n -= 8;
        if (o->dst && o->pos < o->cap)
            o->dst[o->pos] = (uint8_t)(o->acc >> o->n);
        o->pos++;
    }
}

/* Packs count values of bits each; returns the bytes it takes (written to
 * dst when dst is not NULL and they fit in cap). Runs of three or more
 * repeat, the rest are copied, at most 128 values a control. */
uint32_t ic_rle_pack(const uint8_t *src, uint32_t count, unsigned bits, uint8_t *dst, uint32_t cap)
{
    struct outbits o;
    uint32_t i = 0;
    o.dst = dst;
    o.cap = cap;
    o.pos = 0;
    o.acc = 0;
    o.n = 0;
    while (i < count) {
        uint32_t run = 1, start, k;
        while (i + run < count && run < 128 && src[i + run] == src[i])
            run++;
        if (run >= 3) {
            putbits(&o, 257 - run, 8);
            putbits(&o, src[i], (int)bits);
            i += run;
            continue;
        }
        start = i;
        while (i < count && i - start < 128) {
            if (i + 2 < count && src[i] == src[i + 1] && src[i] == src[i + 2])
                break;
            i++;
        }
        putbits(&o, i - start - 1, 8);
        for (k = start; k < i; k++)
            putbits(&o, src[k], (int)bits);
    }
    if (o.n)
        putbits(&o, 0, 8 - o.n);
    return o.pos;
}

/* ---- classic pictures -------------------------------------------------- */

void ic_planar_to_chunky(const struct ic_planar *p, uint8_t *dst)
{
    uint32_t row = ((uint32_t)(p->width + 15) >> 4) << 1;
    uint32_t x, y, d;
    for (y = 0; y < p->height; y++)
        for (x = 0; x < p->width; x++) {
            uint8_t v = 0;
            if (p->data)
                for (d = 0; d < p->depth; d++)
                    if (p->data[d * row * p->height + y * row + (x >> 3)] & (0x80 >> (x & 7)))
                        v |= (uint8_t)(1 << d);
            dst[y * p->width + x] = v;
        }
}

/* ---- PNG ---------------------------------------------------------------- */

static const uint8_t png_sig[8] = { 0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A };

static int is_png(const uint8_t *p, uint32_t len)
{
    int i;
    if (len < 8)
        return 0;
    for (i = 0; i < 8; i++)
        if (p[i] != png_sig[i])
            return 0;
    return 1;
}

static int is_id(const uint8_t *p, const char *id)
{
    return p[0] == (uint8_t)id[0] && p[1] == (uint8_t)id[1] && p[2] == (uint8_t)id[2] && p[3] == (uint8_t)id[3];
}

/* The length of one PNG (signature to IEND's CRC), or 0. */
static uint32_t png_length(const uint8_t *p, uint32_t len)
{
    uint32_t pos = 8;
    if (!is_png(p, len))
        return 0;
    while (pos + 12 <= len) {
        uint32_t size = be32(p + pos);
        if (size > len - pos - 12)
            return 0;
        if (is_id(p + pos + 4, "IEND"))
            return pos + 12 + size;
        pos += 12 + size;
    }
    return 0;
}

static uint8_t paeth(int a, int b, int c)
{
    int p = a + b - c;
    int pa = p > a ? p - a : a - p;
    int pb = p > b ? p - b : b - p;
    int pc = p > c ? p - c : c - p;
    if (pa <= pb && pa <= pc)
        return (uint8_t)a;
    return (uint8_t)(pb <= pc ? b : c);
}

/* Undoes one pass's filters in place: rows of 1 + rowbytes. */
static int unfilter(uint8_t *data, uint32_t rows, uint32_t rowbytes, uint32_t bpp)
{
    uint32_t y, x;
    uint8_t *prev = NULL;
    for (y = 0; y < rows; y++) {
        uint8_t *row = data + y * (rowbytes + 1);
        uint8_t ft = row[0];
        uint8_t *r = row + 1;
        for (x = 0; x < rowbytes; x++) {
            int a = x >= bpp ? r[x - bpp] : 0;
            int b = prev ? prev[x] : 0;
            int c = (prev && x >= bpp) ? prev[x - bpp] : 0;
            switch (ft) {
            case 0: break;
            case 1: r[x] = (uint8_t)(r[x] + a); break;
            case 2: r[x] = (uint8_t)(r[x] + b); break;
            case 3: r[x] = (uint8_t)(r[x] + ((a + b) >> 1)); break;
            case 4: r[x] = (uint8_t)(r[x] + paeth(a, b, c)); break;
            default: return IC_ERR_DATA;
            }
        }
        prev = r;
    }
    return IC_OK;
}

struct png_info {
    uint32_t width, height;
    uint8_t depth, ctype, interlace;
    uint32_t channels;
    const uint8_t *plte;
    uint32_t plte_n;
    const uint8_t *trns;
    uint32_t trns_n;
};

static uint32_t sample(const uint8_t *r, uint32_t i, uint32_t depth)
{
    if (depth == 8)
        return r[i];
    if (depth == 16)
        return r[i * 2];            /* the high byte */
    return (r[(i * depth) >> 3] >> (8 - depth - ((i * depth) & 7))) & ((1u << depth) - 1);
}

/* One decoded row's pixel x (of a pass) into ARGB at out. */
static void put_pixel(const struct png_info *pi, const uint8_t *r, uint32_t x, uint8_t *out)
{
    uint32_t a = 255, rr, g, b, v, c = pi->channels;
    uint32_t scale = pi->depth < 8 ? 255u / ((1u << pi->depth) - 1) : 1;
    switch (pi->ctype) {
    case 0:                         /* grey */
        v = sample(r, x, pi->depth);
        if (pi->trns && pi->trns_n >= 2) {
            uint32_t t = be16(pi->trns);
            uint32_t full = pi->depth == 16 ? be16(r + x * 2) : v;
            if (full == t)
                a = 0;
        }
        rr = g = b = v * scale;
        break;
    case 2:                         /* RGB */
        rr = sample(r, x * c, pi->depth);
        g = sample(r, x * c + 1, pi->depth);
        b = sample(r, x * c + 2, pi->depth);
        if (pi->trns && pi->trns_n >= 6) {
            if (pi->depth == 16) {
                if (be16(r + x * 6) == be16(pi->trns) && be16(r + x * 6 + 2) == be16(pi->trns + 2) &&
                    be16(r + x * 6 + 4) == be16(pi->trns + 4))
                    a = 0;
            } else if (rr == be16(pi->trns) && g == be16(pi->trns + 2) && b == be16(pi->trns + 4)) {
                a = 0;
            }
        }
        break;
    case 3:                         /* palette */
        v = sample(r, x, pi->depth);
        if (v < pi->plte_n) {
            rr = pi->plte[v * 3];
            g = pi->plte[v * 3 + 1];
            b = pi->plte[v * 3 + 2];
        } else {
            rr = g = b = 0;
        }
        if (pi->trns && v < pi->trns_n)
            a = pi->trns[v];
        break;
    case 4:                         /* grey and alpha */
        rr = g = b = sample(r, x * 2, pi->depth);
        a = sample(r, x * 2 + 1, pi->depth);
        break;
    default:                        /* 6: RGBA */
        rr = sample(r, x * 4, pi->depth);
        g = sample(r, x * 4 + 1, pi->depth);
        b = sample(r, x * 4 + 2, pi->depth);
        a = sample(r, x * 4 + 3, pi->depth);
        break;
    }
    out[0] = (uint8_t)a;
    out[1] = (uint8_t)rr;
    out[2] = (uint8_t)g;
    out[3] = (uint8_t)b;
}

static const uint8_t adam7[7][4] = {    /* x start, y start, x step, y step */
    { 0, 0, 8, 8 }, { 4, 0, 8, 8 }, { 0, 4, 4, 8 }, { 2, 0, 4, 4 },
    { 0, 2, 2, 4 }, { 1, 0, 2, 2 }, { 0, 1, 1, 2 } };

static uint32_t pass_rowbytes(const struct png_info *pi, uint32_t w)
{
    return (w * pi->channels * pi->depth + 7) >> 3;
}

int ic_png_decode(struct ic_env *env, struct ic_icon *owner, const uint8_t *png, uint32_t len, struct ic_argb **out)
{
    struct png_info pi;
    uint32_t pos = 8, idat_total = 0, raw_size, got, p, bpp;
    uint8_t *idat = NULL, *raw = NULL;
    struct ic_argb *pic;
    struct inflate_state *st;
    int r = IC_ERR_DATA, npass, k;

    *out = NULL;
    if (!is_png(png, len))
        return IC_ERR_FORMAT;
    for (k = 0; k < (int)sizeof pi; k++)
        ((uint8_t *)&pi)[k] = 0;
    /* the chunks: IHDR first, then what we need */
    while (pos + 12 <= len) {
        uint32_t size = be32(png + pos);
        const uint8_t *body = png + pos + 8;
        if (size > len - pos - 12)
            return IC_ERR_SHORT;
        if (is_id(png + pos + 4, "IHDR") && size >= 13) {
            pi.width = be32(body);
            pi.height = be32(body + 4);
            pi.depth = body[8];
            pi.ctype = body[9];
            pi.interlace = body[12];
        } else if (is_id(png + pos + 4, "PLTE")) {
            pi.plte = body;
            pi.plte_n = size / 3;
        } else if (is_id(png + pos + 4, "tRNS")) {
            pi.trns = body;
            pi.trns_n = size;
        } else if (is_id(png + pos + 4, "IDAT")) {
            idat_total += size;
        } else if (is_id(png + pos + 4, "IEND")) {
            break;
        }
        pos += 12 + size;
    }
    switch (pi.ctype) {
    case 0: pi.channels = 1; break;
    case 2: pi.channels = 3; break;
    case 3: pi.channels = 1; break;
    case 4: pi.channels = 2; break;
    case 6: pi.channels = 4; break;
    default: return IC_ERR_FORMAT;
    }
    if (pi.width == 0 || pi.height == 0 || pi.width > 4096 || pi.height > 4096 || idat_total == 0 ||
        !(pi.depth == 1 || pi.depth == 2 || pi.depth == 4 || pi.depth == 8 || pi.depth == 16) ||
        (pi.ctype == 3 && (pi.depth > 8 || !pi.plte)) || ((pi.ctype & 1) == 0 && pi.ctype != 0 && pi.depth < 8) ||
        pi.interlace > 1)
        return IC_ERR_FORMAT;

    npass = pi.interlace ? 7 : 1;
    raw_size = 0;
    for (k = 0; k < npass; k++) {
        uint32_t pw, ph;
        if (pi.interlace) {
            pw = (pi.width + adam7[k][2] - 1 - adam7[k][0]) / adam7[k][2];
            ph = (pi.height + adam7[k][3] - 1 - adam7[k][1]) / adam7[k][3];
            if (pi.width <= adam7[k][0]) pw = 0;
            if (pi.height <= adam7[k][1]) ph = 0;
        } else {
            pw = pi.width;
            ph = pi.height;
        }
        if (pw && ph)
            raw_size += ph * (pass_rowbytes(&pi, pw) + 1);
    }

    idat = env->alloc(env, idat_total);
    raw = env->alloc(env, raw_size);
    st = new_state(env);
    pic = ic_own(env, owner, (uint32_t)sizeof *pic + pi.width * pi.height * 4);
    if (!idat || !raw || !st || !pic) {
        r = IC_ERR_NOMEM;
        goto done;
    }
    /* the IDATs, joined */
    p = 0;
    pos = 8;
    while (pos + 12 <= len) {
        uint32_t size = be32(png + pos);
        if (is_id(png + pos + 4, "IDAT")) {
            ic_copy(idat + p, png + pos + 8, size);
            p += size;
        } else if (is_id(png + pos + 4, "IEND")) {
            break;
        }
        pos += 12 + size;
    }
    if (run_zlib(st, idat, idat_total, raw, raw_size, &got) != IC_OK || got != raw_size)
        goto done;

    pic->width = (uint16_t)pi.width;
    pic->height = (uint16_t)pi.height;
    pic->argb = (uint8_t *)(pic + 1);
    bpp = (pi.channels * pi.depth + 7) >> 3;
    p = 0;
    for (k = 0; k < npass; k++) {
        uint32_t pw, ph, x0, y0, dx, dy, rb, y, x;
        if (pi.interlace) {
            x0 = adam7[k][0]; y0 = adam7[k][1]; dx = adam7[k][2]; dy = adam7[k][3];
            pw = pi.width > x0 ? (pi.width - x0 + dx - 1) / dx : 0;
            ph = pi.height > y0 ? (pi.height - y0 + dy - 1) / dy : 0;
        } else {
            x0 = y0 = 0; dx = dy = 1;
            pw = pi.width;
            ph = pi.height;
        }
        if (!pw || !ph)
            continue;
        rb = pass_rowbytes(&pi, pw);
        if (unfilter(raw + p, ph, rb, bpp) != IC_OK)
            goto done;
        for (y = 0; y < ph; y++) {
            const uint8_t *row = raw + p + y * (rb + 1) + 1;
            for (x = 0; x < pw; x++)
                put_pixel(&pi, row, x, pic->argb + (((y0 + y * dy) * pi.width) + x0 + x * dx) * 4);
        }
        p += ph * (rb + 1);
    }
    *out = pic;
    r = IC_OK;
done:
    if (idat) env->free(env, idat);
    if (raw) env->free(env, raw);
    if (st) env->free(env, st);
    return r;
}

/* ---- reading ------------------------------------------------------------ */

static struct ic_icon *new_icon(struct ic_env *env)
{
    struct ic_icon *icon = env->alloc(env, (uint32_t)sizeof *icon);
    if (icon) {
        icon->currentx = icon->currenty = (int32_t)IC_NO_POSITION;
    }
    return icon;
}

/* A length-prefixed string at *pos. NULL when it doesn't fit. */
static char *read_string(struct ic_env *env, struct ic_icon *icon, const uint8_t *buf, uint32_t len,
                         uint32_t *pos, int *err)
{
    uint32_t n;
    char *s;
    if (*pos + 4 > len) {
        *err = IC_ERR_SHORT;
        return NULL;
    }
    n = be32(buf + *pos);
    if (n > len - *pos - 4) {
        *err = IC_ERR_SHORT;
        return NULL;
    }
    s = ic_own(env, icon, n + 1);
    if (!s) {
        *err = IC_ERR_NOMEM;
        return NULL;
    }
    ic_copy(s, buf + *pos + 4, n);
    s[n] = 0;
    *pos += 4 + n;
    return s;
}

static int read_planar(struct ic_env *env, struct ic_icon *icon, const uint8_t *buf, uint32_t len,
                       uint32_t *pos, struct ic_planar **out)
{
    struct ic_planar *p;
    const uint8_t *h;
    if (*pos + 20 > len)
        return IC_ERR_SHORT;
    h = buf + *pos;
    p = ic_own(env, icon, (uint32_t)sizeof *p);
    if (!p)
        return IC_ERR_NOMEM;
    p->left = (int16_t)be16(h);
    p->top = (int16_t)be16(h + 2);
    p->width = be16(h + 4);
    p->height = be16(h + 6);
    p->depth = be16(h + 8);
    p->planepick = h[14];
    p->planeonoff = h[15];
    if (p->depth > 8 || p->width > 1024 || p->height > 1024)
        return IC_ERR_FORMAT;
    *pos += 20;
    p->size = (((uint32_t)p->width + 15) >> 4) * 2 * p->height * p->depth;
    if (p->size > len - *pos)
        return IC_ERR_SHORT;
    if (p->size) {
        p->data = ic_own(env, icon, p->size);
        if (!p->data)
            return IC_ERR_NOMEM;
        ic_copy(p->data, buf + *pos, p->size);
    }
    *pos += p->size;
    *out = p;
    return IC_OK;
}

static int read_form(struct ic_env *env, struct ic_icon *icon, const uint8_t *f, uint32_t flen, uint32_t flags)
{
    uint32_t p = 12, end = 8 + be32(f + 4), w = 0, h = 0;
    int nimag = 0, nargb = 0;
    if (end > flen)
        end = flen;
    while (p + 8 <= end) {
        uint32_t size = be32(f + p + 4);
        const uint8_t *body = f + p + 8;
        uint32_t blen = size > end - p - 8 ? end - p - 8 : size;
        if (is_id(f + p, "FACE") && blen >= 6) {
            w = body[0] + 1u;
            h = body[1] + 1u;
            icon->has_face = 1;
            icon->frameless = body[2] & 1;
            icon->aspect = body[3];
        } else if (is_id(f + p, "IMAG") && blen >= 10 && w && nimag < 2) {
            struct ic_palpic *pp;
            uint32_t isize = be16(body + 6) + 1u, psize = be16(body + 8) + 1u;
            unsigned depth = body[5] ? body[5] : 8;
            icon->formats |= IC_FMT_OS35;
            if (!(flags & IC_READ_NOPICTURES)) {
                pp = ic_own(env, icon, (uint32_t)sizeof *pp + w * h);
                if (!pp)
                    return IC_ERR_NOMEM;
                pp->width = (uint16_t)w;
                pp->height = (uint16_t)h;
                pp->pixels = (uint8_t *)(pp + 1);
                pp->transparent = (body[2] & 1) ? body[0] : -1;
                if (10 + isize > blen)
                    return IC_ERR_DATA;
                if (body[3]) {
                    if (ic_rle_unpack(body + 10, isize, depth, pp->pixels, w * h) != IC_OK)
                        return IC_ERR_DATA;
                } else {
                    if (isize < w * h)
                        return IC_ERR_DATA;
                    ic_copy(pp->pixels, body + 10, w * h);
                }
                if (body[2] & 2) {
                    uint32_t n = body[1] + 1u;
                    if (10 + isize + psize > blen)
                        return IC_ERR_DATA;
                    pp->palette = ic_own(env, icon, n * 3);
                    if (!pp->palette)
                        return IC_ERR_NOMEM;
                    pp->ncolours = (uint16_t)n;
                    if (body[4]) {
                        if (ic_rle_unpack(body + 10 + isize, psize, 8, pp->palette, n * 3) != IC_OK)
                            return IC_ERR_DATA;
                    } else {
                        if (psize < n * 3)
                            return IC_ERR_DATA;
                        ic_copy(pp->palette, body + 10 + isize, n * 3);
                    }
                }
                icon->pal[nimag] = pp;
            }
            nimag++;
        } else if (is_id(f + p, "ARGB") && blen > 10 && w && nargb < 2) {
            icon->formats |= IC_FMT_ARGB;
            if (!(flags & IC_READ_NOPICTURES)) {
                struct ic_argb *a = ic_own(env, icon, (uint32_t)sizeof *a + w * h * 4);
                struct inflate_state *st = new_state(env);
                uint32_t got;
                int r;
                if (!a || !st) {
                    if (st)
                        env->free(env, st);
                    return IC_ERR_NOMEM;
                }
                a->width = (uint16_t)w;
                a->height = (uint16_t)h;
                a->argb = (uint8_t *)(a + 1);
                r = run_zlib(st, body + 10, blen - 10, a->argb, w * h * 4, &got);
                env->free(env, st);
                if (r != IC_OK || got != w * h * 4)
                    return IC_ERR_DATA;
                icon->argb[nargb] = a;
            }
            nargb++;
        }
        p += 8 + size + (size & 1);
    }
    return IC_OK;
}

/* NewIcons: pictures in the IM1= and IM2= tool types, seven bits a
 * character, as the Team's reader (amigachrome scripts/amiga_icon.py) and
 * the NewIcons icons in its icon library have them. */
struct ni_state {
    char **lines;                   /* this picture's tool types */
    uint32_t nlines, li, ci;
    const char *src;
};

static uint32_t ni_decode(struct ni_state *st, unsigned bits, uint8_t *out, uint32_t entries, int new_line)
{
    uint32_t got = 0, acc = 0, loop = 0;
    int nb = 0;
    st->ci = 0;
    if (new_line)
        st->src = "";
    while (got < entries) {
        uint32_t v;
        if (loop) {
            v = 0;
            loop--;
        } else {
            uint32_t c;
            if (!st->src[st->ci]) {
                st->li++;
                if (st->li >= st->nlines)
                    break;
                st->src = st->lines[st->li] + 4;
                st->ci = 0;
                nb = 0;
                if (!st->src[0])
                    continue;
            }
            c = (uint8_t)st->src[st->ci++];
            if (c < 0xA0) {
                v = c - 0x20;
            } else if (c < 0xD1) {
                v = c - 0x51;
            } else {
                loop = c - 0xD1;
                v = 0;
            }
        }
        acc = ((acc & 0xFFFF) << 7) | (v & 0x7F);
        nb += 7;
        while (nb >= (int)bits && got < entries) {
            out[got++] = (uint8_t)((acc >> (nb - (int)bits)) & ((1u << bits) - 1));
            nb -= (int)bits;
        }
    }
    return got;
}

static int read_newicons(struct ic_env *env, struct ic_icon *icon)
{
    int which, nt = 0, slot = 0;
    char **t;
    for (t = icon->tooltypes; t && *t; t++)
        nt++;
    for (which = 1; which <= 2; which++) {
        struct ni_state st;
        char **lines;
        uint32_t n = 0, w, h, colours, bits, i;
        struct ic_palpic *pp;
        const char *head;
        if (!nt)
            return IC_OK;
        lines = ic_own(env, icon, (uint32_t)(nt * sizeof(char *)));
        if (!lines)
            return IC_ERR_NOMEM;
        for (t = icon->tooltypes; *t; t++)
            if ((*t)[0] == 'I' && (*t)[1] == 'M' && (*t)[2] == '0' + which && (*t)[3] == '=')
                lines[n++] = *t;
        if (!n || ic_strlen(lines[0]) < 9)
            continue;
        head = lines[0] + 4;
        w = (uint8_t)head[1] - 0x21u;
        h = (uint8_t)head[2] - 0x21u;
        colours = (((uint8_t)head[3] - 0x21u) << 6) + ((uint8_t)head[4] - 0x21u);
        if ((int32_t)w <= 0 || (int32_t)h <= 0 || (int32_t)colours <= 0 || w > 256 || h > 256 || colours > 256)
            continue;
        pp = ic_own(env, icon, (uint32_t)sizeof *pp + w * h + colours * 3);
        if (!pp)
            return IC_ERR_NOMEM;
        pp->width = (uint16_t)w;
        pp->height = (uint16_t)h;
        pp->pixels = (uint8_t *)(pp + 1);
        pp->palette = pp->pixels + w * h;
        pp->transparent = head[0] == 'B' ? 0 : -1;
        st.lines = lines;
        st.nlines = n;
        st.li = 0;
        st.src = head + 5;
        i = ni_decode(&st, 8, pp->palette, colours * 3, 0);
        pp->ncolours = (uint16_t)(i / 3);
        for (bits = 1; (1u << bits) < colours; bits++)
            ;
        if (ni_decode(&st, bits, pp->pixels, w * h, 1) < w * h)
            continue;
        icon->pal[slot++] = pp;
        icon->formats |= IC_FMT_NEWICONS;
    }
    return IC_OK;
}

static int read_classic(struct ic_env *env, struct ic_icon *icon, const uint8_t *buf, uint32_t len, uint32_t flags)
{
    uint32_t render, select, has_tool, has_tt, has_drawer, has_toolwin, pos = 78, i;
    int r, err = IC_OK;

    icon->formats = IC_FMT_CLASSIC;
    icon->gleft = (int16_t)be16(buf + 8);
    icon->gtop = (int16_t)be16(buf + 10);
    icon->gwidth = be16(buf + 12);
    icon->gheight = be16(buf + 14);
    icon->gflags = be16(buf + 16);
    icon->gactivation = be16(buf + 18);
    icon->gtype = be16(buf + 20);
    render = be32(buf + 22);
    select = be32(buf + 26);
    icon->guserdata = be32(buf + 44);
    icon->type = buf[48];
    has_tool = be32(buf + 50);
    has_tt = be32(buf + 54);
    icon->currentx = (int32_t)be32(buf + 58);
    icon->currenty = (int32_t)be32(buf + 62);
    has_drawer = be32(buf + 66);
    has_toolwin = be32(buf + 70);
    icon->stacksize = (int32_t)be32(buf + 74);

    if (has_drawer) {
        struct ic_drawer *d;
        if (pos + 56 > len)
            return IC_ERR_SHORT;
        d = ic_own(env, icon, (uint32_t)sizeof *d);
        if (!d)
            return IC_ERR_NOMEM;
        ic_copy(d->newwindow, buf + pos, 48);
        d->left = (int16_t)be16(buf + pos);
        d->top = (int16_t)be16(buf + pos + 2);
        d->width = be16(buf + pos + 4);
        d->height = be16(buf + pos + 6);
        d->currentx = (int32_t)be32(buf + pos + 48);
        d->currenty = (int32_t)be32(buf + pos + 52);
        icon->drawer = d;
        pos += 56;
    }
    if (render && (r = read_planar(env, icon, buf, len, &pos, &icon->planar[0])) != IC_OK)
        return r;
    if (select && (r = read_planar(env, icon, buf, len, &pos, &icon->planar[1])) != IC_OK)
        return r;

    /* The strings. Some old icons stop early: what was read is kept. */
    if (has_tool)
        icon->defaulttool = read_string(env, icon, buf, len, &pos, &err);
    if (err == IC_OK && has_tt) {
        uint32_t n;
        if (pos + 4 > len) {
            err = IC_ERR_SHORT;
        } else {
            n = be32(buf + pos) / 4;
            pos += 4;
            n = n ? n - 1 : 0;
            if (n > 4096)
                n = 4096;
            icon->tooltypes = ic_own(env, icon, (n + 1) * (uint32_t)sizeof(char *));
            if (!icon->tooltypes)
                return IC_ERR_NOMEM;
            for (i = 0; i < n && err == IC_OK; i++)
                icon->tooltypes[i] = read_string(env, icon, buf, len, &pos, &err);
            if (err != IC_OK && i)
                icon->tooltypes[i - 1] = NULL;
        }
    }
    if (err == IC_OK && has_toolwin)
        icon->toolwindow = read_string(env, icon, buf, len, &pos, &err);
    if (err == IC_OK && icon->drawer && (icon->guserdata & 0xFF)) {
        if (pos + 6 <= len) {
            icon->drawer->flags = be32(buf + pos);
            icon->drawer->viewmodes = be16(buf + pos + 4);
            pos += 6;
        } else {
            err = IC_ERR_SHORT;
        }
    }
    if (err == IC_ERR_NOMEM)
        return err;

    /* OS 3.5's FORM ICON, after the classic part */
    if (err == IC_OK) {
        for (i = pos; i + 12 <= len; i++) {
            if (is_id(buf + i, "FORM") && is_id(buf + i + 8, "ICON")) {
                uint32_t flen = 8 + be32(buf + i + 4);
                uint8_t *copy;
                if (flen > len - i)
                    flen = len - i;
                copy = ic_own(env, icon, flen);
                if (!copy)
                    return IC_ERR_NOMEM;
                ic_copy(copy, buf + i, flen);
                icon->form = copy;
                icon->formlen = flen;
                r = read_form(env, icon, copy, flen, flags);
                if (r == IC_ERR_NOMEM)
                    return r;
                break;
            }
        }
    }
    if (!(flags & (IC_READ_NOPICTURES | IC_READ_NONEWICONS)) && !icon->pal[0])
        return read_newicons(env, icon);
    return IC_OK;
}

static int read_png(struct ic_env *env, struct ic_icon *icon, const uint8_t *buf, uint32_t len, uint32_t flags)
{
    uint32_t start = 0, k;
    int have_attrs = 0, ntt = 0, r;
    uint32_t dx = 50, dy = 50, dw = 400, dh = 150, has_drawer = 0;
    struct ic_drawer *d = NULL;

    icon->formats = IC_FMT_PNG;
    icon->type = IC_WBPROJECT;
    icon->stacksize = 4096;
    icon->guserdata = IC_DISKREVISION;
    for (k = 0; k < 2 && start < len; k++) {
        uint32_t plen = png_length(buf + start, len - start), pos = 8;
        uint8_t *copy;
        if (!plen)
            break;
        copy = ic_own(env, icon, plen);
        if (!copy)
            return IC_ERR_NOMEM;
        ic_copy(copy, buf + start, plen);
        icon->png[k] = copy;
        icon->pnglen[k] = plen;
        while (pos + 12 <= plen) {
            uint32_t size = be32(copy + pos);
            const uint8_t *b = copy + pos + 8;
            if (is_id(copy + pos + 4, "IHDR") && size >= 13 && k == 0) {
                icon->gwidth = (uint16_t)be32(b);
                icon->gheight = (uint16_t)be32(b + 4);
            } else if (is_id(copy + pos + 4, "icOn") && !have_attrs) {
                uint32_t q = 0;
                have_attrs = 1;
                /* count the tool types first */
                while (q + 4 <= size) {
                    uint32_t tag = be32(b + q);
                    q += 4;
                    if (tag == IC_ATTR_TOOLWINDOW || tag == IC_ATTR_DEFAULTTOOL || tag == IC_ATTR_TOOLTYPE) {
                        while (q < size && b[q])
                            q++;
                        q++;
                        if (tag == IC_ATTR_TOOLTYPE)
                            ntt++;
                    } else if ((tag >= IC_ATTR_ICONX && tag <= IC_ATTR_VIEWMODES2) || tag == IC_ATTR_DRAWERFLAGS2) {
                        q += 4;
                    } else {
                        break;      /* an unknown tag: its size can't be known */
                    }
                }
                if (ntt) {
                    icon->tooltypes = ic_own(env, icon, (uint32_t)(ntt + 1) * (uint32_t)sizeof(char *));
                    if (!icon->tooltypes)
                        return IC_ERR_NOMEM;
                }
                ntt = 0;
                q = 0;
                while (q + 4 <= size) {
                    uint32_t tag = be32(b + q), v;
                    q += 4;
                    if (tag == IC_ATTR_TOOLWINDOW || tag == IC_ATTR_DEFAULTTOOL || tag == IC_ATTR_TOOLTYPE) {
                        uint32_t s = q, n;
                        char *str;
                        while (q < size && b[q])
                            q++;
                        n = q - s;
                        q++;
                        str = ic_own(env, icon, n + 1);
                        if (!str)
                            return IC_ERR_NOMEM;
                        ic_copy(str, b + s, n);
                        str[n] = 0;
                        if (tag == IC_ATTR_TOOLTYPE)
                            icon->tooltypes[ntt++] = str;
                        else if (tag == IC_ATTR_DEFAULTTOOL)
                            icon->defaulttool = str;
                        else
                            icon->toolwindow = str;
                        continue;
                    }
                    if (!((tag >= IC_ATTR_ICONX && tag <= IC_ATTR_VIEWMODES2) || tag == IC_ATTR_DRAWERFLAGS2) ||
                        q + 4 > size)
                        break;
                    v = be32(b + q);
                    q += 4;
                    if (tag >= IC_ATTR_DRAWERX && tag <= IC_ATTR_DRAWERFLAGS) {
                        has_drawer = 1;
                    }
                    switch (tag) {
                    case IC_ATTR_ICONX: icon->currentx = (int32_t)v; break;
                    case IC_ATTR_ICONY: icon->currenty = (int32_t)v; break;
                    case IC_ATTR_DRAWERX: dx = v; break;
                    case IC_ATTR_DRAWERY: dy = v; break;
                    case IC_ATTR_DRAWERWIDTH: dw = v; break;
                    case IC_ATTR_DRAWERHEIGHT: dh = v; break;
                    case IC_ATTR_STACKSIZE: icon->stacksize = (int32_t)v; break;
                    case IC_ATTR_TYPE: icon->type = (uint8_t)v; break;
                    case IC_ATTR_FRAMELESS: icon->frameless = v ? 1 : 0; break;
                    default: break;
                    }
                    if (tag == IC_ATTR_DRAWERFLAGS || tag == IC_ATTR_VIEWMODES || tag == IC_ATTR_DD_CURRENTX ||
                        tag == IC_ATTR_DD_CURRENTY) {
                        if (!d) {
                            d = ic_own(env, icon, (uint32_t)sizeof *d);
                            if (!d)
                                return IC_ERR_NOMEM;
                        }
                        if (tag == IC_ATTR_DRAWERFLAGS) d->flags = v;
                        else if (tag == IC_ATTR_VIEWMODES) d->viewmodes = (uint16_t)v;
                        else if (tag == IC_ATTR_DD_CURRENTX) d->currentx = (int32_t)v;
                        else d->currenty = (int32_t)v;
                    }
                }
            }
            pos += 12 + size;
        }
        start += plen;
    }
    if (!icon->png[0])
        return IC_ERR_FORMAT;
    if (has_drawer) {
        if (!d) {
            d = ic_own(env, icon, (uint32_t)sizeof *d);
            if (!d)
                return IC_ERR_NOMEM;
        }
        d->left = (int16_t)dx;
        d->top = (int16_t)dy;
        d->width = (uint16_t)dw;
        d->height = (uint16_t)dh;
        icon->drawer = d;
    }
    icon->gflags = IC_GFLG_GADGIMAGE | (icon->png[1] ? IC_GFLG_GADGHIMAGE : IC_GFLG_GADGHCOMP);
    if (!(flags & IC_READ_NOPICTURES))
        for (k = 0; k < 2; k++)
            if (icon->png[k] && (r = ic_png_decode(env, icon, icon->png[k], icon->pnglen[k], &icon->argb[k])) != IC_OK)
                return r;
    return IC_OK;
}

int ic_read(struct ic_env *env, const uint8_t *buf, uint32_t len, uint32_t flags, struct ic_icon **out)
{
    struct ic_icon *icon;
    int r;
    *out = NULL;
    if (!buf)
        return IC_ERR_FORMAT;
    if (!is_png(buf, len) && (len < 78 || be16(buf) != IC_DISKMAGIC))
        return len < 78 && !is_png(buf, len) ? (len >= 2 && be16(buf) == IC_DISKMAGIC ? IC_ERR_SHORT : IC_ERR_FORMAT)
                                             : IC_ERR_FORMAT;
    icon = new_icon(env);
    if (!icon)
        return IC_ERR_NOMEM;
    r = is_png(buf, len) ? read_png(env, icon, buf, len, flags) : read_classic(env, icon, buf, len, flags);
    if (r != IC_OK) {
        ic_free(env, icon);
        return r;
    }
    *out = icon;
    return IC_OK;
}

/* ---- writing ------------------------------------------------------------ */

struct wr {
    uint8_t *out;
    uint32_t cap, pos;
};

static void w8(struct wr *w, uint32_t v)
{
    if (w->out && w->pos < w->cap)
        w->out[w->pos] = (uint8_t)v;
    w->pos++;
}
static void w16(struct wr *w, uint32_t v) { w8(w, v >> 8); w8(w, v); }
static void w32(struct wr *w, uint32_t v) { w16(w, v >> 16); w16(w, v); }
static void wbytes(struct wr *w, const uint8_t *p, uint32_t n) { while (n--) w8(w, *p++); }
static void wstring(struct wr *w, const char *s)
{
    uint32_t n = ic_strlen(s) + 1;
    w32(w, n);
    wbytes(w, (const uint8_t *)s, n);
}
/* A placeholder for a pointer the file says is there. */
#define PRESENT 1UL

static unsigned bits_for(uint32_t colours)
{
    unsigned b = 1;
    while ((1u << b) < colours && b < 8)
        b++;
    return b;
}

/* FORM ICON from the palette pictures (FACE, IMAG). */
static void write_form(struct wr *w, const struct ic_icon *icon)
{
    uint32_t start, k, maxpal = 0, ncol0 = 0;
    const struct ic_palpic *p0 = icon->pal[0];
    for (k = 0; k < 2; k++)
        if (icon->pal[k] && icon->pal[k]->ncolours * 3u > maxpal)
            maxpal = icon->pal[k]->ncolours * 3u;
    ncol0 = p0->ncolours;
    w32(w, 0x464F524DUL);           /* FORM */
    start = w->pos;
    w32(w, 0);                      /* its size, below */
    w32(w, 0x49434F4EUL);           /* ICON */
    w32(w, 0x46414345UL);           /* FACE */
    w32(w, 6);
    w8(w, p0->width - 1u);
    w8(w, p0->height - 1u);
    w8(w, icon->frameless ? 1 : 0);
    w8(w, icon->aspect);
    w16(w, maxpal ? maxpal - 1 : 0);
    for (k = 0; k < 2; k++) {
        const struct ic_palpic *p = icon->pal[k];
        uint32_t ncol, isz, psz = 0, csize, n = (uint32_t)p0->width * p0->height;
        unsigned bits;
        if (!p)
            continue;
        ncol = p->ncolours ? p->ncolours : ncol0;
        bits = bits_for(ncol ? ncol : 2);
        isz = ic_rle_pack(p->pixels, n, bits, NULL, 0);
        if (p->ncolours)
            psz = ic_rle_pack(p->palette, p->ncolours * 3u, 8, NULL, 0);
        csize = 10 + isz + psz;
        w32(w, 0x494D4147UL);       /* IMAG */
        w32(w, csize);
        w8(w, p->transparent >= 0 ? (uint32_t)p->transparent : 0);
        w8(w, ncol ? ncol - 1 : 0);
        w8(w, (p->transparent >= 0 ? 1u : 0u) | (p->ncolours ? 2u : 0u));
        w8(w, 1);                   /* the image run-length coded */
        w8(w, p->ncolours ? 1 : 0); /* and the palette */
        w8(w, bits);
        w16(w, isz - 1);
        w16(w, psz ? psz - 1 : 0);
        if (w->out && w->pos + isz <= w->cap)
            ic_rle_pack(p->pixels, n, bits, w->out + w->pos, isz);
        w->pos += isz;
        if (psz) {
            if (w->out && w->pos + psz <= w->cap)
                ic_rle_pack(p->palette, p->ncolours * 3u, 8, w->out + w->pos, psz);
            w->pos += psz;
        }
        if (csize & 1)
            w8(w, 0);
    }
    if (w->out && start + 4 <= w->cap) {
        uint32_t size = w->pos - start - 4;
        w->out[start] = (uint8_t)(size >> 24);
        w->out[start + 1] = (uint8_t)(size >> 16);
        w->out[start + 2] = (uint8_t)(size >> 8);
        w->out[start + 3] = (uint8_t)size;
    }
}

static void write_classic(struct wr *w, const struct ic_icon *icon)
{
    uint32_t k, n = 0;
    const struct ic_drawer *d = icon->drawer;

    w16(w, IC_DISKMAGIC);
    w16(w, IC_DISKVERSION);
    /* do_Gadget */
    w32(w, 0);
    w16(w, (uint16_t)icon->gleft);
    w16(w, (uint16_t)icon->gtop);
    w16(w, icon->gwidth);
    w16(w, icon->gheight);
    w16(w, icon->gflags);
    w16(w, icon->gactivation);
    w16(w, icon->gtype);
    w32(w, icon->planar[0] ? PRESENT : 0);
    w32(w, icon->planar[1] ? PRESENT : 0);
    w32(w, 0);                      /* GadgetText */
    w32(w, 0);                      /* MutualExclude */
    w32(w, 0);                      /* SpecialInfo */
    w16(w, 0);                      /* GadgetID */
    w32(w, icon->guserdata);
    w8(w, icon->type);
    w8(w, 0);
    w32(w, icon->defaulttool ? PRESENT : 0);
    w32(w, icon->tooltypes ? PRESENT : 0);
    w32(w, (uint32_t)icon->currentx);
    w32(w, (uint32_t)icon->currenty);
    w32(w, d ? PRESENT : 0);
    w32(w, icon->toolwindow ? PRESENT : 0);
    w32(w, (uint32_t)icon->stacksize);
    if (d) {
        uint8_t nw[48];
        ic_copy(nw, d->newwindow, 48);
        nw[0] = (uint8_t)((uint16_t)d->left >> 8); nw[1] = (uint8_t)d->left;
        nw[2] = (uint8_t)((uint16_t)d->top >> 8);  nw[3] = (uint8_t)d->top;
        nw[4] = (uint8_t)(d->width >> 8);          nw[5] = (uint8_t)d->width;
        nw[6] = (uint8_t)(d->height >> 8);         nw[7] = (uint8_t)d->height;
        wbytes(w, nw, 48);
        w32(w, (uint32_t)d->currentx);
        w32(w, (uint32_t)d->currenty);
    }
    for (k = 0; k < 2; k++) {
        const struct ic_planar *p = icon->planar[k];
        uint32_t size;
        if (!p)
            continue;
        size = (((uint32_t)p->width + 15) >> 4) * 2 * p->height * p->depth;
        w16(w, (uint16_t)p->left);
        w16(w, (uint16_t)p->top);
        w16(w, p->width);
        w16(w, p->height);
        w16(w, p->depth);
        w32(w, size ? PRESENT : 0);
        w8(w, p->planepick);
        w8(w, p->planeonoff);
        w32(w, 0);                  /* NextImage */
        if (p->data && p->size >= size) {
            wbytes(w, p->data, size);
        } else {
            while (size--)
                w8(w, 0);
        }
    }
    if (icon->defaulttool)
        wstring(w, icon->defaulttool);
    if (icon->tooltypes) {
        char **t;
        for (t = icon->tooltypes; *t; t++)
            n++;
        w32(w, (n + 1) * 4);
        for (t = icon->tooltypes; *t; t++)
            wstring(w, *t);
    }
    if (icon->toolwindow)
        wstring(w, icon->toolwindow);
    if (d && (icon->guserdata & 0xFF)) {
        w32(w, d->flags);
        w16(w, d->viewmodes);
    }
    if (icon->pictures_changed) {
        if (icon->pal[0])
            write_form(w, icon);
    } else if (icon->form) {
        wbytes(w, icon->form, icon->formlen);
    }
}

/* icOn's body from the icon's fields, in the order the Team's writer and
 * the AmigaOS 4 icons use: places, the drawer, stack, type, then strings,
 * the tool window last. */
static void icon_attrs(struct wr *w, const struct ic_icon *icon)
{
    const struct ic_drawer *d = icon->drawer;
    char **t;
#define NUM(tag, v) do { w32(w, tag); w32(w, (uint32_t)(v)); } while (0)
#define TEXT(tag, s) do { w32(w, tag); wbytes(w, (const uint8_t *)(s), ic_strlen(s) + 1); } while (0)
    if ((uint32_t)icon->currentx != IC_NO_POSITION)
        NUM(IC_ATTR_ICONX, icon->currentx);
    if ((uint32_t)icon->currenty != IC_NO_POSITION)
        NUM(IC_ATTR_ICONY, icon->currenty);
    if (d) {
        NUM(IC_ATTR_DRAWERX, (int32_t)d->left);
        NUM(IC_ATTR_DRAWERY, (int32_t)d->top);
        NUM(IC_ATTR_DRAWERWIDTH, d->width);
        NUM(IC_ATTR_DRAWERHEIGHT, d->height);
        if (d->flags)
            NUM(IC_ATTR_DRAWERFLAGS, d->flags);
        if (d->viewmodes)
            NUM(IC_ATTR_VIEWMODES, d->viewmodes);
        if (d->currentx)
            NUM(IC_ATTR_DD_CURRENTX, d->currentx);
        if (d->currenty)
            NUM(IC_ATTR_DD_CURRENTY, d->currenty);
    }
    NUM(IC_ATTR_STACKSIZE, icon->stacksize);
    NUM(IC_ATTR_TYPE, icon->type);
    if (icon->frameless)
        NUM(IC_ATTR_FRAMELESS, 1);
    if (icon->defaulttool)
        TEXT(IC_ATTR_DEFAULTTOOL, icon->defaulttool);
    for (t = icon->tooltypes; t && *t; t++)
        TEXT(IC_ATTR_TOOLTYPE, *t);
    if (icon->toolwindow)
        TEXT(IC_ATTR_TOOLWINDOW, icon->toolwindow);
#undef NUM
#undef TEXT
}

/* A PNG icon: each frame as read, the first with a new icOn after IHDR and
 * its old icOn left out. */
static void write_png(struct wr *w, const struct ic_icon *icon)
{
    uint32_t k;
    for (k = 0; k < 2; k++) {
        const uint8_t *p = icon->png[k];
        uint32_t pos = 8, len = icon->pnglen[k];
        if (!p)
            continue;
        wbytes(w, p, 8);
        while (pos + 12 <= len) {
            uint32_t size = be32(p + pos), total = 12 + size;
            if (k == 0 && is_id(p + pos + 4, "icOn")) {
                pos += total;
                continue;
            }
            wbytes(w, p + pos, total);
            if (k == 0 && is_id(p + pos + 4, "IHDR")) {
                /* the attributes, measured first, then written as a chunk */
                struct wr m = { NULL, 0, 0 };
                uint32_t n, start;
                icon_attrs(&m, icon);
                n = m.pos;
                w32(w, n);
                start = w->pos;
                w32(w, 0x69634F6EUL);   /* icOn */
                icon_attrs(w, icon);
                if (w->out && w->pos <= w->cap)
                    w32(w, ic_crc32(0, w->out + start, n + 4));
                else
                    w32(w, 0);
            }
            pos += total;
        }
    }
}

uint32_t ic_write(struct ic_env *env, const struct ic_icon *icon, uint8_t *out, uint32_t cap)
{
    struct wr w;
    (void)env;
    w.out = out;
    w.cap = out ? cap : 0;
    w.pos = 0;
    if (!icon)
        return 0;
    if ((icon->formats & IC_FMT_PNG) && icon->png[0])
        write_png(&w, icon);
    else
        write_classic(&w, icon);
    return w.pos;
}
