/* ob_show.c: the desktop's No icons, what OpenBench does when the setting changes (ob_show.h).
 * MIT, Copyright (c) 2026 AmigaChrome Team. */
#include <string.h>

#include "ob_show.h"

void ob_show_init(ob_show *s, int none)
{
    s->none = none != 0;
}

int ob_show_set(ob_show *s, int none)
{
    none = none != 0;
    if (none == s->none) return OB_SHOW_KEEP;
    s->none = none;
    return none ? OB_SHOW_HIDE : OB_SHOW_BACK;
}

int ob_show_menu(ob_show *s, int checked, int *write)
{
    *write = checked != 0;                  /* written even when it was so already: the menu is what the user sees */
    return ob_show_set(s, checked);
}

int ob_show_cleanup(ob_show *s, int *write)
{
    if (!s->none) {
        *write = -1;
        return OB_SHOW_KEEP;
    }
    *write = 0;
    return ob_show_set(s, 0);
}

int ob_show_icons(const ob_show *s)
{
    return !s->none;
}

int ob_show_checked(const ob_show *s)
{
    return s->none;
}

void ob_seen_init(ob_seen *s)
{
    memset(s, 0, sizeof *s);
    s->has_labels = s->has_layout = -1;
}

/* One variable against what was seen of it: 1 when it differs, and it is kept */
static int seen_one(char *was, signed char *had, const char *now)
{
    int changed;
    if (!now) {
        changed = *had != 0;
        *had = 0;
        was[0] = 0;
        return changed;
    }
    changed = *had != 1 || strncmp(was, now, OB_SEEN_MAX - 1) != 0;
    *had = 1;
    strncpy(was, now, OB_SEEN_MAX - 1);
    was[OB_SEEN_MAX - 1] = 0;
    return changed;
}

int ob_seen_changed(ob_seen *s, const char *labels, const char *layout)
{
    int a = seen_one(s->labels, &s->has_labels, labels);
    int b = seen_one(s->layout, &s->has_layout, layout);
    return a || b;
}
