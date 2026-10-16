# Step L2: wbspy's logs

What OS 3.2's own programs called in `workbench.library`, recorded in lab `wbl2` on 13 October 2026. The findings are in `docs/design/L2-Workbench-Calls-Recorded.md`. These are the raw logs, as `wbspy SAVE` wrote them.

## The lab

- `wbl2`, a lab copy of Instance-32 (AmigaOS 3.2.3, Kickstart 3.2.3 47.115, the AC090 native engine). Never an instance.
- R1's `workbench.library` (this branch's build) in `LIBS:`. OS 3.2's own (47.42, 1.1.2025, 185,732 bytes) moved to `SYS:Storage/OpenUp-Superseded/workbench.library`, where R1 loads it from. `icon.library` was left as it was.
- `ENV:OpenBench/Spy-workbench.library` set to 8192, so the spy is on from the first open at boot.
- `wbspy` 0.2 and `r1swap` 0.1 in `C:`.

## A line

```
000005  2.318ms [« IPrefs] workbench WBConfig(-84) d0=… d1=… d2=… a0=… a1=… a2=… a3=… a4=… tags={tag=data, …} tagmem={i:bytes'text'>bytes'text'} -> result raw=bytes 'text'
```

- The call's number, its time since the first call in the file, and the caller (a Shell command's own name; IPrefs's process is named `« IPrefs »`, written `\253 IPrefs`).
- The entry and its offset, and the registers as the call was made.
- `tags`: its tag list, up to 8 items. `tagmem` (new in wbspy 0.2): 12 bytes behind each tag's data when it points at memory, before the call, and after it (`>`) where they changed. A value below $1000 is never read.
- `-> result`, and `raw`: 32 bytes behind `WBConfig`'s value.
- Addresses differ from boot to boot. Tags are named by `decode.py`, which reads the NDK's `workbench/workbench.h` on the stove.

## The files

| File | Boot | What was done |
| --- | --- | --- |
| `ob-00-boot-wbspy01.txt` | OpenBench's boot (today's Startup-Sequence: `SYS:System/OpenBench`, no `LoadWB`) | The boot, with the first build (wbspy 0.1, no `tagmem`) |
| `ob-01-boot.txt` | the same | The boot again, with wbspy 0.2 |
| `ob-02-wbinfo.txt` | the same | `Run >NIL: C:WBInfo SYS:` |
| `ob-03-deficons.txt` | the same | `Run >NIL: C:DefIcons` (no call) |
| `ob-04` to `ob-09` | the same | `Copy` of a `Workbench.prefs` variant to `ENV:Sys/`; IPrefs applies it (A, B, the original, C, D, the original) |
| `ob-10-prefsworkbench-open.txt` | the same | `SYS:Prefs/Classic/Workbench` opened |
| `ob-11-prefsworkbench-save.txt` | the same | "No Volume Gauge" clicked off, then Save |
| `ob-12-restore-orig.txt` | the same | The original `Workbench.prefs` copied back to `ENVARC:` and `ENV:` |
| `ob-13-wbpattern-use.txt` | the same | `WBPattern FROM ENVARC:Sys/WBPattern.prefs USE` (no call) |
| `ob-14-font-use.txt` | the same | `Font FROM ENVARC:Sys/Font.prefs USE` |
| `ob-15-tail.txt` | the same | The rest, before the reboot (OpenUpMenu retrying) |
| `lw-01-boot-after-reset.txt` | `LoadWB` at boot, R1 answering | The boot. OS 3.2's Workbench failed as it started; the lab reset and booted again; this is that second boot |
| `lw-02-boot-r1public-crash.txt` | the same, with the version fix | The boot. The Workbench failed again |
| `sw-01-boot-nodesktop.txt` | no desktop at boot | The boot, before anything was started |
| `sw-02-loadwb-run.txt`, `sw-03-tail.txt` | the same | `r1swap ORIGINAL`, `LoadWB` (before the version fix: "invalid resident library") |
| `sw-04-boot-nodesktop.txt` | no desktop at boot, the version fix | The boot |
| `sw-05-loadwb-up.txt` | the same | `r1swap ORIGINAL`, `Run >NIL: C:LoadWB`, `r1swap R1`: OS 3.2's Workbench running, R1 answering again |
| `wb-01`, `wb-02` | OS 3.2's Workbench running | `Workbench.prefs` variant A, then the original |
| `wb-03-font-use.txt`, `wb-04-wbpattern-use.txt` | the same | Font and WBPattern `USE` (WBPattern: no call) |
| `wb-05`, `wb-06` | the same | Prefs/Workbench opened; "No Volume Gauge" clicked, Save |
| `wb-07-restore-orig.txt` | the same | The original `Workbench.prefs` back |
| `wb-08-wbinfo.txt` | the same | `Run >NIL: C:WBInfo SYS:`; its window closed with Cancel |
| `wb-09-deficons.txt` | the same | `Run >NIL: C:DefIcons` (no call) |
| `wb-10-loadwb-again.txt` | the same | `C:LoadWB` a second time, R1 answering |

## The tools here

- `mkprefs.py OUT_DIR` writes the `Workbench.prefs` variants A to D. Each field has a value that can be told apart in the log.
- `decode.py LOG…` names each tag from the NDK's header, and sums up the calls by caller.

MIT, Copyright (c) 2026 AmigaChrome Team.
