# workbench.library: OpenBench is the library

| Document | Detail |
| --- | --- |
| Kind | Design |
| Date | 13 October 2026 |
| Status | **Version 0.2, with the user's decisions of 13 October 2026** (section 9). Version 0.1 proposed a library acting as OpenBench's face, with OS 3.2's Workbench still possible beside it. The user chose otherwise: OS 3.2's `workbench.library` is retired, and OpenBench becomes `workbench.library` itself. Nothing hooks yet. Step L1 is built and hooks nothing: the lists, a test build under its own name, and the side-by-side test. L2 is approved (section 8) |
| Decisions it carries out | 13 October 2026: a real `workbench.library` for Team 4, its design approved before anything hooks. Then the decisions recorded on openamigabench #22 (section 9) |
| Builds on | amigachrome `docs/design/Design-Workbench-Replacement.md` 0.2, the replacement approved on 9 October. This design takes its section 4 further, and the decisions of 13 October replace its proxy and forwarding rules for `workbench.library`. Also amigachrome-guest `libraries/OPEN_LIBRARIES.md`; OpenBench 0.1.22 (`desktop/openbench.c`); R1's proxy (`workbench/wblib.c`, `proxy/`) |
| Marks | (measured) seen on OS 3.2.3 in lab `wblib-a` by `WorkbenchABITest`, or read from the NDK 3.2 autodoc and headers; (recorded) in a Team document; (estimate) the Team's judgement; (to record) not known yet, recorded at step L2 |

> **In short**
> - **OpenBench is `workbench.library`.** OS 3.2's library is retired. One library does all of it:
>   - it holds the lists programs register and the Workbench settings;
>   - it answers every call;
>   - it runs the desktop: the root window, drawers, menus, AppIcons, the Information window.
>
>   There is no separate desktop program to talk to, and nothing is forwarded.
> - **OS 3.2's face, then the Team's.** The 19 entries of `wb_lib.sfd` 47.1 come first, with the same registers, structures and answers. The Team's own calls follow them, and the library is version 48.
> - **`LoadWB` and `StartWorkbench` start OpenBench.** A second call while it runs does nothing.
> - **There is no Quit to Workbench.** OpenBench runs until the machine is restarted or powered off.
> - **App objects are kept before the desktop is up** and shown once it is. OS 3.2 refuses them.
> - **One voice for the Workbench settings.** The library holds them; nothing else reads `Workbench.prefs`.
> - **Shift at boot is the rescue:** OS 3.2's library and `LoadWB`, for that boot only. It is not a desktop to switch to.
> - **Measured first.** The list calls are built, and compared with OS 3.2's in a lab, side by side under the test name `openbench.library`. With OS 3.2's Workbench running, 114 cases give 4 differences, all questions for the desktop that L3 answers (section 7).

## 1. What this is

- **The Team's own `workbench.library`, which is OpenBench.** On 13 October the user retired OS 3.2's `workbench.library` in favour of OpenBench. OpenBench takes its name and offers OS 3.2's calls and the Team's extensions.
- **What changes from the replacement design of 9 October:**
  - There is no separate desktop process started by the library, and no private port between them: the desktop's work is in the library.
  - There is no proxy mode for `workbench.library` and no hand-over to OS 3.2's. R1's proxy stays only as the lab tool that records OS 3.2's private calls (L2).
  - The library is version 48, not 47.42, because the Team's calls come after OS 3.2's.
- **What doesn't change:** OS 3.2's 19 entries keep their offsets, registers and answers. So every program written for OS 3.2's `workbench.library` works unchanged.
- **It isn't `openworkbench.library`.** amigachrome's `Design-OpenWorkbench-Library.md` is the theme, rules and patch library the Open tools share. This design lives here, beside the code, with a pointer from amigachrome's `docs/design/` (decision 10).
- **"Hooks" means**, for this design, any of these:
  - being installed as `LIBS:workbench.library`, in a lab or on a machine;
  - registering with Intuition;
  - any `SetFunction`;
  - a public port;
  - a process of the library's own.

  L1 does none of them.

## 2. The face: OS 3.2's 19 entries

### 2.1 The entries

From `wb_lib.sfd` 47.1 (measured). Bias 30. "OS 3.2, no Workbench" is what OS 3.2's library answers while its Workbench isn't running (measured in lab `wblib-a`).

| Offset | Entry | Registers | The Team's library | OS 3.2, no Workbench | L1 |
| --- | --- | --- | --- | --- | --- |
| -30 | `UpdateWorkbench` | A0 name, A1 lock, D0 action | re-reads that object if its drawer is open (section 3.3) | nothing happens | does nothing |
| -36 | `QuoteWorkbench` (private) | D0 | as OS 3.2's (to record, L2) | to record | 0 |
| -42 | `StartWorkbench` (private) | D0 flags, D1 pointer | starts OpenBench; a second call while it runs does nothing (section 4) | starts OS 3.2's Workbench | 0 |
| -48 | `AddAppWindowA` | D0 id, D1 userdata, A0 window, A1 port, A2 tags | the list; kept before the desktop is up | NULL | built |
| -54 | `RemoveAppWindow` | A0 | the list | FALSE | built |
| -60 | `AddAppIconA` | D0, D1, A0 text, A1 port, A2 lock, A3 DiskObject, A4 tags | the list; drawn on the root window | NULL | built |
| -66 | `RemoveAppIcon` | A0 | the list | FALSE | built |
| -72 | `AddAppMenuItemA` | D0, D1, A0 text, A1 port, A2 tags | the list; the menus are rebuilt | NULL | built |
| -78 | `RemoveAppMenuItem` | A0 | the list | FALSE | built |
| -84 | `WBConfig` (private) | D0 tag, D1 value | kept and applied (section 5) | to record | 0 |
| -90 | `WBInfo` | A0 lock, A1 name, A2 screen | the library's own Information window (section 3.4) | to record | 0 |
| -96 | `OpenWorkbenchObjectA` | A0 name, A1 tags | opens it | to record | 0 |
| -102 | `CloseWorkbenchObjectA` | A0 name, A1 tags | closes it | to record | 0 |
| -108 | `WorkbenchControlA` | A0 name, A1 tags | the settings and the questions (2.5) | settings work; questions in 2.5 | the settings |
| -114 | `AddAppWindowDropZoneA` | A0 AppWindow, D0, D1, A1 tags | the list | (needs an AppWindow) | built |
| -120 | `RemoveAppWindowDropZone` | A0 AppWindow, A1 zone | the list | (needs an AppWindow) | built |
| -126 | `ChangeWorkbenchSelectionA` | A0 name, A1 hook, A2 tags | changes the selection | to record | 0 |
| -132 | `MakeWorkbenchObjectVisibleA` | A0 name, A1 tags | scrolls it into view | to record | 0 |
| -138 | `WhichWorkbenchObjectA` | A0 window, D0 x, D1 y, A1 tags | answers from its own windows | to record | 0 |

**App objects before the desktop is up** (decision 3):

- OS 3.2 refuses them while its Workbench isn't running (measured: NULL from every Add call).
- The Team's library keeps them, and shows them once the desktop is up. A program that adds its AppIcon early no longer loses it. The desktop starts in the same boot, right after IPrefs, so the wait is short.
- With no Workbench running, the Team's library answers just as OS 3.2's does once its Workbench runs (measured, section 7). `WorkbenchABITest` therefore differs from OS 3.2 there, on purpose.

### 2.2 AppWindows and drop zones (measured)

- `AddAppWindowA` takes a NULL window and a NULL port, as OS 3.2's does. Nothing is ever sent to a NULL port, and a NULL window is never under the pointer.
- A tag it doesn't know is ignored.
- **A drop zone needs all four sides.** That is one of `WBDZA_Left` or `WBDZA_RelRight`, one of `WBDZA_Top` or `WBDZA_RelBottom`, one of `WBDZA_Width` or `WBDZA_RelWidth`, and one of `WBDZA_Height` or `WBDZA_RelHeight`. `WBDZA_Box` gives all four. Otherwise the call returns NULL, with IoErr 116 (`ERROR_REQUIRED_ARG_MISSING`).
- **The values themselves aren't checked.** A zone of width 0, of width -5, outside the window or larger than it is accepted.
- `RemoveAppWindowDropZone`:
  - a zone of another AppWindow: FALSE, IoErr 205 (`ERROR_OBJECT_NOT_FOUND`);
  - a NULL AppWindow or a NULL zone: FALSE, IoErr 116;
  - a zone removed: TRUE, IoErr 0;
  - a zone removed again: FALSE, IoErr 205.
- Removing an AppWindow removes its zones.
- **Where a zone is** is worked out each time against the window's size. It is clipped to the window, and empty if nothing of it is left. Where two zones overlap, the first added wins (estimate; checked with real drops at L4).

### 2.3 AppIcons (measured)

- A NULL port and a NULL label are taken. A NULL DiskObject gives NULL. A tag it doesn't know is ignored.
- `WBAPPICONA_SupportsOpen` is on unless turned off. The other `Supports` tags are off unless turned on (the autodoc).
- The label is the library's own copy.
- The DiskObject's pictures are copied too, unless `WBAPPICONA_PropagatePosition` is given. The autodoc: "Workbench will make a copy of that DiskObject's icon imagery, allowing you to free the DiskObject". The copy is made with `DupDiskObjectA` at L3. L1 keeps the program's pointer only.

### 2.4 AppMenuItems (measured)

| Rule | OS 3.2, and the Team's |
| --- | --- |
| Items in the Tools menu | 62, from every program together; ResetWB is the 63rd. On Instance-32 OpenUpMenu's two items were already there, so a program could add 60 |
| Sub items under one item (`WBAPPMENUA_GetKey`, then `WBAPPMENUA_UseKey`) | 31 |
| Items under one new menu title (`WBAPPMENUA_GetTitleKey`) | 63 |
| New menu titles | 27 |
| Keys | never 0, and a title's key differs from an item's |
| `WBAPPMENUA_UseKey` with a key nobody gave | NULL |
| A label of only `-`, `_` or `~` | a separator (V44), added like any item |
| A command key already in use, or an empty one | the item is added, without the key (the autodoc) |
| Removing a parent before its sub items | TRUE; the sub item can still be removed afterwards |
| Removing an item | TRUE for any item that isn't NULL, even one already removed; FALSE for NULL |
| A NULL port, a NULL label | taken (a NULL label is an empty one) |

A parent shows only once it has a sub item, and a title only once it has an item (the autodoc; `wbc_menu_visible`).

### 2.5 WorkbenchControlA

| Tags | The Team's library | OS 3.2, no Workbench | OS 3.2, running | L1 |
| --- | --- | --- | --- | --- |
| `Get/SetDefaultStackSize` | the setting | works; 4096 | works | built |
| `Get/SetTypeRestartTime` | the setting | works; 3 seconds | works | built |
| `Get/SetGlobalFlags` (V47) | the setting | works; 0 | works | built |
| `Add/RemoveHiddenDeviceName`, `Get/FreeHiddenDeviceList` | the setting. Names match without case; removing a name that isn't there is TRUE | works | works | built |
| `Get/SetCopyHook`, `DeleteHook`, `TextInputHook`, `DiskInfoHook` | kept, and called where OS 3.2 calls them | works; all NULL | the same | built |
| `Add/RemSetupCleanupHook` (V45) | a list, called at start and at shutdown; removing one twice is FALSE | works | works | built |
| `IsOpen` | the desktop's answer | TRUE, with FALSE in the answer | TRUE, with the answer | as with no Workbench |
| `GetProgramList`, `GetSelectedIconList`, `GetOpenDrawerList`, `DuplicateSearchPath`, and their `Free` tags | the desktop's answer | FALSE | TRUE | FALSE until L3 |
| `RedrawAppIcon` | redraws it | to record | to record | FALSE until L3 |
| A tag nobody knows; no tags; a NULL tag list | TRUE | TRUE | TRUE | built |

The reserved tags (`WBA_Reserved1` to `79`) are OS 3.2's private ones. Which programs use which, WBCtrl's label options among them, is recorded at L2, and each is answered then.

### 2.6 AppMessages

The layout of `workbench/workbench.h`. `wb_core.h` writes it, and `openbench.c` checks every offset against the NDK when it is compiled (measured: 86 bytes with the 68k's alignment).

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

The reply port is the library's own. A program may reply late, and its message still comes back to a port that exists; the locks and the message are freed then. A program that never replies keeps its locks, as on OS 3.2.

### 2.7 The private calls, and who uses them

| Who | What | Known |
| --- | --- | --- |
| `C:LoadWB` | `StartWorkbench(flags, pointer)` | that it does; what the values mean: to record |
| IPrefs | `WBConfig(tag, value)` at boot, before the desktop is up, and on each prefs change; `WorkbenchControlA` for the Workbench prefs | that it does (the replacement design 1.1); which tags and values: to record |
| WBCtrl | `WorkbenchControlA` with reserved tags (label shadow and outline) | to record |
| icon.library | `UpdateWorkbench` when an icon is written with `ICONPUTA_NotifyWorkbench` | the autodoc |
| anyone | `QuoteWorkbench(n)` | the SFD says it returns a string; nothing else is known: to record |
| OpenUpMenu (the Team's) | `AddAppMenuItemA` for its OpenUp and Preferences sub-menus | measured: its two items were in OS 3.2's Tools menu |

Step L2 records these with `wbspy` (R1's spy) in a lab copy where R1's proxy is installed, never on an instance (decision 8).

## 3. One library that is the desktop

### 3.1 What the library holds and does

| Part | What |
| --- | --- |
| The lists | AppWindows and their zones, AppIcons, AppMenuItems (`wb_core`), under one `SignalSemaphore` |
| The settings | the Workbench settings, from IPrefs, WBCtrl and programs (section 5) |
| The desktop | OpenBench's work as it is today: the root window, the volumes, drawers in OpenDrawer's windows, menus, starting programs, the Information window, the bar, thumbnails, the built-in prefs and Tata. Its code moves from `desktop/` into the library |
| Its processes | the desktop's process (named `Workbench`, as programs expect), its helpers (pictures, thumbnails, starting programs), and the reply port for AppMessages |
| Intuition | `AlohaWorkbench` and the `WORKBENCH` ARexx port, as OpenBench 0.1.22 does today |

### 3.2 Inside the library

- **A program's call never waits on drawing.** Add and Remove calls change a list under the semaphore and return. The desktop's process is told with a signal and redraws when it can. OpenBench's own work already follows this rule: nothing waits on a disk.
- **Some calls need an answer from the desktop:** `OpenWorkbenchObjectA`, `ChangeWorkbenchSelectionA`, `MakeWorkbenchObjectVisibleA`, `WhichWorkbenchObjectA` and `WBInfo`. They can come from any task, so each is a message from the caller's task to the desktop's process. The desktop's process does the work and replies, and the caller waits for the reply.
- **The desktop's process never calls into the library in a way that waits on itself.**
- **The semaphore is never held while drawing or while waiting on DOS.** The desktop copies what it draws and lets go.

### 3.3 UpdateWorkbench

The caller may free the name and unlock the parent as soon as the call returns; the autodoc's own example does. So the library keeps a copy of the name and a `DupLock` of the parent in a pending set. The set holds 64 at most (estimate); beyond that, it becomes "re-read every open drawer". The desktop re-reads each object if its drawer is open.

### 3.4 WBInfo

This is the library's own Information window, which is OpenBench's `ob_info.c` today.

- The autodoc says `WBInfo` "may be called many times by different tasks", and that it is "the LVO that Workbench calls" for its own Information window.
- So the desktop's Icons > Information goes through the library's vector too.
- A program that replaces `WBInfo` with `SetFunction`, as the autodoc invites, then replaces the desktop's as well.
- Each window opens in a helper process, so several can be open at once and none blocks the desktop.

### 3.5 The Team's calls, after OS 3.2's 19

Version 48 adds the Team's calls from offset -144 on (decision 6). They are listed as each is designed. The first candidates come from OpenBench's work today:

| Candidate | What |
| --- | --- |
| A screen-mode follow call | what OpenBench's Resize to Window does today (0.1.22), for a program to ask for |
| The benches | the several benches of the replacement design's section 5.12 |
| The bar | the Workbench bar's see-through and hiding options (its section 5.10) |

Each new call gets its own row after `wb_lib.sfd`'s, an autodoc, and a test. A program that needs one opens version 48.

### 3.6 More than one core

The library is "anchored", in `OPEN_LIBRARIES.md`'s words: its lists and its desktop belong to the 68k's world. Its state is behind one `SignalSemaphore`, never `Forbid()`. So it stays correct when callers run on other cores and their calls are carried to the anchor core.

## 4. Starting, running and the rescue

- **`LoadWB` calls `StartWorkbench`, which starts OpenBench** (the desktop's process). A second call while it runs does nothing, as on OS 3.2. The Startup-Sequence may run `LoadWB`, or OpenBench as it does today; both end in the same desktop.
- **There is no Quit to Workbench** and no other desktop to change to. OpenBench runs until the machine is restarted or powered off. A hardware power-off comes later.
- **App objects added before the desktop is up are kept** (section 2.1).
- **Shift at boot is the rescue.** OpenUp keeps OS 3.2's `workbench.library` in `SYS:Storage/OpenUp-Superseded/`. With Shift held at boot, that boot uses OS 3.2's library and `LoadWB`, as a machine without the Team's would. The next boot is normal again. It is a rescue, not a desktop to switch to.
- **A machine without RTG.** OpenBench today runs `LoadWB` on a screen that isn't a true-colour RTG one. With OS 3.2's library retired, OpenBench draws on palette screens instead, using the AGA fallback it has had since 0.1. This is checked in an AGA lab at L4.
- **What it needs:** OS 3.2 (V47) and a 68020 or better. On anything older the library refuses to load.

## 5. One voice for the Workbench settings

- **The library holds the Workbench settings.** IPrefs reads `ENV:Sys/Workbench.prefs` and calls the library (`WBConfig`, `WorkbenchControlA`), as it called OS 3.2's. WBCtrl and programs call `WorkbenchControlA`.
- **Nothing else reads `Workbench.prefs`** (decision 5). OpenBench reads it itself today (0.1.19); that work moves into the library's handling of IPrefs' calls.
- **OpenBench's own settings** (the look, the bar, the benches) stay in `ENV:OpenBench/` and the Open preferences. None of Workbench's settings are copied there.
- **Before the desktop is up,** IPrefs' calls are kept, and applied in the order they came when the desktop starts.

## 6. Safety and versions

- **Nothing hooks before its step.** L1 is a library under its own name, `openbench.library`, and a test program. Neither is installed as `workbench.library` anywhere, neither registers with anything, and nothing patches anything.
- **Version 48** (decisions 6 and 7), with the Team's name in the ID string. A program that asks for 47 or less gets OS 3.2's 19 entries, unchanged. A program that asks for 48 gets the Team's calls too.
- **A handle is looked up, never followed.** Every handle a program gives back is searched for in the library's lists before it is used. A stale or made-up handle is answered "not found", where OS 3.2's can hang (section 7.2).
- **The library refuses to be expunged** while a program holds one of its objects or an AppMessage is out.
- **Shift at boot** gives OS 3.2's library and `LoadWB` for that boot (section 4).

## 7. Testing

### 7.1 On the host

`tests/host/test_wb_core.c` runs from `tests/host/run.sh`, with the address and undefined-behaviour checkers: 128 checks. It covers:

- the lists, zones, menus and limits;
- the settings and the notes;
- running out of memory at each step;
- keeping objects before the desktop is up, with OS 3.2's refusal as an option;
- the AppMessage's bytes.

Every allocation is counted.

### 7.2 Side by side with OS 3.2's

`tests/amiga/WorkbenchABITest` opens the system's `workbench.library` and `openbench.library` side by side. It makes the same calls through each and lists each difference. Nothing is replaced. It ran in lab `wblib-a`, a copy of Instance-32: OpenBench as the desktop, OS 3.2.3's `workbench.library` 47.42, and `openbench.library` 48.1 copied into the lab's `LIBS:`.

| Run (`ALL RISKY`) | Checks | Differ | The differences |
| --- | --- | --- | --- |
| OS 3.2's Workbench running (`LoadWB` in the lab) | 114 | 4 | `GetProgramList`, `GetSelectedIconList`, `GetOpenDrawerList` and `DuplicateSearchPath`: questions for the desktop, which L1 doesn't have |
| No OS 3.2 Workbench running | 114 | 78 | by design (decision 3). OS 3.2's refuses every Add call, so nothing after them runs; the Team's keeps the objects and gives the answers OS 3.2's gives with its Workbench running |

Before the decisions, `openwb.library` (47.42, refusing as OS 3.2 does) gave 60 checks and 0 differences with no Workbench running. Earlier rounds found and fixed eleven differences in the library from OS 3.2's answers:

- the NULL window and NULL port rules;
- the four sides a zone needs, and IoErr 116;
- `RemoveHiddenDeviceName` on a missing name;
- an unknown `WorkbenchControlA` tag;
- `IsOpen` with no desktop;
- the type restart time's 3 seconds;
- a NULL menu label;
- `RemoveAppMenuItem`'s answer.

**The RISKY cases** run only when asked:

- Removing an AppWindow a second time, or an AppIcon a second time, **hangs OS 3.2's library**. This was measured twice: the lab stopped, and Tata later recorded "a jump to an odd address". The test asks those of the Team's library only, which answers FALSE.
- Removing an AppMenuItem a second time: TRUE from both.
- Removing a drop zone a second time: FALSE, IoErr 205, from both.
- A drop zone on a NULL AppWindow: NULL from both.
- An AppIcon with a NULL label: taken by both. With a NULL DiskObject: NULL from both.
- An AppMenuItem with a NULL label: taken by both (an empty label in the Team's).

### 7.3 Later

- L2's `wbspy` log is the contract for the private calls.
- From L3: `WorkbenchABITest` against the library installed as `workbench.library` in a lab, with real drops on AppWindows, AppIcons and zones through Cradle's input, and screenshots.

## 8. The order of work

| Step | What | Hooks? | Done when |
| --- | --- | --- | --- |
| **L1 (this PR)** | `wb_core` (the lists, zones, menus, settings, notes, AppMessage layout); `openbench.library` under its test name; `WorkbenchABITest` | **No** | host tests pass; `WorkbenchABITest` differs only where the design says (the desktop's questions, and objects kept before the desktop is up) |
| L2 (approved) | Record the private calls with `wbspy`: R1 installed in a lab copy only, never an instance | In a lab only | the log of `LoadWB`, IPrefs, WBCtrl, the prefs editors and DefIcons |
| L3 | The library with the desktop in it: OpenBench's code moved into `workbench.library` 48; `StartWorkbench`; `WBConfig`; the AppMessages and their reply port; AppIcons' pictures copied | Yes, in a lab | the lab boots to OpenBench as `workbench.library`; `WorkbenchABITest` runs against it; Shift at boot gives OS 3.2's |
| L4 | AppIcons on the desktop, the Tools menu, drops on AppWindows and zones, `WBInfo`, the questions of 2.5; an AGA lab | Yes, in a lab | the App lines of the replacement design's Appendix A, with screenshots; the programs of its Appendix B that use AppWindows and AppIcons |
| L5 | One voice (section 5); the Team's first calls after entry 19 | Yes, in a lab | IPrefs' changes reach the desktop; each new call has its test |
| L6 | OpenUp: install with `retire`, Uninstall, the Shift rescue | On a machine, on the user's word | a clean install and uninstall; Shift at boot from cold |

## 9. The user's decisions (13 October 2026)

Recorded on openamigabench #22.

| # | Question | Decision |
| --- | --- | --- |
| 1 | The split between library and desktop | The desktop's work goes in the library: `workbench.library` is OpenBench |
| 2 | Who decides the mode | No forwarding mode. OS 3.2's library is retired in favour of OpenBench, called `workbench.library` |
| 3 | Add calls with no desktop | Kept until the desktop is up, not failed as OS 3.2 does |
| 4 | Changing desktop without a reboot | There is no Quit to Workbench: only restart or power off. A hardware power-off comes later |
| 5 | Settings | One voice: `workbench.library` holds the Workbench settings, and nothing else reads `Workbench.prefs` |
| 6 | Extensions | The Team's calls go after OS 3.2's 19 entries, as version 48 |
| 7 | Version | 48.x |
| 8 | L2 in a lab | Yes: R1 in a lab copy only, never an instance |
| — | Safe boot | Shift at boot gives OS 3.2's library and `LoadWB` for that boot only, from `SYS:Storage/OpenUp-Superseded/`. A rescue, not a desktop to switch to |
| 9 | Test name | `openbench.library` |
| 10 | Where the design lives | openamigabench, with a pointer from amigachrome's `docs/design/` |
| — | `LoadWB` and `StartWorkbench` | They start OpenBench; a second call while it runs does nothing |
| — | `WBInfo` | The library's own Information window, since the desktop is in the library |

## Sources

- NDK 3.2: `SFD/wb_lib.sfd` 47.1, `Include_H/workbench/workbench.h`, `Autodocs/wb.doc` (read on the Team's machine; never committed).
- amigachrome: `docs/design/Design-Workbench-Replacement.md` 0.2; `docs/design/Design-OpenWorkbench-Library.md` 0.3.
- amigachrome-guest: `libraries/OPEN_LIBRARIES.md`.
- This repository: `desktop/openbench.c` (0.1.22), `workbench/wblib.c` (R1), `proxy/`, `tools/wbspy.c`.
- Measured: `WorkbenchABITest` in lab `wblib-a`, 13 October 2026.
- Clean room: AROS's `workbench.library` was not needed for this step and was not read for it. No code from Hyperion's or AROS's.
