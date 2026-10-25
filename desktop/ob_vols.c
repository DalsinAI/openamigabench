/* ob_vols.c: the mounted volumes' fingerprint (ob_vols.h).
 * MIT, Copyright (c) 2026 AmigaChrome Team. */
#include "ob_vols.h"

/* unsigned is 32 bits on the 68k and on the host */
static unsigned step(unsigned h, unsigned v)
{
    return ((h << 5) + h) ^ v;              /* h * 33 ^ v: a shift and an add on the 68k */
}

/* every bit of h into every bit of the result (MurmurHash3's finish), so the sums of near names are far apart */
static unsigned spread(unsigned h)
{
    h ^= h >> 16;
    h *= 0x85ebca6bu;
    h ^= h >> 13;
    h *= 0xc2b2ae35u;
    h ^= h >> 16;
    return h;
}

void ob_volfp_init(ob_volfp *f)
{
    f->n = f->sum = f->mix = 0;
}

void ob_volfp_add(ob_volfp *f, const unsigned char *name, unsigned len, long days, long minute, long tick,
                  unsigned long drive)
{
    unsigned h = 5381, i;
    for (i = 0; i < len; i++) h = step(h, name[i]);
    h = step(h, len);
    h = step(h, (unsigned)days);
    h = step(h, (unsigned)minute);
    h = step(h, (unsigned)tick);
    h = step(h, (unsigned)drive);
    h = spread(h);
    f->n++;
    f->sum += h;
    f->mix ^= h;
}

int ob_volfp_same(const ob_volfp *a, const ob_volfp *b)
{
    return a->n == b->n && a->sum == b->sum && a->mix == b->mix;
}
