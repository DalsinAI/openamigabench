# Restart: OpenBench

_Written 9 October 2026, when the repository was made. Read this first when work resumes; the live PR list wins if they disagree._

## What this repo is

Our own `workbench.library`, `icon.library` and desktop (OpenBench), replacing Hyperion's on AmigaOS 3.2. The design is amigachrome `docs/design/Design-Workbench-Replacement.md`. The shape, the repository and the name were decided on 9 October 2026.

## Where it stands

Step R1, first cut: both libraries forward every entry to Hyperion's originals, loaded privately, and wbspy records the calls. Built on the stove, not yet run in a lab.

## Next steps

1. Run R1 in a lab (README, "Trying R1 in a lab"). Record the boot with wbspy, and keep the log as the contract for `StartWorkbench`, `WBConfig` and `QuoteWorkbench`.
2. `wbprobe`: every entry's cases, run against both libraries.
3. R2: icon.library's stateless helpers taken over, then reading and writing every format.

## workbench.library (13 October 2026)

`docs/design/Design-Open-Workbench-Library.md` (0.2) is the design of the Team's own `workbench.library`. The user decided on 13 October: OS 3.2's library is retired and `workbench.library` is OpenBench itself, version 48, with the Team's calls after OS 3.2's 19; Shift at boot gives OS 3.2's library and `LoadWB` for that boot only, as a rescue. Step L1 is built and hooks nothing: `workbench/wb_core.c` (the lists and settings), `openbench.library` (under its test name) and `tests/amiga/WorkbenchABITest`. Next is L2: R1 in a lab copy, so that `wbspy` records the private calls (approved).

## Decisions still open (design §10)

4 (icon.library first, alone), 5 (classic drawer windows as an option), 6 (Delete to the trash), 7 (the desktop's canvas), 8 (Scalos), 9 (the bar's defaults).
