/* proxy: see proxy.h. The library's Open, Close and Expunge; loading the
 * original privately; and the spy.
 *
 * Built with no C library (-nostdlib): exec and dos only, through their
 * inline calls, and the few helpers below.
 *
 * MIT, Copyright (c) 2026 Dalsin Limited. */
#include <stddef.h>
#include <exec/types.h>
#include <exec/execbase.h>
#include <exec/resident.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/var.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include "proxy.h"

struct ExecBase *SysBase;
struct DosLibrary *DOSBase;

/* The stubs read these from assembler: they must not move (checked on the
 * 68k, where pointers are 4 bytes; a 64-bit host's syntax check skips it). */
#ifdef __mc68000__
typedef char check_orig[offsetof(struct ProxyBase, orig) == PB_ORIG ? 1 : -1];
typedef char check_spy[offsetof(struct ProxyBase, spy) == PB_SPY ? 1 : -1];
#endif

#define ORIGINALS "SYS:Storage/OpenUp-Superseded"   /* where OpenUp's retire lines put Hyperion's files */
#define DEFAULT_SPY 2048

/* ---- small helpers (no C library) ---- */

void *memset(void *d, int c, size_t n)
{
    UBYTE *p = d;
    while (n--) *p++ = (UBYTE)c;
    return d;
}

void *memcpy(void *d, const void *s, size_t n)
{
    UBYTE *p = d;
    const UBYTE *q = s;
    while (n--) *p++ = *q++;
    return d;
}

static void copy_str(char *d, const char *s, int n)
{
    int i = 0;
    while (s && s[i] && i < n - 1) { d[i] = s[i]; i++; }
    d[i] = 0;
}

static ULONG to_ulong(const char *s)
{
    ULONG v = 0;
    while (*s >= '0' && *s <= '9') v = v * 10 + (ULONG)(*s++ - '0');
    return v;
}

/* An address the spy may read: not low memory, nor the chips' and CIAs'
 * registers (reading a CIA's ICR clears it), nor the AutoConfig space. */
static int readable(ULONG p)
{
    if (p < 0x400) return 0;
    if (p >= 0x00A00000UL && p < 0x00E00000UL) return 0;
    if (p >= 0x00E80000UL && p < 0x00F00000UL) return 0;
    return 1;
}

/* ---- the original, loaded privately ---- */

/* The RomTag in a loaded file: its match word, pointing at itself. */
static struct Resident *find_romtag(BPTR seg)
{
    for (; seg; seg = *(BPTR *)BADDR(seg)) {
        ULONG *hunk = (ULONG *)BADDR(seg);
        ULONG bytes = hunk[-1];                 /* the allocation, the size and next longwords included */
        UWORD *p = (UWORD *)(hunk + 1), *end;
        if (bytes < 8 + sizeof(struct Resident)) continue;
        end = (UWORD *)((UBYTE *)hunk - 4 + bytes - sizeof(struct Resident));
        for (; p <= end; p++)
            if (*p == RTC_MATCHWORD && ((struct Resident *)p)->rt_MatchTag == (struct Resident *)p)
                return (struct Resident *)p;
    }
    return NULL;
}

/* ENV:OpenBench/Spy-workbench.library (or -icon.library): the spy on from
 * the first open, so the boot's calls are recorded; its value is how many
 * calls the ring keeps (0 or not a number: DEFAULT_SPY). */
static void spy_from_env(struct ProxyBase *b)
{
    char var[64], v[16];
    ULONG n;
    copy_str(var, "OpenBench/Spy-", sizeof var);
    copy_str(var + 14, b->name, sizeof var - 14);
    if (GetVar((STRPTR)var, (STRPTR)v, sizeof v, GVF_GLOBAL_ONLY) <= 0) return;
    n = to_ulong(v);
    if (!n) n = DEFAULT_SPY;
    if (n > 65536) n = 65536;
    b->spy = proxy_spy_new(n);
}

/* Loads Hyperion's original from the originals' drawer, makes its base,
 * links it under our private name and opens it. 1 when b->orig is set. */
static int load_original(struct ProxyBase *b)
{
    char path[200];
    BPTR seg;
    struct Resident *rt;
    struct Library *lib;
    if (GetVar((STRPTR)"OpenBench/Originals", (STRPTR)path, 160, GVF_GLOBAL_ONLY) <= 0)
        copy_str(path, ORIGINALS, sizeof path);
    if (!AddPart((STRPTR)path, (STRPTR)b->name, sizeof path)) {
        b->error = ERROR_LINE_TOO_LONG;
        return 0;
    }
    if (!(seg = LoadSeg((STRPTR)path))) {
        b->error = IoErr();
        return 0;
    }
    if (!(rt = find_romtag(seg))) {
        UnLoadSeg(seg);
        b->error = PB_ERR_NOROMTAG;
        return 0;
    }
    if (rt->rt_Flags & RTF_AUTOINIT) {
        /* as InitResident would, but linked under our private name */
        ULONG *it = (ULONG *)rt->rt_Init;      /* data size, functions, data table, init */
        lib = MakeLibrary((CONST_APTR)it[1], (CONST_APTR)it[2], (ULONG (*)())it[3], it[0], seg);
        if (!lib) {
            UnLoadSeg(seg);
            b->error = PB_ERR_INIT;
            return 0;
        }
        lib->lib_Node.ln_Name = (char *)b->private_name;
        lib->lib_Node.ln_Type = NT_LIBRARY;
        AddLibrary(lib);
    } else {
        /* its own init links it under its public name: taken off and renamed */
        if (!(lib = (struct Library *)InitResident(rt, seg))) {
            UnLoadSeg(seg);
            b->error = PB_ERR_INIT;
            return 0;
        }
        Forbid();
        Remove(&lib->lib_Node);
        lib->lib_Node.ln_Name = (char *)b->private_name;
        Enqueue(&SysBase->LibList, &lib->lib_Node);
        Permit();
    }
    b->orig_seg = seg;
    /* opened like any library: its own Open runs */
    if (!(b->orig = OpenLibrary((STRPTR)b->private_name, 0))) {
        b->error = PB_ERR_OPEN;
        return 0;                               /* it stays linked; a later open may try again */
    }
    b->error = 0;
    return 1;
}

/* ---- the library's lifecycle ---- */

struct Library *__attribute__((used)) proxy_init(register struct ProxyBase *b __asm("d0"),
                                                 register BPTR seg __asm("a0"),
                                                 register struct ExecBase *sb __asm("a6"))
{
    SysBase = sb;
    if (!(DOSBase = (struct DosLibrary *)OpenLibrary((STRPTR)"dos.library", 39)))
        return NULL;
    b->lib.lib_Node.ln_Type = NT_LIBRARY;
    b->lib.lib_Node.ln_Name = (char *)proxy_def.name;
    b->lib.lib_Flags = LIBF_SUMUSED | LIBF_CHANGED;
    b->lib.lib_Version = proxy_def.version;     /* exactly 3.2.3's (design section 6) */
    b->lib.lib_Revision = proxy_def.revision;
    b->lib.lib_IdString = (APTR)proxy_def.id;
    b->magic = PB_MAGIC;
    b->layout = PB_VERSION;
    b->nfuncs = proxy_def.nfuncs;
    b->seglist = seg;
    b->name = proxy_def.name;
    b->private_name = proxy_def.private_name;
    b->funcs = proxy_def.funcs;
    InitSemaphore(&b->lock);
    /* The original is loaded on the first open, in the opener's process:
     * init runs inside the library loader, where loading a second file,
     * and whatever it opens in turn, is better not done. */
    return &b->lib;
}

struct Library *__attribute__((used)) proxy_open(register struct ProxyBase *b __asm("a6"),
                                                 register ULONG version __asm("d0"))
{
    struct Task *me;
    (void)version;
    b->lib.lib_OpenCnt++;                       /* first: nothing expunges us while we load */
    b->lib.lib_Flags &= ~LIBF_DELEXP;
    if (b->orig)
        return &b->lib;
    me = FindTask(NULL);
    if (b->loading == me) {
        /* the original's own init opened us again: there is nothing to forward to yet */
        b->lib.lib_OpenCnt--;
        return NULL;
    }
    ObtainSemaphore(&b->lock);
    if (!b->orig) {
        if (me->tc_Node.ln_Type != NT_PROCESS) {
            b->error = PB_ERR_NOTPROC;
        } else {
            b->loading = me;
            if (load_original(b) && !b->spy) spy_from_env(b);
            b->loading = NULL;
        }
    }
    ReleaseSemaphore(&b->lock);
    if (!b->orig) {
        b->lib.lib_OpenCnt--;
        return NULL;
    }
    return &b->lib;
}

/* R1 never goes away: Workbench keeps both libraries open, and the
 * original's code must stay while anything could still forward to it. */
BPTR __attribute__((used)) proxy_close(register struct ProxyBase *b __asm("a6"))
{
    if (b->lib.lib_OpenCnt) b->lib.lib_OpenCnt--;
    return 0;
}

BPTR __attribute__((used)) proxy_expunge(register struct ProxyBase *b __asm("a6"))
{
    (void)b;
    return 0;
}

ULONG __attribute__((used)) proxy_null(void)
{
    return 0;
}

/* ---- the spy ---- */

struct spy_ring *proxy_spy_new(ULONG n)
{
    ULONG bytes = sizeof(struct spy_ring) + (n - 1) * sizeof(struct spy_entry);
    struct spy_ring *r = AllocVec(bytes, MEMF_PUBLIC | MEMF_CLEAR);
    if (!r) return NULL;
    r->magic = SPY_MAGIC;
    r->count = n;
    r->bytes = bytes;
    return r;
}

void proxy_spy_free(struct spy_ring *r)
{
    if (r) {
        r->magic = 0;
        FreeVec(r);
    }
}

/* The task's name: a Shell command's own name when it has one, so LoadWB
 * and IPrefs show as themselves rather than as "Background CLI". */
static void task_name(struct Task *t, char *out)
{
    const char *n = t->tc_Node.ln_Name;
    if (t->tc_Node.ln_Type == NT_PROCESS && ((struct Process *)t)->pr_CLI) {
        struct CommandLineInterface *cli = BADDR(((struct Process *)t)->pr_CLI);
        UBYTE *b = cli->cli_CommandName ? (UBYTE *)BADDR(cli->cli_CommandName) : NULL;
        if (b && b[0]) {
            int len = b[0], i, start = len > SPY_TASK - 1 ? len - (SPY_TASK - 1) : 0;
            for (i = start; i < len; i++) out[i - start] = (char)b[1 + i];
            out[len - start] = 0;
            return;
        }
    }
    copy_str(out, n, SPY_TASK);
}

/* A tag list, as far as SPY_TAGS items, through TAG_MORE and TAG_SKIP. */
static int copy_tags(ULONG *out, const ULONG *t)
{
    int n = 0, hops = 0, guard = 64;
    while (t && readable((ULONG)t) && n < SPY_TAGS && guard--) {
        ULONG tag = t[0], data = t[1];
        if (tag == 0) break;                            /* TAG_DONE */
        if (tag == 1) { t += 2; continue; }             /* TAG_IGNORE */
        if (tag == 2) {                                 /* TAG_MORE */
            if (++hops > 4) break;
            t = (const ULONG *)data;
            continue;
        }
        if (tag == 3) { t += 2 + 2 * data; continue; }  /* TAG_SKIP */
        out[2 * n] = tag;
        out[2 * n + 1] = data;
        n++;
        t += 2;
    }
    return n;
}

static struct spy_entry *spy_slot(struct spy_ring *r)
{
    struct spy_entry *e = &r->e[r->next];
    r->next = r->next + 1 >= r->count ? 0 : r->next + 1;
    memset(e, 0, sizeof *e);
    e->seq = ++r->seq;
    return e;
}

/* regs: d0-d7 then a0-a6, as the stub saved them; a6 is our base. */
void __attribute__((used)) proxy_spy_before(LONG idx, ULONG *regs)
{
    struct ProxyBase *b = (struct ProxyBase *)regs[14];
    struct spy_ring *r;
    struct spy_entry *e;
    const struct spy_func *f;
    static const UBYTE keep[8] = { 0, 1, 2, 8, 9, 10, 11, 12 };    /* d0 d1 d2 a0 a1 a2 a3 a4 */
    int i;
    Forbid();
    r = b->spy;
    if (!r || r->magic != SPY_MAGIC || idx < 0 || idx >= b->nfuncs) {
        Permit();
        return;
    }
    f = &b->funcs[idx];
    e = spy_slot(r);
    e->task = FindTask(NULL);
    e->idx = (UBYTE)idx;
    e->phase = SPY_BEFORE;
    for (i = 0; i < 8; i++) e->regs[i] = regs[keep[i]];
    task_name(e->task, e->task_name);
    if (f->str_reg >= 0 && readable(regs[(int)f->str_reg]))
        copy_str(e->str, (const char *)regs[(int)f->str_reg], SPY_STR);
    if (f->tag_reg >= 0 && regs[(int)f->tag_reg])
        e->ntags = (UBYTE)copy_tags(e->tags, (const ULONG *)regs[(int)f->tag_reg]);
    if (f->raw_reg >= 0 && readable(regs[(int)f->raw_reg])) {
        memcpy(e->raw, (const void *)regs[(int)f->raw_reg], SPY_RAW);
        e->nraw = SPY_RAW;
    }
    Permit();
}

void __attribute__((used)) proxy_spy_after(LONG idx, ULONG result, struct ProxyBase *b)
{
    struct spy_ring *r;
    struct spy_entry *e;
    Forbid();
    r = b->spy;
    if (!r || r->magic != SPY_MAGIC || idx < 0 || idx >= b->nfuncs) {
        Permit();
        return;
    }
    e = spy_slot(r);
    e->task = FindTask(NULL);
    e->idx = (UBYTE)idx;
    e->phase = SPY_AFTER;
    e->regs[0] = result;
    task_name(e->task, e->task_name);
    if (b->funcs[idx].raw_result && readable(result)) {
        memcpy(e->raw, (const void *)result, SPY_RAW);
        e->nraw = SPY_RAW;
    }
    Permit();
}
