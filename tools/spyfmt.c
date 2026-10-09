/* spyfmt: see spyfmt.h.
 * MIT, Copyright (c) 2026 Dalsin Limited. */
#include "spyfmt.h"

#include <stdio.h>
#include <string.h>

static const char *const reg_names[8] = { "d0", "d1", "d2", "a0", "a1", "a2", "a3", "a4" };

/* Appends to out at *len, never past n - 1. */
static void add(char *out, int n, int *len, const char *s)
{
    while (*s && *len < n - 1) out[(*len)++] = *s++;
    out[*len] = 0;
}

/* One character as C would write it inside quote marks q, into buf (at
 * least 5 bytes). Bytes C has no name for are written as three octal
 * digits, which end there, whatever follows. */
static void escape(char *buf, unsigned char c, char q)
{
    if (c == '\\' || c == (unsigned char)q) { buf[0] = '\\'; buf[1] = (char)c; buf[2] = 0; }
    else if (c == '\n') strcpy(buf, "\\n");
    else if (c == '\r') strcpy(buf, "\\r");
    else if (c == '\t') strcpy(buf, "\\t");
    else if (c < 32 || c >= 127) snprintf(buf, 5, "\\%03o", c);
    else { buf[0] = (char)c; buf[1] = 0; }
}

static void add_char(char *out, int n, int *len, unsigned char c, char q)
{
    char buf[8];
    escape(buf, c, q);
    add(out, n, len, buf);
}

int spy_format(char *out, int n, const spy_line *l)
{
    char buf[64];
    int len = 0, i;
    if (n <= 0) return 0;
    out[0] = 0;
    snprintf(buf, sizeof buf, "%06lu ", (unsigned long)l->seq);
    add(out, n, &len, buf);
    if (l->time_ms >= 0) {
        snprintf(buf, sizeof buf, "%8ld.%03dms ", l->time_ms, l->time_us);
        add(out, n, &len, buf);
    }
    {
        /* the caller's name, escaped like any string, in 15 columns: an
         * escape that wouldn't fit whole is left out, never cut */
        char name[20], piece[8];
        int nl = 0;
        const unsigned char *p = (const unsigned char *)(l->task ? l->task : "");
        for (; *p; p++) {
            int pl;
            escape(piece, *p, ']');
            pl = (int)strlen(piece);
            if (nl + pl > 15) break;
            memcpy(name + nl, piece, (size_t)pl);
            nl += pl;
        }
        while (nl < 15) name[nl++] = ' ';
        name[nl] = 0;
        snprintf(buf, sizeof buf, "[%s] %s %s(%d)", name, l->lib ? l->lib : "?", l->func ? l->func : "?", l->offset);
        add(out, n, &len, buf);
    }
    for (i = 0; i < 8; i++) {
        snprintf(buf, sizeof buf, " %s=$%08lX", reg_names[i], (unsigned long)l->regs[i]);
        add(out, n, &len, buf);
    }
    if (l->str && l->str[0]) {
        const unsigned char *p = (const unsigned char *)l->str;
        add(out, n, &len, " \"");
        for (; *p; p++) add_char(out, n, &len, *p, '"');
        add(out, n, &len, "\"");
    }
    if (l->ntags > 0 && l->tags) {
        add(out, n, &len, " tags={");
        for (i = 0; i < l->ntags; i++) {
            snprintf(buf, sizeof buf, "%s$%08lX=$%08lX", i ? ", " : "", (unsigned long)l->tags[2 * i],
                     (unsigned long)l->tags[2 * i + 1]);
            add(out, n, &len, buf);
        }
        add(out, n, &len, "}");
    }
    if (l->returned) snprintf(buf, sizeof buf, " -> $%08lX", (unsigned long)l->result);
    else snprintf(buf, sizeof buf, " -> (no result recorded)");
    add(out, n, &len, buf);
    if (l->nraw > 0 && l->raw) {
        add(out, n, &len, " raw=");
        for (i = 0; i < l->nraw; i++) {
            snprintf(buf, sizeof buf, "%02X", l->raw[i]);
            add(out, n, &len, buf);
        }
        /* and as text, where it is text */
        add(out, n, &len, " '");
        for (i = 0; i < l->nraw; i++) {
            unsigned char c = l->raw[i];
            if (c >= 32 && c < 127) add_char(out, n, &len, c, '\'');
            else add(out, n, &len, ".");
        }
        add(out, n, &len, "'");
    }
    return len;
}
