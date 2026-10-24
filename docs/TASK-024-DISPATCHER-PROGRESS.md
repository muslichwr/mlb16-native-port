# TASK-024 - Dispatcher progress correlation

## Summary

**Classification: DISPATCH_PROGRESS_UNRESOLVED.** TASK-023 observed one dispatcher entry without caller or return instrumentation. This task reconstructs the startup call and immediate continuation statically, then uses one bounded observer in one fresh isolated boot. That boot does not reproduce dispatcher entry: both the 35- and 170-second summaries show startup entries/returns **1/0**, caller-dispatch **0/0**, dispatcher **0/0**, and immediate continuation **0/0**. No dispatcher entry or startup return appears elsewhere in the complete observer output.

Consequently neither dispatcher non-return nor a blocked post-dispatch continuation is established. The actual caller and selected state for TASK-023's earlier invocation cannot be recovered retrospectively from its entry-only record. The static startup route is supported, but its execution is not observed in this run.

**BOOT TEST: YA**

**TASK-024: COMPLETE** (the bounded investigation is complete; dispatch progress remains unresolved).

## Evidence

Read first: `AGENTS.md`, `docs/PROJECT_STATE.md`, `docs/TASK-023-FLIP-CALLBACK-BOUNDARY.md`, and `docs/TASK-020-DXTJOB-BOOT-EXPECTATION.md`. The historical TASK-020 original-HDD boot is a different validation state and is not used to diagnose this isolated boot.

Evidence root: `F:/Games/MLB16PS3/build-mlb16-native-pinned/task024/`. Generated PPU code and the local ELF were inspected read-only and remain outside Git. References below use `F:/Games/MLB16PS3/recompiled-bounded/ppu_recomp_000.cpp`.

| Evidence | Result |
| --- | --- |
| Saved TASK-023 log | Dispatcher entry at line 35000; no caller/return fields. Its 35-/170-second summaries have dispatcher entries 1, startup returns 0. |
| Read-only ELF lookup | Manager vtable `0102EC98`, slot `+18`, OPD `010A1E98`: code `000136F8`, TOC `010D5780`. Saved as `static-dispatch.json`. |
| Canonical build | `build-results.json`: configure, observer compilation/link, standalone isolation-resolver compilation all exit 0. No generated PPU object rebuilt. |
| Linked coverage | `function-table-coverage.json`: startup, dispatcher and continuation pointers resolve to wrappers. Local linked disassembly verifies startup's indirect edge, direct continuation wrapper and all five dispatch-local virtual call sites. |
| Isolation gates | `isolation-preflight.json` and `launch-isolation-gate.json`: PASS. |
| Complete observer output | `observations.json`, four lines with original log line numbers; 35-/170-second summaries agree. |
| Termination / preservation | `boot-result.json`, `preservation.json`, full original before/after manifests, `mirror-unmounted.json`. |
| Restored configuration | `restore-results.json`, `final-verification.json`: normal build/link exit 0, all eight task options default/cache OFF. |

## Static dispatcher map

| Boundary | Static evidence / behavior |
| --- | --- |
| Containing caller | `00011CE8`, starting at source line 2237. Near line 3850 it loads the object's vtable slot `+18`, loads its OPD code/TOC, and calls indirectly with return link **`0001353C`**. The inspected manager vtable resolves that slot to `000136F8`. |
| Dispatcher entry | `000136F8`, line 3991. Entry r3 becomes the object base retained in r30. Its frame is `0xA0` bytes; saved caller LR is at new SP `+B0`. |
| Initial admission | Read `[object+24]`. Null jumps to normal exit `00013850`; non-null enters `00013734`. |
| Selected object's first call | Vtable `+08`, return link `00013750`. |
| Selected object's next call | Vtable `+0C`, return link `00013778`, r4 is initially zero and later the previous iteration's `+14` result. Its r3 return is retained as a search comparison value. |
| Selection / transition | Selected object's `+14` call returns at `000137A0`. Search the pointer array at manager `+20`, bounded by unsigned count `+18`; each candidate's `+14` result (return link `000137E8`) is compared with the retained value. First equal object is selected, or zero on no match. |
| Current object's final call | Vtable `+10`, return link `0001383C`. Then write the selected pointer to manager `+24`. A non-null pointer loops to `00013734`; zero reaches the normal exit. |
| Normal return | Exit `00013850` restores LR, preserved registers and stack, then returns. There is a single normal C++ return and no tail-trampoline exit in this function. Entry alone cannot show whether it remains in a selected method or keeps transitioning objects. |
| Immediate continuation | After return at `0001353C`, startup restores TOC and directly calls **`00014EB4`**, return link **`00013544`** (line 3860). It then calls `00014F08` at `0001354C`, followed by additional calls before startup's epilogue. Those later descendants were not traced. |

`00014EB4` (line 5697) has a null check on a global pointer and a conditional helper call, then a normal return. This task observes only its entry/return as the first immediate continuation; it does not investigate that helper.

Startup calls the dispatcher **inside** `00011CE8`, before startup returns. Therefore dispatcher entry with no startup return is structurally possible and does not itself identify a stall.

## Entry / return evidence

| Selected boundary | 35 seconds, log line 81254 | 170 seconds, log line 81414 |
| --- | ---: | ---: |
| Startup entries / returns | 1 / 0 | 1 / 0 |
| Startup's dispatcher-call entries / returns | 0 / 0 | 0 / 0 |
| Dispatcher entries / returns | 0 / 0 | 0 / 0 |
| Immediate `00014EB4` entries / returns after dispatch | 0 / 0 | 0 / 0 |
| Dispatch-local state-call entries / returns | 0 / 0 | 0 / 0 |

The other two observer lines are ARM at line 1 and `STARTUP_ENTER` at line 26: thread **1**, caller LR **`00011CB8`**, startup object **`16023800`**. This agrees with the previously reconstructed startup call. The complete observer output contains no later dispatch entry, return, continuation or startup-return event; their first occurrences would be logged independently of the summaries.

This is not a dispatcher non-return observation: there is no entry to pair with a return in this run. TASK-023's entry count cannot be combined with TASK-024's return count across separate processes.

## Selected state / object

Static object fields supported by the code are:

| Offset | Established use |
| --- | --- |
| `+00` | Vtable pointer. |
| `+18` | Unsigned number of entries searched. |
| `+20` | Array base, four-byte object pointers. |
| `+24` | Current selected object pointer; replaced after a completed iteration. |

The startup object is observed as `16023800`. **No dispatch-selected object, state identifier or dispatch-time vtable is observed in TASK-024.** The observer deliberately reads those fields only upon dispatcher entry and in subsequent snapshots after an entry. Its zero state-call counters do not imply a null current state or an empty object array. No high-level subsystem name is assigned to these objects.

## Post-dispatch continuation

The static immediate continuation is `00014EB4` at `00013544`. Its wrapper is restricted to the same startup host-thread context after a completed dispatch. Neither completed dispatch nor that continuation is observed. There is therefore **no first downstream boundary after a completed dispatch established by runtime evidence** in this task, and no evidence that this continuation is blocked.

## Diagnostic scope and build verification

One project observer, `src/dispatcher_progress_diagnostic.cpp`, is enabled only with `MLB16_TASK024_DIAGNOSTIC=ON` and `MLB16_TASK024_TRACE=1`. It wraps startup, dispatcher, the immediate continuation and the existing indirect-call boundary. Indirect observations are limited to startup's `1353C -> 136F8` call and the dispatcher's five own call sites, checked against its context and exact stack frame. Selected methods' descendants are forwarded without tracing. Only the first eight state edges and first two outer entries/returns can print, plus two counter snapshots. This run printed four observer lines.

Every original function executes once with its original arguments and result. No guest memory, registers, state, callback, return or completion flag is changed. No GCM/RSX/runtime semantics or timing controls are changed; observation has ordinary overhead. The monitor sleeps only on its own observer thread. Earlier TASK-018/019/020/021B/022/022R/023 diagnostics remain OFF throughout the boot. No generic toolkit source was modified, so generic-runtime synthetic regressions are not applicable.

The canonical target compiled only the new observer and linked against the existing PPU objects. After boot, a negative configure check correctly rejected simultaneous TASK-023/TASK-024 enablement. All eight options were then set OFF, canonical configure/link passed again, and the normal target excludes all diagnostic sources and wrappers. PM image 77's canonical source/startup integration remains present; no legacy toolkit or `tmp/task*` source inputs are used.

## Isolation / termination / original-data audit

Fresh root: `F:/Games/MLB16PS3/build-mlb16-native-pinned/task024`. `tools/run_task024_boot.py` selects this root and trace variable and reuses the unchanged TASK-022R/TASK-023 isolation lifecycle.

- HDD0: `task024/hdd0`; four independently copied installed WAD/header seeds from intact RPCS3 data, 10,158,864,840 bytes total.
- HDD1/cache: `task024/hdd1/cache`, including title-specific cache children.
- Disc VFS: `task024/vfs`, with real experiment directories and 40 file-only links to protected originals. No original directory is aliased.
- Temp, profile, shader-cache and dev-flash roots are under the experiment root. Native resolver checks and writable canaries pass. Attempts to open the original disc WAD or its mirror link for writing are denied with Windows error 32 while original-file handles are held.

Exactly **one boot** ran, with the canonical executable and a fresh one-boot guard. **Termination: FAIL_SAFE_TIMEOUT**, after **180.656 seconds**, external-kill exit code 1. This is not a natural game exit or evidence of a crash. Timeout is not classified as a game failure.

**Original-data preservation: PASS.** Full before/after manifests match exactly for **505 entries / 435 files / 34,330,559,625 hashed bytes**, including sizes, mtimes, attributes, file identities and link counts. This covers the original game/native HDD and RPCS3 HDD trees, retaining historical TASK-022 additions. All 40 experiment disc-file links are disarmed before original locks are released. No original WAD or other content is copied over, repaired, truncated, deleted or modified.

Experiment inventory records **29 additions / 6 metadata changes / 0 removals** between its snapshots, including audit/log artifacts, HDD/cache directories, a SYS.MD5 file and profile/shader-cache files. These writes remain inside the disposable root and are expected. The 40 links are separately removed afterward as recorded in `mirror-unmounted.json`; original targets remain unchanged.

## Classification

**DISPATCH_PROGRESS_UNRESOLVED**

The previous entry-only invocation cannot establish return or caller identity; the sole new run does not reproduce its entry. No evidence uniquely supports a non-returning dispatcher, a blocked continuation, or continued progress after completed dispatch. No semantic fix is justified.

## What is not proven / uncertainties

- The actual caller, selected object or eventual return of TASK-023's earlier dispatcher invocation. The startup virtual call is a static route, not retrospectively observed caller identity.
- Why the two isolated runs reach different startup progress. Non-reproduction does not prove the observer caused it, a permanent deadlock, or a defective dispatcher.
- Whether a selected state method would return or cycle, or whether dispatcher return should occur before shutdown.
- Which earlier startup branch/wait occupies TASK-024. The observer intentionally does not sample arbitrary descendants or reopen flip internals.
- Rendering or gameplay success. The four-file sandbox seed is not a complete RPCS3 installation equivalence test.
- A DXT/SPURS, scheduler, DMA, GCM or RSX defect. None was investigated or changed.

## Files changed

TASK-024 additions to the existing uncommitted investigation delta:

- `cmake/mlb16-pm-integration.cmake`: adds only the TASK-024 default-OFF, mutually exclusive integration block; earlier task blocks retained.
- `src/dispatcher_progress_diagnostic.cpp`: bounded read-only forwarding observer.
- `tools/run_task024_boot.py`: fresh root selection using the existing isolation/audit helper.
- `docs/TASK-024-DISPATCHER-PROGRESS.md`: this report.
- `docs/superpowers/plans/2026-10-24-task024-dispatcher.md`: scoped execution/check plan.

The shared isolation helper and prior diagnostics/reports are retained. Game-derived code, executable, copied seeds, bulk logs and linked disassembly remain outside Git. HEAD remains `2ebaa0a2cf98e02964fabf3a966e20f586fcde8b`; toolkit pin remains clean at `bc5b3d30503169a025c88bd9e6195728fc8ad68d`, with no pointer change. All 45 previously protected build inputs match. No staging, commit or push; staging remains empty.

## Next smallest step

Reassess startup progress using the **saved TASK-023 and TASK-024 ordinary logs**, looking for the last shared completed startup boundary before `0001353C`. Identify one upstream boundary that separates those runs before authorizing another probe. Do not select a GCM/RSX subsystem or an arbitrary state-method descendant from the absent dispatch observation. No second boot is part of TASK-024.

## How to verify

Inspect `task024/static-dispatch.json`, linked coverage/disassembly, `build-results.json`, `build-gate.json`, both isolation gates, four-line `observations.json`, `boot-result.json`, `preservation.json`, complete original manifests, `mirror-unmounted.json`, `experiment-changes.json`, `restore-results.json` and `final-verification.json`. Recheck canonical cache options OFF, normal build inputs excluding probes, toolkit clean/pinned and index empty. These saved results require no additional boot. Exact configure/build/compile argument lists and exit codes are retained in the build-result JSON files. The fresh preparation/boot commands were PowerShell `python tools/run_task024_boot.py --prepare` and `python tools/run_task024_boot.py --boot`; preserve their one-boot guard.
