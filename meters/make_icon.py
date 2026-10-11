#!/usr/bin/env python3
"""OpenMeters' icon, as the Open family makes them (openamigaup ICONS.md): an
OS 3.5 GlowIcon with two frames, the panel at rest and the one shown while the
icon is chosen, from art/OpenMeters.png and art/OpenMeters-selected.png.

    python3 meters/make_icon.py AMIGACHROME OUT.info [STACK]

AMIGACHROME is an amigachrome checkout (its scripts/amiga_icon.py writes the
icon). As OpenUp's pack does it: each 64 x 64 picture is shown at 85 per
cent, every pixel solid or see-through, dithered for a 16-bit screen, and the
chosen frame gets the OS 3.5 glow. The icon is a tool, left unplaced so
Workbench puts it where there is room, with the tooltypes OpenMeters' own
MAKEICON writes: they are read from openmeters.c (make_icon), so the two
always agree. MAKEICON run on the program later keeps this picture and writes
the tooltypes again.

The pictures are drawn in openamigaup (tools/open_icons_art.py, openmeters,
in the Gorilla style) and copied here; draw them there and copy them again to
change them. Needs Pillow. MIT, Copyright (c) 2026 Dalsin Limited.
"""
from __future__ import annotations

import re
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
ART = HERE / "art"
ICON_SCALE = 0.85        # as openamigaup's build_pack.py, and as Gorilla's are shown
WBTOOL = 3


def tooltypes() -> list[str]:
    """MAKEICON's tooltypes, from openmeters.c's make_icon (its tt[] list)."""
    src = (HERE / "openmeters.c").read_text(encoding="latin-1")
    m = re.search(r"static\s+int\s+make_icon\(void\).*?static\s+STRPTR\s+tt\[\]\s*=\s*\{(.*?)NULL\s*\}", src, re.S)
    if not m:
        raise SystemExit("openmeters.c: make_icon's tooltypes (tt[]) not found")
    return re.findall(r'"((?:[^"\\]|\\.)*)"', m.group(1))


def main() -> int:
    if len(sys.argv) not in (3, 4):
        print(__doc__.split("\n\n")[1])
        return 2
    amigachrome, out = Path(sys.argv[1]), Path(sys.argv[2])
    stack = int(sys.argv[3]) if len(sys.argv) == 4 else 16384
    sys.path.insert(0, str(amigachrome / "scripts"))
    import amiga_icon
    from PIL import Image

    def hard(im: Image.Image) -> tuple[int, int, bytes]:
        im = im.resize((round(im.width * ICON_SCALE), round(im.height * ICON_SCALE)), Image.LANCZOS)
        rgba = bytearray(im.tobytes())
        for i in range(0, len(rgba), 4):
            rgba[i:i + 4] = rgba[i:i + 3] + b"\xff" if rgba[i + 3] >= 110 else b"\0\0\0\0"
        return im.width, im.height, bytes(rgba)

    rest = Image.open(ART / "OpenMeters.png").convert("RGBA")
    chosen = Image.open(ART / "OpenMeters-selected.png").convert("RGBA")
    (w, h, a), (_, _, b) = hard(rest), hard(chosen)
    pics = [(w, h, amiga_icon.dither_rgb565(w, h, a)),
            (w, h, amiga_icon.dither_rgb565(w, h, amiga_icon.glow_selected(w, h, b)))]
    none = amiga_icon.NO_POSITION
    meta = {"type": WBTOOL, "defaultTool": None, "toolTypes": tooltypes(), "toolWindow": None, "stack": stack,
            "x": none, "y": none, "drawer": None}
    out.write_bytes(amiga_icon.write_os35(meta, pics))
    print(f"{out} ({out.stat().st_size} bytes)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
