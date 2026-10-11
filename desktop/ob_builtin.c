/* ob_builtin: see ob_builtin.h. MIT, Copyright (c) 2026 Dalsin Limited. */
#include <string.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include "ob_builtin.h"

typedef struct ob_builtin {
    const char *name;
    int (*entry)(int, char **);
    ULONG stack;
} ob_builtin;

/* each built-in's main(): declared here, defined in its own code */
#define BUILTIN(n, e, s) int e(int, char **);
#include "ob_builtin_list.h"
#undef BUILTIN

static const ob_builtin builtins[] = {
#define BUILTIN(n, e, s) { n, e, s },
#include "ob_builtin_list.h"
#undef BUILTIN
    { NULL, NULL, 0 }
};

#define NB (sizeof builtins / sizeof builtins[0] - 1)
static struct Task *volatile running_task[NB + 1];

/* The process every built-in starts in: it finds itself by its name, runs, and says it is gone. */
static void builtin_entry(void)
{
    struct Task *me = FindTask(NULL);
    unsigned i;
    for (i = 0; i < NB; i++)
        if (!strcmp(me->tc_Node.ln_Name, builtins[i].name)) break;
    if (i >= NB) return;
    builtins[i].entry(0, NULL);
    Forbid();
    running_task[i] = NULL;
    /* ends in dos.library under Forbid: OpenBench waits for running_task before it goes */
}

/* 1 when path's directory is dir ("SYS:Prefs"): the same place, whatever the path is spelt with. */
static int in_dir(const char *path, const char *dir)
{
    char d[256];
    size_t k = (size_t)((const char *)PathPart((CONST_STRPTR)path) - path);
    BPTR a, b;
    int same = 0;
    if (k >= sizeof d) return 0;
    memcpy(d, path, k);
    d[k] = 0;
    if (!(a = Lock((CONST_STRPTR)d, ACCESS_READ))) return 0;
    if ((b = Lock((CONST_STRPTR)dir, ACCESS_READ))) {
        same = SameLock(a, b) == LOCK_SAME;
        UnLock(b);
    }
    UnLock(a);
    return same;
}

/* Where a built-in may be asked for (Kestrel, Wren and Finch, 11 October 2026: by its name alone, a drawer called
 * Work:Windows or ENVARC:OpenInput would have started an editor): by its bare name ("OpenSound"), or as a file in
 * SYS:Prefs, or in SYS:WBStartup (OpenUp-Setup's first start). Never a drawer. */
static int builtin_place(const char *path)
{
    BPTR l;
    if (!strchr(path, ':') && !strchr(path, '/')) return 1;
    if ((l = Lock((CONST_STRPTR)path, ACCESS_READ))) {
        struct FileInfoBlock *fib = AllocDosObject(DOS_FIB, NULL);
        int dir = fib && Examine(l, fib) && fib->fib_DirEntryType > 0;
        if (fib) FreeDosObject(DOS_FIB, fib);
        UnLock(l);
        if (dir) return 0;
    }
    return in_dir(path, "SYS:Prefs") || in_dir(path, "SYS:WBStartup");
}

int ob_builtin_open(const char *path)
{
    const char *file = (const char *)FilePart((CONST_STRPTR)path);
    unsigned i;
    for (i = 0; i < NB; i++) {
        if (stricmp(file, builtins[i].name)) continue;
        if (!builtin_place(path)) return 0;
        Forbid();
        if (running_task[i]) {
            Signal(running_task[i], SIGBREAKF_CTRL_F);   /* running: to the front */
            Permit();
            return 1;
        }
        Permit();
        {
            struct Process *p = CreateNewProcTags(NP_Entry, (ULONG)builtin_entry, NP_Name, (ULONG)builtins[i].name,
                                                  NP_StackSize, builtins[i].stack, NP_Input, 0, NP_Output, 0,
                                                  NP_CloseInput, FALSE, NP_CloseOutput, FALSE, NP_CurrentDir, 0, TAG_DONE);
            if (!p) return 0;
            running_task[i] = &p->pr_Task;
        }
        return 1;
    }
    return 0;
}

int ob_builtin_running(void)
{
    unsigned i;
    int n = 0;
    for (i = 0; i < NB; i++) if (running_task[i]) n++;
    return n;
}

void ob_builtin_quit_all(void)
{
    unsigned i;
    int t;
    Forbid();
    for (i = 0; i < NB; i++) if (running_task[i]) Signal(running_task[i], SIGBREAKF_CTRL_C);
    Permit();
    for (t = 0; t < 250 && ob_builtin_running(); t++) Delay(1);
}
