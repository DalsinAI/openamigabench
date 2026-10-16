#!/usr/bin/env python3
"""Annotate wbspy logs: each tag's name from the NDK's workbench.h (read on
the stove, never copied into the repository), the caller, the entry.
decode.py LOG... prints the log with a "#" line under each call naming its
tags, and a summary of (task, entry, tags) at the end.
MIT, Copyright (c) 2026 AmigaChrome Team."""
import collections, os, re, sys

H = os.path.join(os.environ.get('STOVE', os.path.expanduser('~/AmigaChrome/stoves/os32')), 'ndk/Include_H/workbench/workbench.h')
names = {}
for line in open(H, encoding='latin-1'):
    m = re.match(r'#define\s+(\w+)\s+\(WBA_Dummy\s*\+\s*(\d+)\)', line)
    if m:
        v = 0x8000A000 + int(m.group(2))
        names.setdefault(v, []).append(m.group(1))

def tagname(v):
    n = names.get(v)
    if not n:
        return 'WBA_Dummy+%d (no name in the NDK)' % (v - 0x8000A000) if 0x8000A000 <= v < 0x8000B000 else '$%08X' % v
    return '/'.join(n) + ' (+%d)' % (v - 0x8000A000)

LINE = re.compile(r'^(\d+)\s+(?:([\d.]+)ms\s+)?\[(.{15})\] (\w+) (\w+)\((-\d+)\)(.*)$')
summary = collections.OrderedDict()
for path in sys.argv[1:]:
    print('#### ' + os.path.basename(path))
    for line in open(path, encoding='latin-1'):
        line = line.rstrip('\n')
        print(line)
        m = LINE.match(line)
        if not m:
            continue
        task, func, rest = m.group(3).strip(), m.group(5), m.group(7)
        tags = re.findall(r'\$([0-9A-F]{8})=\$([0-9A-F]{8})', rest.split(' tags={')[1]) if ' tags={' in rest else []
        regs = dict(re.findall(r'(d0|d1|d2|a0|a1|a2|a3|a4)=\$([0-9A-F]{8})', rest))
        notes = []
        for t, d in tags:
            notes.append('%s=$%s' % (tagname(int(t, 16)), d))
        if func == 'WBConfig':
            notes.append('WBConfig tag d0=%d value d1=$%s' % (int(regs['d0'], 16), regs['d1']))
        if func == 'StartWorkbench':
            notes.append('flags d0=$%s ptr d1=$%s' % (regs['d0'], regs['d1']))
        if func == 'QuoteWorkbench':
            notes.append('string d0=%d' % int(regs['d0'], 16))
        if notes:
            print('    # ' + '; '.join(notes))
        key = (os.path.basename(path), task, func, tuple(tagname(int(t, 16)) for t, _ in tags),
               ('tag %d' % int(regs['d0'], 16)) if func == 'WBConfig' else '')
        summary[key] = summary.get(key, 0) + 1
print('\n#### summary: log | task | entry | tags or WBConfig tag | calls')
for (log, task, func, tags, extra), n in summary.items():
    print('%s | %s | %s | %s | %d' % (log, task, func, ', '.join(tags) or extra, n))
