/* ob_png: OpenBench's own PNG reader (the user, 11 October 2026: "we just write our own then, side
 * step the issue": Hyperion's picture.datatype wrote to a wild address when asked for its pixels on
 * Instance-32). Written from the PNG specification and RFC 1950/1951; no datatype, no other library.
 *
 * Reads 8-bit greyscale, grey with alpha, RGB, RGBA and palette pictures (16-bit ones are taken to 8),
 * not interlaced. The picture comes back as 0xAARRGGBB words, a row after the other, in memory from
 * AllocVec (MEMF_FAST when there is any), so it can be decoded in a process of its own: nothing here
 * uses the C library's heap.
 *
 * MIT, Copyright (c) 2026 Dalsin Limited. */
#ifndef OB_PNG_H
#define OB_PNG_H

#include <exec/types.h>

/* The picture in the file, or NULL (not a PNG it reads, or no memory). Free it with FreeVec. */
ULONG *ob_png_load(const char *path, LONG *width, LONG *height);

#endif
