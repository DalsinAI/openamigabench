/* wb_core: the Team's workbench.library lists, in portable C. See wb_core.h.
 * MIT, Copyright (c) 2026 Dalsin Limited. */
#include "wb_core.h"

/* the tags, workbench/workbench.h */
#define T_APPICON_SUPPORTS_FIRST WBC_TAG(1)    /* SupportsOpen .. SupportsFormatDisk: 1..10 */
#define T_APPICON_SUPPORTS_LAST  WBC_TAG(10)
#define T_APPICON_EMPTYTRASH     WBC_TAG(11)
#define T_APPICON_PROPAGATE      WBC_TAG(12)
#define T_APPICON_RENDERHOOK     WBC_TAG(13)
#define T_APPICON_NOTIFYSELECT   WBC_TAG(14)
#define T_APPICON_EJECTDISK      WBC_TAG(158)
#define T_APPMENU_COMMANDKEY     WBC_TAG(15)
#define T_APPMENU_GETKEY         WBC_TAG(65)
#define T_APPMENU_USEKEY         WBC_TAG(66)
#define T_APPMENU_GETTITLEKEY    WBC_TAG(77)
#define T_DZ_LEFT      WBC_TAG(26)
#define T_DZ_RELRIGHT  WBC_TAG(27)
#define T_DZ_TOP       WBC_TAG(28)
#define T_DZ_RELBOTTOM WBC_TAG(29)
#define T_DZ_WIDTH     WBC_TAG(30)
#define T_DZ_RELWIDTH  WBC_TAG(31)
#define T_DZ_HEIGHT    WBC_TAG(32)
#define T_DZ_RELHEIGHT WBC_TAG(33)
#define T_DZ_HOOK      WBC_TAG(35)
/* WBDZA_Box (34) carries a struct IBox pointer: the library reads it and
 * passes Left, Top, Width and Height instead. */

/* ---- small helpers (no C library) ---- */

static uint32_t slen(const char *p)
{
    uint32_t n = 0;
    while (p && p[n])
        n++;
    return n;
}

static char *sdup(struct wbc_env *env, const char *p)
{
    uint32_t n = slen(p), i;
    char *d = env->alloc(env, n + 1);
    if (!d)
        return NULL;
    for (i = 0; i < n; i++)
        d[i] = p[i];
    d[n] = 0;
    return d;
}

static int upper(int c)
{
    return (c >= 'a' && c <= 'z') ? c - 32 : c;
}

/* names compared without case, as AmigaDOS compares device names */
static int same_name(const char *a, const char *b)
{
    while (*a && *b && upper((unsigned char)*a) == upper((unsigned char)*b)) {
        a++;
        b++;
    }
    return *a == 0 && *b == 0;
}

static void append(struct wbc_obj **head, struct wbc_obj *o)
{
    while (*head)
        head = &(*head)->next;
    o->next = NULL;
    *head = o;
}

static int unlink_obj(struct wbc_obj **head, struct wbc_obj *o)
{
    for (; *head; head = &(*head)->next)
        if (*head == o) {
            *head = o->next;
            o->next = NULL;
            return 1;
        }
    return 0;
}

static struct wbc_obj *new_obj(struct wbc_state *s, enum wbc_kind kind, uint32_t id, uint32_t userdata, void *port)
{
    struct wbc_obj *o = s->env->alloc(s->env, sizeof *o);
    if (!o)
        return NULL;
    o->priv = o;
    o->magic = WBC_MAGIC;
    o->kind = (uint8_t)kind;
    o->serial = ++s->serial;
    o->id = id;
    o->userdata = userdata;
    o->port = port;
    return o;
}

static void free_obj(struct wbc_state *s, struct wbc_obj *o)
{
    if (o->kind == WBC_APPICON && o->u.ai.text)
        s->env->free(s->env, o->u.ai.text);
    if (o->kind == WBC_APPMENU && o->u.mi.text)
        s->env->free(s->env, o->u.mi.text);
    o->magic = 0;
    o->priv = NULL;
    s->env->free(s->env, o);
}

/* ---- the state ---- */

void wbc_init(struct wbc_state *s, struct wbc_env *env)
{
    struct wbc_state z = { 0 };
    *s = z;
    s->env = env;
    s->next_key = 0;
    s->set.default_stack = 4096;            /* the autodoc's default */
    s->set.type_restart = 3;                /* OS 3.2.3's, measured (WorkbenchABITest, lab wblib-a) */
    /* App objects added before the desktop is up are kept, and shown once it
     * is (the user's decision 3, 13 October 2026). OS 3.2 refuses them
     * instead (wb.doc NOTES; WorkbenchABITest): set this to 1 for its rule. */
    s->refuse_without_desktop = 0;
}

void wbc_free_all(struct wbc_state *s)
{
    struct wbc_obj *o, *n, *z, *zn;
    struct wbc_name *h, *hn;
    struct wbc_hooknode *k, *kn;
    for (o = s->windows; o; o = n) {
        n = o->next;
        for (z = o->u.aw.zones; z; z = zn) {
            zn = z->next;
            free_obj(s, z);
        }
        free_obj(s, o);
    }
    for (o = s->icons; o; o = n) {
        n = o->next;
        free_obj(s, o);
    }
    for (o = s->menus; o; o = n) {
        n = o->next;
        free_obj(s, o);
    }
    for (h = s->set.hidden; h; h = hn) {
        hn = h->next;
        s->env->free(s->env, h);
    }
    for (k = s->set.setup_cleanup; k; k = kn) {
        kn = k->next;
        s->env->free(s->env, k);
    }
    s->windows = s->icons = s->menus = NULL;
    s->set.hidden = NULL;
    s->set.setup_cleanup = NULL;
}

int wbc_empty(const struct wbc_state *s)
{
    return !s->windows && !s->icons && !s->menus;
}

struct wbc_obj *wbc_lookup(const struct wbc_state *s, enum wbc_kind kind, const void *handle)
{
    const struct wbc_obj *o, *z;
    if (!handle)
        return NULL;
    switch (kind) {
    case WBC_APPWINDOW: o = s->windows; break;
    case WBC_APPICON: o = s->icons; break;
    case WBC_APPMENU: o = s->menus; break;
    case WBC_DROPZONE:
        for (o = s->windows; o; o = o->next)
            for (z = o->u.aw.zones; z; z = z->next)
                if ((const void *)z == handle)
                    return (struct wbc_obj *)z;
        return NULL;
    default: return NULL;
    }
    for (; o; o = o->next)
        if ((const void *)o == handle)
            return (struct wbc_obj *)o;
    return NULL;
}

unsigned wbc_count(const struct wbc_state *s, enum wbc_kind kind)
{
    const struct wbc_obj *o, *z;
    unsigned n = 0;
    switch (kind) {
    case WBC_APPWINDOW: for (o = s->windows; o; o = o->next) n++; break;
    case WBC_APPICON: for (o = s->icons; o; o = o->next) n++; break;
    case WBC_APPMENU: for (o = s->menus; o; o = o->next) n++; break;
    case WBC_DROPZONE:
        for (o = s->windows; o; o = o->next)
            for (z = o->u.aw.zones; z; z = z->next)
                n++;
        break;
    }
    return n;
}

static int may_add(const struct wbc_state *s)
{
    return s->desktop || !s->refuse_without_desktop;
}

/* ---- AppWindows and drop zones ---- */

int wbc_add_appwindow(struct wbc_state *s, uint32_t id, uint32_t userdata, void *window, void *port,
                      const struct wbc_tag *tags, struct wbc_obj **out)
{
    struct wbc_obj *o;
    (void)tags;                             /* none are defined for AddAppWindowA */
    *out = NULL;
    if (!may_add(s))
        return WBC_ERR_NODESK;
    /* A NULL window or port is taken, as 3.2 takes them (WorkbenchABITest,
     * lab wblib-a): nothing is sent to a NULL port, and a NULL window is
     * never under the pointer. */
    if (!(o = new_obj(s, WBC_APPWINDOW, id, userdata, port)))
        return WBC_ERR_NOMEM;
    o->u.aw.window = window;
    append(&s->windows, o);
    wbc_note(s, WBC_NOTE_APPWINDOWS);
    *out = o;
    return WBC_OK;
}

int wbc_remove_appwindow(struct wbc_state *s, void *handle)
{
    struct wbc_obj *o = wbc_lookup(s, WBC_APPWINDOW, handle), *z, *zn;
    if (!o)
        return WBC_ERR_UNKNOWN;
    /* "Before the AppWindow is removed, all its drop zones will be removed first." */
    for (z = o->u.aw.zones; z; z = zn) {
        zn = z->next;
        free_obj(s, z);
    }
    o->u.aw.zones = NULL;
    unlink_obj(&s->windows, o);
    free_obj(s, o);
    wbc_note(s, WBC_NOTE_APPWINDOWS);
    return WBC_OK;
}

int wbc_add_dropzone(struct wbc_state *s, void *aw, uint32_t id, uint32_t userdata,
                     const struct wbc_tag *tags, struct wbc_obj **out)
{
    struct wbc_obj *w = wbc_lookup(s, WBC_APPWINDOW, aw), *z;
    struct wbc_zonebox b = { 0, 0, 0, 0, 0, 0, 0, 0 };
    void *hook = NULL;
    unsigned given = 0;                     /* 1 left, 2 top, 4 width, 8 height */
    *out = NULL;
    if (!aw)
        return WBC_ERR_ARGS;
    if (!w)
        return WBC_ERR_UNKNOWN;
    for (; tags && tags->tag; tags++) {
        int16_t v = (int16_t)tags->data;
        switch (tags->tag) {
        case T_DZ_LEFT: b.left = v; b.rel_right = 0; given |= 1; break;
        case T_DZ_RELRIGHT: b.left = v; b.rel_right = 1; given |= 1; break;
        case T_DZ_TOP: b.top = v; b.rel_bottom = 0; given |= 2; break;
        case T_DZ_RELBOTTOM: b.top = v; b.rel_bottom = 1; given |= 2; break;
        case T_DZ_WIDTH: b.width = v; b.rel_width = 0; given |= 4; break;
        case T_DZ_RELWIDTH: b.width = v; b.rel_width = 1; given |= 4; break;
        case T_DZ_HEIGHT: b.height = v; b.rel_height = 0; given |= 8; break;
        case T_DZ_RELHEIGHT: b.height = v; b.rel_height = 1; given |= 8; break;
        case T_DZ_HOOK: hook = (void *)(uintptr_t)tags->data; break;
        default: break;
        }
    }
    /* OS 3.2 refuses a zone unless its left, top, width and height are all
     * given, each fixed or relative, with IoErr 116, ERROR_REQUIRED_ARG_MISSING
     * (WorkbenchABITest, lab wblib-a). Their values are not checked there. */
    if (given != 15)
        return WBC_ERR_ARGS;
    if (!(z = new_obj(s, WBC_DROPZONE, id, userdata, w->port)))
        return WBC_ERR_NOMEM;
    z->u.dz.aw = w;
    z->u.dz.box = b;
    z->u.dz.hook = hook;
    append(&w->u.aw.zones, z);
    wbc_note(s, WBC_NOTE_APPWINDOWS);
    *out = z;
    return WBC_OK;
}

int wbc_remove_dropzone(struct wbc_state *s, void *aw, void *zone)
{
    struct wbc_obj *w = wbc_lookup(s, WBC_APPWINDOW, aw), *z;
    if (!aw || !zone)
        return WBC_ERR_ARGS;                /* "A value of NULL will be ignored": FALSE, IoErr 116 (measured) */
    if (!w)
        return WBC_ERR_UNKNOWN;
    for (z = w->u.aw.zones; z; z = z->next)
        if ((void *)z == zone)
            break;
    if (!z)
        return WBC_ERR_UNKNOWN;             /* "not registered with the AppWindow" */
    unlink_obj(&w->u.aw.zones, z);
    free_obj(s, z);
    wbc_note(s, WBC_NOTE_APPWINDOWS);
    return WBC_OK;
}

struct wbc_obj *wbc_find_appwindow(const struct wbc_state *s, void *window)
{
    struct wbc_obj *o;
    for (o = s->windows; o; o = o->next)
        if (o->u.aw.window == window)
            return o;
    return NULL;
}

int wbc_zone_box(const struct wbc_obj *zone, int32_t win_w, int32_t win_h, struct wbc_box *out)
{
    const struct wbc_zonebox *b = &zone->u.dz.box;
    int32_t l = b->rel_right ? win_w + b->left : b->left;
    int32_t t = b->rel_bottom ? win_h + b->top : b->top;
    int32_t w = b->rel_width ? win_w + b->width : b->width;
    int32_t h = b->rel_height ? win_h + b->height : b->height;
    int32_t r = l + w, d = t + h;
    /* "The zone is guaranteed to be a valid area": clipped to the window */
    if (l < 0) l = 0;
    if (t < 0) t = 0;
    if (r > win_w) r = win_w;
    if (d > win_h) d = win_h;
    out->left = l;
    out->top = t;
    out->width = r > l ? r - l : 0;
    out->height = d > t ? d - t : 0;
    return out->width > 0 && out->height > 0;
}

struct wbc_obj *wbc_zone_at(const struct wbc_obj *aw, int32_t x, int32_t y, int32_t win_w, int32_t win_h)
{
    struct wbc_obj *z;
    struct wbc_box b;
    for (z = aw->u.aw.zones; z; z = z->next)
        if (wbc_zone_box(z, win_w, win_h, &b) && x >= b.left && y >= b.top &&
            x < b.left + b.width && y < b.top + b.height)
            return z;
    return NULL;
}

/* ---- AppIcons ---- */

int wbc_add_appicon(struct wbc_state *s, uint32_t id, uint32_t userdata, const char *text, void *port,
                    uint32_t lock, void *diskobj, const struct wbc_tag *tags, struct wbc_obj **out)
{
    struct wbc_obj *o;
    uint32_t sup = 1u << WBC_AMCLASS_OPEN;  /* "This tag defaults to TRUE" for Open alone */
    void *hook = NULL;
    uint8_t prop = 0, notify = 0;
    *out = NULL;
    if (!may_add(s))
        return WBC_ERR_NODESK;
    if (!diskobj)
        return WBC_ERR_ARGS;                /* its picture is needed to draw it; a NULL port is taken, as 3.2's */
    for (; tags && tags->tag; tags++) {
        uint32_t t = tags->tag, bit = 32;
        if (t >= T_APPICON_SUPPORTS_FIRST && t <= T_APPICON_SUPPORTS_LAST)
            bit = t - T_APPICON_SUPPORTS_FIRST;     /* SupportsOpen is AMCLASSICON_Open (0), and so on */
        else if (t == T_APPICON_EMPTYTRASH)
            bit = WBC_AMCLASS_EMPTYTRASH;
        else if (t == T_APPICON_EJECTDISK)
            bit = WBC_AMCLASS_EJECTDISK;
        else if (t == T_APPICON_PROPAGATE)
            prop = tags->data != 0;
        else if (t == T_APPICON_RENDERHOOK)
            hook = (void *)(uintptr_t)tags->data;
        else if (t == T_APPICON_NOTIFYSELECT)
            notify = tags->data != 0;
        if (bit < 32) {
            if (tags->data)
                sup |= 1u << bit;
            else
                sup &= ~(1u << bit);
        }
    }
    if (!(o = new_obj(s, WBC_APPICON, id, userdata, port)))
        return WBC_ERR_NOMEM;
    if (text && !(o->u.ai.text = sdup(s->env, text))) {
        free_obj(s, o);
        return WBC_ERR_NOMEM;
    }
    o->u.ai.diskobj = diskobj;
    o->u.ai.lock = lock;
    o->u.ai.supports = sup;
    o->u.ai.render_hook = hook;
    o->u.ai.propagate = prop;
    o->u.ai.notify_select = notify;
    append(&s->icons, o);
    wbc_note(s, WBC_NOTE_APPICONS);
    *out = o;
    return WBC_OK;
}

int wbc_remove_appicon(struct wbc_state *s, void *handle)
{
    struct wbc_obj *o = wbc_lookup(s, WBC_APPICON, handle);
    if (!o)
        return WBC_ERR_UNKNOWN;
    unlink_obj(&s->icons, o);
    free_obj(s, o);
    wbc_note(s, WBC_NOTE_APPICONS);
    return WBC_OK;
}

int wbc_appicon_supports(const struct wbc_obj *icon, unsigned amclass)
{
    return amclass < 32 && (icon->u.ai.supports & (1u << amclass)) != 0;
}

/* ---- AppMenuItems ---- */

int wbc_is_separator(const char *text)
{
    /* V44: "any menu label consisting entirely of '-', '_' or '~'" */
    if (!text || !*text)
        return 0;
    for (; *text; text++)
        if (*text != '-' && *text != '_' && *text != '~')
            return 0;
    return 1;
}

static struct wbc_obj *by_key(const struct wbc_state *s, uint32_t key)
{
    struct wbc_obj *o;
    if (!key)
        return NULL;
    for (o = s->menus; o; o = o->next)
        if (o->u.mi.key == key)
            return o;
    return NULL;
}

static unsigned children(const struct wbc_state *s, const struct wbc_obj *parent)
{
    struct wbc_obj *o;
    unsigned n = 0;
    for (o = s->menus; o; o = o->next)
        if (o->u.mi.parent == parent && o->u.mi.role != WBC_MENU_TITLE && !(o->flags & WBC_OF_REMOVED))
            n++;
    return n;
}

static unsigned titles(const struct wbc_state *s)
{
    struct wbc_obj *o;
    unsigned n = 0;
    for (o = s->menus; o; o = o->next)
        if (o->u.mi.role == WBC_MENU_TITLE)
            n++;
    return n;
}

static int key_in_use(const struct wbc_state *s, char k)
{
    struct wbc_obj *o;
    for (o = s->menus; o; o = o->next)
        if (o->u.mi.command_key && upper((unsigned char)o->u.mi.command_key) == upper((unsigned char)k))
            return 1;
    return 0;
}

int wbc_add_appmenu(struct wbc_state *s, uint32_t id, uint32_t userdata, const char *text, void *port,
                    const struct wbc_tag *tags, struct wbc_obj **out, uint32_t *key_out, int *key_given)
{
    struct wbc_obj *o, *parent = NULL;
    const char *cmd = NULL;
    int get_key = 0, get_title = 0, use = 0;
    uint32_t use_key = 0;
    uint8_t role = WBC_MENU_ITEM;
    *out = NULL;
    *key_out = 0;
    *key_given = 0;
    if (!may_add(s))
        return WBC_ERR_NODESK;
    /* A NULL label and a NULL port are taken, as 3.2 takes them
     * (WorkbenchABITest RISKY, lab wblib-a): the item has an empty label. */
    for (; tags && tags->tag; tags++) {
        switch (tags->tag) {
        case T_APPMENU_COMMANDKEY: cmd = (const char *)(uintptr_t)tags->data; break;
        case T_APPMENU_GETKEY: get_key = tags->data != 0; break;          /* a ULONG * */
        case T_APPMENU_GETTITLEKEY: get_title = tags->data != 0; break;   /* a ULONG * */
        case T_APPMENU_USEKEY: use = 1; use_key = tags->data; break;
        default: break;
        }
    }
    if (use && use_key) {
        if (!(parent = by_key(s, use_key)))
            return WBC_ERR_KEY;
        if (parent->u.mi.role == WBC_MENU_PARENT) {
            if (children(s, parent) >= WBC_MAX_SUBITEMS)
                return WBC_ERR_FULL;
            get_key = 0;                    /* a sub item has no sub items of its own */
            get_title = 0;
        } else if (parent->u.mi.role == WBC_MENU_TITLE) {
            if (children(s, parent) >= WBC_MAX_TITLEITEMS)
                return WBC_ERR_FULL;
            get_title = 0;
        } else {
            return WBC_ERR_KEY;
        }
    }
    if (get_title) {
        role = WBC_MENU_TITLE;
        if (titles(s) >= WBC_MAX_TITLES)
            return WBC_ERR_FULL;
    } else {
        if (get_key)
            role = WBC_MENU_PARENT;
        if (!parent && children(s, NULL) >= WBC_MAX_TOOLS)
            return WBC_ERR_FULL;
    }
    if (!(o = new_obj(s, WBC_APPMENU, id, userdata, port)))
        return WBC_ERR_NOMEM;
    if (!(o->u.mi.text = sdup(s->env, text ? text : ""))) {
        free_obj(s, o);
        return WBC_ERR_NOMEM;
    }
    o->u.mi.separator = (uint8_t)wbc_is_separator(text);
    o->u.mi.role = role;
    o->u.mi.parent = parent;
    /* "If the string is empty, it will be ignored. Also, if the command key
     * is already in use by a different menu item it will be ignored, too." */
    if (cmd && cmd[0] && !o->u.mi.separator && role != WBC_MENU_TITLE && !key_in_use(s, cmd[0]))
        o->u.mi.command_key = cmd[0];
    if (role != WBC_MENU_ITEM) {
        o->u.mi.key = ++s->next_key;        /* never 0: "always check for the key value to be non-zero" */
        *key_out = o->u.mi.key;
        *key_given = 1;
    }
    append(&s->menus, o);
    wbc_note(s, WBC_NOTE_MENUS);
    *out = o;
    return WBC_OK;
}

int wbc_remove_appmenu(struct wbc_state *s, void *handle)
{
    struct wbc_obj *o = wbc_lookup(s, WBC_APPMENU, handle), *c;
    if (!o)
        return WBC_ERR_UNKNOWN;
    /* Items still attached to it stay the program's to remove; they no
     * longer show, as nothing holds them. */
    for (c = s->menus; c; c = c->next)
        if (c->u.mi.parent == o) {
            c->u.mi.parent = NULL;
            c->flags |= WBC_OF_REMOVED;
        }
    unlink_obj(&s->menus, o);
    free_obj(s, o);
    wbc_note(s, WBC_NOTE_MENUS);
    return WBC_OK;
}

int wbc_menu_visible(const struct wbc_state *s, const struct wbc_obj *m)
{
    if (m->flags & WBC_OF_REMOVED)
        return 0;                           /* its parent or title went */
    if (m->u.mi.role != WBC_MENU_ITEM && !children(s, m))
        return 0;                           /* "will not appear until ... the first sub item" */
    return 1;
}

/* ---- WorkbenchControlA's settings ---- */

int wbc_add_hidden(struct wbc_state *s, const char *name)
{
    struct wbc_name *n, **p;
    uint32_t len = slen(name), i;
    if (!name || !len)
        return WBC_ERR_ARGS;
    for (p = &s->set.hidden; *p; p = &(*p)->next)
        if (same_name((*p)->name, name))
            return WBC_OK;                  /* already there */
    if (!(n = s->env->alloc(s->env, (uint32_t)sizeof *n + len)))
        return WBC_ERR_NOMEM;
    for (i = 0; i < len; i++)
        n->name[i] = name[i];
    n->name[len] = 0;
    *p = n;
    wbc_note(s, WBC_NOTE_HIDDEN);
    return WBC_OK;
}

int wbc_remove_hidden(struct wbc_state *s, const char *name)
{
    struct wbc_name **p, *n;
    if (!name)
        return WBC_ERR_ARGS;
    for (p = &s->set.hidden; *p; p = &(*p)->next)
        if (same_name((*p)->name, name)) {
            n = *p;
            *p = n->next;
            s->env->free(s->env, n);
            wbc_note(s, WBC_NOTE_HIDDEN);
            return WBC_OK;
        }
    return WBC_ERR_UNKNOWN;
}

int wbc_is_hidden(const struct wbc_state *s, const char *name)
{
    const struct wbc_name *n;
    for (n = s->set.hidden; n; n = n->next)
        if (same_name(n->name, name))
            return 1;
    return 0;
}

int wbc_add_setup_cleanup(struct wbc_state *s, void *hook)
{
    struct wbc_hooknode *n, **p;
    if (!hook)
        return WBC_ERR_ARGS;
    for (p = &s->set.setup_cleanup; *p; p = &(*p)->next)
        if ((*p)->hook == hook)
            return WBC_OK;
    if (!(n = s->env->alloc(s->env, sizeof *n)))
        return WBC_ERR_NOMEM;
    n->hook = hook;
    *p = n;
    return WBC_OK;
}

int wbc_rem_setup_cleanup(struct wbc_state *s, void *hook)
{
    struct wbc_hooknode **p, *n;
    for (p = &s->set.setup_cleanup; *p; p = &(*p)->next)
        if ((*p)->hook == hook) {
            n = *p;
            *p = n->next;
            s->env->free(s->env, n);
            return WBC_OK;
        }
    return WBC_ERR_UNKNOWN;
}

/* ---- notes ---- */

void wbc_note(struct wbc_state *s, uint32_t bits)
{
    s->notes |= bits;
}

uint32_t wbc_take_notes(struct wbc_state *s)
{
    uint32_t n = s->notes;
    s->notes = 0;
    return n;
}

/* ---- AppMessages ---- */

static void put16(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v >> 8);
    p[1] = (uint8_t)v;
}

static void put32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v >> 24);
    p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8);
    p[3] = (uint8_t)v;
}

static uint32_t get16(const uint8_t *p)
{
    return ((uint32_t)p[0] << 8) | p[1];
}

static uint32_t get32(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3];
}

void wbc_appmsg_encode(const struct wbc_appmsg *m, uint8_t buf[WBC_APPMSG_SIZE])
{
    int i;
    put16(buf + WBC_AM_OFF_TYPE, m->type);
    put32(buf + WBC_AM_OFF_USERDATA, m->userdata);
    put32(buf + WBC_AM_OFF_ID, m->id);
    put32(buf + WBC_AM_OFF_NUMARGS, (uint32_t)m->numargs);
    put32(buf + WBC_AM_OFF_ARGLIST, m->arglist);
    put16(buf + WBC_AM_OFF_VERSION, WBC_AM_VERSION);
    put16(buf + WBC_AM_OFF_CLASS, m->amclass);
    put16(buf + WBC_AM_OFF_MOUSEX, (uint16_t)m->mousex);
    put16(buf + WBC_AM_OFF_MOUSEY, (uint16_t)m->mousey);
    put32(buf + WBC_AM_OFF_SECONDS, m->seconds);
    put32(buf + WBC_AM_OFF_MICROS, m->micros);
    for (i = WBC_AM_OFF_RESERVED; i < WBC_APPMSG_SIZE; i++)
        buf[i] = 0;
}

void wbc_appmsg_decode(const uint8_t buf[WBC_APPMSG_SIZE], struct wbc_appmsg *m)
{
    m->type = (uint16_t)get16(buf + WBC_AM_OFF_TYPE);
    m->userdata = get32(buf + WBC_AM_OFF_USERDATA);
    m->id = get32(buf + WBC_AM_OFF_ID);
    m->numargs = (int32_t)get32(buf + WBC_AM_OFF_NUMARGS);
    m->arglist = get32(buf + WBC_AM_OFF_ARGLIST);
    m->amclass = (uint16_t)get16(buf + WBC_AM_OFF_CLASS);
    m->mousex = (int16_t)get16(buf + WBC_AM_OFF_MOUSEX);
    m->mousey = (int16_t)get16(buf + WBC_AM_OFF_MOUSEY);
    m->seconds = get32(buf + WBC_AM_OFF_SECONDS);
    m->micros = get32(buf + WBC_AM_OFF_MICROS);
}

void wbc_appmsg_for(const struct wbc_obj *o, struct wbc_appmsg *m)
{
    struct wbc_appmsg z = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    *m = z;
    m->id = o->id;
    m->userdata = o->userdata;
    switch (o->kind) {
    case WBC_APPWINDOW: m->type = WBC_AMTYPE_APPWINDOW; break;
    case WBC_DROPZONE: m->type = WBC_AMTYPE_APPWINDOWZONE; break;
    case WBC_APPICON: m->type = WBC_AMTYPE_APPICON; break;
    case WBC_APPMENU: m->type = WBC_AMTYPE_APPMENUITEM; break;
    }
}
