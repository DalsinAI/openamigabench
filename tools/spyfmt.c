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

int spy_format(char *out, int n, const spy_line *l)
{
    char buf[64];
    int len = 0, i;
    if (n <= 0) return 0;
    out[0] = 0;
    snprintf(buf, sizeof buf, "%06lu [%-15.15s] %s %s(%d)", (unsigned long)l->seq, l->task ? l->task : "",
             l->lib ? l->lib : "?", l->func ? l->func : "?", l->offset);
    add(out, n, &len, buf);
    if (l->after) {
        snprintf(buf, sizeof buf, " -> $%08lX", (unsigned long)l->regs[0]);
        add(out, n, &len, buf);
    } else {
        for (i = 0; i < 8; i++) {
            snprintf(buf, sizeof buf, " %s=$%08lX", reg_names[i], (unsigned long)l->regs[i]);
            add(out, n, &len, buf);
        }
        if (l->str && l->str[0]) {
            add(out, n, &len, " \"");
            add(out, n, &len, l->str);
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
    }
    if (l->nraw > 0 && l->raw) {
        add(out, n, &len, " raw=");
        for (i = 0; i < l->nraw; i++) {
            snprintf(buf, sizeof buf, "%02X", l->raw[i]);
            add(out, n, &len, buf);
        }
        /* and as text, where it is text */
        add(out, n, &len, " '");
        for (i = 0; i < l->nraw; i++) {
            char c[2];
            c[0] = l->raw[i] >= 32 && l->raw[i] < 127 ? (char)l->raw[i] : '.';
            c[1] = 0;
            add(out, n, &len, c);
        }
        add(out, n, &len, "'");
    }
    return len;
}
