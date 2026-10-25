/* ob_show.h: the desktop's No icons (OpenBench 0.1.37; Salvor's OS list, item 59; the user, 24 October 2026, 21:08:
 * "offer a no-icons mode too... for workbench itself"): what OpenBench does when the setting changes, apart from the
 * drawing. Plain C with no Amiga headers, tested on the host (tests/host/test_ob_show.c).
 *
 * The setting is ENV:Sys/DesktopIcons ("none" or "show"; openamigaprefs Common/deskicons.h), set by OpenPrefs' Icons
 * page, by OpenBench's Window > No icons, and turned off by Icons > Clean Up. Its notify shared the signal of
 * ENV:Sys/IconLabels' up to 0.1.38 (the 16 signals a task has were all taken); since 0.1.39 every notify is a message
 * on one port (ob_notify.h), and a change of any of the three asks for one look: the desktop's icons are read again
 * only when IconLabels or IconLayout did change (ob_seen_changed).
 *
 * MIT, Copyright (c) 2026 AmigaChrome Team. */
#ifndef OB_SHOW_H
#define OB_SHOW_H

typedef struct ob_show {
    int none;                               /* 1: no icons on the desktop */
} ob_show;

enum {
    OB_SHOW_KEEP = 0,                       /* nothing changes */
    OB_SHOW_HIDE,                           /* the icons go: none stays selected, a press or a drag is dropped, the
                                             * desktop is cleared to its backdrop; their places are kept */
    OB_SHOW_BACK                            /* they come back where they were: drawn again */
};

void ob_show_init(ob_show *s, int none);

/* The setting as ENV:Sys/DesktopIcons now says (none: 1 for no icons): what to do. OpenBench's own writes come back
 * through the notify as the setting it already has, and change nothing. */
int ob_show_set(ob_show *s, int none);

/* Window > No icons picked, its checkmark now checked: the setting to write into *write (1 none, 0 the icons), and
 * what to do. */
int ob_show_menu(ob_show *s, int checked, int *write);

/* Icons > Clean Up. With No icons on it goes off first: *write 0 and OB_SHOW_BACK; otherwise *write -1 (nothing to
 * write) and OB_SHOW_KEEP. The icons are laid out again either way. */
int ob_show_cleanup(ob_show *s, int *write);

/* 1 while the desktop's icons are drawn and take clicks and drops: disks, icons left out, and AppIcons when the
 * desktop has them */
int ob_show_icons(const ob_show *s);

/* The Window menu's No icons checkmark */
int ob_show_checked(const ob_show *s);

/* What the shared signal last found in ENV:Sys/IconLabels and ENV:Sys/IconLayout. */
#define OB_SEEN_MAX 128
typedef struct ob_seen {
    char labels[OB_SEEN_MAX], layout[OB_SEEN_MAX];
    signed char has_labels, has_layout;     /* 1 there, 0 no variable, -1 not looked at yet */
} ob_seen;

void ob_seen_init(ob_seen *s);

/* 1 when IconLabels or IconLayout (each NULL when there is no variable) differ from what was seen last, which they
 * then are; the first look is a change. Only the first OB_SEEN_MAX - 1 characters count (OpenBench reads less). */
int ob_seen_changed(ob_seen *s, const char *labels, const char *layout);

#endif
