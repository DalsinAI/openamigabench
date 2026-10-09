/* stub: one forwarding entry of a proxy library (proxy.h), in assembler.
 *
 *   PROXY_STUB(n, off)   defines stub_n, the library's entry -off.
 *
 * Every register the caller set is passed on untouched: the stub keeps A6
 * (our base) on the stack, loads the original's base, calls the
 * original's own entry at the same offset, and puts A6 back. Library
 * entries take their arguments in registers only, so nothing else needs
 * moving. With the spy on, the stub saves every register and records the
 * call before it is made; proxy_spy_before gives back a token, kept on the
 * stack, with which the result is put into the same record after the call.
 *
 * The spy path's stack, as the result is recorded:
 *   [idx][token][result][base][d0 d1 a0 a1][token slot][return]
 * so the token slot is 24 bytes up when the token is pushed.
 *
 * MIT, Copyright (c) 2026 Dalsin Limited. */
#ifndef PROXY_STUB_H
#define PROXY_STUB_H

#include "proxy.h"

#define PROXY_STUB(N, OFF) __asm(                                   \
    "\t.text\n\t.even\n"                                            \
    "\t.globl _stub_" #N "\n"                                       \
    "_stub_" #N ":\n"                                               \
    "\ttst.l " PB_SPY_S "(a6)\n"                                    \
    "\tbeq.s 2f\n"                                                  \
    /* the spy: a slot for its token, every register saved, the call recorded */ \
    "\tsubq.l #4,sp\n"                                              \
    "\tmovem.l d0-d7/a0-a6,-(sp)\n"                                 \
    "\tmove.l sp,-(sp)\n"                                           \
    "\tmove.l #" #N ",-(sp)\n"                                      \
    "\tjsr _proxy_spy_before\n"                                     \
    "\taddq.l #8,sp\n"                                              \
    "\tmove.l d0,60(sp)\n"                                          \
    "\tmovem.l (sp)+,d0-d7/a0-a6\n"                                 \
    "\tmove.l a6,-(sp)\n"                                           \
    "\tmove.l " PB_ORIG_S "(a6),a6\n"                               \
    "\tjsr -" #OFF "(a6)\n"                                         \
    "\tmove.l (sp)+,a6\n"                                           \
    /* the result put with the call; what the call left in d0-d1/a0-a1 kept */ \
    "\tmovem.l d0-d1/a0-a1,-(sp)\n"                                 \
    "\tmove.l a6,-(sp)\n"                                           \
    "\tmove.l d0,-(sp)\n"                                           \
    "\tmove.l 24(sp),-(sp)\n"                                       \
    "\tmove.l #" #N ",-(sp)\n"                                      \
    "\tjsr _proxy_spy_after\n"                                      \
    "\tlea 16(sp),sp\n"                                             \
    "\tmovem.l (sp)+,d0-d1/a0-a1\n"                                 \
    "\taddq.l #4,sp\n"                                              \
    "\trts\n"                                                       \
    /* no spy: straight through */                                  \
    "2:\tmove.l a6,-(sp)\n"                                         \
    "\tmove.l " PB_ORIG_S "(a6),a6\n"                               \
    "\tjsr -" #OFF "(a6)\n"                                         \
    "\tmove.l (sp)+,a6\n"                                           \
    "\trts\n")

/* The library's first bytes: run as a program, it returns at once. */
#define PROXY_START() __asm(                                        \
    "\t.text\n"                                                     \
    "\t.globl _proxy_start\n"                                       \
    "_proxy_start:\n"                                               \
    "\tmoveq #-1,d0\n"                                              \
    "\trts\n")

#endif
