/* proxy: what our workbench.library and icon.library share in step R1
 * (amigachrome docs/design/Design-Workbench-Replacement.md, sections 4.5
 * and 8). Each library forwards every entry to Hyperion's original, which
 * it loads privately from SYS:Storage/OpenUp-Superseded/ and links into
 * exec's list under a private name, so OpenLibrary("workbench.library")
 * still finds ours. The spy (wbspy) records every call while it is on.
 *
 * The base: struct Library, then our fields at fixed offsets, because the
 * forwarding stubs read them from assembler (PB_ORIG, PB_SPY).
 *
 * MIT, Copyright (c) 2026 Dalsin Limited. */
#ifndef PROXY_H
#define PROXY_H

#include <exec/types.h>
#include <exec/libraries.h>
#include <exec/semaphores.h>
#include <dos/dos.h>

#define PB_MAGIC 0x4F42504EUL       /* "OBPN": an OpenBench proxy base */
#define PB_VERSION 1                /* this layout */

/* Where the stubs find the original's base and the spy (bytes from the base). */
#define PB_ORIG 40
#define PB_SPY 44
#define PB_ORIG_S "40"
#define PB_SPY_S "44"

struct spy_ring;

struct ProxyBase {
    struct Library lib;             /* 34 bytes */
    UWORD pad;                      /* 34 */
    ULONG magic;                    /* 36: PB_MAGIC */
    struct Library *orig;           /* 40: the original, opened by its private name */
    struct spy_ring *spy;           /* 44: the spy's ring, or NULL */
    UWORD layout;                   /* PB_VERSION */
    UWORD nfuncs;                   /* how many entries the table has */
    BPTR seglist;                   /* ours */
    BPTR orig_seg;                  /* the original's */
    struct SignalSemaphore lock;    /* loading the original */
    struct Task *loading;           /* the task loading it, while it does */
    const char *name;               /* "workbench.library" */
    const char *private_name;       /* "OpenUp.original.workbench.library" */
    const struct spy_func *funcs;   /* each entry's name and what the spy copies (wbspy reads the names) */
    LONG error;                     /* why the original couldn't be had (IoErr, or ours below) */
    struct Library *orig_linked;    /* the original, made and linked, before it is opened */
};

/* Our own error codes in ProxyBase.error (above DOS's). */
#define PB_ERR_NOROMTAG 1001        /* the file has no RomTag */
#define PB_ERR_INIT 1002            /* its init gave no library */
#define PB_ERR_OPEN 1003            /* opening it by its private name failed */
#define PB_ERR_NOTPROC 1004         /* first opened by a task, which can't load files */

/* ---- the spy (wbspy) ---- */

#define SPY_STR 48
#define SPY_TAGS 8
#define SPY_RAW 32
#define SPY_TASK 16

/* One call: written as it is made, completed with its result when it
 * returns (if its slot hasn't been reused by then). */
struct spy_entry {
    ULONG seq;                      /* 0 when the slot was never written */
    ULONG time_hi, time_lo;         /* the EClock when it was made: both libraries' calls merge by it */
    struct Task *task;
    UBYTE idx;                      /* the entry: 0 is the first function (-30) */
    UBYTE returned;                 /* 1 once the result is in */
    UBYTE ntags;
    UBYTE nraw;
    ULONG regs[8];                  /* d0 d1 d2 a0 a1 a2 a3 a4 as the call was made */
    ULONG result;                   /* d0 when it returned */
    char task_name[SPY_TASK];
    char str[SPY_STR];              /* the call's string argument, when it has one */
    ULONG tags[SPY_TAGS * 2];       /* its tag list, when it has one (tag, data) */
    UBYTE raw[SPY_RAW];             /* bytes behind a pointer, for the private calls */
};

struct spy_ring {
    ULONG magic;                    /* SPY_MAGIC */
    ULONG count;                    /* calls the ring keeps */
    ULONG next;                     /* the next slot to write */
    ULONG seq;                      /* the last sequence number given: the calls recorded */
    ULONG bytes;                    /* the allocation, for FreeVec's sake (AllocVec) */
    ULONG eclock_freq;              /* EClock ticks a second, 0 when there is no clock */
    struct spy_entry e[1];
};
#define SPY_MAGIC 0x53505932UL      /* "SPY2" */

/* What proxy_spy_before gives the stub, and the stub gives back with the
 * result: the slot, and the low bits of the call's sequence number, so a
 * slot reused meanwhile is left alone. 0: nothing recorded. */
#define SPY_TOKEN(seq, slot) (0x80000000UL | (((seq) & 0x7FFFUL) << 16) | (slot))

/* What the spy copies for each entry (indexes into the saved registers:
 * 0-7 are d0-d7, 8-14 are a0-a6). -1: nothing. */
struct spy_func {
    const char *name;
    BYTE str_reg;                   /* a string argument */
    BYTE tag_reg;                   /* a tag list */
    BYTE raw_reg;                   /* bytes behind a register before the call */
    BYTE raw_result;                /* 1: bytes behind the result after the call */
};

#define R_D0 0
#define R_D1 1
#define R_D2 2
#define R_A0 8
#define R_A1 9
#define R_A2 10
#define R_A3 11
#define R_A4 12

/* Called by the stubs, in the caller's task, with its stack: small. */
ULONG proxy_spy_before(LONG idx, ULONG *regs);
void proxy_spy_after(LONG idx, ULONG token, ULONG result, struct ProxyBase *base);

/* The ring for a library: a new one of n entries (AllocVec, MEMF_PUBLIC). */
struct spy_ring *proxy_spy_new(ULONG n);
void proxy_spy_free(struct spy_ring *r);

/* The library's lifecycle, shared by both (proxy.c). The library's own
 * file gives its tables. */
struct ProxyDef {
    const char *name;               /* "workbench.library" */
    const char *private_name;
    const char *id;                 /* the ID string */
    UWORD version, revision;        /* exactly 3.2.3's */
    UWORD nfuncs;
    const struct spy_func *funcs;   /* nfuncs entries */
};

extern const struct ProxyDef proxy_def;  /* in each library's own file */

#endif
