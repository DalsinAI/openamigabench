/* ob_malloc: malloc, calloc, realloc and free for everything linked into OpenBench, from exec's AllocVec,
 * which any task may call. libnix's own heap is one per program and not safe between tasks; the dock and the
 * built-ins (ob_builtin.h) run as processes of their own inside OpenBench, so they all use these instead.
 * MIT, Copyright (c) 2026 Dalsin Limited. */
#include <string.h>
#include <exec/memory.h>
#include <proto/exec.h>

/* each block: its size in the longword before it */
void *malloc(size_t n)
{
    ULONG *p = AllocVec((ULONG)n + 8, MEMF_ANY);
    if (!p) return NULL;
    p[1] = (ULONG)n;
    return p + 2;
}

void free(void *q)
{
    if (q) FreeVec((ULONG *)q - 2);
}

void *calloc(size_t a, size_t b)
{
    size_t n = a * b;
    void *q;
    if (b && n / b != a) return NULL;
    if ((q = malloc(n))) memset(q, 0, n);
    return q;
}

void *realloc(void *q, size_t n)
{
    void *r;
    size_t had;
    if (!q) return malloc(n);
    if (!n) { free(q); return NULL; }
    had = ((ULONG *)q)[-1];
    if (n <= had) return q;
    if (!(r = malloc(n))) return NULL;
    memcpy(r, q, had);
    free(q);
    return r;
}
