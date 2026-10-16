#!/usr/bin/env python3
"""Workbench.prefs variants for step L2 (lab only): each field set to a value
that can be told apart in wbspy's log, so each tag IPrefs passes to
workbench.library is matched to its field (prefs/workbench.h, WBNC).
  mkprefs.py OUT_DIR    writes A.prefs to D.prefs there
Copy one to the lab's ENV:Sys/Workbench.prefs and IPrefs applies it.
MIT, Copyright (c) 2026 AmigaChrome Team."""
import os, struct, sys

OUT = sys.argv[1]
os.makedirs(OUT, exist_ok=True)

def chunk(cid, data):
    return cid + struct.pack('>I', len(data)) + data + (b'\0' if len(data) & 1 else b'')

def prefs(stack, restart, lockpens, notitle, nogauge, delay, copybuf, flags, hidden=(), title=None):
    wbnc = struct.pack('>IIIhhhhHiHH', stack, restart, 16, -1, -1, 1, 1, 1, 25, 1, 1)
    wbnc += struct.pack('>IHHHHII', 0, lockpens, notitle, nogauge, delay, copybuf, flags)
    assert len(wbnc) == 50
    body = b'PREF' + chunk(b'PRHD', bytes(6)) + chunk(b'WBNC', wbnc)
    for h in hidden:
        body += chunk(b'WBHD', h.encode('latin-1') + b'\0')
    if title is not None:
        body += chunk(b'WBTF', title.encode('latin-1') + b'\0')
    return b'FORM' + struct.pack('>I', len(body)) + body

open(os.path.join(OUT, 'A.prefs'), 'wb').write(
    prefs(0x2000, 5, 1, 1, 0, 77, 0x20000, 0x00010081, hidden=('DF0', 'RAM'), title='L2 %wv %fc'))
open(os.path.join(OUT, 'B.prefs'), 'wb').write(
    prefs(0x3000, 7, 0, 0, 1, 99, 0, 0x00000002, hidden=('CD0',)))
# C: the title bar off alone, pens not locked, a copy buffer, to tell those apart
open(os.path.join(OUT, 'C.prefs'), 'wb').write(
    prefs(0x1000, 3, 0, 1, 1, 50, 0x30000, 0))
# D: pens locked alone
open(os.path.join(OUT, 'D.prefs'), 'wb').write(
    prefs(0x1000, 3, 1, 0, 1, 50, 0, 0))
print(sorted(os.listdir(OUT)))
