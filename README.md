# OpenBench

Our own Workbench for AmigaOS 3.2: a `workbench.library`, an `icon.library` and a desktop, OpenBench, that replace Hyperion's. Part of the Open family. MIT licensed and free.

- **icon.library** reads and writes every icon format, PNG icons included. It draws them with alpha on true-colour screens and in the screen's pens elsewhere.
- **workbench.library** keeps exactly 3.2's interface: every entry programs, IPrefs and `LoadWB` call, and no new ones.
- **OpenBench** is the desktop. It uses the drawer windows already approved and built as OpenDrawer, OpenFiles' engine for copying and deleting, and OpenLook's themes. The root is always a backdrop. The bar at the top of the screen can be see-through and can hide itself.

The design is in amigachrome: `docs/design/Design-Workbench-Replacement.md`. Its shape was approved on 9 October 2026. This repository was named then, and the desktop was named OpenBench.

## Status: step R1, not yet run

Both libraries exist as **proxies**. Each one forwards every call to Hyperion's original, which it loads privately from `SYS:Storage/OpenUp-Superseded/`. Nothing changes on screen. From here, groups of calls are taken over one at a time, each once it behaves exactly like the original in a lab (the design's §4.5 and §8).

| Part | What it does now |
| --- | --- |
| `workbench/wblib.c` | `workbench.library`, version 47.42 like 3.2.3's. Its 19 entries are forwarded |
| `icon/iconlib.c` | `icon.library`, version 47.5 like 3.2.3's. Its 30 entries are forwarded |
| `proxy/` | What both share: loading the original under a private name (`OpenUp.original.workbench.library`, for example), the forwarding stubs, and the spy |
| `tools/wbspy` | Records every call into both libraries in a lab: the calling program, the registers, any name or tag list, and the result. It saves one line per call, both libraries merged in the order the calls were made (by the EClock). Its log is the contract for the private entries (`StartWorkbench`, `WBConfig`, `QuoteWorkbench`) |
| `lab/Install-R1`, `lab/Uninstall-R1` | Put our libraries in a lab instance with Hyperion's kept aside, and put Hyperion's back |

Nothing here has been run on an Amiga yet. It is for **lab instances only** until the oracle runs in the design's §7 pass. OpenBench needs AmigaOS 3.2 and a 68020 or better (the design's §6). Both the libraries and `Install-R1` refuse anything older.

## Building

`build.sh` builds with the Team's os32 stove (m68k-amigaos-gcc and NDK 3.2; the NDK is never in this repository). It writes `workbench.library`, `icon.library`, `wbspy` and the two lab scripts to `build/os3/`. Pull requests are built on the Team's runner. `tests/host/run.sh` runs the host tests.

## Trying R1 in a lab

1. Make a lab: `lab_instance.py copy` from a 3.2.3 instance (amigachrome `docs/development/labs.md`).
2. Copy `build/os3/` into the lab's disk. In a Shell there, `cd` to it and run `Install-R1`. Hyperion's libraries go to `SYS:Storage/OpenUp-Superseded/`, and ours into `LIBS:`.
3. To record the boot as well:
   ```
   SetEnv SAVE OpenBench/Spy-workbench.library 4096
   SetEnv SAVE OpenBench/Spy-icon.library 4096
   ```
4. Reboot. Workbench should look and behave exactly as before.
5. Check the libraries and save the recording:
   - `wbspy STATUS` shows both libraries and their originals;
   - `wbspy SAVE RAM:boot.txt` saves every call so far: `LoadWB`'s `StartWorkbench`, IPrefs' `WBConfig`s, and the rest.
6. `Uninstall-R1` and a reboot put Hyperion's back.

## Licence

MIT, Copyright (c) 2026 Dalsin Limited. See `LICENSE`. AROS's libraries were read for behaviour only, and no code was taken from them or from Hyperion's.
