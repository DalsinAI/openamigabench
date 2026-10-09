/* wbspy: records the calls into our workbench.library and icon.library in
 * step R1, for the lab only (design section 7). Its log is the contract
 * for the private entries (StartWorkbench, WBConfig, QuoteWorkbench):
 * every call, from which program, with what, and what came back.
 *
 *   wbspy STATUS                 each library: the original's base, the spy
 *   wbspy START [ENTRIES n]      the spy on, keeping the last n calls (2048)
 *   wbspy SAVE file              the calls kept so far, both libraries merged
 *                                in the order they were made (by the EClock)
 *   wbspy STOP                   the spy off, and its memory given back
 *   ... WORKBENCH or ICON        one library only (both without either)
 *
 * To record the boot as well: SetEnv SAVE OpenBench/Spy-workbench.library 4096
 * (and Spy-icon.library), reboot, then wbspy SAVE.
 *
 * MIT, Copyright (c) 2026 Dalsin Limited. */
#include <stdlib.h>
#include <string.h>
#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/rdargs.h>
#include <devices/timer.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/timer.h>

#include "../proxy/proxy.h"
#include "spyfmt.h"

static const char version[] __attribute__((used)) =
    "$VER: wbspy 0.1 (9.10.2026) OpenBench R1, MIT, Copyright (c) 2026 Dalsin Limited";

#define TEMPLATE "STATUS/S,START/S,ENTRIES/K/N,SAVE/K,STOP/S,WORKBENCH/S,ICON/S"
enum { A_STATUS, A_START, A_ENTRIES, A_SAVE, A_STOP, A_WORKBENCH, A_ICON, A_COUNT };

static const char *const names[2] = { "workbench.library", "icon.library" };

/* The EClock, as the libraries read it for each call (closed at the end of main). */
struct Device *TimerBase;
static struct timerequest timer_io;
static int timer_tried;
static ULONG eclock_freq(void);
static const char *const short_names[2] = { "workbench", "icon" };

/* Our proxy's base, opened, or NULL (and why, said). */
static struct ProxyBase *open_proxy(const char *name)
{
    struct ProxyBase *b = (struct ProxyBase *)OpenLibrary((CONST_STRPTR)name, 0);
    if (!b) {
        Printf((CONST_STRPTR)"wbspy: %s doesn't open\n", (LONG)name);
        return NULL;
    }
    if (b->magic != PB_MAGIC || b->layout != PB_VERSION) {
        Printf((CONST_STRPTR)"wbspy: %s isn't OpenBench's R1 library\n", (LONG)name);
        CloseLibrary(&b->lib);
        return NULL;
    }
    return b;
}

static void status(struct ProxyBase *b)
{
    struct spy_ring *r;
    ULONG seq = 0, count = 0;
    int on;
    /* what STOP may free, read while nothing else runs */
    Forbid();
    r = b->spy;
    if ((on = r && r->magic == SPY_MAGIC)) { seq = r->seq; count = r->count; }
    Permit();
    Printf((CONST_STRPTR)"%s %ld.%ld: original %s at $%08lx", (LONG)b->name, (LONG)b->lib.lib_Version,
           (LONG)b->lib.lib_Revision, (LONG)b->private_name, (LONG)b->orig);
    if (b->error) Printf((CONST_STRPTR)" (last error %ld)", b->error);
    if (on) Printf((CONST_STRPTR)"; spy on, %ld calls recorded, the last %ld kept\n", (LONG)seq,
                   (LONG)(seq < count ? seq : count));
    else PutStr((CONST_STRPTR)"; spy off\n");
}

/* 1 when the spy is on (now or already). */
static int start(struct ProxyBase *b, ULONG n)
{
    struct spy_ring *r;
    if (b->spy) {
        Printf((CONST_STRPTR)"%s: the spy is already on\n", (LONG)b->name);
        return 1;
    }
    if (!(r = proxy_spy_new(n))) {
        Printf((CONST_STRPTR)"%s: no memory for %ld calls\n", (LONG)b->name, (LONG)n);
        return 0;
    }
    Forbid();
    if (!b->spy) { SPY_INSTALL(b, r); r = NULL; }
    Permit();
    if (r) proxy_spy_free(r);
    else Printf((CONST_STRPTR)"%s: the spy is on, keeping the last %ld calls\n", (LONG)b->name, (LONG)n);
    return 1;
}

static void stop(struct ProxyBase *b)
{
    struct spy_ring *r;
    /* calls write to the ring only under Forbid, after looking at b->spy:
     * once it is NULL, nothing is writing */
    Forbid();
    r = b->spy;
    b->spy = NULL;
    Permit();
    if (r) {
        proxy_spy_free(r);
        Printf((CONST_STRPTR)"%s: the spy is off\n", (LONG)b->name);
    }
}

/* Copies of both libraries' rings, taken in one Forbid, so the merged
 * save is the same moment for both. 1, or 0 when a copy couldn't be
 * allocated (a ring that isn't on is simply not there). */
static int snapshot_both(struct ProxyBase *base[2], struct spy_ring *copy[2])
{
    ULONG bytes[2] = { 0, 0 };
    int lib, ok = 1, again = 1, tries = 0;
    while (again && tries++ < 4) {
        again = 0;
        Forbid();
        for (lib = 0; lib < 2; lib++) {
            struct spy_ring *r = base[lib] ? base[lib]->spy : NULL;
            bytes[lib] = r && r->magic == SPY_MAGIC ? r->bytes : 0;
        }
        Permit();
        /* allocated outside the Forbid, which AllocVec may break */
        for (lib = 0; lib < 2; lib++) {
            if (copy[lib]) { FreeVec(copy[lib]); copy[lib] = NULL; }
            if (bytes[lib] && !(copy[lib] = AllocVec(bytes[lib], MEMF_ANY))) ok = 0;
        }
        if (!ok) break;
        Forbid();
        for (lib = 0; lib < 2; lib++) {
            struct spy_ring *r = base[lib] ? base[lib]->spy : NULL;
            ULONG now = r && r->magic == SPY_MAGIC ? r->bytes : 0;
            if (now != bytes[lib]) again = 1;          /* a ring came or went meanwhile */
            else if (now) CopyMem(r, copy[lib], now);
        }
        Permit();
    }
    if (ok && again) ok = 0;
    if (!ok)
        for (lib = 0; lib < 2; lib++)
            if (copy[lib]) { FreeVec(copy[lib]); copy[lib] = NULL; }
    return ok;
}

typedef struct rec {
    const struct spy_entry *e;
    int lib;
} rec;

static int by_time;

static int compare(const void *pa, const void *pb)
{
    const rec *a = pa, *b = pb;
    if (by_time) {
        if (a->e->time_hi != b->e->time_hi) return a->e->time_hi < b->e->time_hi ? -1 : 1;
        if (a->e->time_lo != b->e->time_lo) return a->e->time_lo < b->e->time_lo ? -1 : 1;
    }
    if (a->lib != b->lib) return a->lib - b->lib;
    return a->e->seq < b->e->seq ? -1 : a->e->seq > b->e->seq;
}

static unsigned long long eclock(const struct spy_entry *e)
{
    return ((unsigned long long)e->time_hi << 32) | e->time_lo;
}

/* Both libraries' calls, in the order they were made, to fh. 0 when they
 * couldn't be copied (no memory) or the file couldn't be written. */
static int save(struct ProxyBase *base[2], BPTR fh)
{
    struct spy_ring *ring[2] = { NULL, NULL };
    rec *recs = NULL;
    ULONG n = 0, k, i, freq = 0;
    int lib, ok = 1;
    unsigned long long first = 0;
    static char line[1024];
    if (!snapshot_both(base, ring)) {
        PutStr((CONST_STRPTR)"wbspy: no memory to copy the recording\n");
        SetIoErr(ERROR_NO_FREE_STORE);
        return 0;
    }
    for (lib = 0; lib < 2; lib++)
        if (ring[lib]) n += ring[lib]->count;
    if (n && !(recs = malloc(n * sizeof *recs))) ok = 0;
    n = 0;
    /* the EClock orders both libraries' calls, when both rings have it at the same rate */
    by_time = 1;
    for (lib = 0; lib < 2; lib++)
        if (ring[lib]) {
            if (!ring[lib]->eclock_freq || (freq && ring[lib]->eclock_freq != freq)) by_time = 0;
            freq = ring[lib]->eclock_freq;
        }
    for (lib = 0; ok && lib < 2; lib++)
        for (k = 0; ring[lib] && k < ring[lib]->count; k++)
            if (ring[lib]->e[k].seq) { recs[n].e = &ring[lib]->e[k]; recs[n].lib = lib; n++; }
    if (ok && n) {
        qsort(recs, n, sizeof *recs, compare);
        first = eclock(recs[0].e);
    }
    for (k = 0; ok && k < n; k++) {
        const struct spy_entry *e = recs[k].e;
        struct ProxyBase *b = base[recs[k].lib];
        spy_line l;
        memset(&l, 0, sizeof l);
        l.seq = e->seq;
        l.time_ms = -1;
        if (by_time && freq) {
            unsigned long long d = eclock(e) - first;
            unsigned long long us = d / freq * 1000000ULL + d % freq * 1000000ULL / freq;
            l.time_ms = (long)(us / 1000);          /* 24 days fit; a recording is minutes */
            l.time_us = (int)(us % 1000);
        }
        l.task = e->task_name;
        l.lib = short_names[recs[k].lib];
        l.func = e->idx < b->nfuncs ? b->funcs[e->idx].name : "?";
        l.offset = -(30 + 6 * (int)e->idx);
        for (i = 0; i < 8; i++) l.regs[i] = e->regs[i];
        l.returned = e->returned;
        l.result = e->result;
        l.str = e->str;
        l.ntags = e->ntags;
        l.tags = e->tags;
        l.nraw = e->nraw;
        l.raw = e->raw;
        spy_format(line, sizeof line - 1, &l);
        strcat(line, "\n");
        if (FPuts(fh, (CONST_STRPTR)line)) ok = 0;
    }
    free(recs);
    for (lib = 0; lib < 2; lib++)
        if (ring[lib]) FreeVec(ring[lib]);
    return ok;
}

int main(void)
{
    LONG args[A_COUNT] = { 0 };
    struct RDArgs *rda;
    struct ProxyBase *base[2] = { NULL, NULL };
    BPTR fh = 0;
    int i, rc = RETURN_OK, want[2];
    ULONG n = 2048;

    if (!(rda = ReadArgs((CONST_STRPTR)TEMPLATE, args, NULL))) {
        PrintFault(IoErr(), (CONST_STRPTR)"wbspy");
        return RETURN_FAIL;
    }
    want[0] = args[A_WORKBENCH] || !args[A_ICON];
    want[1] = args[A_ICON] || !args[A_WORKBENCH];
    if (args[A_ENTRIES]) n = *(ULONG *)args[A_ENTRIES];
    if (n < 16) n = 16;
    if (n > 65536) n = 65536;
    for (i = 0; i < 2; i++)
        if (want[i] && !(base[i] = open_proxy(names[i]))) rc = RETURN_WARN;
    for (i = 0; i < 2; i++)
        if (base[i] && args[A_START] && !start(base[i], n)) rc = RETURN_FAIL;
    if (args[A_SAVE]) {
        /* written as one file, in the order the calls were made */
        if (!(fh = Open((CONST_STRPTR)args[A_SAVE], MODE_NEWFILE)) || !save(base, fh)) {
            PrintFault(IoErr(), (CONST_STRPTR)args[A_SAVE]);
            rc = RETURN_FAIL;
        }
        /* buffered writes may only fail as the file closes */
        if (fh && !Close(fh)) {
            PrintFault(IoErr(), (CONST_STRPTR)args[A_SAVE]);
            rc = RETURN_FAIL;
        }
    }
    for (i = 0; i < 2; i++) {
        if (!base[i]) continue;
        if (args[A_STOP]) stop(base[i]);
        if (args[A_STATUS] || (!args[A_START] && !args[A_STOP] && !args[A_SAVE])) status(base[i]);
        CloseLibrary(&base[i]->lib);
    }
    if (TimerBase) CloseDevice((struct IORequest *)&timer_io);
    FreeArgs(rda);
    return rc;
}

static ULONG eclock_freq(void)
{
    struct EClockVal ev;
    if (!timer_tried) {
        timer_tried = 1;
        if (!OpenDevice((CONST_STRPTR)TIMERNAME, UNIT_ECLOCK, (struct IORequest *)&timer_io, 0))
            TimerBase = timer_io.tr_node.io_Device;
    }
    return TimerBase ? ReadEClock(&ev) : 0;
}

/* The ring: the same layout the libraries use (proxy.h), made here, with
 * the EClock's rate, so a save can turn the calls' times into milliseconds. */
struct spy_ring *proxy_spy_new(ULONG n)
{
    ULONG bytes = sizeof(struct spy_ring) + (n - 1) * sizeof(struct spy_entry);
    struct spy_ring *r = AllocVec(bytes, MEMF_PUBLIC | MEMF_CLEAR);
    if (!r) return NULL;
    r->magic = SPY_MAGIC;
    r->count = n;
    r->bytes = bytes;
    r->eclock_freq = eclock_freq();
    return r;
}

void proxy_spy_free(struct spy_ring *r)
{
    if (r) {
        r->magic = 0;
        FreeVec(r);
    }
}
