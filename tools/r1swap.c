/* r1swap: for step L2 in a lab only. Under R1, OS 3.2's own Workbench
 * (LoadWB) fails as it starts (tests/amiga/l2-logs/README.md). r1swap
 * lets it start: it swaps the names of R1's workbench.library and the
 * original that R1 loaded privately, so that for a moment
 * "workbench.library" is the original itself.
 *
 *   r1swap ORIGINAL   the original answers to "workbench.library"
 *   r1swap R1         R1 answers to it again (as it did at boot)
 *   r1swap            which one answers now
 *
 * Run ORIGINAL, then LoadWB, then R1: every program that opens
 * workbench.library afterwards gets R1, and wbspy records it. Nothing is
 * loaded or freed; only the two names in exec's list are exchanged.
 *
 * MIT, Copyright (c) 2026 Dalsin Limited. */
#include <string.h>
#include <exec/types.h>
#include <exec/execbase.h>
#include <dos/dos.h>
#include <dos/rdargs.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include "../proxy/proxy.h"

static const char version[] __attribute__((used)) =
    "$VER: r1swap 0.1 (13.10.2026) OpenBench L2 lab tool, MIT, Copyright (c) 2026 Dalsin Limited";

extern struct ExecBase *SysBase;

#define TEMPLATE "ORIGINAL/S,R1/S"

/* R1's base: the library named "workbench.library" or by the original's
 * private name that carries R1's magic. Called under Forbid. */
static struct ProxyBase *find_r1(void)
{
    struct Node *n;
    for (n = SysBase->LibList.lh_Head; n->ln_Succ; n = n->ln_Succ) {
        struct ProxyBase *b = (struct ProxyBase *)n;
        if (n->ln_Name && b->lib.lib_PosSize >= sizeof(struct ProxyBase) && b->magic == PB_MAGIC &&
            b->layout == PB_VERSION && b->name && !strcmp(b->name, "workbench.library"))
            return b;
    }
    return NULL;
}

int main(void)
{
    LONG args[2] = { 0, 0 };
    struct RDArgs *rda;
    struct ProxyBase *b;
    int rc = RETURN_OK;
    if (!(rda = ReadArgs((CONST_STRPTR)TEMPLATE, args, NULL))) {
        PrintFault(IoErr(), (CONST_STRPTR)"r1swap");
        return RETURN_FAIL;
    }
    Forbid();
    b = find_r1();
    if (b && b->orig) {
        struct Library *o = b->orig;
        int r1_public = !strcmp(b->lib.lib_Node.ln_Name, "workbench.library");
        if ((args[0] && r1_public) || (args[1] && !r1_public)) {
            char *t = b->lib.lib_Node.ln_Name;
            b->lib.lib_Node.ln_Name = o->lib_Node.ln_Name;
            o->lib_Node.ln_Name = t;
            r1_public = !r1_public;
        }
        Permit();
        Printf((CONST_STRPTR)"\"workbench.library\" is now %s (R1 at $%08lx, the original at $%08lx)\n",
               (LONG)(r1_public ? "R1" : "the original"), (LONG)b, (LONG)o);
        Printf((CONST_STRPTR)"  R1: \"%s\" %ld.%ld, open %ld; the original: \"%s\" %ld.%ld, open %ld\n",
               (LONG)b->lib.lib_Node.ln_Name, (LONG)b->lib.lib_Version, (LONG)b->lib.lib_Revision,
               (LONG)b->lib.lib_OpenCnt, (LONG)o->lib_Node.ln_Name, (LONG)o->lib_Version,
               (LONG)o->lib_Revision, (LONG)o->lib_OpenCnt);
    } else {
        Permit();
        PutStr((CONST_STRPTR)"r1swap: R1's workbench.library isn't in memory, or has no original\n");
        rc = RETURN_WARN;
    }
    FreeArgs(rda);
    return rc;
}
