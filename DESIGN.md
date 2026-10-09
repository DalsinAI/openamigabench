# OpenBench: the design

The design lives in amigachrome, beside the Team's other designs: **`docs/design/Design-Workbench-Replacement.md`**.

What it says, in short:

- **Three pieces:**
  - our `icon.library`;
  - a thin `workbench.library` holding what programs register (AppIcons, AppWindows, AppMenuItems, drop zones);
  - the desktop, OpenBench (`SYS:System/OpenBench`, process `Workbench`, ARexx port `WORKBENCH`).
- **Proxy first.**
  - Each library forwards to Hyperion's original until each group of entries matches it in a lab. A `DiskObject` or App object always goes back to the code that made it.
  - Hold Shift, or set `ENV:OpenBench/Original`, and everything is forwarded: the safe start.
  - A desktop that won't start falls back to Hyperion's.
- **No new entry points.**
  - 3.2's tables exactly: 19 entries in `workbench.library` and 30 in `icon.library`.
  - The versions reported are exactly 3.2.3's (47.42, 47.5).
  - The library and the desktop talk through a private port.
- **Parity with Workbench 47** is the measure. A checklist taken from the 3.2 release notes is the design's Appendix A, and its differences on purpose are listed there.
- **Steps R1 to R8**, with our `icon.library` (PNG icons on Hyperion's Workbench) as the first release.
