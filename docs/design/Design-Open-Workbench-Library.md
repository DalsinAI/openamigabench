# workbench.library: the Team's own, as OpenBench's face

| Document | Detail |
| --- | --- |
| Kind | Design |
| Date | 13 October 2026 |
| Status | **Draft 0.1, for the user's approval.** Nothing in it hooks Workbench yet. Step L1 (the lists, a test build under its own name, and the side-by-side test) is built with it and hooks nothing (section 8) |
| Decision it carries out | 13 October 2026: a real `workbench.library` for Team 4, with the user's design approval before anything hooks |
| Builds on | amigachrome `docs/design/Design-Workbench-Replacement.md` 0.2 (the shape approved on 9 October: its section 4 is this library's outline, R4 its step); amigachrome-guest `libraries/OPEN_LIBRARIES.md` ("workbench.library needs the user's design approval before it hooks anything"); OpenBench 0.1.22 (`desktop/openbench.c`); R1's proxy (`workbench/wblib.c`, `proxy/`) |
| Marks | (measured) seen on OS 3.2.3 in lab `wblib-a` by `WorkbenchABITest`, or read from the NDK 3.2 autodoc and headers; (recorded) in a Team document; (estimate) the Team's judgement; (to record) not known yet, recorded at step L2 |

> **In short**
> - **OpenBench is the program; the library is its face.** Programs, IPrefs and `LoadWB` call `workbench.library`. The library holds what they register (AppWindows, drop zones, AppIcons, AppMenuItems) and the Workbench settings, and answers at once. What needs a desktop (drawing an AppIcon, opening a drawer, the Information window) goes to OpenBench through a private port. OpenBench never patches the library, and the library never draws.
> - **Exactly OS 3.2's face:** the 19 entries of `wb_lib.sfd` 47.1, the same registers, the same structures and the same answers, corners included. No new entries. The library and OpenBench meet through a private interface behind the library base.
> - **OS 3.2's Workbench stays possible.** The library starts with no desktop and keeps the settings IPrefs sends. The first desktop to come decides: OpenBench registers and the library answers itself, or `LoadWB` calls `StartWorkbench` and the library hands everything to OS 3.2's own, which it loads privately (R1's proxy). Shift at boot, a machine without RTG, and OpenBench's Quit to Workbench all still give OS 3.2's Workbench.
> - **One voice for the Workbench settings.** The library holds them. IPrefs, WBCtrl and programs change them through the library's calls, and OpenBench reads them from the library instead of reading `ENV:Sys/Workbench.prefs` itself.
> - **Measured first.** The list calls are built and compared with OS 3.2's in a lab, side by side under a test name. With OS 3.2's Workbench running: 114 cases, 4 differ, all four of them questions that need a desktop, which step L3 answers. With no Workbench running: 60 cases, none differ (section 7).
> - **Ten decisions** are in section 9, each with a recommendation.

## 1. What this is, and what it isn't

- **It is the R4 of the approved replacement design,** worked out in detail. The replacement design (amigachrome, section 4) settled the outline: the library holds the lists, the desktop draws, proxy first, no new entries. This document turns that into calls, structures, measured answers and an order of work. It also records what has changed since 9 October:
  - OpenBench now **boots in place of `LoadWB`** (0.1, 11 October). It isn't started by `StartWorkbench`, and on a machine without RTG it runs `LoadWB` itself (`desktop/openbench.c`). Section 4 follows from this.
  - OpenBench already reads the Workbench prefs itself (0.1.19) and registers with Intuition itself (`AlohaWorkbench`, 0.1.22). Sections 3 and 5 say what moves.
  - OpenBench's head comment planned to take the App calls "as Directory Opus 5 takes them", by patching. This design replaces that plan. Nothing is patched, because the library is the Team's own.
- **It isn't `openworkbench.library`.** amigachrome's `Design-OpenWorkbench-Library.md` is a different library: the theme, rules and patch table the Open tools share. This document lives here, beside the code, rather than in amigachrome's `docs/design/`, so two near-identical names never sit in one folder. A one-line pointer from amigachrome can follow.
- **"Hooks" means,** for this design:
  - being installed as `LIBS:workbench.library`, in a lab or on a machine;
  - registering with Intuition;
  - any `SetFunction`;
  - a public port or a process of its own;
  - OpenBench registering with it.

  None of that is done before the user approves this design. Step L1 does none of it (section 8).

## 2. The face: everything OS 3.2's workbench.library offers

### 2.1 The entries

From `wb_lib.sfd` 47.1 (measured). Bias 30. "No desktop" is what OS 3.2 answers while no Workbench runs (measured in lab `wblib-a`, with OpenBench as the desktop and OS 3.2's Workbench not started).

| Offset | Entry | Registers | Answered by | With no desktop | L1 |
| --- | --- | --- | --- | --- | --- |
| -30 | `UpdateWorkbench` | A0 name, A1 lock, D0 action | a note to the desktop: re-read one object | nothing happens | does nothing |
| -36 | `QuoteWorkbench` (private) | D0 | to record (L2) | to record | 0 |
| -42 | `StartWorkbench` (private) | D0 flags, D1 pointer | the library: section 4 | starts OS 3.2's Workbench | 0 |
| -48 | `AddAppWindowA` | D0 id, D1 userdata, A0 window, A1 port, A2 tags | the library's list | NULL | built |
| -54 | `RemoveAppWindow` | A0 | the library's list | FALSE | built |
| -60 | `AddAppIconA` | D0, D1, A0 text, A1 port, A2 lock, A3 DiskObject, A4 tags | the library's list, then a note | NULL | built |
| -66 | `RemoveAppIcon` | A0 | the library's list, then a note | FALSE | built |
| -72 | `AddAppMenuItemA` | D0, D1, A0 text, A1 port, A2 tags | the library's list, then a note | NULL | built |
| -78 | `RemoveAppMenuItem` | A0 | the library's list, then a note | FALSE | built |
| -84 | `WBConfig` (private) | D0 tag, D1 value | the library keeps it and passes it on | to record | 0 |
| -90 | `WBInfo` | A0 lock, A1 name, A2 screen | the desktop; the call waits (section 3.4) | to record | 0 |
| -96 | `OpenWorkbenchObjectA` | A0 name, A1 tags | the desktop, with a reply | to record | 0 |
| -102 | `CloseWorkbenchObjectA` | A0 name, A1 tags | the desktop, with a reply | to record | 0 |
| -108 | `WorkbenchControlA` | A0 name, A1 tags | settings: the library; questions: the desktop (2.5) | settings work; questions in 2.5 | the settings |
| -114 | `AddAppWindowDropZoneA` | A0 AppWindow, D0, D1, A1 tags | the library's list | (needs an AppWindow) | built |
| -120 | `RemoveAppWindowDropZone` | A0 AppWindow, A1 zone | the library's list | (needs an AppWindow) | built |
| -126 | `ChangeWorkbenchSelectionA` | A0 name, A1 hook, A2 tags | the desktop, with a reply | to record | 0 |
| -132 | `MakeWorkbenchObjectVisibleA` | A0 name, A1 tags | the desktop, with a reply | to record | 0 |
| -138 | `WhichWorkbenchObjectA` | A0 window, D0 x, D1 y, A1 tags | the desktop, with a reply | to record | 0 |

### 2.2 AppWindows and drop zones (measured)

- `AddAppWindowA` takes a NULL window and a NULL port: OS 3.2 returns a handle for both, and so does the Team's. Nothing is ever sent to a NULL port, and a NULL window is never under the pointer.
- A tag it doesn't know is ignored.
- **A drop zone needs all four sides.** That is one of `WBDZA_Left` or `WBDZA_RelRight`, one of `WBDZA_Top` or `WBDZA_RelBottom`, one of `WBDZA_Width` or `WBDZA_RelWidth`, and one of `WBDZA_Height` or `WBDZA_RelHeight`. `WBDZA_Box` gives all four. Otherwise OS 3.2 returns NULL with IoErr 116 (`ERROR_REQUIRED_ARG_MISSING`).
- **The values themselves aren't checked.** A zone of width 0, of width -5, outside the window or larger than it is accepted.
- `RemoveAppWindowDropZone`:
  - a zone of another AppWindow: FALSE, IoErr 205 (`ERROR_OBJECT_NOT_FOUND`);
  - a NULL AppWindow or a NULL zone: FALSE, IoErr 116;
  - a zone removed: TRUE, IoErr 0.
- Removing an AppWindow removes its zones (the autodoc, and OS 3.2's answers agree).
- Where a zone is: worked out each time against the window's size, clipped to the window, and empty if nothing of it is left. Where two zones overlap, the first added wins (estimate; checked with real drops at L4).

### 2.3 AppIcons (measured)

- A NULL port is taken. A tag it doesn't know is ignored.
- `WBAPPICONA_SupportsOpen` is on unless turned off; the other `Supports` tags are off unless turned on (the autodoc).
- The label is the library's own copy. The DiskObject's pictures are copied too, unless `WBAPPICONA_PropagatePosition` is given. The autodoc: "Workbench will make a copy of that DiskObject's icon imagery, allowing you to free the DiskObject". The copy is made with `DupDiskObjectA` at L3, through whichever `icon.library` is in the system. L1 keeps the program's pointer only.
- A NULL label is taken; a NULL DiskObject gives NULL (measured, the RISKY cases of section 7.2).

### 2.4 AppMenuItems (measured)

| Rule | OS 3.2 |
| --- | --- |
| Items in the Tools menu | 62, from every program together. Workbench's own ResetWB is the 63rd. On Instance-32 OpenUpMenu's two items were already there, so a program could add 60 |
| Sub items under one item (`WBAPPMENUA_GetKey`, then `WBAPPMENUA_UseKey`) | 31 |
| Items under one new menu title (`WBAPPMENUA_GetTitleKey`) | 63 |
| New menu titles | 27 |
| Keys | never 0, and a title's key differs from an item's |
| `WBAPPMENUA_UseKey` with a key nobody gave | NULL |
| A label of only `-`, `_` or `~` | a separator (V44), added like any item |
| A command key already in use, or an empty one | the item is added, without the key (the autodoc) |
| Removing a parent before its sub items | TRUE; the sub item can still be removed afterwards (TRUE) |
| Removing an item | TRUE for any item that isn't NULL, even one already removed; FALSE for NULL |
| A NULL port, a NULL label | taken (a NULL label is an empty one) |

A parent shows only once it has a sub item, and a title only once it has an item (the autodoc). The desktop asks the library which items show (`wbc_menu_visible`).

### 2.5 WorkbenchControlA

Each tag has one owner. "No desktop" is OS 3.2's answer while no Workbench runs, "running" while it runs (both measured).

| Tags | Owner | No desktop | Running | L1 |
| --- | --- | --- | --- | --- |
| `Get/SetDefaultStackSize` | the library | works; 4096 | works | built |
| `Get/SetTypeRestartTime` | the library | works; 3 seconds | works | built |
| `Get/SetGlobalFlags` (V47) | the library | works; 0 | works | built |
| `Add/RemoveHiddenDeviceName`, `Get/FreeHiddenDeviceList` | the library | works. Names match without case. Removing a name that isn't there is TRUE | the same | built |
| `Get/SetCopyHook`, `DeleteHook`, `TextInputHook`, `DiskInfoHook` | the library; the desktop calls them | works; all NULL | the same | built |
| `Add/RemSetupCleanupHook` (V45) | the library's list; the desktop calls each at start, reset and quit | works; removing one twice is FALSE | the same | built |
| `IsOpen` | the desktop | TRUE, with FALSE in the answer | TRUE, with the answer | as with no desktop |
| `GetProgramList`, `GetSelectedIconList`, `GetOpenDrawerList`, `DuplicateSearchPath`, and their `Free` tags | the desktop | FALSE | TRUE | FALSE until L3 |
| `RedrawAppIcon` | the desktop | to record | to record | FALSE until L3 |
| A tag nobody knows; no tags; a NULL tag list | — | TRUE | TRUE | built |

The reserved tags (`WBA_Reserved1` to `79`) are OS 3.2's private ones. Which programs use which (WBCtrl's label options among them) is recorded at L2, and each is given an owner then.

### 2.6 AppMessages

The layout of `workbench/workbench.h`. `wb_core.h` writes it, and `openwb.c` checks every offset against the NDK when it is compiled (measured: 86 bytes with the 68k's alignment).

| Offset | Field | Value |
| --- | --- | --- |
| 0 | `am_Message` | exec's message; its reply port is the library's (below) |
| 20 | `am_Type` | 7 AppWindow, 8 AppIcon, 9 AppMenuItem, 10 drop zone |
| 22 | `am_UserData` | the program's |
| 26 | `am_ID` | the program's |
| 30 | `am_NumArgs` | the icons dropped (for an AppIcon, as the autodoc describes); 0 for a double-click or a menu item |
| 34 | `am_ArgList` | one `WBArg` per icon, each with its own lock; NULL when there are none |
| 38 | `am_Version` | `AM_VERSION` (1) |
| 40 | `am_Class` | `AMCLASSICON_*` for an AppIcon's menu items, `Selected` and `Unselected` |
| 42, 44 | `am_MouseX`, `am_MouseY` | where it happened |
| 46, 50 | `am_Seconds`, `am_Micros` | when |
| 54 | `am_Reserved[8]` | 0 |

- **The reply port is the library's,** served by a small process of its own (the handler), which lives as long as the library. A program may reply late, or after OpenBench has quit, and the message still comes back to a port that exists. The handler frees the locks and the message. A program that never replies keeps its locks, as on OS 3.2.
- **OpenBench never builds an AppMessage itself.** It asks the library to send one for an object, with the icons and the place (section 3.2), so the reply always reaches the handler.

### 2.7 The private calls, and who uses them

| Who | What | Known |
| --- | --- | --- |
| `C:LoadWB` | `StartWorkbench(flags, pointer)` | that it does; what the values mean: to record |
| IPrefs | `WBConfig(tag, value)` at boot, before any desktop, and on each prefs change; `WorkbenchControlA` for the Workbench prefs | that it does (the replacement design 1.1, from the release notes); which tags and values: to record |
| WBCtrl | `WorkbenchControlA` with reserved tags (label shadow and outline) | to record |
| icon.library | `UpdateWorkbench` when an icon is written with `ICONPUTA_NotifyWorkbench` | the autodoc |
| anyone | `QuoteWorkbench(n)` | the SFD says it returns a string; nothing else is known: to record |
| OpenUpMenu (the Team's) | `AddAppMenuItemA` for its OpenUp and Preferences sub-menus | measured: its two items were in OS 3.2's Tools menu |

`AlohaWorkbench` is Intuition's, not this library's. OpenBench calls it today (0.1.22) and keeps calling it (section 3.1).

Step L2 records these with `wbspy` (R1's spy) in a lab where R1's proxy is installed: every call that `LoadWB`, IPrefs, WBCtrl, the prefs editors and DefIcons make, with their registers and results. Its log is the contract for the private entries. Installing R1 in a lab copy needs the user's word (question 8).

## 3. OpenBench and the library: who does what

### 3.1 The split

| Work | The library | OpenBench |
| --- | --- | --- |
| The 19 entries programs call | answers them; keeps the lists and the settings | — |
| AppWindows, drop zones | keeps them; works out a zone's box | asks the library which zone is under a drop, then asks it to send the AppMessage |
| AppIcons | keeps them, with a copy of the pictures | draws them on the root window (calling the render hook), and their Icons menu items by `Supports` |
| AppMenuItems | keeps them, with the limits, keys and separators | builds its menus from them, with what shows |
| AppMessages | sends them and takes the replies (the handler) | says what was dropped or chosen, and where |
| Settings (stack, flags, hidden devices, hooks) | holds them: one voice (section 5) | reads them; calls the copy, delete, text input and disk info hooks; calls the setup and cleanup hooks at start, reset and quit |
| `OpenWorkbenchObjectA` and the other calls that need a desktop | sends a request and waits for the reply; fails at once when no desktop is registered | opens, closes, selects, shows, answers `WhichWorkbenchObjectA` |
| `UpdateWorkbench` | keeps a copy of the name and a `DupLock` of the parent in a pending set (the replacement design's rule: 64 at most, then "re-read every open drawer") | re-reads that object if its drawer is open |
| `WBInfo` | sends a request; the call waits until the window closes | opens its Information window (`ob_info.c`) in a helper process |
| `StartWorkbench` | section 4 | — |
| Intuition (`AlohaWorkbench`), the `Workbench` process name, the `WORKBENCH` ARexx port | — | as now (0.1.22) |
| Drawing, the look, drawers, starting programs | — | as now |

### 3.2 How they meet

- **No new entries.** OpenBench opens `workbench.library` like any program. It then reads a private interface behind the library base: a magic number, a version, and a table of functions (register, unregister, read the lists, send an AppMessage, take the notes). If the magic number or the version doesn't match, OpenBench works as it does today, with no App objects.
- **Register.** OpenBench gives the library its private port. From then on the library answers as a desktop is there.
- **Unregister** comes before OpenBench lets go of the screen at quit, so requests never wait on a desktop that has gone. Any request in hand is answered with failure.
- **Reading the lists.** OpenBench takes the library's semaphore shared, walks the list it wants, copies what it draws, and lets go. It never holds the semaphore while drawing or waiting on DOS.
- **Notes.** A change sets a bit (`WBC_NOTE_*`). One message goes to OpenBench while none is out, carrying every bit set since. No change is lost and the port never floods.
- **Requests** (the calls that need a desktop) are messages with a reply. The caller waits and OpenBench answers on its own process. No call from OpenBench into the library ever waits on OpenBench.

### 3.3 More than one core

The library is "anchored" in `OPEN_LIBRARIES.md`'s words: its lists belong to the 68k's world. Its state is behind one `SignalSemaphore`, never `Forbid()`, so it stays correct when callers run on other cores and their calls are carried to the anchor core.

### 3.4 WBInfo

The autodoc says `WBInfo` "may be called many times by different tasks", and that it is "the LVO that Workbench calls" for its own Information window. So OpenBench's Icons > Information goes through the library's vector too. A program that replaces `WBInfo` with `SetFunction`, as the autodoc invites, then replaces OpenBench's as well. Each call is one request. OpenBench opens each window in a helper process, so several can be open at once and none blocks the desktop (question 7).

## 4. With OS 3.2's Workbench still possible

### 4.1 The three ways a machine runs

| | `LIBS:workbench.library` | Desktop | What the library does |
| --- | --- | --- | --- |
| A, today | OS 3.2's | OpenBench, or OS 3.2's Workbench | — (OpenBench has no App objects, as now) |
| B | the Team's | OpenBench | answers itself |
| C | the Team's | OS 3.2's Workbench (`LoadWB`) | forwards every call to OS 3.2's own, loaded privately (R1's proxy, `OpenUp.original.workbench.library`) |

C happens whenever the Startup-Sequence or OpenBench runs `LoadWB`: Shift held at boot, a machine without RTG (OpenBench runs `LoadWB` itself), no `SYS:System/OpenBench`, `ENV:OpenBench/Desktop` set to `Hyperion`, or OpenBench's Quit to Workbench.

### 4.2 The first desktop decides

The replacement design had each library choose its mode once, when it is first opened. That no longer fits:

- IPrefs opens `workbench.library` and calls `WBConfig` before any desktop starts;
- OpenBench decides whether it can run (RTG or not) only once it is running.

So the library starts **undecided**:

- **Undecided.** The list calls answer as OS 3.2's do with no desktop (NULL, FALSE). `WBConfig` and the `WorkbenchControlA` settings are kept, in the order they came.
- **OpenBench registers.** The library answers itself (B). The settings it kept are OpenBench's to read.
- **`StartWorkbench` is called** (`LoadWB`) while no desktop is registered. The library loads OS 3.2's library privately, replays the kept settings to it in their order, calls its `StartWorkbench`, and forwards everything from then on (C). Shift at boot, or `ENV:OpenBench/Original`, makes it forward from the first call, as the replacement design says.
- **`StartWorkbench` while OpenBench is registered:** nothing happens, as OS 3.2's does when a Workbench already runs.

Programs see one rule throughout: OS 3.2's answers, with or without a desktop (measured, section 2).

### 4.3 Changing desktop without a reboot

- **OpenBench's Quit to Workbench** (B to C). OpenBench unregisters. The library loads OS 3.2's privately and replays the settings. Then it **adds again every App object** programs hold, keeping each program's handle and translating it to OS 3.2's from then on. Then `LoadWB` starts OS 3.2's Workbench. Nothing a program holds is lost. The replacement design (4.5) already describes this hand-over.
- **Back to OpenBench** (C to B): only after a reboot (question 4). OS 3.2's Workbench owns its lists once it runs, and a second hand-over is not worth its risk.

### 4.4 What doesn't change

- OpenBench keeps registering with Intuition (`AlohaWorkbench`) and keeps its process and port names.
- OpenBench works with OS 3.2's `workbench.library` (A) as it does today, without App objects.
- The library doesn't load on anything older than OS 3.2 (V47) or a 68020; the system's stays.

## 5. One voice for the Workbench settings

Today two programs read the same file:

- IPrefs reads `ENV:Sys/Workbench.prefs` and hands it to OS 3.2's Workbench (`WBConfig`, `WorkbenchControlA`);
- OpenBench reads the same file itself (0.1.19) for the default stack and the hidden devices.

With the library in (B), the library is the one voice:

| Who | Does what |
| --- | --- |
| Prefs/Workbench | writes the file, as now |
| IPrefs | reads it and calls the library, as it calls OS 3.2's |
| WBCtrl, programs | call `WorkbenchControlA` |
| The library | holds the settings, answers `Get`, and tells OpenBench of each change (a note) |
| OpenBench | reads the settings from the library, never from the file. Its own settings (the look, the bar, the benches) stay in `ENV:OpenBench/` and the Open preferences, and none of Workbench's are copied there |

In A, OpenBench keeps reading the file, as now. In C, OS 3.2's Workbench gets the settings through the library, as it would without it (question 5).

## 6. Safety

- **Nothing hooks before approval.** L1 is a library under its own name, `openwb.library`, and a test program. Neither is installed as `workbench.library` anywhere, neither registers with anything, and nothing patches anything.
- **The library refuses to be expunged** while a program holds one of its objects or an AppMessage is out.
- **A handle is looked up, never followed.** Every handle a program gives back is searched for in the library's lists before it is used. A stale or made-up one is answered "not found", where OS 3.2's can hang (section 7.2).
- **Shift at boot and `ENV:OpenBench/Original`** give OS 3.2's library and desktop (C from the start).
- **Version.** 47.42, as OS 3.2.3's (question 6), with the Team's name in the ID string.

## 7. Testing

### 7.1 On the host

`tests/host/test_wb_core.c`, run by `tests/host/run.sh` with the address and undefined-behaviour checkers: 126 checks on the lists, zones, menus, limits, settings, notes, running out of memory at each step, and the AppMessage's bytes. Every allocation is counted, so a leak fails the test.

### 7.2 Side by side with OS 3.2's

`tests/amiga/WorkbenchABITest` opens the system's `workbench.library` and `openwb.library` side by side, makes the same calls through each, and lists each difference. Nothing is replaced. It ran in lab `wblib-a`, a copy of Instance-32: OpenBench as the desktop, OS 3.2.3's `workbench.library` 47.42, and `openwb.library` copied into the lab's `LIBS:`.

| Run | Checks | Differ | The differences |
| --- | --- | --- | --- |
| No Workbench running (OpenBench the desktop), `ALL RISKY` | 60 | 0 | — |
| OS 3.2's Workbench running (`LoadWB` in the lab), `ALL` | 105 | 4 | `GetProgramList`, `GetSelectedIconList`, `GetOpenDrawerList` and `DuplicateSearchPath`: questions for a desktop, which L1 doesn't have |
| OS 3.2's Workbench running, `ALL RISKY` | 114 | 4 | the same four |

Rounds before these runs found and fixed eleven differences in the library: the NULL window and NULL port rules, the four sides a zone needs, IoErr 116, `RemoveHiddenDeviceName` on a missing name, an unknown `WorkbenchControlA` tag, `IsOpen` with no desktop, the type restart time's 3 seconds, a NULL menu label, and `RemoveAppMenuItem`'s answer. The Tools count is compared counting other programs' items, which the test reads from OS 3.2's own menu strip.

**The RISKY cases** (a NULL label, a NULL DiskObject, a zone on no AppWindow, a handle removed twice) run only when asked:

- Removing an AppWindow a second time, or an AppIcon a second time, **hangs OS 3.2's library** (measured twice: the lab stopped, and Tata later recorded "a jump to an odd address"). The test now asks those of the Team's library only, which answers FALSE.
- Removing an AppMenuItem a second time: OS 3.2 answers TRUE, and TRUE for any item that isn't NULL. The Team's answers the same, after looking the handle up.
- Removing a drop zone a second time: FALSE, IoErr 205, both.
- A drop zone on a NULL AppWindow: NULL, both.
- An AppIcon with a NULL label: taken, both. With a NULL DiskObject: NULL, both.
- An AppMenuItem with a NULL label: taken by OS 3.2, and by the Team's, with an empty label.

### 7.3 Later

- L2's `wbspy` log is the contract for the private calls.
- L3 to L5: `WorkbenchABITest` against the library installed under its real name in a lab, with OpenBench registered; real drops on AppWindows, AppIcons and zones through Cradle's input, with screenshots; the hand-over of 4.3 with a program holding each kind of object.

## 8. The order of work

| Step | What | Hooks? | Done when |
| --- | --- | --- | --- |
| **L1 (this PR)** | `wb_core` (the lists, zones, menus, settings, notes, AppMessage layout); `openwb.library` under its test name; `WorkbenchABITest` | **No** | host tests pass; `WorkbenchABITest` differs only in what needs a desktop |
| L2 | Record the private calls with `wbspy`, R1 installed in a lab copy only | In a lab only, on the user's word (question 8) | the log of `LoadWB`, IPrefs, WBCtrl, the prefs editors, DefIcons |
| L3 | The real library: the private interface, the handler process, AppMessages, requests, the undecided start (4.2), `WBConfig`, the pictures copied for AppIcons | Yes, in a lab | `WorkbenchABITest` against it under its real name, with a test desktop that only answers |
| L4 | OpenBench registers: AppIcons on the desktop, the Tools menu, drops on AppWindows and zones, `WBInfo` | Yes, in a lab | the App lines of the replacement design's Appendix A, with screenshots; the programs of its Appendix B with AppWindows and AppIcons |
| L5 | One voice (section 5); the hand-over to OS 3.2's (4.3) | Yes, in a lab | IPrefs' changes reach OpenBench; Quit to Workbench with App objects out |
| L6 | OpenUp: install with `retire`, Uninstall | On a machine, on the user's word | a clean install and uninstall; Shift at boot |

## 9. Decisions for the user

1. **The split** (sections 1 and 3). The library is the face (lists, settings, AppMessages, requests) and OpenBench is the program (drawing, menus, drops, windows). They meet through a private interface behind the base, and nothing is patched. *Recommended: yes.*
2. **The first desktop decides** (4.2), instead of the library choosing at its first open. *Recommended: yes,* because IPrefs opens the library before any desktop, and OpenBench decides about RTG only once it runs.
3. **No desktop, no App objects** (2.1). Add calls fail while no desktop runs, as OS 3.2's do (measured), rather than being kept until one comes. *Recommended: as OS 3.2,* since programs already handle it and OpenBench starts right after IPrefs.
4. **Changing desktop without a reboot** (4.3). Keep OpenBench's Quit to Workbench, handing App objects over to OS 3.2's; go back to OpenBench only after a reboot. *Recommended: yes.*
5. **One voice** (5). With the library in, OpenBench reads the Workbench settings only from the library. *Recommended: yes.*
6. **Version:** 47.42, as OS 3.2.3's and as the replacement design says, rather than `OPEN_LIBRARIES.md`'s 47.100 and up. *Recommended: 47.42,* because a later OS 3.2 revision may add entries, and 47.100 would tell programs they are there.
7. **WBInfo** (3.4). OpenBench's Information window in a helper process, the caller waiting, rather than the window's code inside the library in the caller's task (the replacement design's 4.1). *Recommended: OpenBench's,* so there is one Information window and the library stays small.
8. **L2 in a lab.** May R1's proxy be installed as `workbench.library` in a lab copy (never an instance) to record the private calls with `wbspy`? *Recommended: yes;* without it the private calls can only be guessed.
9. **The test name** `openwb.library`, not `openworkbench.library`, which names the theme library. *Recommended: yes.*
10. **Where this design lives:** here in `openamigabench`, beside the code, with a pointer from amigachrome's `docs/design/`. *Recommended: yes.*

## Sources

- NDK 3.2: `SFD/wb_lib.sfd` 47.1, `Include_H/workbench/workbench.h`, `Autodocs/wb.doc` (read on the Team's machine; never committed).
- amigachrome: `docs/design/Design-Workbench-Replacement.md` 0.2; `docs/design/Design-OpenWorkbench-Library.md` 0.3.
- amigachrome-guest: `libraries/OPEN_LIBRARIES.md`.
- This repository: `desktop/openbench.c` (0.1.22), `workbench/wblib.c` (R1), `proxy/`, `tools/wbspy.c`.
- Measured: `WorkbenchABITest` in lab `wblib-a`, 13 October 2026.
- Clean room: AROS's `workbench.library` was not needed for this step and was not read for it. No code from Hyperion's or AROS's.
