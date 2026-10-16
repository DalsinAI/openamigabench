/* ob_builtin: programs built into OpenBench (the Team, 11 October 2026: the Open preferences in OpenBench
 * natively, as the dock is). Each is its own program's code, compiled into OpenBench with -DIN_OPENBENCH and
 * its main() renamed (-Dmain=<name>_main), listed in ob_builtin_list.h. When its path is opened (a drawer,
 * the dock, the cog, OBOpen: SYS:Prefs/OpenLook, or just "OpenLook") OpenBench starts it as a process of its
 * own, with no input or output, as Workbench starts a tool; one at a time: opened again while it runs, it is
 * sent Ctrl-F (a built-in brings its window to the front on Ctrl-F).
 *
 * The rules a built-in follows (as OpenDock's code does inside OpenBench):
 *  - its library bases are OpenBench's (extern under IN_OPENBENCH): it opens no library OpenBench has open,
 *    or opens its own under its own names;
 *  - its globals are static, or named for it, so two built-ins never share one;
 *  - malloc, calloc, realloc and free are OpenBench's (ob_malloc.c: AllocVec, safe from any task), so the C
 *    library's heap is never shared between tasks; no stdio (Output() is 0);
 *  - it returns from its main() when it is done, everything freed, and quits on Ctrl-C (OpenBench sends it
 *    when it quits).
 *
 * MIT, Copyright (c) 2026 AmigaChrome Team. */
#ifndef OB_BUILTIN_H
#define OB_BUILTIN_H

#include <exec/types.h>
#include <exec/tasks.h>

/* 1 when path names a built-in (it is started, or sent Ctrl-F when it runs already), else 0. */
int ob_builtin_open(const char *path);
/* How many are running. */
int ob_builtin_running(void);
/* Ctrl-C to every one running, then up to 5 s for them to finish. */
void ob_builtin_quit_all(void);

#endif
