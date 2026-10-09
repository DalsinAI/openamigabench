/* spyfmt: one recorded call as a line of text (wbspy SAVE). Plain C, so the
 * host tests can check it (tests/host/test_spyfmt.c).
 *
 * MIT, Copyright (c) 2026 Dalsin Limited. */
#ifndef SPYFMT_H
#define SPYFMT_H

#include <stdint.h>

typedef struct spy_line {
    uint32_t seq;               /* the call's number in its library */
    long time_ms;               /* milliseconds since the first call saved; -1 when there is no clock */
    int time_us;                /* and the microseconds over (0-999) */
    const char *task;           /* the caller's name */
    const char *lib;            /* "workbench", "icon" */
    const char *func;           /* "WBConfig" */
    int offset;                 /* -84 */
    uint32_t regs[8];           /* d0 d1 d2 a0 a1 a2 a3 a4 as the call was made */
    int returned;               /* 1: result holds what came back */
    uint32_t result;
    const char *str;            /* the string argument, or "" */
    int ntags;
    const uint32_t *tags;       /* ntags pairs */
    int nraw;
    const uint8_t *raw;
} spy_line;

/* Writes the line (no newline) into out, at most n bytes with the 0. Returns
 * its length. Strings are written with C's escapes, so one call is always
 * one line, whatever its strings hold. */
int spy_format(char *out, int n, const spy_line *l);

#endif
