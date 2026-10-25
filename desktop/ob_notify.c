/* ob_notify.c: the ENV: files OpenBench follows, and what a change of each asks for (ob_notify.h).
 * MIT, Copyright (c) 2026 AmigaChrome Team. */
#include "ob_notify.h"

unsigned ob_notify_add(unsigned changed, unsigned long user_data)
{
    if (user_data < OB_FILE_WBPREFS || user_data > OB_FILE_LAST) return changed;
    return changed | 1u << (unsigned)user_data;
}

unsigned ob_notify_todo(unsigned changed)
{
    unsigned todo = 0;
    if (changed & 1u << OB_FILE_WBPREFS) todo |= OB_DO_WBPREFS;
    if (changed & 1u << OB_FILE_PATTERN) todo |= OB_DO_PICTURE;
    if (changed & 1u << OB_FILE_FONT) todo |= OB_DO_FONT;
    if (changed & (1u << OB_FILE_LABELS | 1u << OB_FILE_LAYOUT | 1u << OB_FILE_DESKICONS)) todo |= OB_DO_ICONS;
    return todo;
}
