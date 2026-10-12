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
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/icon.h>

static const char version[] __attribute__((used)) =
    "$VER: iconprobe 0.1 (12.10.2026) OpenBench R2, MIT, Copyright (c) 2026 Dalsin Limited";

struct Library *IconBase;
static struct Library *os_base, *our_base;
static int show_all;
static ULONG cases, diffs;

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
    LONG ok, width, height, size[2], trans[2], frameless, aspect, mapped, newicon, real2;
    UBYTE *data[2];
    struct ColorRegister *pal[2];
};

static void ask(struct Library *lib, struct DiskObject *d, struct pictures *p)
{
    memset(p, 0, sizeof *p);
    IconBase = lib;
    p->ok = IconControl(d,
        ICONCTRLA_GetWidth, &p->width, ICONCTRLA_GetHeight, &p->height,
        ICONCTRLA_GetPaletteSize1, &p->size[0], ICONCTRLA_GetPaletteSize2, &p->size[1],
        ICONCTRLA_GetTransparentColor1, &p->trans[0], ICONCTRLA_GetTransparentColor2, &p->trans[1],
        ICONCTRLA_GetImageData1, &p->data[0], ICONCTRLA_GetImageData2, &p->data[1],
        ICONCTRLA_GetPalette1, &p->pal[0], ICONCTRLA_GetPalette2, &p->pal[1],
        ICONCTRLA_GetFrameless, &p->frameless, ICONCTRLA_IsPaletteMapped, &p->mapped,
        ICONCTRLA_IsNewIcon, &p->newicon, ICONCTRLA_HasRealImage2, &p->real2,
        TAG_DONE);
}

static void colour_pictures(const char *name, struct DiskObject *a, struct DiskObject *b)
{
    struct pictures x, y;
    int k;
    ask(os_base, a, &x);
    ask(our_base, b, &y);
    num("colour: mapped", name, x.mapped, y.mapped);
    num("colour: NewIcon", name, x.newicon, y.newicon);
    num("colour: second real", name, x.real2, y.real2);
    num("colour: frameless", name, x.frameless, y.frameless);
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
            sprintf(w, "colour %d: palette", k + 1);
            bytes(w, name, (UBYTE *)x.pal[k], (ULONG)x.size[k] * 3, (UBYTE *)y.pal[k], (ULONG)y.size[k] * 3);
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
        colour_pictures(w, ra, rb);
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
    num("read", name, a != NULL, b != NULL);
    if (!a || !b)
        num("read error", name, ea, eb);
    if (a && b) {
        fields(name, a, b);
        colour_pictures(name, a, b);   /* each library asked about its own object */
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
            int n = (int)strlen(fib->fib_FileName);
            if (n <= 5 || strcmp(fib->fib_FileName + n - 5, ".info") != 0)
                continue;
            fib->fib_FileName[n - 5] = 0;
            strncpy(path, dir, sizeof path - 1);
            path[sizeof path - 1] = 0;
            if (path[0] && path[strlen(path) - 1] != ':' && path[strlen(path) - 1] != '/')
                strncat(path, "/", sizeof path - strlen(path) - 1);
            strncat(path, fib->fib_FileName, sizeof path - strlen(path) - 1);
            one_icon(path, temp);
        }
    }
    if (fib)
        FreeDosObject(DOS_FIB, fib);
    if (lock)
        UnLock(lock);
}

/* ---- the calls that keep no state ---- */

static void tool_types(void)
{
    static const char *const tt_src[] = {
        "DONOTWAIT", "FILE=S:Startup-Sequence", "Tool=OpenEdit", "(OFF)", "OFFX=1", "EMPTY=", "  SPACE=1",
        "a=b=c", "=novalue", "LONG=" "0123456789012345678901234567890123456789", NULL
    };
    static const char *const queries[] = {
        "DONOTWAIT", "donotwait", "FILE", "TOOL", "OFF", "(OFF)", "FIL", "OFFX", "EMPTY", "SPACE", "  SPACE",
        "A", "a=b", "", "LONG", "MISSING", NULL
    };
    static const char *const values[][2] = {
        { "text|ascii", "text" }, { "text|ascii", "ASCII" }, { "text|ascii", "asc" }, { "text|ascii", "ascii2" },
        { "a||b", "" }, { "", "a" }, { "", "" }, { "one", "one" }, { "one|", "" }, { "|one", "one" },
        { "a|b|c", "b" }, { "a b|c", "a b" }, { "x", "X" }, { "\xC4|\xF6", "\xE4" }, { NULL, NULL }
    };
    static const char *const names[] = {
        "Disk", "copy_of_Disk", "copy_2_of_Disk", "copy_99_of_Disk", "copy_x_of_Disk", "Copy_Of_Disk", "copy_of_",
        "copy_0_of_Disk", "copy_2_of_", "A very long name of thirty characters", "123456789012345678901234567890",
        "copy_123456789_of_x", "", NULL
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
        str(w, "tool types", a, b);
        for (len = 4; len <= 40; len += 9) {
            memset(a, 0x55, sizeof a);
            memset(b, 0x55, sizeof b);
            IconBase = os_base;
            BumpRevisionLength((STRPTR)a, (CONST_STRPTR)names[i], len);
            IconBase = our_base;
            BumpRevisionLength((STRPTR)b, (CONST_STRPTR)names[i], len);
            snprintf(w, sizeof w, "BumpRevisionLength %lu \"%s\"", (unsigned long)len, names[i]);
            str(w, "tool types", a, b);
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
        counts[lib][2] = FreeFree(&fl, m) ? 1 : 0;
        counts[lib][3] = FreeFree(&fl, (APTR)&fl) ? 1 : 0;
        FreeFreeList(&fl);
    }
    num("fl_NumFree after 3", "free list", counts[0][0], counts[1][0]);
    num("fl_NumFree after 24", "free list", counts[0][1], counts[1][1]);
    num("FreeFree of one of its own", "free list", counts[0][2], counts[1][2]);
    num("FreeFree of a stranger", "free list", counts[0][3], counts[1][3]);
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
    if (args[A_DIRS]) {
        char **d;
        for (d = (char **)args[A_DIRS]; *d; d++)
            one_dir(*d, temp);
    } else {
        for (i = 0; default_dirs[i]; i++)
            one_dir(default_dirs[i], temp);
    }
    printf("%s %lu/%lu\n", diffs ? "DIFFER" : "ALL SAME", (unsigned long)(cases - diffs), (unsigned long)cases);
    CloseLibrary(our_base);
    CloseLibrary(os_base);
    FreeArgs(rd);
    return diffs ? 5 : 0;
}
