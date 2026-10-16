# workbench.library L2: the calls OS 3.2's programs make

| Document | Detail |
| --- | --- |
| Kind | Record (step L2 of `Design-Open-Workbench-Library.md`) |
| Date | 13 October 2026 |
| Status | Recorded in lab `wbl2` only. Nothing hooks on an instance |
| Approved by | The user, 13 October 2026: "Yes, lab copy only" (decision 8 of the design) |
| Feeds | The design on openamigabench #22: its sections 2.1, 2.5, 2.7 and 5, and step L3 |
| Raw logs | `tests/amiga/l2-logs/` (index in its README) |
| Marks | (measured) seen in the logs; (inferred) the Team's reading of what was seen, not checked another way; (unknown) seen, but what it means is not known yet |

> **In short**
> - **IPrefs is the main user of the private calls.** At boot, and each time `Workbench.prefs` changes, it calls `WorkbenchControlA` with public and reserved tags. It also calls `WBConfig` with tags 1 to 5. Each field of `Workbench.prefs` that reaches workbench.library is matched to its tag below.
> - **`LoadWB` calls `StartWorkbench(0, BPTR)`.** It answers TRUE the first time, and FALSE while a Workbench runs.
> - **Nothing else recorded calls a private entry.** `QuoteWorkbench` was never called. The prefs editors, `WBInfo`, `DefIcons` and the Team's tools use public entries only.
> - **WBCtrl couldn't be recorded.** It is not part of OS 3.2.3, and it is on none of the Team's OS 3.2 media.
> - **R1 can't let OS 3.2's own Workbench start** while R1 answers to `workbench.library`: the Workbench fails as it starts. The new lab tool `r1swap` works round it. The Workbench then runs under R1, and what programs call it is recorded.

## 1. How it was recorded

- **The lab.** `wbl2`, copied from Instance-32 (AmigaOS 3.2.3, Kickstart 3.2.3 47.115). R1's proxy was built from this branch with the os32 stove (`build.sh`, `-Werror`, `-fno-delete-null-pointer-checks`) and copied to `LIBS:workbench.library`. OS 3.2's own `workbench.library` 47.42 (1.1.2025) was moved to `SYS:Storage/OpenUp-Superseded/`, where R1 loads it from.
- **No hidden entries.** The original's RomTag gives 19 entries and a base of 6,120 bytes (measured, read on the host). R1's 19 stubs cover every entry it has.
- **The spy.** `ENV:OpenBench/Spy-workbench.library` was set to 8192, so every call is kept from the first open at boot. Between steps: `wbspy SAVE`, then `STOP` and `START`, so each log holds one step.
- **wbspy 0.2.** The spy now also keeps 12 bytes behind each tag's data, before the call and after it. That shows the strings and structures IPrefs passes, and what a Get tag was answered.
- **Three kinds of boot.**
  1. **OpenBench's boot**, as the Startup-Sequence runs it today: `SYS:System/OpenBench`, no `LoadWB`. OS 3.2's library answers with no Workbench running.
  2. **`LoadWB` at boot**, with OpenBench moved aside in the lab. OS 3.2's Workbench fails as it starts (section 2).
  3. **No desktop at boot, then `LoadWB` by hand** with `r1swap` (section 2). OS 3.2's Workbench runs, and R1 records what programs call it.
- **Workbench.prefs variants.** `mkprefs.py` wrote four files. Each field has a value that can be told apart. Copying one to `ENV:Sys/Workbench.prefs` makes IPrefs apply it, so each tag is matched to its field.

| Variant | Stack | Restart | LockPens | No title bar | No volume gauge | Title delay | Copy buffer | Flags | Hidden | Title format |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| The lab's own | 4096 | 3 | 0 | 0 | 1 | 50 | 0 | 0 | none | none |
| A | 8192 | 5 | 1 | 1 | 0 | 77 | $20000 | $10081 | DF0, RAM | `L2 %wv %fc` |
| B | 12288 | 7 | 0 | 0 | 1 | 99 | 0 | 2 | CD0 | none |
| C | 4096 | 3 | 0 | 1 | 1 | 50 | $30000 | 0 | none | none |
| D | 4096 | 3 | 1 | 0 | 1 | 50 | 0 | 0 | none | none |

## 2. What the lab showed about R1

- **The original's version was 0.** R1 made the original's base with `MakeLibrary`, which leaves `lib_Version` at 0 (measured: 0.42). Fixed: R1 now sets the version and ID from the original's RomTag, as `ramlib` does (measured after: 47.42). This matters to `LoadWB`, which needs version 47. Given the original at 0.42, it stopped with "invalid resident library", return code 20 (measured).
- **OS 3.2's Workbench fails to start while R1 answers to `workbench.library`** (measured twice, with and without the version fix). `LoadWB` calls `StartWorkbench`, which answers TRUE. Then the Workbench process fails: "Program failed (error #80000006)". The lab's task guard reports a jump to the odd address $4EF900E1 in that process. In the first run the lab then reset.
- **With `r1swap` it starts** (measured). `r1swap ORIGINAL` lets the original answer to "workbench.library" for a moment. `LoadWB` starts the Workbench. `r1swap R1` gives the name back. The Workbench ran normally from then on (its WBStartup, its windows, the Information window), and every other program's calls went through R1.
- **Why** (inferred): as it starts, OS 3.2's Workbench finds "workbench.library" by name and uses that base as its own. R1's base is 132 bytes, not 6,120, so it reads past it. The Team didn't read OS 3.2's code to check this. The A/B result above is the evidence.
- **For the Team's library this is no constraint.** OpenBench as `workbench.library` has one base, its own. It does mean that the Workbench's start under `r1swap` is not recorded. `LoadWB`'s own `StartWorkbench` is recorded at boot, since it answers before the Workbench fails.

## 3. What each program called

The answers are OS 3.2's (measured). "No Workbench" is OpenBench's boot. "Workbench" is OS 3.2's Workbench running.

### 3.1 IPrefs (47.32)

IPrefs calls the library at boot, before any desktop. It calls it again each time a prefs file it watches changes. Its process is named `« IPrefs »`.

**At boot** (`ob-01`), in this order:

| # | Entry | Tags or value | What it is | Answer |
| --- | --- | --- | --- | --- |
| 1 | `WorkbenchControlA` | `Reserved12` (+57) = a pointer, `Reserved19` (+64) = 1 | +57 points at `wbp_EmbossRect` and what follows it (-1,-1,1,1, then `Borderless`): the icon border (bytes measured; meaning inferred). +64 = 1 in this first call only (unknown) | TRUE |
| 2 | `WorkbenchControlA` | `SetDefaultStackSize`, `SetTypeRestartTime`, `Reserved21` (+68), `Reserved14` (+59), `SetGlobalFlags`, `Reserved11` (+56), `Reserved38` (+114) | The settings (3.1.1) | TRUE |
| 3, 4 | `WBConfig` | tag 1, tag 2, value 0 | The two backdrop patterns, cleared | 0 |
| 5 | `WBConfig` | tag 4, a pointer | The system font (topaz 8) | 0 |
| 6 | `WorkbenchControlA` | `Reserved27` (+85), `Reserved26` (+84): two pointers to longwords | A question; both longwords stay 0 (unknown) | TRUE |
| 7 | `WBConfig` | tag 3, a pointer | The icon font (CGTriumvirate 13) | 0 |
| 8 | `WBConfig` | tag 5, value 0 | Always 0 here (unknown) | 0 |
| 9 | `WorkbenchControlA` | `Reserved18` (+63) | Lock pens: a pointer to the pens when on, NULL when off | TRUE |
| 10 | `WorkbenchControlA` | `GetHiddenDeviceList` (+42) | The hidden devices, read | TRUE; a `List` pointer written to the longword |
| — | `WorkbenchControlA` | `Reserved9` (+54), once for each name to drop | Its data points at the name | TRUE |
| 11 | `WorkbenchControlA` | `FreeHiddenDeviceList` (+43) | The list given back | TRUE |
| — | `WorkbenchControlA` | `Reserved8` (+53), once for each name to add | Its data points at the name | TRUE |
| 12 | `WorkbenchControlA` | `Reserved10` (+55) = 1 | Follows every hidden-device update (inferred: "apply") | TRUE |
| 13 | `WorkbenchControlA` | `Reserved30` (+102) | The screen title format: a string, or NULL with none | TRUE |
| 14, 15 | `WBConfig` | tags 1, 2, value 0 | Cleared again | 0 |
| later | `WBConfig` | tag 1, then tag 2, pointers | The backdrop patterns, about 0.8 s later. Tag 1's structure holds 1920 and 1080 (the root window's picture). Tag 2's holds 64 and 64 (the drawers' pattern) | 0 |
| later | rows 1, 2 and 9 to 13 again | the same | A second pass, about 1.3 s after the first (inferred: `Workbench.prefs` noticed again) | TRUE |

**When `Workbench.prefs` changes** (Prefs/Workbench's Save, or a copy to `ENV:Sys`): rows 1 (without +64), 2 and 9 to 13, in that order, with the hidden-device tags where needed (measured: `ob-04` to `ob-11`, `wb-01`, `wb-02`, `wb-06`). No `WBConfig`.

**When `Font.prefs` changes** (`Font … USE`): row 6, then `WBConfig` tag 3 (measured: `ob-14`, `wb-03`). This time `WBConfig` answers the previous value: the pointer IPrefs gave at boot.

**When `WBPattern.prefs` is used unchanged**: no call (measured: `ob-13`, `wb-04`).

#### 3.1.1 Workbench.prefs to tags (measured with the variants)

| Field (`prefs/workbench.h`) | Tag | Value |
| --- | --- | --- |
| `wbp_DefaultStackSize` | `WBCTRLA_SetDefaultStackSize` (+22) | as it is |
| `wbp_TypeRestartTime` | `WBCTRLA_SetTypeRestartTime` (+48) | as it is |
| `wbe_DisableTitleBar` | `WBA_Reserved21` (+68) | inverted: 1 = the title bar is shown |
| `wbe_DisableVolumeGauge` | `WBA_Reserved14` (+59) | inverted: 1 = the gauge is shown |
| `wbe_LockPens` | `WBA_Reserved18` (+63) | a pointer to a pen table when set, NULL when not |
| `wbe_TitleUpdateDelay` | `WBA_Reserved38` (+114) | as it is |
| `wbe_Flags` | `WBCTRLA_SetGlobalFlags` (+130) | as it is |
| `wbp_EmbossRect`, `wbp_Borderless` | `WBA_Reserved12` (+57) | a pointer to them (not varied: inferred) |
| `WBHD` chunks | `WBA_Reserved9` (+54) to drop, `WBA_Reserved8` (+53) to add, then `WBA_Reserved10` (+55) = 1 | one name each |
| `WBTF` chunk | `WBA_Reserved30` (+102) | the format string, or NULL |
| (none found) | `WBA_Reserved11` (+56) | always 1 (unknown) |
| `wbe_CopyBufferSize` | none | Not passed in any variant: $20000 and $30000 gave the same tags as 0 |
| `wbe_IconMemoryType`, icon precision, name length, NewIcons, colour icons | none | Not passed to workbench.library (inferred: they are icon.library's) |

### 3.2 LoadWB (47.1)

| Entry | Arguments | Answer |
| --- | --- | --- |
| `StartWorkbench` | D0 = 0. D1 = a BPTR (times 4, it is the address in A2) | TRUE when it starts the Workbench (`lw-01`, `lw-02`). FALSE when one already runs; `LoadWB` then ends with return code 5 (`wb-10`) |

`LoadWB` opens `workbench.library` at version 47 or more. Below that it stops with "invalid resident library", return code 20 (measured, section 2).

### 3.3 The prefs editors

| Program | Entry | When | Answer |
| --- | --- | --- | --- |
| Prefs/Workbench (47.33) | `AddAppWindowA` (no tags) | as it opens | NULL with no Workbench; an AppWindow with one |
| Prefs/Workbench | `RemoveAppWindow` | as it closes | TRUE |
| Prefs/Workbench | nothing else | its settings | It reads and writes `Workbench.prefs` itself. IPrefs then calls the library (3.1) |
| Font, WBPattern (`USE` from a Shell) | nothing | | Their files only. IPrefs calls the library |

### 3.4 WBInfo, DefIcons, WBCtrl

| Program | Entry | Arguments | Answer |
| --- | --- | --- | --- |
| `C:WBInfo SYS:` | `WBInfo` | A0 a lock, A1 "disk", A2 the screen. Called from a process named after the object ("disk") | 0 with no Workbench. 1 with one, once its window is closed |
| `C:DefIcons` (47.9) | nothing | | It works through icon.library only |
| WBCtrl | not recorded | | Not part of OS 3.2.3: not in the lab, and not on Workbench3.2.adf, Update3.2.3 or the 3.2 CD. The CD's notes mention `WBCtrl LABELSHADOW` and `LABELOUTLINE` for icon.library 47.3, and its FAQ says not to use WBCtrl any more. Nothing was downloaded |

### 3.5 OpenBench 0.1.22 and the Team's tools, as they run today

| Caller | Entry | Tags | Answer, no Workbench | Answer, Workbench |
| --- | --- | --- | --- | --- |
| OpenBench (its main process) | nothing | | | |
| OpenBench's dock (`OpenBench Dock`) | `AddAppWindowA` | none | NULL | not run |
| `C:OpenUpMenu` | `AddAppMenuItemA` "OpenUp", then "Preferences" | `WBAPPMENUA_GetKey` (+65) | NULL; it tries again every 2 s | a menu item, whose key is written back |
| `C:OpenUpMenu` | `AddAppMenuItemA` for each sub-item ("OpenFiles" to "OpenUp Setup") | `WBAPPMENUA_UseKey` (+66) | not reached | a menu item each |
| `C:OpenDock` | `AddAppWindowA`; `RemoveAppWindow` and a new `AddAppWindowA` when the screen changes | none | NULL | an AppWindow |
| `C:OpenDock` | `OpenWorkbenchObjectA` "SYS:Utilities/OpenPuTTY/OpenPuTTY" (a click on the dock) | none | FALSE | not tried |
| `window.class` (ReAction) | `WorkbenchControlA` | `WBCTRLA_AddSetupCleanupHook` (+78) | TRUE | TRUE |

## 4. The private calls

| Entry or tag | Who | Seen |
| --- | --- | --- |
| `QuoteWorkbench` (-36) | nobody | Never called in any run |
| `StartWorkbench` (-42) | `LoadWB` | Section 3.2 |
| `WBConfig` (-84), tags 1 to 5 | IPrefs | Section 3.1. No other tag, and no other caller |
| `WBA_Reserved8` to `12`, `14`, `18`, `19`, `21`, `26`, `27`, `30`, `38` | IPrefs | Section 3.1 |
| Any other reserved tag | nobody | Not seen |

## 5. What the Team's workbench.library (48) must answer

| Call | It must |
| --- | --- |
| `StartWorkbench(0, BPTR)` | Start OpenBench and answer TRUE. While it runs, answer FALSE and do nothing else, so `LoadWB` ends with 5 as on OS 3.2. D1 isn't needed to start it (what the BPTR is for is unknown) |
| `WBConfig(1, p)`, `WBConfig(2, p)` | Keep the pattern IPrefs gives for the root window (1) and the drawers (2); 0 clears it. Answer the value it held before (0 the first time) |
| `WBConfig(3, p)`, `WBConfig(4, p)` | Keep the icon font (3) and the system font (4). Answer the value held before: IPrefs gets back the pointer it gave last time |
| `WBConfig(5, 0)` | Accept it and answer 0 |
| `WBConfig`, any other tag | Answer 0 (not seen) |
| `WorkbenchControlA` with IPrefs' tags (3.1.1) | Keep each setting and answer TRUE, before the desktop is up too (decision 3). The inverted tags (+59, +68) and the pointer tags (+57, +63, +102) as measured. +53 and +54 add and drop a hidden device by name; +55 applies them |
| `Reserved26`, `Reserved27` (+84, +85) | Answer TRUE and leave 0 in both longwords, as OS 3.2 did (what they ask is unknown) |
| `Reserved19`, `Reserved11` (+64, +56) | Accept them and answer TRUE (meaning unknown) |
| `GetHiddenDeviceList`, `FreeHiddenDeviceList` | As the design's 2.5 (built in L1) |
| `AddAppMenuItemA` with `GetKey` and `UseKey` | Build the sub-menu: write the key at the `GetKey` pointer, and accept it with `UseKey`. Keep them before the desktop is up, so OpenUpMenu's first try succeeds and it stops retrying |
| `AddAppWindowA`, `RemoveAppWindow` | As the design's 2.1 and 2.2. OpenDock, OpenBench's dock and Prefs/Workbench add theirs with no tags |
| `OpenWorkbenchObjectA(path)` | Open or start it, and answer TRUE |
| `WBInfo(lock, name, screen)` | Open the library's own Information window, and answer TRUE once it closes |
| `WorkbenchControlA` `AddSetupCleanupHook` | As the design's 2.5 (built in L1) |
| `QuoteWorkbench` | Nothing in OS 3.2.3's own programs needs it. Answer as the design's 2.1 says |

`Workbench.prefs`' copy buffer size never reaches the library. If OpenBench wants it, it has to come another way (a question for L3).

## 6. Not recorded, and why

- **WBCtrl**: not on the Team's OS 3.2 media (3.4).
- **The Workbench's own calls into its library**: they don't go through R1, so wbspy can't see them. And while the Workbench starts under `r1swap`, the original answers directly.
- **IPrefs at boot with OS 3.2's Workbench already running**: IPrefs runs before `LoadWB`, so there is no such boot. Its later passes were recorded with the Workbench running (`wb-01`, `wb-02`, `wb-06`), and they match those with no Workbench.
- **What +56, +64, +84, +85, `WBConfig` tag 5 and `StartWorkbench`'s BPTR mean**: the values are recorded; their meaning is not known yet.
- **Calls nobody made**: `UpdateWorkbench`, `AddAppIconA`, the drop zones, `CloseWorkbenchObjectA`, `ChangeWorkbenchSelectionA`, `MakeWorkbenchObjectVisibleA`, `WhichWorkbenchObjectA`. No OS 3.2 program in these runs used them.
- **icon.library**: not proxied in this lab. Only `workbench.library` was installed.

## 7. Doing it again

1. Build with `build.sh` (os32 stove). Make a lab copy of an instance; never use an instance.
2. In the lab: `Rename LIBS:workbench.library SYS:Storage/OpenUp-Superseded/workbench.library`. Copy R1's `workbench.library` to `LIBS:`, and `wbspy` and `r1swap` to `C:`. `SetEnv SAVE OpenBench/Spy-workbench.library 8192`. Reboot.
3. Record with `wbspy WORKBENCH SAVE file STOP`, then `wbspy WORKBENCH START ENTRIES 8192` before the next step.
4. For OS 3.2's Workbench: boot with no desktop, then `r1swap ORIGINAL`, `Run >NIL: C:LoadWB`, wait, `r1swap R1`.
5. To undo: copy `SYS:Storage/OpenUp-Superseded/workbench.library` back to `LIBS:` and reboot.

MIT, Copyright (c) 2026 AmigaChrome Team.
