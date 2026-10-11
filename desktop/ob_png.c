/* ob_png: OpenBench's own PNG reader (ob_png.h). Written from the PNG specification (chunks, the five
 * row filters, colour types) and RFC 1950/1951 (zlib's wrapper and inflate: stored, fixed and dynamic
 * Huffman blocks, decoded canonically a bit at a time). Integer only; memory from AllocVec.
 *
 * MIT, Copyright (c) 2026 Dalsin Limited. */
#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include "ob_png.h"

static APTR big_alloc(ULONG n)
{
    APTR p = AllocVec(n, MEMF_FAST);
    return p ? p : AllocVec(n, MEMF_ANY);
}

/* ---- inflate (RFC 1951) ---------------------------------------------------------------------- */

typedef struct {
    const UBYTE *in, *in_end;
    ULONG bitbuf;
    int bitcnt;
    int bad;                                /* read past the end */
    UBYTE *out;
    ULONG outpos, outmax;
} inf_state;

typedef struct {
    UWORD count[16];                        /* codes of each length */
    UWORD symbol[288];                      /* symbols, by code */
} huff;

static int bits(inf_state *s, int n)
{
    ULONG v;
    while (s->bitcnt < n) {
        if (s->in >= s->in_end) { s->bad = 1; return 0; }
        s->bitbuf |= (ULONG)*s->in++ << s->bitcnt;
        s->bitcnt += 8;
    }
    v = s->bitbuf & ((1UL << n) - 1);
    s->bitbuf >>= n;
    s->bitcnt -= n;
    return (int)v;
}

/* 0, or -1 when the lengths don't make a code (more codes of a length than there is room for). */
static int build(huff *h, const UBYTE *len, int n)
{
    UWORD offs[16];
    int i, left = 1;
    for (i = 0; i < 16; i++) h->count[i] = 0;
    for (i = 0; i < n; i++) h->count[len[i]]++;
    h->count[0] = 0;
    for (i = 1; i < 16; i++) {
        left <<= 1;
        left -= h->count[i];
        if (left < 0) return -1;
    }
    offs[1] = 0;
    for (i = 1; i < 15; i++) offs[i + 1] = (UWORD)(offs[i] + h->count[i]);
    for (i = 0; i < n; i++)
        if (len[i]) h->symbol[offs[len[i]]++] = (UWORD)i;
    return 0;
}

static int decode(inf_state *s, const huff *h)
{
    int code = 0, first = 0, index = 0, len;
    for (len = 1; len < 16; len++) {
        int count;
        code |= bits(s, 1);
        if (s->bad) return -1;
        count = h->count[len];
        if (code - first < count) return h->symbol[index + (code - first)];
        index += count;
        first += count;
        first <<= 1;
        code <<= 1;
    }
    return -1;
}

static const UWORD lbase[29] = { 3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35, 43, 51, 59, 67, 83, 99, 115,
                                 131, 163, 195, 227, 258 };
static const UBYTE lext[29] = { 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0 };
static const UWORD dbase[30] = { 1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385, 513, 769, 1025, 1537,
                                 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577 };
static const UBYTE dext[30] = { 0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13 };

/* One block's codes, to its end-of-block symbol. 0, or -1 on a bad stream. */
static int codes(inf_state *s, const huff *lencode, const huff *distcode)
{
    for (;;) {
        int sym = decode(s, lencode);
        if (sym < 0) return -1;
        if (sym < 256) {
            if (s->outpos >= s->outmax) return -1;
            s->out[s->outpos++] = (UBYTE)sym;
        } else if (sym == 256) {
            return 0;
        } else {
            ULONG len, dist;
            sym -= 257;
            if (sym >= 29) return -1;
            len = lbase[sym] + (ULONG)bits(s, lext[sym]);
            sym = decode(s, distcode);
            if (sym < 0 || sym >= 30) return -1;
            dist = dbase[sym] + (ULONG)bits(s, dext[sym]);
            if (s->bad || dist > s->outpos || s->outpos + len > s->outmax) return -1;
            while (len--) {
                s->out[s->outpos] = s->out[s->outpos - dist];
                s->outpos++;
            }
        }
    }
}

static int stored(inf_state *s)
{
    ULONG len;
    s->bitbuf = 0;                          /* to the byte: the bits held are the rest of this one */
    s->bitcnt = 0;
    if (s->in + 4 > s->in_end) return -1;
    len = s->in[0] | ((ULONG)s->in[1] << 8);
    if ((s->in[2] | ((ULONG)s->in[3] << 8)) != (~len & 0xffff)) return -1;
    s->in += 4;
    if (s->in + len > s->in_end || s->outpos + len > s->outmax) return -1;
    while (len--) s->out[s->outpos++] = *s->in++;
    return 0;
}

static int fixed(inf_state *s)
{
    static huff lencode, distcode;
    static int made;
    if (!made) {
        UBYTE len[288];
        int i;
        for (i = 0; i < 144; i++) len[i] = 8;
        for (; i < 256; i++) len[i] = 9;
        for (; i < 280; i++) len[i] = 7;
        for (; i < 288; i++) len[i] = 8;
        build(&lencode, len, 288);
        for (i = 0; i < 30; i++) len[i] = 5;
        build(&distcode, len, 30);
        made = 1;                            /* the tables never change: made once, then only read */
    }
    return codes(s, &lencode, &distcode);
}

static int dynamic(inf_state *s)
{
    static const UBYTE order[19] = { 16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15 };
    UBYTE len[320];
    huff lencode, distcode;
    int nlen = bits(s, 5) + 257, ndist = bits(s, 5) + 1, ncode = bits(s, 4) + 4, i;
    if (s->bad || nlen > 286 || ndist > 30) return -1;
    for (i = 0; i < 19; i++) len[order[i]] = (UBYTE)(i < ncode ? bits(s, 3) : 0);
    if (build(&lencode, len, 19) < 0) return -1;
    for (i = 0; i < nlen + ndist; ) {
        int sym = decode(s, &lencode), rep = 0;
        UBYTE v = 0;
        if (sym < 0) return -1;
        if (sym < 16) { len[i++] = (UBYTE)sym; continue; }
        if (sym == 16) {
            if (!i) return -1;
            v = len[i - 1];
            rep = 3 + bits(s, 2);
        } else if (sym == 17) rep = 3 + bits(s, 3);
        else rep = 11 + bits(s, 7);
        if (s->bad || i + rep > nlen + ndist) return -1;
        while (rep--) len[i++] = v;
    }
    if (!len[256]) return -1;                 /* no end-of-block code */
    if (build(&lencode, len, nlen) < 0 || build(&distcode, len + nlen, ndist) < 0) return -1;
    return codes(s, &lencode, &distcode);
}

/* A zlib stream (RFC 1950) inflated into out. The bytes written, or -1. */
static LONG zlib_inflate(const UBYTE *in, ULONG inlen, UBYTE *out, ULONG outmax)
{
    inf_state s;
    int last;
    if (inlen < 2 || (in[0] & 15) != 8 || ((in[0] << 8) | in[1]) % 31 || (in[1] & 0x20)) return -1;
    s.in = in + 2;
    s.in_end = in + inlen;
    s.bitbuf = 0;
    s.bitcnt = 0;
    s.bad = 0;
    s.out = out;
    s.outpos = 0;
    s.outmax = outmax;
    do {
        int type, r;
        last = bits(&s, 1);
        type = bits(&s, 2);
        if (s.bad) return -1;
        r = type == 0 ? stored(&s) : type == 1 ? fixed(&s) : type == 2 ? dynamic(&s) : -1;
        if (r < 0) return -1;
    } while (!last);
    return (LONG)s.outpos;
}

/* ---- the PNG --------------------------------------------------------------------------------- */

static ULONG be32(const UBYTE *p) { return ((ULONG)p[0] << 24) | ((ULONG)p[1] << 16) | ((ULONG)p[2] << 8) | p[3]; }

static UBYTE *read_file(const char *path, ULONG *len)
{
    BPTR f = Open((CONST_STRPTR)path, MODE_OLDFILE);
    struct FileInfoBlock *fib;
    UBYTE *buf = NULL;
    if (!f) return NULL;
    if ((fib = AllocDosObject(DOS_FIB, NULL))) {
        if (ExamineFH(f, fib) && fib->fib_Size > 8 && fib->fib_Size < (64L << 20) && (buf = big_alloc((ULONG)fib->fib_Size))) {
            if (Read(f, buf, fib->fib_Size) != fib->fib_Size) { FreeVec(buf); buf = NULL; }
            else *len = (ULONG)fib->fib_Size;
        }
        FreeDosObject(DOS_FIB, fib);
    }
    Close(f);
    return buf;
}

static int paeth(int a, int b, int c)
{
    int p = a + b - c, pa = p > a ? p - a : a - p, pb = p > b ? p - b : b - p, pc = p > c ? p - c : c - p;
    if (pa <= pb && pa <= pc) return a;
    return pb <= pc ? b : c;
}

ULONG *ob_png_load(const char *path, LONG *width, LONG *height)
{
    static const UBYTE sig[8] = { 0x89, 'P', 'N', 'G', 13, 10, 26, 10 };
    ULONG flen = 0, pos, w = 0, h = 0, idat_len = 0, rowbytes, raw_len, x, y, i;
    UBYTE *file = read_file(path, &flen), *idat = NULL, *raw = NULL, depth = 0, ctype = 0, inter = 0;
    ULONG pal[256];
    int channels, bpp_bits, bpp, have_ihdr = 0;
    ULONG *argb = NULL;
    *width = *height = 0;
    if (!file) return NULL;
    for (i = 0; i < 8; i++) if (file[i] != sig[i]) goto out;
    for (i = 0; i < 256; i++) pal[i] = 0xff000000UL | (i << 16) | (i << 8) | i;
    /* the chunks: IHDR, PLTE, tRNS, and the IDATs' sizes */
    for (pos = 8; pos + 12 <= flen; ) {
        ULONG n = be32(file + pos), type = be32(file + pos + 4);
        const UBYTE *d = file + pos + 8;
        if (n > flen - pos - 12) goto out;
        if (type == 0x49484452UL && n >= 13) {                /* IHDR */
            w = be32(d); h = be32(d + 4); depth = d[8]; ctype = d[9]; inter = d[12];
            have_ihdr = 1;
        } else if (type == 0x504C5445UL) {                     /* PLTE */
            for (i = 0; i < n / 3 && i < 256; i++) pal[i] = 0xff000000UL | ((ULONG)d[i * 3] << 16) | ((ULONG)d[i * 3 + 1] << 8) | d[i * 3 + 2];
        } else if (type == 0x74524E53UL && ctype == 3) {       /* tRNS, for a palette */
            for (i = 0; i < n && i < 256; i++) pal[i] = (pal[i] & 0x00ffffffUL) | ((ULONG)d[i] << 24);
        } else if (type == 0x49444154UL) {                     /* IDAT */
            idat_len += n;
        } else if (type == 0x49454E44UL) break;                /* IEND */
        pos += n + 12;
    }
    if (!have_ihdr || !w || !h || w > 8192 || h > 8192 || inter || !idat_len) goto out;
    switch (ctype) {
    case 0: channels = 1; break;
    case 2: channels = 3; break;
    case 3: channels = 1; break;
    case 4: channels = 2; break;
    case 6: channels = 4; break;
    default: goto out;
    }
    if (!((depth == 8 || depth == 16) || ((ctype == 0 || ctype == 3) && (depth == 1 || depth == 2 || depth == 4)))) goto out;
    if (ctype == 3 && depth == 16) goto out;
    bpp_bits = channels * depth;
    bpp = bpp_bits >= 8 ? bpp_bits / 8 : 1;
    rowbytes = (w * (ULONG)bpp_bits + 7) / 8;
    raw_len = h * (rowbytes + 1);
    /* the IDATs one after the other, then inflated */
    if (!(idat = big_alloc(idat_len)) || !(raw = big_alloc(raw_len))) goto out;
    for (pos = 8, i = 0; pos + 12 <= flen; ) {
        ULONG n = be32(file + pos), type = be32(file + pos + 4);
        if (type == 0x49444154UL) { CopyMem((APTR)(file + pos + 8), idat + i, n); i += n; }
        else if (type == 0x49454E44UL) break;
        pos += n + 12;
    }
    FreeVec(file);
    file = NULL;
    if (zlib_inflate(idat, idat_len, raw, raw_len) != (LONG)raw_len) goto out;
    FreeVec(idat);
    idat = NULL;
    /* the rows' filters undone, in place */
    for (y = 0; y < h; y++) {
        UBYTE *row = raw + y * (rowbytes + 1), f = row[0], *cur = row + 1, *prev = y ? cur - (rowbytes + 1) : NULL;
        for (i = 0; i < rowbytes; i++) {
            int a = i >= (ULONG)bpp ? cur[i - bpp] : 0, b = prev ? prev[i] : 0, c = (prev && i >= (ULONG)bpp) ? prev[i - bpp] : 0;
            switch (f) {
            case 0: break;
            case 1: cur[i] = (UBYTE)(cur[i] + a); break;
            case 2: cur[i] = (UBYTE)(cur[i] + b); break;
            case 3: cur[i] = (UBYTE)(cur[i] + ((a + b) >> 1)); break;
            case 4: cur[i] = (UBYTE)(cur[i] + paeth(a, b, c)); break;
            default: goto out;
            }
        }
    }
    /* to 0xAARRGGBB */
    if (!(argb = big_alloc(w * h * 4))) goto out;
    for (y = 0; y < h; y++) {
        const UBYTE *s = raw + y * (rowbytes + 1) + 1;
        ULONG *d = argb + y * w;
        int step = depth == 16 ? 2 : 1;                       /* 16-bit: the high byte of each sample */
        for (x = 0; x < w; x++) {
            ULONG p;
            if (depth < 8) {
                int per = 8 / depth, shift = (per - 1 - (int)(x % per)) * depth, v = (s[x / per] >> shift) & ((1 << depth) - 1);
                p = ctype == 3 ? pal[v] : pal[v * 255 / ((1 << depth) - 1)];
            } else {
                const UBYTE *q = s + x * (ULONG)channels * step;
                switch (ctype) {
                case 0: p = pal[q[0]]; break;
                case 3: p = pal[q[0]]; break;
                case 2: p = 0xff000000UL | ((ULONG)q[0] << 16) | ((ULONG)q[step] << 8) | q[2 * step]; break;
                case 4: p = ((ULONG)q[step] << 24) | ((ULONG)q[0] << 16) | ((ULONG)q[0] << 8) | q[0]; break;
                default: p = ((ULONG)q[3 * step] << 24) | ((ULONG)q[0] << 16) | ((ULONG)q[step] << 8) | q[2 * step]; break;
                }
            }
            d[x] = p;
        }
    }
    *width = (LONG)w;
    *height = (LONG)h;
out:
    if (file) FreeVec(file);
    if (idat) FreeVec(idat);
    if (raw) FreeVec(raw);
    if (argb && !*width) { FreeVec(argb); argb = NULL; }
    return argb;
}
