/* wbspy: records the calls into our workbench.library and icon.library in
 * step R1, for the lab only (design section 7). Its log is the contract
 * for the private entries (StartWorkbench, WBConfig, QuoteWorkbench):
 * every call, from which program, with what, and what came back.
 *
 *   wbspy STATUS                 each library: the original's base, the spy
 *   wbspy START [ENTRIES n]      the spy on, keeping the last n calls (2048)
 *   wbspy SAVE file              the calls kept so far, oldest first, as text
 *   wbspy STOP                   the spy off, and its memory given back
 *   ... WORKBENCH or ICON        one library only (both without either)
 *
 * To record the boot as well: SetEnv SAVE OpenBench/Spy-workbench.library 4096
 * (and Spy-icon.library), reboot, then wbspy SAVE.
 *
 * MIT, Copyright (c) 2026 Dalsin Limited. */
#include <string.h>
#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/rdargs.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include "../proxy/proxy.h"
#include "spyfmt.h"

static const char version[] = "$VER: wbspy 0.1 (9.10.2026) OpenBench R1, MIT, Copyright (c) 2026 Dalsin Limited";

#define TEMPLATE "STATUS/S,START/S,ENTRIES/K/N,SAVE/K,STOP/S,WORKBENCH/S,ICON/S"
enum { A_STATUS, A_START, A_ENTRIES, A_SAVE, A_STOP, A_WORKBENCH, A_ICON, A_COUNT };

static const char *const names[2] = { "workbench.library", "icon.library" };
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
    struct spy_ring *r = b->spy;
    Printf((CONST_STRPTR)"%s %ld.%ld: original %s at $%08lx", (LONG)b->name, (LONG)b->lib.lib_Version,
           (LONG)b->lib.lib_Revision, (LONG)b->private_name, (LONG)b->orig);
    if (b->error) Printf((CONST_STRPTR)" (last error %ld)", b->error);
    if (r) Printf((CONST_STRPTR)"; spy on, %ld calls recorded, %ld kept\n", (LONG)r->seq,
                  (LONG)(r->seq < r->count ? r->seq : r->count));
    else PutStr((CONST_STRPTR)"; spy off\n");
}

static void start(struct ProxyBase *b, ULONG n)
{
    struct spy_ring *r = NULL;
    if (b->spy) {
        Printf((CONST_STRPTR)"%s: the spy is already on\n", (LONG)b->name);
        return;
    }
    if (!(r = proxy_spy_new(n))) {
        Printf((CONST_STRPTR)"%s: no memory for %ld calls\n", (LONG)b->name, (LONG)n);
        return;
    }
    Forbid();
    if (!b->spy) { b->spy = r; r = NULL; }
    Permit();
    if (r) proxy_spy_free(r);
    else Printf((CONST_STRPTR)"%s: the spy is on, keeping the last %ld calls\n", (LONG)b->name, (LONG)n);
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

/* The calls kept, oldest first, appended to the file. 0 when it can't be written. */
static int save(struct ProxyBase *b, int which, BPTR fh)
{
    struct spy_ring *copy = NULL, *r;
    ULONG i, k, bytes = 0;
    static char line[1024];
    Forbid();
    if ((r = b->spy) && r->magic == SPY_MAGIC) {
        bytes = r->bytes;
        /* AllocVec may break the Forbid: the copy is taken after it */
    }
    Permit();
    if (!bytes) return 1;
    if (!(copy = AllocVec(bytes, MEMF_ANY))) return 0;
    Forbid();
    if ((r = b->spy) && r->bytes == bytes) CopyMem(r, copy, bytes);
    else bytes = 0;
    Permit();
    for (k = 0; bytes && k < copy->count; k++) {
        const struct spy_entry *e = &copy->e[(copy->next + k) % copy->count];
        spy_line l;
        if (!e->seq) continue;
        memset(&l, 0, sizeof l);
        l.seq = e->seq;
        l.task = e->task_name;
        l.lib = short_names[which];
        l.func = e->idx < b->nfuncs ? b->funcs[e->idx].name : "?";
        l.offset = -(30 + 6 * (int)e->idx);
        l.after = e->phase == SPY_AFTER;
        for (i = 0; i < 8; i++) l.regs[i] = e->regs[i];
        l.str = e->str;
        l.ntags = e->ntags;
        l.tags = e->tags;
        l.nraw = e->nraw;
        l.raw = e->raw;
        spy_format(line, sizeof line - 1, &l);
        strcat(line, "\n");
        if (FPuts(fh, (CONST_STRPTR)line)) {
            FreeVec(copy);
            return 0;
        }
    }
    FreeVec(copy);
    return 1;
}

int main(void)
{
    LONG args[A_COUNT] = { 0 };
    struct RDArgs *rda;
    struct ProxyBase *base[2] = { NULL, NULL };
    BPTR fh = 0;
    int i, rc = RETURN_OK, want[2];
    ULONG n = 2048;

    (void)version;
    if (!(rda = ReadArgs((CONST_STRPTR)TEMPLATE, args, NULL))) {
        PrintFault(IoErr(), (CONST_STRPTR)"wbspy");
        return RETURN_FAIL;
    }
    want[0] = args[A_WORKBENCH] || !args[A_ICON];
    want[1] = args[A_ICON] || !args[A_WORKBENCH];
    if (args[A_ENTRIES]) n = *(ULONG *)args[A_ENTRIES];
    if (n < 16) n = 16;
    if (n > 65536) n = 65536;
    if (args[A_SAVE] && !(fh = Open((CONST_STRPTR)args[A_SAVE], MODE_NEWFILE))) {
        PrintFault(IoErr(), (CONST_STRPTR)args[A_SAVE]);
        FreeArgs(rda);
        return RETURN_FAIL;
    }
    for (i = 0; i < 2; i++) {
        if (!want[i]) continue;
        if (!(base[i] = open_proxy(names[i]))) { rc = RETURN_WARN; continue; }
        if (args[A_START]) start(base[i], n);
        if (fh && !save(base[i], i, fh)) {
            PrintFault(IoErr(), (CONST_STRPTR)args[A_SAVE]);
            rc = RETURN_FAIL;
        }
        if (args[A_STOP]) stop(base[i]);
        if (args[A_STATUS] || (!args[A_START] && !args[A_STOP] && !fh)) status(base[i]);
        CloseLibrary(&base[i]->lib);
    }
    if (fh) Close(fh);
    FreeArgs(rda);
    return rc;
}

/* The ring: the same layout the libraries use (proxy.h), made here. */
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
