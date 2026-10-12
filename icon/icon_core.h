/* icon_core: our icon.library's work on icon files, in portable C (step R2,
 * amigachrome docs/design/Design-Workbench-Replacement.md, section 3).
 *
 * Nothing here includes an Amiga header or calls the system. The same file
 * is built into icon.library, into the host tests (tests/host) and, later,
 * as a native body. Memory comes in through struct ic_env.
 *
 * What it reads, every format an .info can be in:
 *   - the classic DiskObject: the gadget, the planar pictures, the default
 *     tool, tool types, tool window, the drawer's window (and 2.0's
 *     NewDrawerData);
 *   - OS 3.5's FORM ICON after it: FACE, IMAG (palette pictures in the
 *     run-length code), and ARGB (true colour with alpha, zlib-packed, as
 *     AROS writes it);
 *   - NewIcons: palette pictures carried in the IM1= and IM2= tool types;
 *   - PNG icons (AmigaOS 4, MorphOS, AROS): a PNG with an icOn chunk for
 *     the Workbench fields, then a second PNG for the selected picture.
 * PNG and ARGB are inflated by our own decoder, written from RFC 1950,
 * 1951 and the PNG specification.
 *
 * What it writes: each icon in the format it was read in. Picture data
 * that was not changed is written back byte for byte; the fields come from
 * the icon as it is now, so a program that changed the tool types keeps
 * its picture.
 *
 * Clean-room: written from the published file layouts (the Team's own
 * readers, amigachrome scripts/amiga_icon.py and amigachrome-guest
 * tools/icons/amigaicon.py) and the autodocs' descriptions of the calls.
 * No code from Hyperion's or AROS's icon.library.
 *
 * MIT, Copyright (c) 2026 Dalsin Limited. */
#ifndef ICON_CORE_H
#define ICON_CORE_H

#include <stddef.h>
#include <stdint.h>

/* What the core asks of the system. alloc returns cleared memory, or NULL. */
struct ic_env {
    void *(*alloc)(struct ic_env *env, uint32_t size);
    void (*free)(struct ic_env *env, void *mem);
};

/* Results */
#define IC_OK         0
#define IC_ERR_FORMAT 1     /* not an icon, or not one we can read */
#define IC_ERR_SHORT  2     /* the file ends early */
#define IC_ERR_NOMEM  3
#define IC_ERR_DATA   4     /* a picture's data is damaged */

/* workbench/workbench.h's values */
#define IC_DISKMAGIC   0xE310
#define IC_DISKVERSION 1
#define IC_DISKREVISION 1           /* do_Gadget.UserData's low byte: NewDrawerData follows */
#define IC_NO_POSITION 0x80000000UL
#define IC_WBDISK    1
#define IC_WBDRAWER  2
#define IC_WBTOOL    3
#define IC_WBPROJECT 4
#define IC_WBGARBAGE 5
#define IC_WBDEVICE  6
#define IC_WBKICK    7
#define IC_WBAPPICON 8

/* intuition/intuition.h's gadget flags used here */
#define IC_GFLG_GADGHCOMP  0x0000
#define IC_GFLG_GADGHBOX   0x0001
#define IC_GFLG_GADGHIMAGE 0x0002
#define IC_GFLG_GADGIMAGE  0x0004

/* Which formats an icon carried when read (ic_icon.formats). */
#define IC_FMT_CLASSIC  0x01        /* a DiskObject (always, unless PNG) */
#define IC_FMT_OS35     0x02        /* FORM ICON with IMAG pictures */
#define IC_FMT_ARGB     0x04        /* FORM ICON with ARGB pictures */
#define IC_FMT_NEWICONS 0x08        /* IM1=/IM2= tool types */
#define IC_FMT_PNG      0x10        /* a PNG icon */

/* ic_read's flags */
#define IC_READ_NOPICTURES 0x01     /* fields only: no picture decoded (Workbench's
                                     * Information window, ICONGETA_GetPaletteMappedIcon FALSE...) */
#define IC_READ_NONEWICONS 0x02     /* IM1=/IM2= tool types left as tool types */

/* A classic picture: struct Image and its planes, as stored. */
struct ic_planar {
    int16_t left, top;
    uint16_t width, height, depth;
    uint8_t planepick, planeonoff;
    uint8_t *data;                  /* depth planes of ((width + 15) / 16) * 2 * height bytes; may be NULL */
    uint32_t size;
};

/* A palette picture (OS 3.5 IMAG, NewIcons). */
struct ic_palpic {
    uint16_t width, height;
    uint8_t *pixels;                /* one byte a pixel */
    uint16_t ncolours;              /* 0: the other picture's palette is used */
    uint8_t *palette;               /* ncolours * 3 bytes, R G B */
    int16_t transparent;            /* the transparent colour, or -1 */
};

/* A true-colour picture: 4 bytes a pixel, A R G B. */
struct ic_argb {
    uint16_t width, height;
    uint8_t *argb;
};

/* The drawer's window: OldDrawerData (a struct NewWindow and the scroll
 * offsets), then 2.0's NewDrawerData. */
struct ic_drawer {
    uint8_t newwindow[48];          /* as stored; left, top, width and height below win over it */
    int16_t left, top;
    uint16_t width, height;
    int32_t currentx, currenty;
    uint32_t flags;                 /* dd_Flags (NewDrawerData) */
    uint16_t viewmodes;             /* dd_ViewModes */
};

struct ic_block;

/* An icon. Strings and arrays may point at memory the icon does not own:
 * the library fills one from a program's DiskObject to write it. */
struct ic_icon {
    uint32_t formats;               /* IC_FMT_ bits, as read */
    /* do_Gadget */
    int16_t gleft, gtop;
    uint16_t gwidth, gheight;
    uint16_t gflags, gactivation, gtype;
    uint32_t guserdata;
    /* the rest of the DiskObject */
    uint8_t type;
    char *defaulttool;
    char **tooltypes;               /* NULL-terminated */
    int32_t currentx, currenty;
    struct ic_drawer *drawer;
    char *toolwindow;
    int32_t stacksize;
    struct ic_planar *planar[2];    /* normal, selected */
    /* OS 3.5 */
    uint8_t has_face, frameless, aspect;
    struct ic_palpic *pal[2];       /* IMAG or NewIcons */
    struct ic_argb *argb[2];        /* ARGB or PNG */
    /* what is written back unchanged */
    const uint8_t *form;            /* the whole FORM ICON, from "FORM" */
    uint32_t formlen;
    const uint8_t *png[2];          /* each PNG frame's bytes, signature to IEND */
    uint32_t pnglen[2];
    uint8_t pictures_changed;       /* set: write the pictures from pal[]/planar, not form */
    struct ic_block *blocks;        /* what this icon owns */
};

/* Reading and writing */
int ic_read(struct ic_env *env, const uint8_t *buf, uint32_t len, uint32_t flags, struct ic_icon **out);
void ic_free(struct ic_env *env, struct ic_icon *icon);
/* Writes the icon to out, if out is not NULL and cap is enough; returns the
 * bytes the icon takes, or 0 when it can't be written. */
uint32_t ic_write(struct ic_env *env, const struct ic_icon *icon, uint8_t *out, uint32_t cap);
/* Memory owned by an icon, freed with it. */
void *ic_own(struct ic_env *env, struct ic_icon *icon, uint32_t size);
char *ic_own_string(struct ic_env *env, struct ic_icon *icon, const char *s);

/* The tool type calls (FindToolType, MatchToolValue, BumpRevision) */
char *ic_find_tool_type(char **tooltypes, const char *name);
int ic_match_tool_value(const char *typestring, const char *value);
/* newname gets at most maxlen characters and a NUL (BumpRevision: 30).
 * Below these maxlens 3.2 leaves newname untouched (measured between 4
 * and 13, and 13 and 22; the next probe gives the exact ones): 0. */
#define IC_BUMP_MIN          9
#define IC_BUMP_MIN_NUMBERED 20
int ic_bump_revision(char *newname, const char *oldname, uint32_t maxlen);

/* The pieces, for the library and the tests */
int ic_inflate(const uint8_t *src, uint32_t srclen, uint8_t *dst, uint32_t dstlen, uint32_t *outlen);
int ic_zlib_inflate(const uint8_t *src, uint32_t srclen, uint8_t *dst, uint32_t dstlen, uint32_t *outlen);
int ic_png_decode(struct ic_env *env, struct ic_icon *owner, const uint8_t *png, uint32_t len, struct ic_argb **out);
int ic_rle_unpack(const uint8_t *src, uint32_t srclen, unsigned bits, uint8_t *dst, uint32_t count);
uint32_t ic_rle_pack(const uint8_t *src, uint32_t count, unsigned bits, uint8_t *dst, uint32_t cap);
void ic_planar_to_chunky(const struct ic_planar *p, uint8_t *dst);
uint32_t ic_crc32(uint32_t crc, const uint8_t *p, uint32_t n);

/* PNG icons' icOn attributes (values as the AmigaOS 4, MorphOS and AROS
 * icons in the Team's icon library carry them; amigachrome
 * scripts/amiga_icon.py). Strings end with a NUL; numbers are 4 bytes. */
#define IC_ATTR_ICONX        0x80001001UL
#define IC_ATTR_ICONY        0x80001002UL
#define IC_ATTR_DRAWERX      0x80001003UL
#define IC_ATTR_DRAWERY      0x80001004UL
#define IC_ATTR_DRAWERWIDTH  0x80001005UL
#define IC_ATTR_DRAWERHEIGHT 0x80001006UL
#define IC_ATTR_DRAWERFLAGS  0x80001007UL
#define IC_ATTR_TOOLWINDOW   0x80001008UL   /* string */
#define IC_ATTR_STACKSIZE    0x80001009UL
#define IC_ATTR_DEFAULTTOOL  0x8000100AUL   /* string */
#define IC_ATTR_TOOLTYPE     0x8000100BUL   /* string, one per tool type */
#define IC_ATTR_VIEWMODES    0x8000100CUL
#define IC_ATTR_DD_CURRENTX  0x8000100DUL
#define IC_ATTR_DD_CURRENTY  0x8000100EUL
#define IC_ATTR_TYPE         0x8000100FUL
#define IC_ATTR_FRAMELESS    0x80001010UL
#define IC_ATTR_DRAWERFLAGS3 0x80001011UL
#define IC_ATTR_VIEWMODES2   0x80001012UL
#define IC_ATTR_DRAWERFLAGS2 0x80001107UL

#endif
