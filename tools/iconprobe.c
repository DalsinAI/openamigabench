/* iconprobe: our icon.library beside Hyperion's (step R2, design section
 * 7). It opens icon.library and openicon.library, makes the same calls
 * through both and prints each difference, then a count. For lab
 * instances only.
 *
 *   iconprobe [DIRS dir ...] [TEMP dir] [ALL]
 *
 * DIRS: the drawers whose icons are read (SYS:, SYS:Prefs, SYS:Utilities,
 * SYS:System, SYS:Tools and SYS:Storage/OpenBench-Icons by default).
 * TEMP: where the written icons go (T:). ALL: print the cases that match
 * as well.
 *
 * Each icon is read by each library, its fields and pictures compared;
 * then each library writes its own object back, and both files are read
 * by Hyperion's library and compared. An object only ever goes back to
 * the library that made it.
 *
 * MIT, Copyright (c) 2026 Dalsin Limited. */
#include <stdio.h>
#include <string.h>
#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/rdargs.h>
#include <intuition/intuition.h>
#include <workbench/workbench.h>
#include <workbench/icon.h>
#include <intuition/imageclass.h>
#include <intuition/screens.h>
#include <graphics/gfxbase.h>
#include <graphics/rastport.h>
#include <graphics/gfx.h>
#include <graphics/text.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/icon.h>
#include <proto/graphics.h>
#include <proto/intuition.h>

static const char version[] __attribute__((used)) =
    "$VER: iconprobe 0.7 (12.10.2026) OpenBench R3, MIT, Copyright (c) 2026 Dalsin Limited";

struct Library *IconBase;
struct GfxBase *GfxBase;
struct IntuitionBase *IntuitionBase;
static struct Library *os_base, *our_base;
static int show_all;
static ULONG cases, diffs, extras;

#define TEMPLATE "DIRS/M,TEMP/K,ALL/S"
enum { A_DIRS, A_TEMP, A_ALL, A_COUNT };

static const char *const default_dirs[] = {
    "SYS:", "SYS:Prefs", "SYS:Utilities", "SYS:System", "SYS:Tools", "SYS:Storage/OpenBench-Icons", NULL
};

/* ---- reporting ---- */

static void same_or_not(const char *what, const char *name, int same, const char *os, const char *ours)
{
    cases++;
    if (!same) {
        diffs++;
        printf("DIFF %s %s: Hyperion %s, ours %s\n", name, what, os, ours);
    } else if (show_all) {
        printf("SAME %s %s: %s\n", name, what, os);
    }
}

static void num(const char *what, const char *name, LONG a, LONG b)
{
    char x[16], y[16];
    sprintf(x, "%ld", (long)a);
    sprintf(y, "%ld", (long)b);
    same_or_not(what, name, a == b, x, y);
}

static void str(const char *what, const char *name, const char *a, const char *b)
{
    int same = (!a && !b) || (a && b && strcmp(a, b) == 0);
    same_or_not(what, name, same, a ? a : "(none)", b ? b : "(none)");
}

static ULONG crc(const UBYTE *p, ULONG n)
{
    ULONG c = 0xFFFFFFFFUL;
    while (n--) {
        int k;
        c ^= *p++;
        for (k = 0; k < 8; k++)
            c = (c >> 1) ^ (0xEDB88320UL & (0UL - (c & 1)));
    }
    return ~c;
}

static void bytes(const char *what, const char *name, const UBYTE *a, ULONG na, const UBYTE *b, ULONG nb)
{
    char x[32], y[32];
    ULONG ca = a ? crc(a, na) : 0, cb = b ? crc(b, nb) : 0;
    sprintf(x, "%lu bytes crc %08lx", (unsigned long)(a ? na : 0), (unsigned long)ca);
    sprintf(y, "%lu bytes crc %08lx", (unsigned long)(b ? nb : 0), (unsigned long)cb);
    same_or_not(what, name, (!a) == (!b) && (!a || (na == nb && ca == cb)), x, y);
}

/* ---- comparing two DiskObjects ---- */

static void image(const char *what, const char *name, struct Image *a, struct Image *b)
{
    char w[48];
    if (!a || !b) {
        num(what, name, a != NULL, b != NULL);
        return;
    }
    sprintf(w, "%s size", what);
    num(w, name, (LONG)a->Width << 16 | a->Height, (LONG)b->Width << 16 | b->Height);
    sprintf(w, "%s depth", what);
    num(w, name, a->Depth, b->Depth);
    sprintf(w, "%s place", what);
    num(w, name, (LONG)a->LeftEdge << 16 | (UWORD)a->TopEdge, (LONG)b->LeftEdge << 16 | (UWORD)b->TopEdge);
    sprintf(w, "%s planes", what);
    num(w, name, a->PlanePick << 8 | a->PlaneOnOff, b->PlanePick << 8 | b->PlaneOnOff);
    if (a->Width == b->Width && a->Height == b->Height && a->Depth == b->Depth) {
        ULONG n = (((ULONG)a->Width + 15) >> 4) * 2 * a->Height * a->Depth;
        sprintf(w, "%s data", what);
        bytes(w, name, (UBYTE *)a->ImageData, n, (UBYTE *)b->ImageData, n);
    }
}

static void fields(const char *name, struct DiskObject *a, struct DiskObject *b)
{
    STRPTR *ta, *tb;
    int i;
    num("magic", name, a->do_Magic, b->do_Magic);
    num("version", name, a->do_Version, b->do_Version);
    num("type", name, a->do_Type, b->do_Type);
    num("gadget size", name, a->do_Gadget.Width << 16 | a->do_Gadget.Height,
        b->do_Gadget.Width << 16 | b->do_Gadget.Height);
    num("gadget place", name, a->do_Gadget.LeftEdge << 16 | (UWORD)a->do_Gadget.TopEdge,
        b->do_Gadget.LeftEdge << 16 | (UWORD)b->do_Gadget.TopEdge);
    num("gadget flags", name, a->do_Gadget.Flags, b->do_Gadget.Flags);
    num("gadget activation", name, a->do_Gadget.Activation, b->do_Gadget.Activation);
    num("gadget type", name, a->do_Gadget.GadgetType, b->do_Gadget.GadgetType);
    num("revision", name, (LONG)a->do_Gadget.UserData, (LONG)b->do_Gadget.UserData);
    str("default tool", name, (char *)a->do_DefaultTool, (char *)b->do_DefaultTool);
    str("tool window", name, (char *)a->do_ToolWindow, (char *)b->do_ToolWindow);
    num("place", name, a->do_CurrentX, b->do_CurrentX);
    num("place y", name, a->do_CurrentY, b->do_CurrentY);
    num("stack", name, a->do_StackSize, b->do_StackSize);
    ta = a->do_ToolTypes;
    tb = b->do_ToolTypes;
    num("tool types there", name, ta != NULL, tb != NULL);
    for (i = 0; ta && tb; i++) {
        char w[24];
        sprintf(w, "tool type %d", i);
        str(w, name, (char *)ta[i], (char *)tb[i]);
        if (!ta[i] || !tb[i])
            break;
    }
    num("drawer data there", name, a->do_DrawerData != NULL, b->do_DrawerData != NULL);
    if (a->do_DrawerData && b->do_DrawerData) {
        struct DrawerData *x = a->do_DrawerData, *y = b->do_DrawerData;
        num("drawer place", name, x->dd_NewWindow.LeftEdge << 16 | (UWORD)x->dd_NewWindow.TopEdge,
            y->dd_NewWindow.LeftEdge << 16 | (UWORD)y->dd_NewWindow.TopEdge);
        num("drawer size", name, x->dd_NewWindow.Width << 16 | x->dd_NewWindow.Height,
            y->dd_NewWindow.Width << 16 | y->dd_NewWindow.Height);
        num("drawer scroll x", name, x->dd_CurrentX, y->dd_CurrentX);
        num("drawer scroll y", name, x->dd_CurrentY, y->dd_CurrentY);
        if ((ULONG)a->do_Gadget.UserData & 0xFF) {
            num("drawer flags", name, x->dd_Flags, y->dd_Flags);
            num("drawer view", name, x->dd_ViewModes, y->dd_ViewModes);
        }
    }
    image("picture", name, (struct Image *)a->do_Gadget.GadgetRender, (struct Image *)b->do_Gadget.GadgetRender);
    if ((a->do_Gadget.Flags & GFLG_GADGHIGHBITS) == GFLG_GADGHIMAGE)
        image("selected picture", name, (struct Image *)a->do_Gadget.SelectRender,
              (struct Image *)b->do_Gadget.SelectRender);
}

/* IconControlA's questions, asked of each library about its own object. */
struct pictures {
    LONG ok, width, height, size[2], trans[2], frameless, mapped, newicon, real2, native;
    UBYTE aspect[4];
    UBYTE *data[2];
    struct ColorRegister *pal[2];
};

static void ask(struct Library *lib, struct DiskObject *d, struct pictures *p)
{
    memset(p, 0, sizeof *p);
    memset(p->aspect, 0x55, sizeof p->aspect);
    IconBase = lib;
    p->ok = IconControl(d,
        ICONCTRLA_GetWidth, &p->width, ICONCTRLA_GetHeight, &p->height,
        ICONCTRLA_GetPaletteSize1, &p->size[0], ICONCTRLA_GetPaletteSize2, &p->size[1],
        ICONCTRLA_GetTransparentColor1, &p->trans[0], ICONCTRLA_GetTransparentColor2, &p->trans[1],
        ICONCTRLA_GetImageData1, &p->data[0], ICONCTRLA_GetImageData2, &p->data[1],
        ICONCTRLA_GetPalette1, &p->pal[0], ICONCTRLA_GetPalette2, &p->pal[1],
        ICONCTRLA_GetFrameless, &p->frameless, ICONCTRLA_IsPaletteMapped, &p->mapped,
        ICONCTRLA_IsNewIcon, &p->newicon, ICONCTRLA_HasRealImage2, &p->real2,
        ICONCTRLA_IsNativeIcon, &p->native, ICONCTRLA_GetAspectRatio, &p->aspect[0],
        TAG_DONE);
}

/* la and lb: the libraries that made a and b, which are asked. */
static void colour_pictures(const char *name, struct Library *la, struct DiskObject *a,
                            struct Library *lb, struct DiskObject *b)
{
    struct pictures x, y;
    int k;
    ask(la, a, &x);
    ask(lb, b, &y);
    if (!x.width && y.newicon && lb == our_base) {
        /* Hyperion's shows no NewIcon colour picture; ours does */
        extras++;
        printf("EXTRA %s: a NewIcon, its colour picture ours only\n", name);
        return;
    }
    num("colour: mapped", name, x.mapped, y.mapped);
    num("colour: NewIcon", name, x.newicon, y.newicon);
    num("colour: second real", name, x.real2, y.real2);
    num("colour: frameless", name, x.frameless, y.frameless);
    num("colour: native", name, x.native, y.native);
    /* a UBYTE *: the bytes after it must be left alone */
    num("colour: aspect bytes", name, (LONG)x.aspect[0] << 24 | x.aspect[1] << 16 | x.aspect[2] << 8 | x.aspect[3],
        (LONG)y.aspect[0] << 24 | y.aspect[1] << 16 | y.aspect[2] << 8 | y.aspect[3]);
    num("colour: size", name, x.width << 16 | x.height, y.width << 16 | y.height);
    for (k = 0; k < 2; k++) {
        char w[40];
        sprintf(w, "colour %d: palette size", k + 1);
        num(w, name, x.size[k], y.size[k]);
        sprintf(w, "colour %d: transparent", k + 1);
        num(w, name, x.trans[k], y.trans[k]);
        if (x.width == y.width && x.height == y.height) {
            sprintf(w, "colour %d: pixels", k + 1);
            bytes(w, name, x.data[k], (ULONG)x.width * x.height, y.data[k], (ULONG)y.width * y.height);
        }
        if (x.size[k] == y.size[k]) {
            ULONG d0 = diffs;
            sprintf(w, "colour %d: palette", k + 1);
            bytes(w, name, (UBYTE *)x.pal[k], (ULONG)x.size[k] * 3, (UBYTE *)y.pal[k], (ULONG)y.size[k] * 3);
            if (diffs != d0 && x.pal[k] && y.pal[k]) {
                /* both palettes, to see what 3.2 did to it */
                ULONG i, n = (ULONG)x.size[k] * 3;
                if (n > 48)
                    n = 48;
                printf("PALETTE %s %d Hyperion", name, k + 1);
                for (i = 0; i < n; i++)
                    printf(" %02x", ((UBYTE *)x.pal[k])[i]);
                printf("\nPALETTE %s %d ours    ", name, k + 1);
                for (i = 0; i < n; i++)
                    printf(" %02x", ((UBYTE *)y.pal[k])[i]);
                printf("\n");
            }
        }
    }
}

static struct TagItem raw_tags[] = {
    { ICONGETA_RemapIcon, FALSE }, { ICONGETA_GenerateImageMasks, FALSE }, { TAG_DONE, 0 }
};

static struct DiskObject *get(struct Library *lib, const char *name, LONG *err)
{
    struct TagItem t[4];
    struct DiskObject *d;
    t[0] = raw_tags[0];
    t[1] = raw_tags[1];
    t[2].ti_Tag = ICONA_ErrorCode;
    t[2].ti_Data = (ULONG)err;
    t[3].ti_Tag = TAG_DONE;
    *err = 0;
    IconBase = lib;
    d = GetIconTagList((CONST_STRPTR)name, t);
    return d;
}

static void put_back(const char *name, struct DiskObject *a, struct DiskObject *b, const char *temp)
{
    char pa[256], pb[256];
    BOOL oka, okb;
    LONG ea = 0, eb = 0;
    struct DiskObject *ra, *rb;
    snprintf(pa, sizeof pa, "%s/iconprobe-os", temp);
    snprintf(pb, sizeof pb, "%s/iconprobe-ours", temp);
    IconBase = os_base;
    oka = PutDiskObject((CONST_STRPTR)pa, a);
    IconBase = our_base;
    okb = PutDiskObject((CONST_STRPTR)pb, b);
    num("written", name, oka, okb);
    if (!oka || !okb)
        return;
    ra = get(os_base, pa, &ea);
    rb = get(os_base, pb, &eb);
    num("written, read back", name, ra != NULL, rb != NULL);
    if (ra && rb) {
        char w[160];
        snprintf(w, sizeof w, "%s (written)", name);
        fields(w, ra, rb);
        colour_pictures(w, os_base, ra, os_base, rb);   /* both read by Hyperion's */
    }
    IconBase = os_base;
    if (ra)
        FreeDiskObject(ra);
    if (rb)
        FreeDiskObject(rb);
    DeleteDiskObject((CONST_STRPTR)pa);
    DeleteDiskObject((CONST_STRPTR)pb);
}

static void one_icon(const char *name, const char *temp)
{
    LONG ea, eb;
    struct DiskObject *a = get(os_base, name, &ea), *b = get(our_base, name, &eb);
    if (!a && b && strstr(name, "/png_")) {
        /* one Hyperion's can't read (PNG icons): ours only, not a difference */
        extras++;
        printf("EXTRA %s: Hyperion can't read it (error %ld), ours can\n", name, (long)ea);
    } else {
        num("read", name, a != NULL, b != NULL);
        if (!a || !b)
            num("read error", name, ea, eb);
    }
    if (a && b) {
        fields(name, a, b);
        colour_pictures(name, os_base, a, our_base, b);   /* each library asked about its own object */
        put_back(name, a, b, temp);
    }
    IconBase = os_base;
    if (a)
        FreeDiskObject(a);
    IconBase = our_base;
    if (b)
        FreeDiskObject(b);
}

/* Every .info in a drawer. */
static void one_dir(const char *dir, const char *temp)
{
    BPTR lock = Lock((CONST_STRPTR)dir, SHARED_LOCK);
    struct FileInfoBlock *fib = AllocDosObject(DOS_FIB, NULL);
    if (lock && fib && Examine(lock, fib)) {
        while (ExNext(lock, fib)) {
            char path[300];
            char *fname = (char *)fib->fib_FileName;
            int n = (int)strlen(fname);
            if (n <= 5 || strcmp(fname + n - 5, ".info") != 0)
                continue;
            fname[n - 5] = 0;
            strncpy(path, dir, sizeof path - 1);
            path[sizeof path - 1] = 0;
            if (path[0] && path[strlen(path) - 1] != ':' && path[strlen(path) - 1] != '/')
                strncat(path, "/", sizeof path - strlen(path) - 1);
            strncat(path, fname, sizeof path - strlen(path) - 1);
            one_icon(path, temp);
        }
    }
    if (fib)
        FreeDosObject(DOS_FIB, fib);
    if (lock)
        UnLock(lock);
}

/* ---- the calls that keep no state ---- */

/* A BumpRevision buffer as text: "(untouched)" when nothing was written,
 * "(no NUL)" when something was but not its end. One at a time. */
static const char *shown(char *buf, ULONG size)
{
    static char out[2][80];
    static int which;
    char *o = out[which ^= 1];
    ULONG k;
    for (k = 0; k < size && (UBYTE)buf[k] == 0x55; k++)
        ;
    if (k == size)
        return "(untouched)";
    if (!memchr(buf, 0, size)) {
        snprintf(o, sizeof out[0], "(no NUL) %.*s", (int)(size < 40 ? size : 40), buf);
        return o;
    }
    return buf;
}

static void tool_types(void)
{
    static const char *const tt_src[] = {
        "DONOTWAIT", "FILE=S:Startup-Sequence", "Tool=OpenEdit", "(OFF)", "OFFX=1", "EMPTY=", "  SPACE=1",
        "a=b=c", "=novalue", "LONG=" "0123456789012345678901234567890123456789", "\tTAB=1", " SP AFTER =2",
        "TRAIL =3", NULL
    };
    static const char *const queries[] = {
        "DONOTWAIT", "donotwait", "FILE", "TOOL", "OFF", "(OFF)", "FIL", "OFFX", "EMPTY", "SPACE", "  SPACE",
        "A", "a=b", "", "LONG", "MISSING", "TAB", "\tTAB", "SP AFTER", "SP AFTER ", "TRAIL", "TRAIL ", "=NOVALUE",
        NULL
    };
    static const char *const values[][2] = {
        { "text|ascii", "text" }, { "text|ascii", "ASCII" }, { "text|ascii", "asc" }, { "text|ascii", "ascii2" },
        { "a||b", "" }, { "", "a" }, { "", "" }, { "one", "one" }, { "one|", "" }, { "|one", "one" },
        { "a|b|c", "b" }, { "a b|c", "a b" }, { "x", "X" }, { "\xC4|\xF6", "\xE4" }, { NULL, NULL }
    };
    static const char *const names[] = {
        "Disk", "copy_of_Disk", "copy_2_of_Disk", "copy_99_of_Disk", "copy_x_of_Disk", "Copy_Of_Disk", "copy_of_",
        "copy_0_of_Disk", "copy_2_of_", "A very long name of thirty characters", "123456789012345678901234567890",
        "copy_123456789_of_x", "", "copy_2Disk", "copy_2_Disk", "copy_", "COPY_OF_x", "copy_of_copy_of_x",
        "copy_-1_of_x", "copyof_x", "copy_ 2_of_x", "copy_1234567890_of_x", "copy_4294967295_of_x", NULL
    };
    STRPTR tt[16];
    int i;
    for (i = 0; tt_src[i]; i++)
        tt[i] = (STRPTR)tt_src[i];
    tt[i] = NULL;
    for (i = 0; queries[i]; i++) {
        UBYTE *a, *b;
        char w[48];
        IconBase = os_base;
        a = FindToolType((CONST_STRPTR *)tt, (CONST_STRPTR)queries[i]);
        IconBase = our_base;
        b = FindToolType((CONST_STRPTR *)tt, (CONST_STRPTR)queries[i]);
        snprintf(w, sizeof w, "FindToolType \"%s\"", queries[i]);
        str(w, "tool types", (char *)a, (char *)b);
        num(w, "tool types (where)", a ? (LONG)(a - (UBYTE *)tt[0]) : -1, b ? (LONG)(b - (UBYTE *)tt[0]) : -1);
    }
    for (i = 0; values[i][0]; i++) {
        BOOL a, b;
        char w[48];
        IconBase = os_base;
        a = MatchToolValue((CONST_STRPTR)values[i][0], (CONST_STRPTR)values[i][1]);
        IconBase = our_base;
        b = MatchToolValue((CONST_STRPTR)values[i][0], (CONST_STRPTR)values[i][1]);
        snprintf(w, sizeof w, "MatchToolValue \"%s\" \"%s\"", values[i][0], values[i][1]);
        num(w, "tool types", a != 0, b != 0);
    }
    for (i = 0; names[i]; i++) {
        char a[64], b[64], w[64];
        ULONG len;
        memset(a, 0x55, sizeof a);
        memset(b, 0x55, sizeof b);
        IconBase = os_base;
        BumpRevision((STRPTR)a, (CONST_STRPTR)names[i]);
        IconBase = our_base;
        BumpRevision((STRPTR)b, (CONST_STRPTR)names[i]);
        snprintf(w, sizeof w, "BumpRevision \"%s\"", names[i]);
        str(w, "tool types", shown(a, sizeof a), shown(b, sizeof b));
        for (len = 0; len <= 40; len++) {
            UBYTE *ra, *rb;
            memset(a, 0x55, sizeof a);
            memset(b, 0x55, sizeof b);
            IconBase = os_base;
            ra = BumpRevisionLength((STRPTR)a, (CONST_STRPTR)names[i], len);
            IconBase = our_base;
            rb = BumpRevisionLength((STRPTR)b, (CONST_STRPTR)names[i], len);
            snprintf(w, sizeof w, "BumpRevisionLength %lu \"%s\"", (unsigned long)len, names[i]);
            str(w, "tool types", shown(a, sizeof a), shown(b, sizeof b));
            num(w, "tool types (result)", ra == NULL ? 0 : ra == (UBYTE *)a ? 1 : 2,
                rb == NULL ? 0 : rb == (UBYTE *)b ? 1 : 2);
        }
    }
}

static void free_lists(void)
{
    int lib;
    LONG counts[2][4];
    for (lib = 0; lib < 2; lib++) {
        struct FreeList fl;
        APTR m;
        int i;
        IconBase = lib ? our_base : os_base;
        memset(&fl, 0, sizeof fl);
        fl.fl_MemList.lh_Head = (struct Node *)&fl.fl_MemList.lh_Tail;   /* an empty list */
        fl.fl_MemList.lh_TailPred = (struct Node *)&fl.fl_MemList.lh_Head;
        for (i = 0; i < 3; i++)
            FreeAlloc(&fl, 32, MEMF_PUBLIC);
        counts[lib][0] = fl.fl_NumFree;
        m = FreeAlloc(&fl, 64, MEMF_PUBLIC | MEMF_CLEAR);
        for (i = 0; i < 20; i++)
            FreeAlloc(&fl, 16, MEMF_PUBLIC);
        counts[lib][1] = fl.fl_NumFree;
        FreeFree(&fl, m);           /* void in NDK 3.2: what it leaves is compared */
        counts[lib][2] = fl.fl_NumFree;
        counts[lib][3] = 0;
        FreeFreeList(&fl);
    }
    num("fl_NumFree after 3", "free list", counts[0][0], counts[1][0]);
    num("fl_NumFree after 24", "free list", counts[0][1], counts[1][1]);
    num("fl_NumFree after FreeFree", "free list", counts[0][2], counts[1][2]);
}

static void defaults(void)
{
    LONG type;
    for (type = WBDISK; type <= WBAPPICON; type++) {
        struct DiskObject *a, *b;
        char w[40];
        IconBase = os_base;
        a = NewDiskObject(type);
        IconBase = our_base;
        b = NewDiskObject(type);
        sprintf(w, "NewDiskObject %ld", (long)type);
        num("made", w, a != NULL, b != NULL);
        if (a && b)
            fields(w, a, b);
        IconBase = os_base;
        if (a) FreeDiskObject(a);
        IconBase = our_base;
        if (b) FreeDiskObject(b);

        IconBase = os_base;
        a = GetDefDiskObject(type);
        IconBase = our_base;
        b = GetDefDiskObject(type);
        sprintf(w, "GetDefDiskObject %ld", (long)type);
        num("made", w, a != NULL, b != NULL);
        if (a && b) {
            num("type", w, a->do_Type, b->do_Type);
            num("drawer data there", w, a->do_DrawerData != NULL, b->do_DrawerData != NULL);
            num("stack", w, a->do_StackSize, b->do_StackSize);
            num("place", w, a->do_CurrentX, b->do_CurrentX);
            num("place y", w, a->do_CurrentY, b->do_CurrentY);
            num("gadget flags", w, a->do_Gadget.Flags, b->do_Gadget.Flags);
            image("picture", w, (struct Image *)a->do_Gadget.GadgetRender, (struct Image *)b->do_Gadget.GadgetRender);
        }
        IconBase = os_base;
        if (a) FreeDiskObject(a);
        IconBase = our_base;
        if (b) FreeDiskObject(b);
    }
    {
        static const char *const objs[] = { "SYS:C/Dir", "S:Startup-Sequence", "SYS:C", "SYS:", "RAM:", "SYS:NoSuchFile", NULL };
        int i;
        for (i = 0; objs[i]; i++) {
            struct DiskObject *a, *b;
            LONG da = -1, db = -1, ea = 0, eb = 0;
            struct TagItem t[4];
            t[0].ti_Tag = ICONGETA_FailIfUnavailable;
            t[0].ti_Data = FALSE;
            t[1].ti_Tag = ICONGETA_IsDefaultIcon;
            t[1].ti_Data = (ULONG)&da;
            t[2].ti_Tag = ICONA_ErrorCode;
            t[2].ti_Data = (ULONG)&ea;
            t[3].ti_Tag = TAG_DONE;
            IconBase = os_base;
            a = GetIconTagList((CONST_STRPTR)objs[i], t);
            t[1].ti_Data = (ULONG)&db;
            t[2].ti_Data = (ULONG)&eb;
            IconBase = our_base;
            b = GetIconTagList((CONST_STRPTR)objs[i], t);
            num("default: made", objs[i], a != NULL, b != NULL);
            num("default: is default", objs[i], da, db);
            num("default: error", objs[i], ea, eb);
            if (a && b) {
                num("default: type", objs[i], a->do_Type, b->do_Type);
                num("default: stack", objs[i], a->do_StackSize, b->do_StackSize);
            }
            IconBase = os_base;
            if (a) FreeDiskObject(a);
            IconBase = our_base;
            if (b) FreeDiskObject(b);
        }
    }
}

/* The global settings, and NewIcons with both switched on. */
static void globals(const char *temp)
{
    static const ULONG gets[][2] = {
        { ICONCTRLA_GetGlobalNewIconsSupport, 0 }, { ICONCTRLA_GetGlobalColorIconSupport, 0 },
        { ICONCTRLA_GetGlobalMaxNameLength, 0 }, { ICONCTRLA_GetGlobalPrecision, 0 },
        { ICONCTRLA_GetGlobalFrameless, 0 }
    };
    LONG was[2] = { 0, 0 };
    ULONG k;
    (void)temp;
    for (k = 0; k < sizeof gets / sizeof gets[0]; k++) {
        LONG a = -12345, b = -12345;
        ULONG ra, rb;
        char w[24];
        IconBase = os_base;
        ra = IconControl(NULL, gets[k][0], &a, TAG_DONE);
        IconBase = our_base;
        rb = IconControl(NULL, gets[k][0], &b, TAG_DONE);
        sprintf(w, "global %lx", (unsigned long)(gets[k][0] & 0xFFFF));
        num(w, "IconControl", a, b);
        num(w, "IconControl (answered)", ra, rb);
    }
    IconBase = os_base;
    IconControl(NULL, ICONCTRLA_GetGlobalNewIconsSupport, &was[0], TAG_DONE);
    IconControl(NULL, ICONCTRLA_SetGlobalNewIconsSupport, TRUE, TAG_DONE);
    IconBase = our_base;
    IconControl(NULL, ICONCTRLA_GetGlobalNewIconsSupport, &was[1], TAG_DONE);
    IconControl(NULL, ICONCTRLA_SetGlobalNewIconsSupport, TRUE, TAG_DONE);
    {
        const char *n = "SYS:Storage/OpenBench-Icons/newicons";
        LONG ea, eb;
        struct DiskObject *a = get(os_base, n, &ea), *b = get(our_base, n, &eb);
        num("read", "NewIcons on", a != NULL, b != NULL);
        if (a && b)
            colour_pictures("NewIcons on", os_base, a, our_base, b);
        IconBase = os_base;
        if (a) FreeDiskObject(a);
        IconBase = our_base;
        if (b) FreeDiskObject(b);
    }
    IconBase = os_base;
    IconControl(NULL, ICONCTRLA_SetGlobalNewIconsSupport, was[0], TAG_DONE);
    IconBase = our_base;
    IconControl(NULL, ICONCTRLA_SetGlobalNewIconsSupport, was[1], TAG_DONE);
}

static BOOL put_err(struct Library *lib, const char *path, struct DiskObject *d, LONG *err)
{
    struct TagItem t[2];
    t[0].ti_Tag = ICONA_ErrorCode;
    t[0].ti_Data = (ULONG)err;
    t[1].ti_Tag = TAG_DONE;
    *err = 0;
    IconBase = lib;
    return PutIconTagList((CONST_STRPTR)path, d, t);
}

/* NewDiskObject's icon written as it is, by each library: 3.2 refuses it. */
static void writing_new(const char *temp)
{
    LONG type;
    char pa[256], pb[256];
    snprintf(pa, sizeof pa, "%s/iconprobe-os", temp);
    snprintf(pb, sizeof pb, "%s/iconprobe-ours", temp);
    for (type = WBDISK; type <= WBKICK; type++) {
        struct DiskObject *a, *b;
        LONG ea = 0, eb = 0;
        BOOL oka = FALSE, okb = FALSE;
        char w[40];
        IconBase = os_base;
        a = NewDiskObject(type);
        IconBase = our_base;
        b = NewDiskObject(type);
        if (a)
            oka = put_err(os_base, pa, a, &ea);
        if (b)
            okb = put_err(our_base, pb, b, &eb);
        sprintf(w, "NewDiskObject %ld written", (long)type);
        num("written", w, oka, okb);
        num("write error", w, ea, eb);
        IconBase = os_base;
        if (a) FreeDiskObject(a);
        DeleteDiskObject((CONST_STRPTR)pa);
        IconBase = our_base;
        if (b) FreeDiskObject(b);
        DeleteDiskObject((CONST_STRPTR)pb);
    }
}

/* ChangeToSelectedIconColor: Hyperion's answers, to learn its sum; ours
 * beside them. Printed, not counted. */
static void selected_colours(void)
{
    static const UBYTE cols[][3] = {
        { 0, 0, 0 }, { 255, 255, 255 }, { 128, 128, 128 }, { 17, 17, 17 }, { 34, 34, 34 }, { 51, 51, 51 },
        { 68, 68, 68 }, { 85, 85, 85 }, { 102, 102, 102 }, { 119, 119, 119 }, { 136, 136, 136 },
        { 153, 153, 153 }, { 170, 170, 170 }, { 187, 187, 187 }, { 204, 204, 204 }, { 221, 221, 221 },
        { 238, 238, 238 }, { 255, 0, 0 }, { 0, 255, 0 }, { 0, 0, 255 }, { 255, 255, 0 }, { 1, 2, 3 },
        { 200, 100, 50 }, { 10, 250, 130 }, { 64, 128, 192 }, { 254, 1, 127 }
    };
    ULONG i;
    for (i = 0; i < sizeof cols / sizeof cols[0]; i++) {
        struct ColorRegister a, b;
        a.red = b.red = cols[i][0];
        a.green = b.green = cols[i][1];
        a.blue = b.blue = cols[i][2];
        IconBase = os_base;
        ChangeToSelectedIconColor(&a);
        IconBase = our_base;
        ChangeToSelectedIconColor(&b);
        {
            char w[32];
            sprintf(w, "%u %u %u", cols[i][0], cols[i][1], cols[i][2]);
            num(w, "ChangeToSelectedIconColor", (LONG)a.red << 16 | a.green << 8 | a.blue,
                (LONG)b.red << 16 | b.green << 8 | b.blue);
        }
    }
}

/* What PutDiskObject writes for gadget and stack values it is given: one
 * of Hyperion's own objects, changed, written by each library and read
 * back by Hyperion's. */
static void writing(const char *temp)
{
    static const UWORD acts[] = { 0, GACT_RELVERIFY, GACT_IMMEDIATE, 7, GACT_TOGGLESELECT | GACT_RELVERIFY };
    static const ULONG stacks[] = { 0, 1000, 4095, 4096, 100000 };
    static const UWORD gflags[] = { 0, 4, 5, 6, 7 };
    static const UWORD gtypes[] = { 0, 1, 3, 4 };
    static const ULONG revs[] = { 0, 1, 2, 0x101 };
    char pa[256], pb[256];
    ULONG k;
    snprintf(pa, sizeof pa, "%s/iconprobe-os", temp);
    snprintf(pb, sizeof pb, "%s/iconprobe-ours", temp);
    for (k = 0; k < 5; k++) {
        struct DiskObject *d, *ra = NULL, *rb = NULL;
        LONG ea = 0, eb = 0;
        BOOL oka, okb;
        char w[40];
        IconBase = os_base;
        d = GetDefDiskObject(k == 1 ? WBDRAWER : WBTOOL);   /* a real picture */
        if (!d)
            break;
        d->do_Gadget.Activation = acts[k];
        d->do_StackSize = stacks[k];
        d->do_Gadget.Flags = gflags[k];
        d->do_Gadget.GadgetType = gtypes[k & 3];
        d->do_Gadget.UserData = (APTR)revs[k & 3];
        oka = put_err(os_base, pa, d, &ea);
        okb = put_err(our_base, pb, d, &eb);
        sprintf(w, "written case %lu", (unsigned long)k);
        num("written", w, oka, okb);
        num("write error", w, ea, eb);
        if (oka && okb) {
            ra = get(os_base, pa, &ea);
            rb = get(os_base, pb, &eb);
            num("read back", w, ra != NULL, rb != NULL);
            if (ra && rb)
                fields(w, ra, rb);
        }
        IconBase = os_base;
        if (ra) FreeDiskObject(ra);
        if (rb) FreeDiskObject(rb);
        FreeDiskObject(d);
        DeleteDiskObject((CONST_STRPTR)pa);
        DeleteDiskObject((CONST_STRPTR)pb);
    }
}

/* What the review of #17 asked about, measured: the named default with
 * its type to fall back on, GetDiskObjectNew(NULL), DupDiskObjectA's
 * tags, a name too long to write, and colour icons switched off. */
static void review_cases(const char *temp)
{
    static const char *const names[] = { "Harddisk", "RAM", "NoSuchName", NULL };
    int i;
    for (i = 0; names[i]; i++) {
        struct DiskObject *a, *b;
        LONG ea = 0, eb = 0;
        struct TagItem t[7];
        char w[48];
        t[4] = raw_tags[0];     /* the file's own pictures, not remapped */
        t[5] = raw_tags[1];
        t[6].ti_Tag = TAG_DONE;
        t[0].ti_Tag = ICONGETA_GetDefaultName;
        t[0].ti_Data = (ULONG)names[i];
        t[1].ti_Tag = ICONGETA_GetDefaultType;
        t[1].ti_Data = WBDISK;
        t[2].ti_Tag = ICONA_ErrorCode;
        t[2].ti_Data = (ULONG)&ea;
        t[3].ti_Tag = TAG_MORE;
        t[3].ti_Data = (ULONG)&t[4];
        IconBase = os_base;
        a = GetIconTagList(NULL, t);
        t[2].ti_Data = (ULONG)&eb;
        IconBase = our_base;
        b = GetIconTagList(NULL, t);
        sprintf(w, "default name %s, type disk", names[i]);
        num("made", w, a != NULL, b != NULL);
        num("error", w, ea, eb);
        if (a && b)
            fields(w, a, b);
        IconBase = os_base;
        if (a) FreeDiskObject(a);
        IconBase = our_base;
        if (b) FreeDiskObject(b);
    }
    {
        struct DiskObject *a, *b;
        IconBase = os_base;
        a = GetDiskObjectNew(NULL);
        IconBase = our_base;
        b = GetDiskObjectNew(NULL);
        num("made", "GetDiskObjectNew(NULL)", a != NULL, b != NULL);
        if (a && b)
            fields("GetDiskObjectNew(NULL)", a, b);
        IconBase = os_base;
        if (a) FreeDiskObject(a);
        IconBase = our_base;
        if (b) FreeDiskObject(b);
    }
    {
        static const ULONG dups[] = { ICONDUPA_DuplicateDrawerData, ICONDUPA_DuplicateDefaultTool,
                                      ICONDUPA_DuplicateToolTypes, ICONDUPA_DuplicateToolWindow,
                                      ICONDUPA_DuplicateImageData, ICONDUPA_DuplicateImages };
        ULONG k;
        LONG e;
        struct DiskObject *srca = get(os_base, "SYS:Storage/OpenBench-Icons/classic_drawer", &e);
        struct DiskObject *srcb = get(our_base, "SYS:Storage/OpenBench-Icons/classic_drawer", &e);
        for (k = 0; srca && srcb && k < sizeof dups / sizeof dups[0]; k++) {
            struct TagItem t[2];
            struct DiskObject *a, *b;
            char w[40];
            t[0].ti_Tag = dups[k];
            t[0].ti_Data = FALSE;
            t[1].ti_Tag = TAG_DONE;
            IconBase = os_base;
            a = DupDiskObjectA(srca, t);
            IconBase = our_base;
            b = DupDiskObjectA(srcb, t);
            sprintf(w, "DupDiskObjectA %lx FALSE", (unsigned long)(dups[k] & 0xFFFF));
            num("made", w, a != NULL, b != NULL);
            if (a && b) {
                struct Image *ia = a->do_Gadget.GadgetRender, *ib = b->do_Gadget.GadgetRender;
                num("drawer data", w, a->do_DrawerData != NULL, b->do_DrawerData != NULL);
                num("default tool", w, a->do_DefaultTool != NULL, b->do_DefaultTool != NULL);
                num("tool types", w, a->do_ToolTypes != NULL, b->do_ToolTypes != NULL);
                num("tool window", w, a->do_ToolWindow != NULL, b->do_ToolWindow != NULL);
                num("picture", w, ia != NULL, ib != NULL);
                if (ia && ib) {
                    num("picture data", w, ia->ImageData != NULL, ib->ImageData != NULL);
                    num("picture data shared", w, ia->ImageData == ((struct Image *)srca->do_Gadget.GadgetRender)->ImageData,
                        ib->ImageData == ((struct Image *)srcb->do_Gadget.GadgetRender)->ImageData);
                }
            }
            IconBase = os_base;
            if (a) FreeDiskObject(a);
            IconBase = our_base;
            if (b) FreeDiskObject(b);
        }
        IconBase = os_base;
        if (srca) FreeDiskObject(srca);
        IconBase = our_base;
        if (srcb) FreeDiskObject(srcb);
    }
    {
        /* a name past the global maximum (25): written? what error? */
        char pa[256], pb[256];
        LONG ea = -1, eb = -1, maxa = 0, maxb = 0;
        BOOL oka, okb;
        struct DiskObject *d;
        BPTR l;
        snprintf(pa, sizeof pa, "%s/iconprobe-os-a-very-long-name-indeed", temp);
        snprintf(pb, sizeof pb, "%s/iconprobe-ours-a-very-long-name-indeed", temp);
        IconBase = os_base;
        IconControl(NULL, ICONCTRLA_GetGlobalMaxNameLength, &maxa, TAG_DONE);
        IconBase = our_base;
        IconControl(NULL, ICONCTRLA_GetGlobalMaxNameLength, &maxb, TAG_DONE);
        num("global max name length", "long name", maxa, maxb);
        IconBase = os_base;
        d = GetDefDiskObject(WBTOOL);
        if (d) {
            oka = put_err(os_base, pa, d, &ea);
            okb = put_err(our_base, pb, d, &eb);
            num("put says", "long name", oka, okb);
            num("put error", "long name", ea, eb);
            strncat(pa, ".info", sizeof pa - strlen(pa) - 1);
            strncat(pb, ".info", sizeof pb - strlen(pb) - 1);
            l = Lock((CONST_STRPTR)pa, SHARED_LOCK);
            oka = l != 0;
            if (l) UnLock(l);
            l = Lock((CONST_STRPTR)pb, SHARED_LOCK);
            okb = l != 0;
            if (l) UnLock(l);
            num("file there", "long name", oka, okb);
            DeleteFile((CONST_STRPTR)pa);
            DeleteFile((CONST_STRPTR)pb);
            IconBase = os_base;
            FreeDiskObject(d);
        }
    }
    {
        /* colour icons off: a GlowIcon's colour picture? */
        LONG was[2] = { 1, 1 }, e;
        struct DiskObject *a, *b;
        IconBase = os_base;
        IconControl(NULL, ICONCTRLA_GetGlobalColorIconSupport, &was[0], ICONCTRLA_SetGlobalColorIconSupport, FALSE,
                    TAG_DONE);
        IconBase = our_base;
        IconControl(NULL, ICONCTRLA_GetGlobalColorIconSupport, &was[1], ICONCTRLA_SetGlobalColorIconSupport, FALSE,
                    TAG_DONE);
        a = get(os_base, "SYS:Storage/OpenBench-Icons/glow_19", &e);
        b = get(our_base, "SYS:Storage/OpenBench-Icons/glow_19", &e);
        num("read", "colour icons off", a != NULL, b != NULL);
        if (a && b)
            colour_pictures("colour icons off", os_base, a, our_base, b);
        IconBase = os_base;
        if (a) FreeDiskObject(a);
        IconControl(NULL, ICONCTRLA_SetGlobalColorIconSupport, was[0], TAG_DONE);
        IconBase = our_base;
        if (b) FreeDiskObject(b);
        IconControl(NULL, ICONCTRLA_SetGlobalColorIconSupport, was[1], TAG_DONE);
    }
}

/* ---- drawing (step R3) ----
 * Both libraries draw into the same 8-plane bitmap of their own (not the
 * screen's, so the answers don't depend on the lab's screen mode), with the
 * Workbench screen's pens. Each case's rectangle and pixels are compared;
 * the first few that differ are shown as hex pens, Hyperion's then ours. */

#define CW 112
#define CH 72
#define AT_X 24
#define AT_Y 16
#define MAX_DUMPS 3              /* for each icon, before and after LayoutIconA */

static struct RastPort canvas;
static UBYTE pixels[2][CW * CH];
static int dumps;

static void grab(UBYTE *out)
{
    int x, y;
    for (y = 0; y < CH; y++)
        for (x = 0; x < CW; x++)
            out[y * CW + x] = (UBYTE)ReadPixel(&canvas, x, y);
}

/* The pens of both pictures, inside the box where either differs from the
 * background (pen 5), so the dump is no bigger than it needs to be. */
static void dump(const char *name)
{
    int x, y, k, x0 = CW, y0 = CH, x1 = -1, y1 = -1;
    for (y = 0; y < CH; y++)
        for (x = 0; x < CW; x++)
            for (k = 0; k < 2; k++)
                if (pixels[k][y * CW + x] != 5) {
                    if (x < x0) x0 = x;
                    if (x > x1) x1 = x;
                    if (y < y0) y0 = y;
                    if (y > y1) y1 = y;
                }
    if (x1 < 0)
        return;
    printf("PIXELS %s: x %d-%d y %d-%d (drawn at %d,%d), Hyperion | ours\n", name, x0, x1, y0, y1, AT_X, AT_Y);
    for (y = y0; y <= y1; y++) {
        for (k = 0; k < 2; k++) {
            printf(k ? " | " : "  ");
            for (x = x0; x <= x1; x++)
                printf("%x", pixels[k][y * CW + x] & 15);
        }
        printf("\n");
    }
}

static void one_drawing(const char *name, struct DiskObject *a, struct DiskObject *b, STRPTR label,
                        struct TagItem *tags, ULONG state)
{
    struct DiskObject *d[2];
    struct Rectangle r[2];
    BOOL ok[2];
    char w[96];
    int k;
    d[0] = a;
    d[1] = b;
    for (k = 0; k < 2; k++) {
        IconBase = k ? our_base : os_base;
        memset(&r[k], 0x55, sizeof r[k]);
        ok[k] = GetIconRectangleA(&canvas, d[k], label, &r[k], tags);
        SetRast(&canvas, 5);
        SetAPen(&canvas, 1);
        DrawIconStateA(&canvas, d[k], label, AT_X, AT_Y, state, tags);  /* VOID in 3.2 */
        grab(pixels[k]);
    }
    snprintf(w, sizeof w, "%s state %lu", name, (unsigned long)state);
    num("rectangle says", w, ok[0], ok[1]);
    if (ok[0] && ok[1]) {
        num("rectangle x", w, (LONG)r[0].MinX << 16 | (UWORD)r[0].MaxX, (LONG)r[1].MinX << 16 | (UWORD)r[1].MaxX);
        num("rectangle y", w, (LONG)r[0].MinY << 16 | (UWORD)r[0].MaxY, (LONG)r[1].MinY << 16 | (UWORD)r[1].MaxY);
    }
    cases++;
    if (memcmp(pixels[0], pixels[1], sizeof pixels[0])) {
        diffs++;
        printf("DIFF %s pixels: Hyperion crc %08lx, ours crc %08lx\n", w, (unsigned long)crc(pixels[0], sizeof pixels[0]),
               (unsigned long)crc(pixels[1], sizeof pixels[1]));
        if (dumps++ < MAX_DUMPS)
            dump(w);
    }
}

static void drawing(void)
{
    static const char *const icons[] = {
        "SYS:Storage/OpenBench-Icons/classic_tool", "SYS:Storage/OpenBench-Icons/classic_drawer",
        "SYS:Storage/OpenBench-Icons/glow_19", NULL
    };
    static const ULONG states[] = { IDS_NORMAL, IDS_SELECTED, IDS_DISABLED };
    struct Screen *scr = NULL;
    struct DrawInfo *dri = NULL;
    struct BitMap *bm = NULL;
    int i, v, s, lay;
    GfxBase = (struct GfxBase *)OpenLibrary((CONST_STRPTR)"graphics.library", 39);
    IntuitionBase = (struct IntuitionBase *)OpenLibrary((CONST_STRPTR)"intuition.library", 39);
    if (GfxBase && IntuitionBase) {
        scr = LockPubScreen(NULL);
        if (scr)
            dri = GetScreenDrawInfo(scr);
        bm = AllocBitMap(CW, CH, 8, BMF_CLEAR, NULL);
    }
    if (!scr || !dri || !bm) {
        printf("drawing: no Workbench screen or bitmap, not tested\n");
        goto done;
    }
    InitRastPort(&canvas);
    canvas.BitMap = bm;
    SetFont(&canvas, GfxBase->DefaultFont);
    {
        struct Rectangle ea, eb;
        memset(&ea, 0x55, sizeof ea);
        memset(&eb, 0x55, sizeof eb);
        IconBase = os_base;
        IconControl(NULL, ICONCTRLA_GetGlobalEmbossRect, &ea, TAG_DONE);
        IconBase = our_base;
        IconControl(NULL, ICONCTRLA_GetGlobalEmbossRect, &eb, TAG_DONE);
        num("emboss x", "globals", (LONG)ea.MinX << 16 | (UWORD)ea.MaxX, (LONG)eb.MinX << 16 | (UWORD)eb.MaxX);
        num("emboss y", "globals", (LONG)ea.MinY << 16 | (UWORD)ea.MaxY, (LONG)eb.MinY << 16 | (UWORD)eb.MaxY);
        printf("drawing: Hyperion's emboss rect %d,%d %d,%d\n", ea.MinX, ea.MinY, ea.MaxX, ea.MaxY);
    }
    printf("drawing: Workbench screen depth %ld, font %s %ld, emboss rect and frameless as globals above\n",
           (long)GetBitMapAttr(scr->RastPort.BitMap, BMA_DEPTH), (char *)GfxBase->DefaultFont->tf_Message.mn_Node.ln_Name,
           (long)GfxBase->DefaultFont->tf_YSize);
    for (i = 0; icons[i]; i++)
        for (lay = 0; lay < 2; lay++) {
            LONG e;
            struct DiskObject *a = get(os_base, icons[i], &e), *b = get(our_base, icons[i], &e);
            dumps = 0;
            if (!a || !b) {
                num("read", icons[i], a != NULL, b != NULL);
            } else {
                if (lay) {
                    BOOL la, lb;
                    IconBase = os_base;
                    la = LayoutIconA(a, scr, NULL);
                    IconBase = our_base;
                    lb = LayoutIconA(b, scr, NULL);
                    num("layout says", icons[i], la, lb);
                }
                for (v = 0; v < 7; v++) {
                    struct TagItem t[3];
                    char w[80];
                    STRPTR label = v == 1 || v == 5 ? (STRPTR)"Label" : NULL;
                    t[0].ti_Tag = TAG_DONE;
                    t[1].ti_Tag = TAG_DONE;
                    t[2].ti_Tag = TAG_DONE;
                    switch (v) {
                    case 2: t[0].ti_Tag = ICONDRAWA_Frameless; t[0].ti_Data = TRUE; break;
                    case 3: t[0].ti_Tag = ICONDRAWA_Borderless; t[0].ti_Data = TRUE; break;
                    case 4: t[0].ti_Tag = ICONDRAWA_EraseBackground; t[0].ti_Data = FALSE; break;
                    case 5: t[0].ti_Tag = ICONDRAWA_DrawInfo; t[0].ti_Data = (ULONG)dri; break;
                    case 6: t[0].ti_Tag = ICONDRAWA_Frameless; t[0].ti_Data = FALSE; break;
                    }
                    snprintf(w, sizeof w, "%s%s %s", icons[i] + 28, lay ? " laid out" : "",
                             (const char *const[]){ "plain", "label", "frameless", "borderless", "no erase",
                                                    "label+drawinfo", "framed" }[v]);
                    for (s = 0; s < 3; s++)
                        one_drawing(w, a, b, label, t, states[s]);
                }
            }
            IconBase = os_base;
            if (a) FreeDiskObject(a);
            IconBase = our_base;
            if (b) FreeDiskObject(b);
        }
done:
    if (bm) {
        WaitBlit();
        FreeBitMap(bm);
    }
    if (dri)
        FreeScreenDrawInfo(scr, dri);
    if (scr)
        UnlockPubScreen(NULL, scr);
    if (IntuitionBase)
        CloseLibrary((struct Library *)IntuitionBase);
    if (GfxBase)
        CloseLibrary((struct Library *)GfxBase);
}

int main(void)
{
    struct RDArgs *rd;
    LONG args[A_COUNT] = { 0 };
    const char *temp = "T:";
    int i;

    rd = ReadArgs((CONST_STRPTR)TEMPLATE, args, NULL);
    if (!rd) {
        PrintFault(IoErr(), (CONST_STRPTR)"iconprobe");
        return 20;
    }
    show_all = args[A_ALL] != 0;
    if (args[A_TEMP])
        temp = (const char *)args[A_TEMP];
    os_base = OpenLibrary((CONST_STRPTR)"icon.library", 44);
    our_base = OpenLibrary((CONST_STRPTR)"openicon.library", 47);
    if (!os_base || !our_base) {
        printf("iconprobe needs icon.library 44 and openicon.library 47 (copy it to LIBS:)\n");
        if (os_base) CloseLibrary(os_base);
        if (our_base) CloseLibrary(our_base);
        FreeArgs(rd);
        return 20;
    }
    printf("Hyperion's: %s", (char *)os_base->lib_IdString);
    printf("ours:       %s", (char *)our_base->lib_IdString);

    tool_types();
    free_lists();
    defaults();
    globals(temp);
    writing(temp);
    writing_new(temp);
    selected_colours();
    review_cases(temp);
    drawing();
    if (args[A_DIRS]) {
        char **d;
        for (d = (char **)args[A_DIRS]; *d; d++)
            one_dir(*d, temp);
    } else {
        for (i = 0; default_dirs[i]; i++)
            one_dir(default_dirs[i], temp);
    }
    printf("%s %lu/%lu, %lu ours only\n", diffs ? "DIFFER" : "ALL SAME", (unsigned long)(cases - diffs),
           (unsigned long)cases, (unsigned long)extras);
    CloseLibrary(our_base);
    CloseLibrary(os_base);
    FreeArgs(rd);
    return diffs ? 5 : 0;
}
