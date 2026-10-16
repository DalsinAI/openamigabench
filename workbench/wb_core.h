/* wb_core: the Team's workbench.library lists, in portable C (step L1 of
 * docs/design/Design-Open-Workbench-Library.md). workbench.library is
 * OpenBench itself (the user's decisions of 13 October 2026); this is the
 * part of it that only keeps lists.
 *
 * The parts of workbench.library that only keep lists and settings:
 *   - AppWindows and their drop zones (AddAppWindowA, RemoveAppWindow,
 *     AddAppWindowDropZoneA, RemoveAppWindowDropZone), with the zones'
 *     boxes worked out against a window's size and the zone under a point;
 *   - AppIcons (AddAppIconA, RemoveAppIcon) and what each one supports;
 *   - AppMenuItems (AddAppMenuItemA, RemoveAppMenuItem): the Tools menu,
 *     sub items, new menu titles, the keys that join them, separators,
 *     command keys and the limits the autodoc gives;
 *   - WorkbenchControlA's settings: the default stack size, the type
 *     restart time, the global flags, the hidden devices, the copy, delete,
 *     text input and disk info hooks, and the setup and cleanup hooks;
 *   - the notes the desktop is sent when any of these change, kept as
 *     bits so that none is lost and none floods it;
 *   - an AppMessage's bytes, at the offsets of workbench/workbench.h.
 *
 * Nothing here includes an Amiga header, calls the system, draws, or
 * hooks anything. The same file is built into the library (openbench.c,
 * which holds the semaphore around every call) and into the host tests
 * (tests/host/test_wb_core.c). Memory comes in through struct wbc_env.
 *
 * A handle a program is given is the address of one of the library's objects, and
 * it is only ever looked up in the library's lists, never read before it is found,
 * so a stale or made-up handle is answered "not found", not followed.
 *
 * Clean-room: written from the NDK 3.2 autodoc (wb.doc) and header
 * (workbench/workbench.h), and from what WorkbenchABITest measures on
 * OS 3.2.3. No code from Hyperion's or AROS's workbench.library.
 *
 * MIT, Copyright (c) 2026 AmigaChrome Team. */
#ifndef WB_CORE_H
#define WB_CORE_H

#include <stddef.h>
#include <stdint.h>

/* What the core asks of the system. alloc returns cleared memory, or NULL. */
struct wbc_env {
    void *(*alloc)(struct wbc_env *env, uint32_t size);
    void (*free)(struct wbc_env *env, void *mem);
};

/* Results. WBC_OK is 0; the others are why a call failed. The library
 * turns them into OS 3.2's answer (NULL or FALSE, and IoErr where 3.2
 * sets it). */
#define WBC_OK          0
#define WBC_ERR_NOMEM   1
#define WBC_ERR_ARGS    2   /* a required argument is NULL */
#define WBC_ERR_UNKNOWN 3   /* not the library's, or already removed */
#define WBC_ERR_FULL    4   /* a menu limit is reached */
#define WBC_ERR_KEY     5   /* a WBAPPMENUA_UseKey that names nothing */
#define WBC_ERR_NODESK  6   /* no desktop, and the rule is to refuse */

/* workbench/workbench.h's values the core needs */
#define WBC_TAG_BASE  (0x80000000UL + 0xA000UL)         /* WBA_Dummy */
#define WBC_TAG(n)    (WBC_TAG_BASE + (n))
#define WBC_AMTYPE_APPWINDOW     7
#define WBC_AMTYPE_APPICON       8
#define WBC_AMTYPE_APPMENUITEM   9
#define WBC_AMTYPE_APPWINDOWZONE 10
#define WBC_AM_VERSION           1

/* The kinds of object */
enum wbc_kind { WBC_APPWINDOW = 1, WBC_DROPZONE, WBC_APPICON, WBC_APPMENU };

/* An AppIcon's "Icons" menu items (WBAPPICONA_Supports...), one bit each,
 * by the AMCLASSICON_ number of the menu item. */
#define WBC_AMCLASS_OPEN        0
#define WBC_AMCLASS_COPY        1
#define WBC_AMCLASS_RENAME      2
#define WBC_AMCLASS_INFORMATION 3
#define WBC_AMCLASS_SNAPSHOT    4
#define WBC_AMCLASS_UNSNAPSHOT  5
#define WBC_AMCLASS_LEAVEOUT    6
#define WBC_AMCLASS_PUTAWAY     7
#define WBC_AMCLASS_DELETE      8
#define WBC_AMCLASS_FORMATDISK  9
#define WBC_AMCLASS_EMPTYTRASH  10
#define WBC_AMCLASS_SELECTED    11
#define WBC_AMCLASS_UNSELECTED  12
#define WBC_AMCLASS_EJECTDISK   13

/* The menu limits, from the autodoc's AddAppMenuItemA NOTES:
 * "You can add only a maximum of 62 menu items to the Tools menu",
 * "Only a maximum of 31 sub items can be added to a menu item",
 * "a maximum of 27 menu entries to the Workbench menu strip",
 * "a maximum of 63 menu items can be added to a menu entry". */
#define WBC_MAX_TOOLS     62
#define WBC_MAX_SUBITEMS  31
#define WBC_MAX_TITLES    27
#define WBC_MAX_TITLEITEMS 63

/* A tag list as the core sees it: the library hands it the pairs it has
 * walked with utility.library's NextTagItem, ending with tag 0. */
struct wbc_tag {
    uint32_t tag;
    uintptr_t data;                 /* a ULONG on the 68k; wide enough for a pointer on a host */
};

/* ---- the objects ---- */

/* A drop zone's box: each edge either from the window's left or top, or
 * relative to its right or bottom (WBDZA_RelRight...), and each size
 * either fixed or relative to the window's (WBDZA_RelWidth...). */
struct wbc_zonebox {
    int16_t left, top, width, height;
    uint8_t rel_right, rel_bottom, rel_width, rel_height;
};

struct wbc_box {
    int32_t left, top, width, height;
};

struct wbc_obj;

/* Every object starts with the word the NDK's stub structures have
 * (struct AppWindow { void *aw_PRIVATE; } and the others), so a handle
 * points at memory of the size programs expect. */
struct wbc_obj {
    void *priv;                     /* the stub's word: the object itself */
    uint32_t magic;
    uint8_t kind;                   /* enum wbc_kind */
    uint8_t flags;                  /* WBC_OF_ */
    uint16_t pad;
    struct wbc_obj *next;           /* in its list, oldest first */
    uint32_t serial;                /* the order objects were added in */
    uint32_t id, userdata;          /* the program's own, back in its AppMessages */
    void *port;                     /* the program's MsgPort (opaque here) */
    union {
        struct {                    /* WBC_APPWINDOW */
            void *window;           /* the program's Window (opaque) */
            struct wbc_obj *zones;  /* its drop zones, oldest first */
        } aw;
        struct {                    /* WBC_DROPZONE */
            struct wbc_obj *aw;     /* the AppWindow it belongs to */
            struct wbc_zonebox box;
            void *hook;             /* WBDZA_Hook */
        } dz;
        struct {                    /* WBC_APPICON */
            char *text;             /* the library's copy of the label */
            void *diskobj;          /* the program's DiskObject (opaque) */
            uint32_t lock;          /* "Currently unused" (a BPTR) */
            uint32_t supports;      /* 1 << WBC_AMCLASS_ for each menu item it answers */
            void *render_hook;      /* WBAPPICONA_RenderHook */
            uint8_t propagate;      /* WBAPPICONA_PropagatePosition */
            uint8_t notify_select;  /* WBAPPICONA_NotifySelectState */
        } ai;
        struct {                    /* WBC_APPMENU */
            char *text;             /* the library's copy of the label */
            char command_key;       /* 0, or the first character of WBAPPMENUA_CommandKeyString */
            uint8_t separator;      /* a label of only '-', '_' or '~' (V44) */
            uint8_t role;           /* WBC_MENU_ */
            uint8_t pad2;
            uint32_t key;           /* the key this item gave out (GetKey, GetTitleKey), or 0 */
            struct wbc_obj *parent; /* the item or title it was added under (UseKey), or NULL: Tools */
        } mi;
    } u;
};

#define WBC_MAGIC 0x57424331UL      /* "WBC1" */
#define WBC_OF_REMOVED 0x01

/* What an AppMenuItem is */
#define WBC_MENU_ITEM   0           /* an item: in Tools, under a title, or a sub item */
#define WBC_MENU_PARENT 1           /* an item that sub items attach to (WBAPPMENUA_GetKey) */
#define WBC_MENU_TITLE  2           /* a new menu title (WBAPPMENUA_GetTitleKey) */

/* ---- the settings WorkbenchControlA keeps ---- */

struct wbc_name {
    struct wbc_name *next;
    char name[1];                   /* allocated to its length */
};

struct wbc_hooknode {
    struct wbc_hooknode *next;
    void *hook;
};

struct wbc_settings {
    uint32_t default_stack;         /* WBCTRLA_Get/SetDefaultStackSize; the autodoc: 4096 */
    uint32_t type_restart;          /* WBCTRLA_Get/SetTypeRestartTime, seconds */
    uint32_t global_flags;          /* WBCTRLA_Get/SetGlobalFlags (V47) */
    struct wbc_name *hidden;        /* WBCTRLA_Add/RemoveHiddenDeviceName, oldest first */
    void *copy_hook, *delete_hook, *text_input_hook, *disk_info_hook;
    struct wbc_hooknode *setup_cleanup;     /* WBCTRLA_Add/RemSetupCleanupHook, oldest first */
};

/* ---- notes to the desktop ---- */

#define WBC_NOTE_APPWINDOWS 0x01    /* an AppWindow or drop zone came or went */
#define WBC_NOTE_APPICONS   0x02    /* an AppIcon came or went: draw the desktop's again */
#define WBC_NOTE_MENUS      0x04    /* an AppMenuItem came or went: build the menus again */
#define WBC_NOTE_SETTINGS   0x08    /* a WorkbenchControlA setting changed */
#define WBC_NOTE_HIDDEN     0x10    /* the hidden devices changed */

/* ---- the whole state, one per library ---- */

struct wbc_state {
    struct wbc_env *env;
    struct wbc_obj *windows;        /* AppWindows, oldest first */
    struct wbc_obj *icons;          /* AppIcons */
    struct wbc_obj *menus;          /* AppMenuItems, titles and parents, in the order added */
    uint32_t serial;                /* the last serial given */
    uint32_t next_key;              /* the next menu key */
    uint32_t notes;                 /* WBC_NOTE_ bits not yet taken by the desktop */
    struct wbc_settings set;
    int refuse_without_desktop;     /* 0 (the default): kept until the desktop is up; 1: OS 3.2's rule, refused */
    int desktop;                    /* 1 while a desktop is registered */
};

void wbc_init(struct wbc_state *s, struct wbc_env *env);
void wbc_free_all(struct wbc_state *s);         /* everything, for Expunge */
int wbc_empty(const struct wbc_state *s);       /* no App object out (Expunge may go) */

/* AppWindows and drop zones */
int wbc_add_appwindow(struct wbc_state *s, uint32_t id, uint32_t userdata, void *window, void *port,
                      const struct wbc_tag *tags, struct wbc_obj **out);
int wbc_remove_appwindow(struct wbc_state *s, void *handle);
int wbc_add_dropzone(struct wbc_state *s, void *aw, uint32_t id, uint32_t userdata,
                     const struct wbc_tag *tags, struct wbc_obj **out);
int wbc_remove_dropzone(struct wbc_state *s, void *aw, void *zone);
struct wbc_obj *wbc_find_appwindow(const struct wbc_state *s, void *window);
/* A zone's box in a window of that size, clipped to it; 0 if it is empty. */
int wbc_zone_box(const struct wbc_obj *zone, int32_t win_w, int32_t win_h, struct wbc_box *out);
/* The zone under (x, y) in the window, the first added winning; NULL if none. */
struct wbc_obj *wbc_zone_at(const struct wbc_obj *aw, int32_t x, int32_t y, int32_t win_w, int32_t win_h);

/* AppIcons */
int wbc_add_appicon(struct wbc_state *s, uint32_t id, uint32_t userdata, const char *text, void *port,
                    uint32_t lock, void *diskobj, const struct wbc_tag *tags, struct wbc_obj **out);
int wbc_remove_appicon(struct wbc_state *s, void *handle);
int wbc_appicon_supports(const struct wbc_obj *icon, unsigned amclass);

/* AppMenuItems. A key given out through WBAPPMENUA_GetKey or GetTitleKey
 * is written to *key_out (the library writes it to the caller's ULONG). */
int wbc_add_appmenu(struct wbc_state *s, uint32_t id, uint32_t userdata, const char *text, void *port,
                    const struct wbc_tag *tags, struct wbc_obj **out, uint32_t *key_out, int *key_given);
int wbc_remove_appmenu(struct wbc_state *s, void *handle);
/* Whether the desktop shows it now: a parent shows once it has a sub
 * item, a title once it has an item (the autodoc's NOTES). */
int wbc_menu_visible(const struct wbc_state *s, const struct wbc_obj *m);
int wbc_is_separator(const char *text);

/* Look a handle up; NULL if it isn't the library's, of that kind. */
struct wbc_obj *wbc_lookup(const struct wbc_state *s, enum wbc_kind kind, const void *handle);
unsigned wbc_count(const struct wbc_state *s, enum wbc_kind kind);

/* WorkbenchControlA's settings */
int wbc_add_hidden(struct wbc_state *s, const char *name);
int wbc_remove_hidden(struct wbc_state *s, const char *name);
int wbc_is_hidden(const struct wbc_state *s, const char *name);
int wbc_add_setup_cleanup(struct wbc_state *s, void *hook);
int wbc_rem_setup_cleanup(struct wbc_state *s, void *hook);

/* Notes */
void wbc_note(struct wbc_state *s, uint32_t bits);
uint32_t wbc_take_notes(struct wbc_state *s);

/* An AppMessage, as the bytes the program reads (big-endian, the 68k's).
 * WBC_APPMSG_SIZE is sizeof(struct AppMessage) with the 68k's 2-byte
 * alignment; the offsets are its fields'. openbench.c checks each against the
 * NDK's struct at compile time. */
#define WBC_APPMSG_SIZE        86
#define WBC_AM_OFF_TYPE        20
#define WBC_AM_OFF_USERDATA    22
#define WBC_AM_OFF_ID          26
#define WBC_AM_OFF_NUMARGS     30
#define WBC_AM_OFF_ARGLIST     34
#define WBC_AM_OFF_VERSION     38
#define WBC_AM_OFF_CLASS       40
#define WBC_AM_OFF_MOUSEX      42
#define WBC_AM_OFF_MOUSEY      44
#define WBC_AM_OFF_SECONDS     46
#define WBC_AM_OFF_MICROS      50
#define WBC_AM_OFF_RESERVED    54

struct wbc_appmsg {
    uint16_t type;                  /* WBC_AMTYPE_ */
    uint32_t userdata, id;
    int32_t numargs;
    uint32_t arglist;               /* the WBArg array's address */
    uint16_t amclass;
    int16_t mousex, mousey;
    uint32_t seconds, micros;
};

/* The message's fields from am_Type on (bytes 20 to 85) into buf; the
 * exec Message in bytes 0 to 19 is the library's to fill. */
void wbc_appmsg_encode(const struct wbc_appmsg *m, uint8_t buf[WBC_APPMSG_SIZE]);
void wbc_appmsg_decode(const uint8_t buf[WBC_APPMSG_SIZE], struct wbc_appmsg *m);
/* The message for an object: its type, id and userdata. */
void wbc_appmsg_for(const struct wbc_obj *o, struct wbc_appmsg *m);

#endif
