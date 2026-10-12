#!/usr/bin/env python3
"""Icons for icon_core's host test, made with Python's own zlib, so our
inflate is checked against an independent deflate.

  icon_fixtures.py OUT_DIR

writes OUT_DIR/<name>.info for each icon, OUT_DIR/<name>.want with the
dump test_icon_core must print for it, and OUT_DIR/manifest.txt. The
layouts are the published ones (OS 1.x to 3.2 DiskObject, OS 3.5 FORM
ICON, AROS ARGB, NewIcons, AmigaOS 4/MorphOS/AROS PNG icons), as the
Team's own readers have them (amigachrome scripts/amiga_icon.py).

MIT, Copyright (c) 2026 Dalsin Limited.
"""
import os
import random
import struct
import sys
import zlib

R = random.Random(1985)
NO_POS = 0x80000000


def crc(b):
    return zlib.crc32(bytes(b)) & 0xFFFFFFFF


# ---------------------------------------------------------------- pictures

def to_planar(idx, w, h, depth):
    row = (w + 15) // 16 * 2
    out = bytearray(row * h * depth)
    for p in range(depth):
        for y in range(h):
            for x in range(w):
                if idx[y * w + x] >> p & 1:
                    out[p * row * h + y * row + x // 8] |= 0x80 >> (x & 7)
    return bytes(out)


def picture(w, h, colours, runs=True):
    """Pixels with runs and noise, so both kinds of run-length code occur."""
    out = bytearray()
    while len(out) < w * h:
        v = R.randrange(colours)
        n = R.randrange(1, 12) if runs and R.random() < 0.5 else 1
        out += bytes([v]) * n
    return bytes(out[:w * h])


class Bits:
    def __init__(self):
        self.acc, self.n, self.out = 0, 0, bytearray()

    def put(self, v, w):
        self.acc = (self.acc << w) | (v & ((1 << w) - 1))
        self.n += w
        while self.n >= 8:
            self.n -= 8
            self.out.append((self.acc >> self.n) & 0xFF)

    def done(self):
        if self.n:
            self.put(0, 8 - self.n)
        return bytes(self.out)


def rle(values, bits):
    """The OS 3.5 run-length code, written independently of icon_core's:
    runs of 2 or more repeat (icon_core repeats 3 or more), so the reader
    is fed a different coding than the writer makes."""
    b, i, n = Bits(), 0, len(values)
    while i < n:
        run = 1
        while i + run < n and run < 128 and values[i + run] == values[i]:
            run += 1
        if run >= 2:
            b.put(257 - run, 8)
            b.put(values[i], bits)
            i += run
            continue
        start = i
        while i < n and i - start < 128 and not (i + 1 < n and values[i] == values[i + 1]):
            i += 1
        b.put(i - start - 1, 8)
        for v in values[start:i]:
            b.put(v, bits)
    if R.random() < 0.3:
        b.put(128, 8)                       # a no-op control, which readers skip
    return b.done()


def chunk(cid, body):
    return cid + struct.pack(">I", len(body)) + body + (b"\0" if len(body) & 1 else b"")


# ---------------------------------------------------------------- the classic part

def classic(o):
    """o: type, tool, tooltypes, toolwin, x, y, stack, drawer, planar [(w,h,depth,idx)], userdata, ptr."""
    ptr = o.get("ptr", 1)
    pl = o["planar"]
    gw, gh = (pl[0][0], pl[0][1]) if pl else (o.get("gw", 0), o.get("gh", 0))
    flags = 0x0004 | (0x0002 if len(pl) > 1 else 0)
    out = struct.pack(">HH", 0xE310, 1)
    out += struct.pack(">IhhHHHHHIIIIIHI", 0, 0, 0, gw, gh, flags, 3, 1,
                       ptr if pl else 0, ptr if len(pl) > 1 else 0, 0, 0, 0, 0, o.get("userdata", 1))
    out += struct.pack(">BBIIIIIIi", o["type"], 0, ptr if o.get("tool") is not None else 0,
                       ptr if o.get("tooltypes") is not None else 0, o["x"] & 0xFFFFFFFF, o["y"] & 0xFFFFFFFF,
                       ptr if o.get("drawer") else 0, ptr if o.get("toolwin") is not None else 0, o["stack"])
    d = o.get("drawer")
    if d:
        nw = struct.pack(">hhHHBBIIIIIII", d["left"], d["top"], d["width"], d["height"], 255, 255, 0, 0x1234,
                         0, 0, 0, 0, 0) + struct.pack(">hhHHH", 90, 40, 0xFFFF, 0xFFFF, 1)
        assert len(nw) == 48
        out += nw + struct.pack(">ii", d["cx"], d["cy"])
    for (w, h, depth, idx) in pl:
        out += struct.pack(">hhHHHIBBI", 0, 0, w, h, depth, ptr, (1 << depth) - 1, 0, 0)
        out += to_planar(idx, w, h, depth)
    s = lambda t: struct.pack(">I", len(t) + 1) + t.encode("latin-1") + b"\0"
    if o.get("tool") is not None:
        out += s(o["tool"])
    if o.get("tooltypes") is not None:
        out += struct.pack(">I", (len(o["tooltypes"]) + 1) * 4)
        for t in o["tooltypes"]:
            out += s(t)
    if o.get("toolwin") is not None:
        out += s(o["toolwin"])
    if d and o.get("userdata", 1) & 0xFF:
        out += struct.pack(">IH", d["flags"], d["viewmodes"])
    return out


def want_fields(o, formats, gw=None, gh=None, gflags=None):
    pl = o.get("planar", [])
    if gw is None:
        gw, gh = (pl[0][0], pl[0][1]) if pl else (0, 0)
        gflags = 0x0004 | (0x0002 if len(pl) > 1 else 0)
    lines = ["formats 0x%x" % formats,
             "type %d stack %d x %d y %d" % (o["type"], o["stack"], o["x"], o["y"]),
             "gadget %d %d 0x%x %d" % (gw, gh, gflags, o.get("userdata", 1))]
    lines.append("tool %s" % (o["tool"] if o.get("tool") is not None else "-"))
    tts = o.get("tooltypes")
    lines.append("tooltypes %s" % ("-" if tts is None else len(tts)))
    for t in tts or []:
        lines.append(" tt %s" % t)
    lines.append("toolwin %s" % (o["toolwin"] if o.get("toolwin") is not None else "-"))
    d = o.get("drawer")
    if d:
        nd = o.get("userdata", 1) & 0xFF
        lines.append("drawer %d %d %d %d %d %d 0x%x %d" % (d["left"], d["top"], d["width"], d["height"], d["cx"],
                                                          d["cy"], d["flags"] if nd else 0, d["viewmodes"] if nd else 0))
    else:
        lines.append("drawer -")
    for k, (w, h, depth, idx) in enumerate(pl):
        lines.append("planar%d %d %d %d %08x" % (k, w, h, depth, crc(to_planar(idx, w, h, depth))))
    return lines


# ---------------------------------------------------------------- PNG

def png_encode(w, h, ctype, depth, rows_bytes, plte=None, trns=None, interlace=False, extra=()):
    """rows_bytes(x0, y0, dx, dy, pw, ph) gives each row's raw bytes for a
    pass; every filter type is used, chosen at random per row."""
    ch = {0: 1, 2: 3, 3: 1, 4: 2, 6: 4}[ctype]
    bpp = max(1, ch * depth // 8)
    passes = [(0, 0, 8, 8), (4, 0, 8, 8), (0, 4, 4, 8), (2, 0, 4, 4), (0, 2, 2, 4), (1, 0, 2, 2), (0, 1, 1, 2)] \
        if interlace else [(0, 0, 1, 1)]
    raw = bytearray()
    for (x0, y0, dx, dy) in passes:
        pw = (w - x0 + dx - 1) // dx if w > x0 else 0
        ph = (h - y0 + dy - 1) // dy if h > y0 else 0
        if not pw or not ph:
            continue
        prev = None
        for row in rows_bytes(x0, y0, dx, dy, pw, ph):
            ft = R.randrange(5)
            f = bytearray(len(row))
            for i in range(len(row)):
                a = row[i - bpp] if i >= bpp else 0
                b = prev[i] if prev else 0
                c = prev[i - bpp] if prev and i >= bpp else 0
                if ft == 0:
                    pr = 0
                elif ft == 1:
                    pr = a
                elif ft == 2:
                    pr = b
                elif ft == 3:
                    pr = (a + b) >> 1
                else:
                    p = a + b - c
                    pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
                    pr = a if pa <= pb and pa <= pc else b if pb <= pc else c
                f[i] = (row[i] - pr) & 0xFF
            raw += bytes([ft]) + f
            prev = row
    z = zlib.compress(bytes(raw), R.choice([0, 1, 6, 9]))
    # split IDAT in pieces to check they are joined
    cut = len(z) // 2
    out = b"\x89PNG\r\n\x1a\n"

    def pc(cid, body):
        return struct.pack(">I", len(body)) + cid + body + struct.pack(">I", zlib.crc32(cid + body) & 0xFFFFFFFF)
    out += pc(b"IHDR", struct.pack(">IIBBBBB", w, h, depth, ctype, 0, 0, 1 if interlace else 0))
    for cid, body in extra:
        out += pc(cid, body)
    if plte:
        out += pc(b"PLTE", plte)
    if trns:
        out += pc(b"tRNS", trns)
    out += pc(b"IDAT", z[:cut]) + pc(b"tEXt", b"Comment\0split") + pc(b"IDAT", z[cut:]) + pc(b"IEND", b"")
    return out


def pack_samples(vals, depth):
    if depth == 8:
        return bytes(vals)
    if depth == 16:
        return b"".join(struct.pack(">H", v) for v in vals)
    b = Bits()
    for v in vals:
        b.put(v, depth)
    return b.done()


def png_case(w, h, ctype, depth, interlace):
    """A random picture, its PNG, and the ARGB a reader must give."""
    ch = {0: 1, 2: 3, 3: 1, 4: 2, 6: 4}[ctype]
    top = (1 << depth) - 1
    samples = [[R.randrange(top + 1) for _ in range(ch)] for _ in range(w * h)]
    plte = trns = None
    if ctype == 3:
        n = min(1 << depth, 200)
        samples = [[R.randrange(n)] for _ in range(w * h)]
        plte = bytes(R.randrange(256) for _ in range(n * 3))
        trns = bytes(R.randrange(256) for _ in range(n // 2))
    elif ctype == 0:
        trns = struct.pack(">H", samples[0][0])
    elif ctype == 2:
        trns = struct.pack(">HHH", *samples[1])

    def rows(x0, y0, dx, dy, pw, ph):
        for y in range(ph):
            vals = []
            for x in range(pw):
                vals += samples[(y0 + y * dy) * w + x0 + x * dx]
            yield pack_samples(vals, depth)

    argb = bytearray()
    scale = 255 // top if depth < 8 else 1
    for s in samples:
        def eight(v):
            return v >> 8 if depth == 16 else v * scale
        if ctype == 0:
            a = 0 if trns and s[0] == struct.unpack(">H", trns)[0] else 255
            argb += bytes([a, eight(s[0]), eight(s[0]), eight(s[0])])
        elif ctype == 2:
            a = 0 if list(s) == list(struct.unpack(">HHH", trns)) else 255
            argb += bytes([a, eight(s[0]), eight(s[1]), eight(s[2])])
        elif ctype == 3:
            v = s[0]
            a = trns[v] if v < len(trns) else 255
            argb += bytes([a]) + plte[v * 3:v * 3 + 3]
        elif ctype == 4:
            argb += bytes([eight(s[1]), eight(s[0]), eight(s[0]), eight(s[0])])
        else:
            argb += bytes([eight(s[3]), eight(s[0]), eight(s[1]), eight(s[2])])
    return rows, plte, trns, bytes(argb)


def icon_chunk(o):
    """icOn's body, as AmigaOS 4 icons write it."""
    out = b""
    num = lambda t, v: struct.pack(">II", t, v & 0xFFFFFFFF)
    text = lambda t, s: struct.pack(">I", t) + s.encode("latin-1") + b"\0"
    if o["x"] != NO_POS:
        out += num(0x80001001, o["x"])
    if o["y"] != NO_POS:
        out += num(0x80001002, o["y"])
    d = o.get("drawer")
    if d:
        out += num(0x80001003, d["left"]) + num(0x80001004, d["top"])
        out += num(0x80001005, d["width"]) + num(0x80001006, d["height"])
        out += num(0x80001007, d["flags"]) + num(0x8000100C, d["viewmodes"])
        out += num(0x8000100D, d["cx"]) + num(0x8000100E, d["cy"])
    out += num(0x80001009, o["stack"]) + num(0x8000100F, o["type"])
    if o.get("tool") is not None:
        out += text(0x8000100A, o["tool"])
    for t in o.get("tooltypes") or []:
        out += text(0x8000100B, t)
    if o.get("toolwin") is not None:
        out += text(0x80001008, o["toolwin"])
    return out


# ---------------------------------------------------------------- NewIcons

def newicons_lines(which, w, h, palette, idx, transparent):
    colours = len(palette) // 3
    bits = 1
    while (1 << bits) < colours:
        bits += 1

    def chars(values, width):
        b = Bits()
        b.acc, b.n, out = 0, 0, []
        acc = n = 0
        for v in values:
            acc = (acc << width) | v
            n += width
            while n >= 7:
                n -= 7
                out.append(((acc >> n) & 0x7F) + 0x20)
        if n:
            out.append(((acc << (7 - n)) & 0x7F) + 0x20)
        return "".join(chr(c) for c in out)

    head = ("B" if transparent else "C") + chr(w + 0x21) + chr(h + 0x21) + chr((colours >> 6) + 0x21) + \
        chr((colours & 63) + 0x21)
    lines = ["IM%d=" % which + head + chars(palette, 8)]
    per = 7 * 6                                 # a multiple of 7 pixels a line: no stray bits
    for i in range(0, len(idx), per):
        lines.append("IM%d=" % which + chars(idx[i:i + per], bits))
    return lines


# ---------------------------------------------------------------- the cases

def main(out_dir):
    os.makedirs(out_dir, exist_ok=True)
    manifest = []

    def save(name, data, want, exact=False):
        open(os.path.join(out_dir, name + ".info"), "wb").write(data)
        open(os.path.join(out_dir, name + ".want"), "w", encoding="latin-1").write("\n".join(want) + "\n")
        manifest.append("%s %d" % (name, 1 if exact else 0))

    drawer = {"left": -3, "top": 20, "width": 400, "height": 150, "cx": 7, "cy": -9, "flags": 2, "viewmodes": 3}

    # 1. classic tool, two pictures, every string
    a = picture(37, 21, 4)
    b = picture(37, 21, 4)
    o = {"type": 3, "tool": "C:Ed", "tooltypes": ["DONOTWAIT", "FILE=S:Startup-Sequence", "(OFF)"],
         "toolwin": "CON:0/0/640/200/Win", "x": 12, "y": 34, "stack": 8192,
         "planar": [(37, 21, 2, a), (37, 21, 2, b)]}
    save("classic_tool", classic(o), want_fields(o, 0x01), exact=True)

    # 2. a drawer with NewDrawerData, one 3-plane picture
    o = {"type": 2, "x": NO_POS, "y": NO_POS, "stack": 0, "drawer": drawer, "planar": [(48, 30, 3, picture(48, 30, 8))]}
    save("classic_drawer", classic(o), want_fields(o, 0x01), exact=True)

    # 3. a 1.x drawer: no NewDrawerData (UserData 0)
    o = {"type": 2, "x": 5, "y": 6, "stack": 4096, "drawer": dict(drawer), "userdata": 0,
         "planar": [(16, 8, 1, picture(16, 8, 2))]}
    save("classic_drawer13", classic(o), want_fields(o, 0x01), exact=True)

    # 4. real files' pointers are addresses, not 1
    o = {"type": 4, "tool": "SYS:Utilities/MultiView", "tooltypes": [], "x": 1, "y": 2, "stack": 4096, "ptr": 0x00C2A3F0,
         "planar": [(20, 10, 2, picture(20, 10, 4))]}
    save("classic_ptrs", classic(o), want_fields(o, 0x01))

    # 5. OS 3.5: FACE and two IMAGs, the second without a palette
    for name, depth_bits, ncol in (("glow_256", 8, 256), ("glow_19", 5, 19), ("glow_2", 1, 2)):
        w, h = 46, 39
        o = {"type": 3, "tool": None, "tooltypes": ["PUBSCREEN=Workbench"], "x": NO_POS, "y": NO_POS, "stack": 4096,
             "planar": [(w, h, 2, picture(w, h, 4))]}
        p0, p1 = picture(w, h, ncol), picture(w, h, ncol)
        pal = bytes(R.randrange(256) for _ in range(ncol * 3))
        trans = R.randrange(ncol)
        i0 = rle(p0, depth_bits)
        pr = rle(pal, 8)
        body0 = struct.pack(">BBBBBBHH", trans, ncol - 1, 3, 1, 1, depth_bits, len(i0) - 1, len(pr) - 1) + i0 + pr
        raw1 = name == "glow_256"         # the second picture uncompressed, in one case
        i1 = p1 if raw1 else rle(p1, depth_bits)
        body1 = struct.pack(">BBBBBBHH", 0, ncol - 1, 0, 0 if raw1 else 1, 0, depth_bits, len(i1) - 1, 0) + i1
        form = b"ICON" + chunk(b"FACE", struct.pack(">BBBBH", w - 1, h - 1, 1, 0x11, ncol * 3 - 1)) + \
            chunk(b"IMAG", body0) + chunk(b"IMAG", body1)
        data = classic(o) + b"FORM" + struct.pack(">I", len(form)) + form
        want = want_fields(o, 0x03)
        want.append("face %d %d frameless 1 aspect 17" % (w, h))
        want.append("pal0 %d %d %d %d %08x %08x" % (w, h, ncol, trans, crc(p0), crc(pal)))
        want.append("pal1 %d %d %d %d %08x -" % (w, h, 0, -1, crc(p1)))
        save(name, data, want, exact=True)

    # 5b. the largest OS 3.5 picture, 256x256 of noise in 256 colours: too
    # big to run-length code in IMAG's 16-bit size, so stored uncoded
    w = h = 256
    o = {"type": 3, "tool": None, "tooltypes": None, "x": NO_POS, "y": NO_POS, "stack": 4096,
         "planar": [(16, 8, 2, picture(16, 8, 4))]}
    p0 = bytes(R.randrange(256) for _ in range(w * h))
    pal = bytes(R.randrange(256) for _ in range(768))
    body0 = struct.pack(">BBBBBBHH", 0, 255, 2, 0, 0, 8, len(p0) - 1, len(pal) - 1) + p0 + pal
    form = b"ICON" + chunk(b"FACE", struct.pack(">BBBBH", w - 1, h - 1, 0, 0, 767)) + chunk(b"IMAG", body0)
    data = classic(o) + b"FORM" + struct.pack(">I", len(form)) + form
    want = want_fields(o, 0x03)
    want.append("face %d %d frameless 0 aspect 0" % (w, h))
    want.append("pal0 %d %d %d %d %08x %08x" % (w, h, 256, -1, crc(p0), crc(pal)))
    save("glow_big", data, want, exact=True)

    # 6. AROS: ARGB chunks after the IMAGs
    w, h = 40, 32
    o = {"type": 3, "tool": None, "tooltypes": None, "x": NO_POS, "y": NO_POS, "stack": 4096,
         "planar": [(w, h, 2, picture(w, h, 4))]}
    argbs = [bytes(R.randrange(256) for _ in range(w * h * 4)), bytes([0, 1, 2, 3] * (w * h))]
    p0 = picture(w, h, 16)
    pal = bytes(R.randrange(256) for _ in range(48))
    i0, pr = rle(p0, 4), rle(pal, 8)
    form = b"ICON" + chunk(b"FACE", struct.pack(">BBBBH", w - 1, h - 1, 0, 0, 47))
    form += chunk(b"IMAG", struct.pack(">BBBBBBHH", 0, 15, 2, 1, 1, 4, len(i0) - 1, len(pr) - 1) + i0 + pr)
    for k, a in enumerate(argbs):
        z = zlib.compress(a, 9 if k == 0 else 1)
        form += chunk(b"ARGB", struct.pack(">IIH", 1, len(z), 0) + z)
    data = classic(o) + b"FORM" + struct.pack(">I", len(form)) + form
    want = want_fields(o, 0x07)
    want.append("face %d %d frameless 0 aspect 0" % (w, h))
    want.append("pal0 %d %d %d %d %08x %08x" % (w, h, 16, -1, crc(p0), crc(pal)))
    for k, a in enumerate(argbs):
        want.append("argb%d %d %d %08x" % (k, w, h, crc(a)))
    save("argb_aros", data, want, exact=True)

    # 7. NewIcons
    w, h = 30, 25
    pal0 = bytes(R.randrange(256) for _ in range(6 * 3))
    pal1 = bytes(R.randrange(256) for _ in range(3 * 3))
    n0, n1 = picture(w, h, 6), picture(w, h, 3)
    tts = ["FILETYPE=text", "*** DON'T EDIT THE FOLLOWING LINES!! ***"] + \
        newicons_lines(1, w, h, pal0, n0, True) + newicons_lines(2, w, h, pal1, n1, False)
    o = {"type": 4, "tool": "SYS:Utilities/More", "tooltypes": tts, "x": 40, "y": 50, "stack": 4096,
         "planar": [(w, h, 1, picture(w, h, 2))]}
    want = want_fields(o, 0x09)
    want.append("pal0 %d %d %d %d %08x %08x" % (w, h, 6, 0, crc(n0), crc(pal0)))
    want.append("pal1 %d %d %d %d %08x %08x" % (w, h, 3, -1, crc(n1), crc(pal1)))
    save("newicons", classic(o), want, exact=True)

    # 8. PNG icons: every colour type and depth, interlaced and not
    cases = [(6, 8, False), (6, 8, True), (6, 16, False), (2, 8, False), (2, 16, True), (3, 1, False), (3, 2, False),
             (3, 4, True), (3, 8, False), (0, 1, False), (0, 2, True), (0, 4, False), (0, 8, False), (0, 16, False),
             (4, 8, False), (4, 16, True)]
    for i, (ctype, depth, inter) in enumerate(cases):
        w, h = R.randrange(1, 70), R.randrange(1, 70)
        o = {"type": [3, 4, 2, 1][i % 4], "tool": "Work:Tools/OpenEdit" if i % 2 else None,
             "tooltypes": ["A=1", "B"] if i % 3 else None, "toolwin": "CON:" if i % 5 == 0 else None,
             "x": NO_POS if i % 4 == 0 else 100 + i, "y": NO_POS if i % 4 == 0 else 7, "stack": 4096 + i,
             "drawer": dict(drawer) if i % 4 == 2 else None, "planar": []}
        rows0, plte, trns, argb0 = png_case(w, h, ctype, depth, inter)
        png0 = png_encode(w, h, ctype, depth, rows0, plte, trns, inter, [(b"icOn", icon_chunk(o))])
        two = i % 2 == 0
        data = png0
        want_argb = [argb0]
        if two:
            rows1, plte1, trns1, argb1 = png_case(w, h, 6, 8, False)
            data += png_encode(w, h, 6, 8, rows1, plte1, trns1, False)
            want_argb.append(argb1)
        want = want_fields(dict(o, userdata=1), 0x10, w, h, 0x0004 | (0x0002 if two else 0))
        for k, a in enumerate(want_argb):
            want.append("argb%d %d %d %08x" % (k, w, h, crc(a)))
        save("png_%02d_c%d_d%d%s" % (i, ctype, depth, "_i" if inter else ""), data, want, exact=True)

    open(os.path.join(out_dir, "manifest.txt"), "w").write("\n".join(manifest) + "\n")

    # 9. raw deflate streams, every level and strategy, for ic_inflate
    with open(os.path.join(out_dir, "deflate.bin"), "wb") as f:
        for i in range(300):
            n = R.choice([0, 1, 2, 100, 1000, 40000, R.randrange(1, 70000)])
            kind = i % 3
            if kind == 0:
                data = bytes(R.randrange(256) for _ in range(n))
            elif kind == 1:
                data = bytes(R.choice(b"abcde ") for _ in range(n))
            else:
                data = (b"OpenBench " * (n // 10 + 1))[:n]
            strategy = [zlib.Z_DEFAULT_STRATEGY, zlib.Z_FIXED, zlib.Z_HUFFMAN_ONLY, zlib.Z_RLE, zlib.Z_FILTERED][i % 5]
            c = zlib.compressobj(R.randrange(10), zlib.DEFLATED, -15, 9, strategy)
            z = c.compress(data) + c.flush()
            f.write(struct.pack(">III", len(data), len(z), crc(data)) + z)


if __name__ == "__main__":
    main(sys.argv[1])
