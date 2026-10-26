/* ob_strip.h: where OpenBench's two backdrop windows go on the Workbench screen (OpenBench 0.1.43): the root, under
 * the screen's title bar, as it always was, and the strip, over the bar's rows, so the backdrop goes on behind the
 * bar. OpenTitle makes the bar see-through only where one layer behind it covers it all, left to right and from the
 * top through its last row (openamigaprefs TitleBar/src/opentitle.c, see_backed): ob_covers_bar is that test.
 * Plain C, all static inline, so openbench.c and the host test (tests/host/test_ob_strip.c) share it.
 * bar_height is the screen's BarHeight, one less than the bar's rows, as Intuition keeps it.
 * MIT, Copyright (c) 2026 AmigaChrome Team. */
#ifndef OB_STRIP_H
#define OB_STRIP_H

typedef struct { long left, top, width, height; } ob_box;

/* the root window: from the row after the bar to the screen's bottom */
static inline ob_box ob_root_box(long scr_w, long scr_h, long bar_height)
{
    ob_box b;
    b.left = 0;
    b.top = bar_height + 1;
    b.width = scr_w;
    b.height = scr_h - bar_height - 1;
    return b;
}

/* the strip: the bar's rows, the screen's width */
static inline ob_box ob_strip_box(long scr_w, long bar_height)
{
    ob_box b;
    b.left = 0;
    b.top = 0;
    b.width = scr_w;
    b.height = bar_height + 1;
    return b;
}

/* 1 when a layer with this box covers the bar all through (see_backed's bounds: MinX <= 0, MinY <= 0,
 * MaxX >= width - 1, MaxY >= BarHeight) */
static inline int ob_covers_bar(ob_box b, long scr_w, long bar_height)
{
    return b.left <= 0 && b.top <= 0 && b.left + b.width - 1 >= scr_w - 1 && b.top + b.height - 1 >= bar_height;
}

#endif
