/* DragTest: the mouse and the keys of a lab, from inside it, at exact places, for testing drag and drop on
 * OpenBench and on OS 3.2.3's own Workbench alike (the Team, 24 October 2026). It writes events into
 * input.device as the mouse and the keyboard would: the pointer is placed in screen pixels
 * (IECLASS_NEWPOINTERPOS, IESUBCLASS_PIXEL), so a test isn't thrown off by the mouse's acceleration.
 *
 *   DragTest LIST                         the screens, and the windows on the front one, with their boxes
 *   DragTest MOVE x y                     the pointer to x, y
 *   DragTest CLICK x y [DOUBLE] [QUAL q]  a left click (or two) at x, y
 *   DragTest DRAG x y x2 y2 [QUAL q] [LATE] [STEPS n] [HOLD ticks] [RIGHT]
 *                                         pressed at x, y, moved to x2, y2 in n steps (12), let go there
 *                                         after a pause (HOLD, 25 ticks); RIGHT: the menu button; LATE: the
 *                                         QUAL keys go down half way, once the drag is under way
 *   DragTest KEY code [QUAL q]            a raw key pressed and let go (hex or decimal)
 *
 * QUAL is a list joined by "+": LSHIFT, RSHIFT, CAPS, CTRL, LALT, RALT, LAMIGA, RAMIGA. Each one's key goes
 * down before the action and up after it, and every event of the action carries it as a qualifier.
 * Coordinates are the front screen's (DragTest LIST names it).
 *
 * For lab instances only. MIT, Copyright (c) 2026 AmigaChrome Team. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <exec/types.h>
#include <dos/dos.h>
#include <dos/rdargs.h>
#include <devices/input.h>
#include <devices/inputevent.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <intuition/screens.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>

const char version[] = "$VER: DragTest 1.0 (24.10.2026) AmigaChrome Team";

extern struct IntuitionBase *IntuitionBase;

static struct MsgPort *port;
static struct IOStdReq *io;

static const struct { const char *name; UWORD qual; UBYTE key; } quals[] = {
    { "LSHIFT", IEQUALIFIER_LSHIFT, 0x60 }, { "SHIFT", IEQUALIFIER_LSHIFT, 0x60 },
    { "RSHIFT", IEQUALIFIER_RSHIFT, 0x61 }, { "CAPS", IEQUALIFIER_CAPSLOCK, 0x62 },
    { "CTRL", IEQUALIFIER_CONTROL, 0x63 }, { "LALT", IEQUALIFIER_LALT, 0x64 }, { "ALT", IEQUALIFIER_LALT, 0x64 },
    { "RALT", IEQUALIFIER_RALT, 0x65 }, { "LAMIGA", IEQUALIFIER_LCOMMAND, 0x66 },
    { "RAMIGA", IEQUALIFIER_RCOMMAND, 0x67 },
};
#define NQUALS (sizeof quals / sizeof quals[0])

static void inject(UBYTE class, UBYTE sub, UWORD code, UWORD qual, APTR addr)
{
    struct InputEvent ie;
    memset(&ie, 0, sizeof ie);
    ie.ie_Class = class;
    ie.ie_SubClass = sub;
    ie.ie_Code = code;
    ie.ie_Qualifier = qual;
    ie.ie_EventAddress = addr;
    io->io_Command = IND_WRITEEVENT;
    io->io_Data = &ie;
    io->io_Length = sizeof ie;
    DoIO((struct IORequest *)io);
}

static struct Screen *front(void)
{
    return IntuitionBase->FirstScreen;
}

static void place(LONG x, LONG y, UWORD qual)
{
    struct IEPointerPixel pp;
    pp.iepp_Screen = front();
    pp.iepp_Position.X = (WORD)x;
    pp.iepp_Position.Y = (WORD)y;
    inject(IECLASS_NEWPOINTERPOS, IESUBCLASS_PIXEL, IECODE_NOBUTTON, qual, &pp);
}

/* "LSHIFT+LAMIGA" -> the qualifier bits, and the keys in order (up to 8) */
static int parse_quals(const char *s, UWORD *qual, UBYTE *keys)
{
    int n = 0;
    *qual = 0;
    while (s && *s) {
        char word[16];
        int k = 0;
        unsigned i;
        while (*s == '+' || *s == ',') s++;
        while (*s && *s != '+' && *s != ',' && k < 15) {
            char c = *s++;
            word[k++] = (char)(c >= 'a' && c <= 'z' ? c - 32 : c);
        }
        word[k] = 0;
        if (!k) break;
        for (i = 0; i < NQUALS; i++)
            if (!strcmp(word, quals[i].name)) break;
        if (i == NQUALS) {
            Printf("DragTest: no qualifier %s\n", (ULONG)word);
            return -1;
        }
        *qual |= quals[i].qual;
        if (n < 8) keys[n++] = quals[i].key;
    }
    return n;
}

static void keys_down(const UBYTE *keys, int n, UWORD qual)
{
    int i;
    UWORD q = 0;
    for (i = 0; i < n; i++) {
        unsigned k;
        for (k = 0; k < NQUALS; k++)
            if (quals[k].key == keys[i]) q |= quals[k].qual;
        inject(IECLASS_RAWKEY, 0, keys[i], q, NULL);
        Delay(2);
    }
    (void)qual;
}

static void keys_up(const UBYTE *keys, int n, UWORD qual)
{
    int i;
    for (i = n - 1; i >= 0; i--) {
        inject(IECLASS_RAWKEY, 0, (UWORD)(keys[i] | IECODE_UP_PREFIX), qual, NULL);
        Delay(2);
    }
}

static void list(void)
{
    struct Screen *s;
    ULONG lock = LockIBase(0);
    for (s = IntuitionBase->FirstScreen; s; s = s->NextScreen) {
        struct Window *w;
        Printf("screen \"%s\" %ld x %ld%s\n", (ULONG)(s->Title ? (char *)s->Title : ""), (LONG)s->Width,
               (LONG)s->Height, (ULONG)(s == IntuitionBase->FirstScreen ? " (front)" : ""));
        for (w = s->FirstWindow; w; w = w->NextWindow)
            Printf("  window \"%s\" at %ld,%ld size %ld x %ld%s%s\n", (ULONG)(w->Title ? (char *)w->Title : ""),
                   (LONG)w->LeftEdge, (LONG)w->TopEdge, (LONG)w->Width, (LONG)w->Height,
                   (ULONG)(w->Flags & WFLG_BACKDROP ? " backdrop" : ""),
                   (ULONG)(w == IntuitionBase->ActiveWindow ? " active" : ""));
    }
    UnlockIBase(lock);
}

int main(void)
{
    enum { A_ACTION, A_XY, A_QUAL, A_STEPS, A_HOLD, A_DOUBLE, A_RIGHT, A_LATE, A_N };
    LONG a[A_N];
    struct RDArgs *rd;
    const char *act;
    LONG **xy, v[4] = { 0, 0, 0, 0 };
    int nv = 0, rc = 0, nk = 0;
    UWORD qual = 0;
    UBYTE keys[8];
    memset(a, 0, sizeof a);
    if (!(rd = ReadArgs((CONST_STRPTR)"ACTION/A,XY/M,QUAL/K,STEPS/K/N,HOLD/K/N,DOUBLE/S,RIGHT/S,LATE/S", a, NULL))) {
        PrintFault(IoErr(), (CONST_STRPTR)"DragTest");
        return 20;
    }
    act = (const char *)a[A_ACTION];
    for (xy = (LONG **)a[A_XY]; xy && *xy && nv < 4; xy++) v[nv++] = strtol((const char *)*xy, NULL, 0);
    if (a[A_QUAL] && (nk = parse_quals((const char *)a[A_QUAL], &qual, keys)) < 0) {
        FreeArgs(rd);
        return 20;
    }
    if (!stricmp(act, "LIST")) {
        list();
        FreeArgs(rd);
        return 0;
    }
    if (!(port = CreateMsgPort()) || !(io = (struct IOStdReq *)CreateIORequest(port, sizeof *io)) ||
        OpenDevice((CONST_STRPTR)"input.device", 0, (struct IORequest *)io, 0)) {
        PutStr((CONST_STRPTR)"DragTest: no input.device\n");
        if (io) DeleteIORequest((struct IORequest *)io);
        if (port) DeleteMsgPort(port);
        FreeArgs(rd);
        return 20;
    }
    if (!stricmp(act, "MOVE") && nv >= 2) {
        place(v[0], v[1], 0);
    } else if (!stricmp(act, "CLICK") && nv >= 2) {
        int i, n = a[A_DOUBLE] ? 2 : 1;
        place(v[0], v[1], 0);
        Delay(10);
        keys_down(keys, nk, qual);
        for (i = 0; i < n; i++) {
            inject(IECLASS_RAWMOUSE, 0, IECODE_LBUTTON, (UWORD)(qual | IEQUALIFIER_LEFTBUTTON | IEQUALIFIER_RELATIVEMOUSE), NULL);
            Delay(3);
            inject(IECLASS_RAWMOUSE, 0, IECODE_LBUTTON | IECODE_UP_PREFIX, (UWORD)(qual | IEQUALIFIER_RELATIVEMOUSE), NULL);
            Delay(i + 1 < n ? 6 : 3);
        }
        keys_up(keys, nk, qual);
    } else if (!stricmp(act, "DRAG") && nv >= 4) {
        LONG steps = a[A_STEPS] ? *(LONG *)a[A_STEPS] : 12, hold = a[A_HOLD] ? *(LONG *)a[A_HOLD] : 25, i;
        UWORD down = a[A_RIGHT] ? IECODE_RBUTTON : IECODE_LBUTTON;
        UWORD held = a[A_RIGHT] ? IEQUALIFIER_RBUTTON : IEQUALIFIER_LEFTBUTTON;
        UWORD q0 = a[A_LATE] ? 0 : qual;            /* LATE: the keys go down once the drag is under way */
        if (steps < 1) steps = 1;
        place(v[0], v[1], 0);
        Delay(10);
        if (!a[A_LATE]) keys_down(keys, nk, qual);
        inject(IECLASS_RAWMOUSE, 0, down, (UWORD)(q0 | held | IEQUALIFIER_RELATIVEMOUSE), NULL);
        Delay(5);
        for (i = 1; i <= steps; i++) {
            if (a[A_LATE] && i == steps / 2 + 1) {
                keys_down(keys, nk, qual);
                q0 = qual;
            }
            place(v[0] + (v[2] - v[0]) * i / steps, v[1] + (v[3] - v[1]) * i / steps, (UWORD)(q0 | held));
            Delay(2);
        }
        Delay(hold);
        inject(IECLASS_RAWMOUSE, 0, (UWORD)(down | IECODE_UP_PREFIX), (UWORD)(qual | IEQUALIFIER_RELATIVEMOUSE), NULL);
        Delay(5);
        keys_up(keys, nk, qual);
    } else if (!stricmp(act, "KEY") && nv >= 1) {
        keys_down(keys, nk, qual);
        inject(IECLASS_RAWKEY, 0, (UWORD)v[0], qual, NULL);
        Delay(3);
        inject(IECLASS_RAWKEY, 0, (UWORD)(v[0] | IECODE_UP_PREFIX), qual, NULL);
        Delay(2);
        keys_up(keys, nk, qual);
    } else {
        PutStr((CONST_STRPTR)"DragTest LIST | MOVE x y | CLICK x y [DOUBLE] | DRAG x y x2 y2 [STEPS n] [HOLD t] [RIGHT] | "
                             "KEY code; QUAL a+b\n");
        rc = 10;
    }
    CloseDevice((struct IORequest *)io);
    DeleteIORequest((struct IORequest *)io);
    DeleteMsgPort(port);
    FreeArgs(rd);
    return rc;
}
