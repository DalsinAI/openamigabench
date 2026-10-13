/* wb_core's host test: the AppWindow, drop zone, AppIcon and AppMenuItem
 * lists, the menu keys and limits, WorkbenchControlA's settings, the notes
 * and the AppMessage's bytes. Every allocation is counted, so a leak or a
 * double free fails the test.
 *   test_wb_core
 * MIT, Copyright (c) 2026 Dalsin Limited. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wb_core.h"

static int failures, checks;

#define CHECK(c, ...) do { checks++; if (!(c)) { failures++; printf("FAIL %s:%d: ", __FILE__, __LINE__); \
    printf(__VA_ARGS__); printf("\n"); } } while (0)

static long live;
static int fail_after = -1;     /* make the n-th allocation fail */

static void *t_alloc(struct wbc_env *e, uint32_t n)
{
    (void)e;
    if (fail_after == 0)
        return NULL;
    if (fail_after > 0)
        fail_after--;
    live++;
    return calloc(1, n ? n : 1);
}

static void t_free(struct wbc_env *e, void *p)
{
    (void)e;
    if (p)
        live--;
    free(p);
}

static struct wbc_env env = { t_alloc, t_free };

/* stand-ins for a program's window, port, DiskObject and hooks */
static int win1, win2, port1, port2, dobj, hook1, hook2;

#define TAG(n) WBC_TAG(n)

static void appwindows(void)
{
    struct wbc_state s;
    struct wbc_obj *a, *b, *z1, *z2, *z3;
    struct wbc_box box;
    struct wbc_tag none[] = { { 0, 0 } };
    wbc_init(&s, &env);

    /* the user's decision 3: kept before the desktop is up */
    CHECK(wbc_add_appwindow(&s, 1, 2, &win1, &port1, none, &a) == WBC_OK && a, "no desktop: kept");
    CHECK(wbc_remove_appwindow(&s, a) == WBC_OK, "removed");
    s.desktop = 1;
    /* OS 3.2 takes a NULL window and a NULL port (WorkbenchABITest) */
    CHECK(wbc_add_appwindow(&s, 1, 2, NULL, &port1, none, &a) == WBC_OK && a, "no window");
    CHECK(wbc_remove_appwindow(&s, a) == WBC_OK, "removed");
    CHECK(wbc_add_appwindow(&s, 1, 2, &win1, NULL, none, &a) == WBC_OK && a, "no port");
    CHECK(wbc_remove_appwindow(&s, a) == WBC_OK, "removed");
    CHECK(wbc_add_appwindow(&s, 1, 2, &win1, &port1, none, &a) == WBC_OK && a, "added");
    CHECK(a->priv == a && a->id == 1 && a->userdata == 2, "fields");
    CHECK(wbc_add_appwindow(&s, 3, 4, &win2, &port2, NULL, &b) == WBC_OK, "second, NULL tags");
    CHECK(wbc_count(&s, WBC_APPWINDOW) == 2, "two");
    CHECK(wbc_find_appwindow(&s, &win2) == b, "found by window");
    CHECK(wbc_take_notes(&s) == WBC_NOTE_APPWINDOWS, "noted once");
    CHECK(wbc_take_notes(&s) == 0, "notes taken");

    {   /* zones: fixed, relative to the right and bottom, relative sizes */
        struct wbc_tag t1[] = { { TAG(26), 10 }, { TAG(28), 20 }, { TAG(30), 100 }, { TAG(32), 50 }, { TAG(35), 0x1234 }, { 0, 0 } };
        struct wbc_tag t2[] = { { TAG(27), (uintptr_t)(intptr_t)-60 }, { TAG(29), (uintptr_t)(intptr_t)-40 }, { TAG(30), 50 }, { TAG(32), 30 }, { 0, 0 } };
        struct wbc_tag t3[] = { { TAG(26), 0 }, { TAG(28), 0 }, { TAG(31), (uintptr_t)(intptr_t)-20 }, { TAG(33), (uintptr_t)(intptr_t)-20 }, { 0, 0 } };
        CHECK(wbc_add_dropzone(&s, NULL, 1, 1, t1, &z1) == WBC_ERR_ARGS, "zone on NULL");
        CHECK(wbc_add_dropzone(&s, &win1, 1, 1, t1, &z1) == WBC_ERR_UNKNOWN, "zone on a window, not an AppWindow");
        {   /* all four of left, top, width and height, or OS 3.2 refuses it */
            struct wbc_tag lt[] = { { TAG(26), 5 }, { TAG(28), 5 }, { 0, 0 } };
            struct wbc_tag three[] = { { TAG(26), 0 }, { TAG(28), 0 }, { TAG(30), 10 }, { 0, 0 } };
            CHECK(wbc_add_dropzone(&s, a, 1, 1, NULL, &z1) == WBC_ERR_ARGS && !z1, "no tags");
            CHECK(wbc_add_dropzone(&s, a, 1, 1, lt, &z1) == WBC_ERR_ARGS, "left and top only");
            CHECK(wbc_add_dropzone(&s, a, 1, 1, three, &z1) == WBC_ERR_ARGS, "three of four");
        }
        CHECK(wbc_add_dropzone(&s, a, 11, 12, t1, &z1) == WBC_OK, "zone 1");
        CHECK(wbc_add_dropzone(&s, a, 21, 22, t2, &z2) == WBC_OK, "zone 2");
        CHECK(wbc_add_dropzone(&s, a, 31, 32, t3, &z3) == WBC_OK, "zone 3");
        CHECK(z1->port == &port1 && z1->u.dz.aw == a, "the zone sends to the window's port");
        CHECK(wbc_zone_box(z1, 640, 400, &box) && box.left == 10 && box.top == 20 && box.width == 100 && box.height == 50,
              "zone 1 box %d,%d %dx%d", (int)box.left, (int)box.top, (int)box.width, (int)box.height);
        CHECK(wbc_zone_box(z2, 640, 400, &box) && box.left == 580 && box.top == 360 && box.width == 50 && box.height == 30,
              "zone 2 box %d,%d %dx%d", (int)box.left, (int)box.top, (int)box.width, (int)box.height);
        CHECK(wbc_zone_box(z2, 80, 60, &box) && box.left == 20 && box.top == 20 && box.width == 50 && box.height == 30,
              "zone 2 follows the window");
        CHECK(wbc_zone_box(z3, 200, 100, &box) && box.width == 180 && box.height == 80, "zone 3 relative size");
        CHECK(!wbc_zone_box(z3, 20, 20, &box), "zone 3 empty in a tiny window");
        CHECK(wbc_zone_box(z1, 50, 30, &box) && box.width == 40 && box.height == 10, "clipped to the window");
        CHECK(wbc_zone_at(a, 15, 25, 640, 400) == z1, "point in zone 1 (first added wins over zone 3)");
        CHECK(wbc_zone_at(a, 600, 370, 640, 400) == z2, "point in zone 2");
        CHECK(wbc_zone_at(a, 300, 300, 640, 400) == z3, "point in zone 3 only");
        CHECK(wbc_zone_at(a, 630, 395, 640, 400) == NULL, "point in none");
        CHECK(wbc_count(&s, WBC_DROPZONE) == 3, "three zones");
        CHECK(wbc_lookup(&s, WBC_DROPZONE, z2) == z2, "zone looked up");
        CHECK(wbc_remove_dropzone(&s, b, z2) == WBC_ERR_UNKNOWN, "zone through the wrong window");
        CHECK(wbc_remove_dropzone(&s, NULL, z2) == WBC_ERR_ARGS, "NULL window ignored");
        CHECK(wbc_remove_dropzone(&s, a, NULL) == WBC_ERR_ARGS, "NULL zone ignored");
        CHECK(wbc_remove_dropzone(&s, a, z2) == WBC_OK, "zone 2 removed");
        CHECK(wbc_remove_dropzone(&s, a, z2) == WBC_ERR_UNKNOWN, "zone 2 again");
        CHECK(wbc_count(&s, WBC_DROPZONE) == 2, "two zones");
    }
    /* removing the window removes its zones */
    CHECK(wbc_remove_appwindow(&s, a) == WBC_OK, "window removed");
    CHECK(wbc_count(&s, WBC_DROPZONE) == 0, "its zones went with it");
    CHECK(wbc_remove_appwindow(&s, a) == WBC_ERR_UNKNOWN, "removed twice");
    CHECK(wbc_remove_appwindow(&s, NULL) == WBC_ERR_UNKNOWN, "NULL");
    CHECK(wbc_remove_appwindow(&s, &win2) == WBC_ERR_UNKNOWN, "not a handle");
    CHECK(wbc_remove_appwindow(&s, b) == WBC_OK && wbc_empty(&s), "all gone");
    wbc_free_all(&s);
}

static void appicons(void)
{
    struct wbc_state s;
    struct wbc_obj *i1, *i2;
    char label[] = "Spooler";
    struct wbc_tag t[] = {
        { TAG(1), 0 },              /* SupportsOpen FALSE */
        { TAG(2), 1 },              /* Copy */
        { TAG(4), 1 },              /* Information */
        { TAG(11), 1 },             /* EmptyTrash */
        { TAG(158), 1 },            /* EjectDisk (V47) */
        { TAG(12), 1 },             /* PropagatePosition */
        { TAG(13), 0x5678 },        /* RenderHook */
        { TAG(14), 1 },             /* NotifySelectState */
        { 0, 0 }
    };
    wbc_init(&s, &env);
    s.desktop = 1;
    CHECK(wbc_add_appicon(&s, 1, 2, label, &port1, 0, NULL, NULL, &i1) == WBC_ERR_ARGS, "no DiskObject");
    CHECK(wbc_add_appicon(&s, 1, 2, label, NULL, 0, &dobj, NULL, &i1) == WBC_OK && i1, "no port, as 3.2");
    CHECK(wbc_remove_appicon(&s, i1) == WBC_OK, "removed");
    CHECK(wbc_add_appicon(&s, 1, 2, label, &port1, 0, &dobj, NULL, &i1) == WBC_OK, "added");
    label[0] = 'X';
    CHECK(strcmp(i1->u.ai.text, "Spooler") == 0, "the label is the library's own copy");
    CHECK(wbc_appicon_supports(i1, WBC_AMCLASS_OPEN), "Open by default");
    CHECK(!wbc_appicon_supports(i1, WBC_AMCLASS_COPY), "Copy not by default");
    CHECK(wbc_add_appicon(&s, 3, 4, NULL, &port2, 0, &dobj, t, &i2) == WBC_OK && !i2->u.ai.text, "no label, tags");
    CHECK(!wbc_appicon_supports(i2, WBC_AMCLASS_OPEN), "Open off");
    CHECK(wbc_appicon_supports(i2, WBC_AMCLASS_COPY) && wbc_appicon_supports(i2, WBC_AMCLASS_INFORMATION), "Copy, Information");
    CHECK(wbc_appicon_supports(i2, WBC_AMCLASS_EMPTYTRASH) && wbc_appicon_supports(i2, WBC_AMCLASS_EJECTDISK), "EmptyTrash, EjectDisk");
    CHECK(!wbc_appicon_supports(i2, WBC_AMCLASS_RENAME) && !wbc_appicon_supports(i2, 40), "Rename not; out of range not");
    CHECK(i2->u.ai.propagate && i2->u.ai.notify_select && i2->u.ai.render_hook == (void *)(uintptr_t)0x5678, "the rest");
    CHECK(wbc_take_notes(&s) == WBC_NOTE_APPICONS, "noted");
    CHECK(wbc_remove_appicon(&s, i1) == WBC_OK && wbc_remove_appicon(&s, i1) == WBC_ERR_UNKNOWN, "removed once");
    CHECK(wbc_remove_appicon(&s, NULL) == WBC_ERR_UNKNOWN, "NULL");
    CHECK(wbc_count(&s, WBC_APPICON) == 1, "one left");
    /* an AppIcon handle is not an AppWindow's */
    CHECK(wbc_remove_appwindow(&s, i2) == WBC_ERR_UNKNOWN, "wrong kind");
    wbc_free_all(&s);
    CHECK(wbc_empty(&s), "freed");
}

static void appmenus(void)
{
    struct wbc_state s;
    struct wbc_obj *m, *parent, *title, *sub, *subs[40], *items[70], *sep;
    uint32_t key = 0, tkey = 0, k2;
    int given, i, n;
    struct wbc_tag getkey[] = { { TAG(65), 1 }, { 0, 0 } };
    struct wbc_tag gettitle[] = { { TAG(77), 1 }, { 0, 0 } };
    struct wbc_tag usekey[] = { { TAG(66), 0 }, { 0, 0 } };
    struct wbc_tag cmd[] = { { TAG(15), (uintptr_t)"q" }, { 0, 0 } };
    struct wbc_tag cmd2[] = { { TAG(15), (uintptr_t)"Q" }, { 0, 0 } };
    struct wbc_tag cmd0[] = { { TAG(15), (uintptr_t)"" }, { 0, 0 } };
    wbc_init(&s, &env);
    s.refuse_without_desktop = 1;   /* OS 3.2's rule, kept as an option */
    CHECK(wbc_add_appmenu(&s, 1, 2, "Item", &port1, NULL, &m, &key, &given) == WBC_ERR_NODESK && !m, "OS 3.2's rule: refused");
    s.refuse_without_desktop = 0;
    s.desktop = 1;
    CHECK(wbc_add_appmenu(&s, 1, 2, NULL, &port1, NULL, &m, &key, &given) == WBC_OK && m && m->u.mi.text && !m->u.mi.text[0],
          "no text, as 3.2: an empty label");
    CHECK(wbc_remove_appmenu(&s, m) == WBC_OK, "removed");
    CHECK(wbc_add_appmenu(&s, 1, 2, "Item", NULL, NULL, &m, &key, &given) == WBC_OK && m, "no port, as 3.2");
    CHECK(wbc_remove_appmenu(&s, m) == WBC_OK, "removed");

    /* a command key: the first character, and only if no other item has it */
    CHECK(wbc_add_appmenu(&s, 1, 2, "Quit thing", &port1, cmd, &m, &key, &given) == WBC_OK && m->u.mi.command_key == 'q' && !given,
          "command key");
    CHECK(wbc_add_appmenu(&s, 1, 2, "Other", &port1, cmd2, &items[0], &key, &given) == WBC_OK && items[0]->u.mi.command_key == 0,
          "the same key again is ignored");
    CHECK(wbc_add_appmenu(&s, 1, 2, "Empty", &port1, cmd0, &items[1], &key, &given) == WBC_OK && items[1]->u.mi.command_key == 0,
          "an empty key is ignored");
    CHECK(wbc_add_appmenu(&s, 1, 2, "-~_-", &port1, NULL, &sep, &key, &given) == WBC_OK && sep->u.mi.separator, "separator");
    CHECK(wbc_is_separator("---") && !wbc_is_separator("--x") && !wbc_is_separator(""), "separators");

    /* a parent with sub items */
    CHECK(wbc_add_appmenu(&s, 5, 6, "More", &port1, getkey, &parent, &key, &given) == WBC_OK && given && key != 0, "GetKey");
    CHECK(!wbc_menu_visible(&s, parent), "a parent shows only once it has a sub item");
    usekey[0].data = key;
    CHECK(wbc_add_appmenu(&s, 7, 8, "Sub 1", &port1, usekey, &sub, &k2, &given) == WBC_OK && sub->u.mi.parent == parent && !given,
          "sub item");
    CHECK(wbc_menu_visible(&s, parent) && wbc_menu_visible(&s, sub), "both show now");
    for (n = 1; n < 40; n++)
        if (wbc_add_appmenu(&s, 7, 8, "Sub", &port1, usekey, &subs[n], &k2, &given) != WBC_OK)
            break;
    CHECK(n == WBC_MAX_SUBITEMS, "31 sub items, then full (%d)", n);
    for (i = 1; i < n; i++)
        wbc_remove_appmenu(&s, subs[i]);
    usekey[0].data = 9999;
    CHECK(wbc_add_appmenu(&s, 7, 8, "Lost", &port1, usekey, &m, &k2, &given) == WBC_ERR_KEY, "an unknown key");
    usekey[0].data = 0;
    CHECK(wbc_add_appmenu(&s, 7, 8, "Key 0", &port1, usekey, &items[2], &k2, &given) == WBC_OK && !items[2]->u.mi.parent,
          "UseKey 0 is Tools");

    /* a new title and its items */
    CHECK(wbc_add_appmenu(&s, 9, 9, "Mine", &port1, gettitle, &title, &tkey, &given) == WBC_OK && given && tkey && tkey != key,
          "GetTitleKey");
    CHECK(!wbc_menu_visible(&s, title), "a title shows once it has an item");
    usekey[0].data = tkey;
    CHECK(wbc_add_appmenu(&s, 9, 9, "In mine", &port1, usekey, &items[3], &k2, &given) == WBC_OK && wbc_menu_visible(&s, title),
          "an item under the title");
    CHECK(wbc_count(&s, WBC_APPMENU) == 9, "nine items (%u)", wbc_count(&s, WBC_APPMENU));
    CHECK(wbc_take_notes(&s) == WBC_NOTE_MENUS, "noted");

    /* removing a parent leaves its sub item to the program, hidden */
    CHECK(wbc_remove_appmenu(&s, parent) == WBC_OK, "parent removed");
    CHECK(!wbc_menu_visible(&s, sub) && wbc_lookup(&s, WBC_APPMENU, sub) == sub, "sub item kept, hidden");
    usekey[0].data = key;
    CHECK(wbc_add_appmenu(&s, 7, 8, "After", &port1, usekey, &m, &k2, &given) == WBC_ERR_KEY, "the parent's key is gone");
    CHECK(wbc_remove_appmenu(&s, sub) == WBC_OK && wbc_remove_appmenu(&s, sub) == WBC_ERR_UNKNOWN, "sub removed once");
    wbc_free_all(&s);

    /* the Tools menu holds 62 */
    wbc_init(&s, &env);
    s.desktop = 1;
    for (n = 0; n < 70; n++)
        if (wbc_add_appmenu(&s, (uint32_t)n, 0, "Tool", &port1, NULL, &items[n], &k2, &given) != WBC_OK)
            break;
    CHECK(n == WBC_MAX_TOOLS, "62 in Tools (%d)", n);
    /* titles: 27 */
    for (n = 0; n < 40; n++) {
        if (wbc_add_appmenu(&s, 0, 0, "Title", &port1, gettitle, &m, &k2, &given) != WBC_OK)
            break;
        tkey = k2;
    }
    CHECK(n == WBC_MAX_TITLES && !k2, "27 titles (%d), no key when refused", n);
    usekey[0].data = tkey;
    for (n = 0; n < 70; n++)
        if (wbc_add_appmenu(&s, 0, 0, "Under", &port1, usekey, &m, &k2, &given) != WBC_OK)
            break;
    CHECK(n == WBC_MAX_TITLEITEMS, "63 under a title (%d)", n);
    wbc_free_all(&s);
}

static void settings(void)
{
    struct wbc_state s;
    wbc_init(&s, &env);
    CHECK(s.set.default_stack == 4096, "the default stack, as the autodoc says");
    CHECK(wbc_add_hidden(&s, "DF1") == WBC_OK && wbc_add_hidden(&s, "df1") == WBC_OK, "hidden, twice");
    CHECK(wbc_add_hidden(&s, "PC0") == WBC_OK && wbc_add_hidden(&s, "") == WBC_ERR_ARGS, "another; empty refused");
    CHECK(wbc_is_hidden(&s, "Df1") && wbc_is_hidden(&s, "PC0") && !wbc_is_hidden(&s, "DF0"), "case-blind");
    CHECK(s.set.hidden && strcmp(s.set.hidden->name, "DF1") == 0 && s.set.hidden->next && !s.set.hidden->next->next,
          "two names, in order");
    CHECK(wbc_take_notes(&s) == WBC_NOTE_HIDDEN, "noted");
    CHECK(wbc_remove_hidden(&s, "df1") == WBC_OK && wbc_remove_hidden(&s, "DF1") == WBC_ERR_UNKNOWN, "removed once");
    CHECK(wbc_add_setup_cleanup(&s, &hook1) == WBC_OK && wbc_add_setup_cleanup(&s, &hook2) == WBC_OK, "two hooks");
    CHECK(wbc_add_setup_cleanup(&s, &hook1) == WBC_OK && s.set.setup_cleanup->next && !s.set.setup_cleanup->next->next,
          "the same hook once");
    CHECK(wbc_add_setup_cleanup(&s, NULL) == WBC_ERR_ARGS, "NULL hook");
    CHECK(wbc_rem_setup_cleanup(&s, &hook1) == WBC_OK && wbc_rem_setup_cleanup(&s, &hook1) == WBC_ERR_UNKNOWN, "removed once");
    CHECK(s.set.setup_cleanup && s.set.setup_cleanup->hook == &hook2, "the other stays");
    wbc_free_all(&s);
}

static void no_memory(void)
{
    struct wbc_state s;
    struct wbc_obj *o;
    uint32_t k;
    int given;
    wbc_init(&s, &env);
    s.desktop = 1;
    fail_after = 0;
    CHECK(wbc_add_appwindow(&s, 0, 0, &win1, &port1, NULL, &o) == WBC_ERR_NOMEM && !o, "window");
    fail_after = 1;                 /* the object, then not its label */
    CHECK(wbc_add_appicon(&s, 0, 0, "x", &port1, 0, &dobj, NULL, &o) == WBC_ERR_NOMEM && !o, "icon's label");
    fail_after = 1;
    CHECK(wbc_add_appmenu(&s, 0, 0, "x", &port1, NULL, &o, &k, &given) == WBC_ERR_NOMEM && !o && !k, "menu's label");
    fail_after = 0;
    CHECK(wbc_add_hidden(&s, "DF0") == WBC_ERR_NOMEM, "hidden");
    fail_after = -1;
    CHECK(wbc_empty(&s), "nothing half-added");
    wbc_free_all(&s);
}

static void appmessages(void)
{
    struct wbc_state s;
    struct wbc_obj *w, *z, *i, *m;
    struct wbc_appmsg a, b;
    uint8_t buf[WBC_APPMSG_SIZE];
    uint32_t k;
    int given, j;
    wbc_init(&s, &env);
    s.desktop = 1;
    wbc_add_appwindow(&s, 0x11111111, 0x22222222, &win1, &port1, NULL, &w);
    {
        struct wbc_tag box[] = { { TAG(26), 0 }, { TAG(28), 0 }, { TAG(30), 10 }, { TAG(32), 10 }, { 0, 0 } };
        wbc_add_dropzone(&s, w, 0x33, 0x44, box, &z);
    }
    wbc_add_appicon(&s, 0x55, 0x66, "i", &port1, 0, &dobj, NULL, &i);
    wbc_add_appmenu(&s, 0x77, 0x88, "m", &port1, NULL, &m, &k, &given);
    wbc_appmsg_for(w, &a);
    CHECK(a.type == 7 && a.id == 0x11111111 && a.userdata == 0x22222222, "AppWindow's message");
    wbc_appmsg_for(z, &a);
    CHECK(a.type == 10 && a.id == 0x33 && a.userdata == 0x44, "drop zone's");
    wbc_appmsg_for(i, &a);
    CHECK(a.type == 8 && a.id == 0x55, "AppIcon's");
    wbc_appmsg_for(m, &a);
    CHECK(a.type == 9 && a.userdata == 0x88, "AppMenuItem's");

    memset(buf, 0xEE, sizeof buf);
    a.type = 8;
    a.userdata = 0x01020304;
    a.id = 0x05060708;
    a.numargs = 2;
    a.arglist = 0x0A0B0C0D;
    a.amclass = 3;
    a.mousex = -5;
    a.mousey = 300;
    a.seconds = 0x11223344;
    a.micros = 999999;
    wbc_appmsg_encode(&a, buf);
    CHECK(buf[20] == 0 && buf[21] == 8, "am_Type at 20");
    CHECK(buf[22] == 1 && buf[25] == 4, "am_UserData at 22");
    CHECK(buf[26] == 5 && buf[29] == 8, "am_ID at 26");
    CHECK(buf[33] == 2 && buf[34] == 0x0A && buf[37] == 0x0D, "am_NumArgs at 30, am_ArgList at 34");
    CHECK(buf[38] == 0 && buf[39] == 1, "am_Version at 38 is AM_VERSION");
    CHECK(buf[41] == 3 && buf[42] == 0xFF && buf[43] == 0xFB && buf[44] == 1 && buf[45] == 44, "am_Class, am_MouseX, am_MouseY");
    CHECK(buf[46] == 0x11 && buf[49] == 0x44, "am_Seconds at 46");
    for (j = 54; j < 86 && !buf[j]; j++)
        ;
    CHECK(j == 86, "am_Reserved cleared");
    CHECK(buf[0] == 0xEE && buf[19] == 0xEE, "the exec Message is left to the library");
    wbc_appmsg_decode(buf, &b);
    CHECK(b.type == a.type && b.userdata == a.userdata && b.id == a.id && b.numargs == a.numargs && b.arglist == a.arglist &&
          b.amclass == a.amclass && b.mousex == a.mousex && b.mousey == a.mousey && b.seconds == a.seconds && b.micros == a.micros,
          "read back");
    wbc_free_all(&s);
}

static void refuse_rule(void)
{
    struct wbc_state s;
    struct wbc_obj *o;
    wbc_init(&s, &env);
    CHECK(wbc_add_appwindow(&s, 0, 0, &win1, &port1, NULL, &o) == WBC_OK, "kept for the desktop to come (the default)");
    s.refuse_without_desktop = 1;   /* OS 3.2's rule */
    CHECK(wbc_add_appicon(&s, 0, 0, "i", &port1, 0, &dobj, NULL, &o) == WBC_ERR_NODESK && !o, "refused by OS 3.2's rule");
    wbc_free_all(&s);
}

int main(void)
{
    appwindows();
    appicons();
    appmenus();
    settings();
    no_memory();
    appmessages();
    refuse_rule();
    CHECK(live == 0, "every allocation freed (%ld left)", live);
    printf("wb_core: %d checks, %d failed\n", checks, failures);
    return failures ? 1 : 0;
}
