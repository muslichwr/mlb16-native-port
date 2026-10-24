# TASK-023 - Flip / synchronous callback boundary

## Summary

Static analysis alone cannot distinguish an unreturned prepare-flip import from an unreturned synchronous callback or later guest work. TASK-022R has a sampled last return link and entry logging, but no paired return evidence. TASK-023 therefore adds one bounded, default-OFF project observer and uses one fresh TASK-022R-style isolated boot.

**Classification: FLIP_BOUNDARY_UNRESOLVED.** All 50 selected import calls, synchronous callbacks and containing `00066530` calls have returned at both 35 and 170 seconds. No selected flip call is outstanding at either summary. Startup return is still unobserved, while dispatcher entry is observed once. This run does not reproduce a pending selected flip import/callback, and the permitted observations do not uniquely prove a downstream guest stall either. The exact point where expected boot progress stops therefore remains unresolved.

The completed selected sequences are positive evidence; absence of startup return alone is not proof of a particular later stall, especially with dispatcher entry reached. The earlier TASK-022R sampled return link is not retrospective proof of a non-returning import. Gameplay or rendering success is not established.

**BOOT TEST: YA**

**TASK-023: COMPLETE**

## Static boundary map

All generated-code references below are to local files in `F:/Games/MLB16PS3/recompiled-bounded/`; they were not edited or copied into the repository.

| Layer | Proven boundary / behavior |
| --- | --- |
| Containing startup transition | `00011CE8` calls `000117F4` at return link `00013330`; the observed chain continues through `00011614`, `000114B4`, `00062C1C` to `00066530` (return link `00062C64`). |
| Selected import | In `00066530`, return link **`00066BE0`** calls **`00F25BCC`** (`ppu_recomp_000.cpp:98934`). Arguments include the GCM context and buffer index. |
| Import identity | `ppu_recomp_028.cpp:130663` dispatches NID **`0B4B62D5`**; actual generated registration `ppu_hle_nids.cpp:1454` names **`cellGcmSetPrepareFlip`**. Nearby SDK NID comments are not used to identify it. |
| Native import body | `third_party/ps3recomp/libs/video/cellGcmSys.c:2832-2858`: validates the buffer, records the selected display buffer, updates flip bookkeeping, optionally invokes a guest callback synchronously, and returns. |
| Synchronous callback predicate | `s_flip_handler_opd && g_ps3_guest_caller && !gcm_flipcb_on_tick_only()`. `GCM_FLIPCB_ONTICK` is not enabled or changed by this experiment. |
| Callback bridge | `mlb16-native/main.cpp:319-324`: `harness_guest_caller` forwards its OPD and eight arguments to `ppu_guest_call`. The prepare-flip call supplies zero callback arguments, including arg0=0. |
| Registered callback | TASK-022R log line 137 records OPD **`010A2D18`**. A read-only file-backed ELF LOAD lookup maps this OPD to code **`000691B0`**, TOC **`010D5780`** (`static-callback.json`). The local lifted callback begins at object 000 line 101578. |
| Callback dispatch return | `runtime/ppu/ppu_loader.cpp:3699+`: resolves the OPD, calls the registered guest function on its callback context, drains its trampoline, restores the calling active context, and returns r3. |

The callback's guest body contains its own branches, an atomic update loop and calls to guest helpers. Their existence does not establish which operation, if any, fails to complete. This task observes only the callback boundary and does not trace or change those internals.

`00066530` has a single normal C++ return and no tail-trampoline exit of its own, so the observer's normal return corresponds to completion of that containing guest invocation. The import wrapper returns only after the unchanged HLE dispatch returns. No callback pointer is replaced.

## Diagnostic scope and build gate

`MLB16_TASK023_DIAGNOSTIC` defaults OFF, is mutually exclusive with the earlier task options, and additionally requires `MLB16_TASK023_TRACE=1`. All TASK-018/019/020/021B/022/022R options remain OFF for this run.

The project observer wraps four guest functions (`00066530`, `00F25BCC`, `00011CE8`, `000136F8`) and the existing native `ppu_guest_call` bridge. It selects only the outer main-thread `00066530` path and its import call at `00066BE0`. Host-thread-local correlation observes only a synchronous callback inside that selected import. Background ticker callbacks and recursive callback internals are forwarded without observation.

Every original function is forwarded exactly once with its original arguments and result. The probe writes no guest memory, registers, callbacks, returns or completion flags. It sets no GCM/RSX/timing controls and adds no sleep to game threads. Its independent observer thread samples counters at 35 and 170 seconds; normal diagnostic overhead is not a guarantee of identical execution timing.

Only the first three sequences are printed as individual events. A 16-slot atomic recent-sequence buffer records subsequent import/callback phase and return evidence; the summaries print the last four records. This bounds diagnostic output while retaining the last completed and outstanding sequence. Atomic fields are sampled asynchronously; stability between the two summaries is stronger evidence than a transient counter difference.

Before boot, canonical build/link passed, all four guest function-table pointers were verified, the original containing function's direct import calls were verified to reach the wrapper, and linked `harness_guest_caller` disassembly was verified to reach `__wrap_ppu_guest_call`. No generated PPU object was rebuilt. No generic runtime source was changed, so generic-runtime synthetic regressions are not applicable.

## Runtime entry / return evidence

Local evidence root: `F:/Games/MLB16PS3/build-mlb16-native-pinned/task023/`. The complete normal-runtime log is `boot.log`; the **31** bounded observer lines, with original line numbers, are saved separately in `observations.json`. Bulk runtime output remains outside Git.

| Boundary | 35-second summary | 170-second summary |
| --- | ---: | ---: |
| `00066530` entries / returns | 50 / 50 | 50 / 50 |
| Selected prepare-flip import entries / returns | 50 / 50 | 50 / 50 |
| Synchronous callback entries / returns | 50 / 50 | 50 / 50 |
| Last containing-path return ID | 50 | 50 |
| Last completed flip sequence | 50 | 50 |
| Startup entries / returns | 1 / 0 | 1 / 0 |
| Dispatcher entries | 1 | 1 |

The summaries occur at log lines **141091** and **1281247**. The first three sequences show individual `PATH_ENTER -> IMPORT_ENTER -> CALLBACK_ENTER -> CALLBACK_RETURN -> IMPORT_RETURN -> PATH_RETURN` ordering. All three import results are 0. The first two containing calls are graphics setup callers `00068BE4` / `00068C10`; path 3 records caller return link **`00062C64`**, matching the TASK-022R startup transition chain. Scope is the selected main-thread call site, not every GCM operation in the process.

Dispatcher entry is logged at line **35000**. There is no `STARTUP_RETURN` event anywhere in the complete run; its first occurrence would be logged even after the summaries. This does not establish dispatcher return/non-return, which was not instrumented.

## Callback correlation

Every selected import has exactly one observed synchronous callback entry and return. The first three callback events carry **OPD `010A2D18`, code `000691B0`, arg0=0**, agreeing with the registered flip handler and the prepare-flip body's synchronous invocation. The recent records retain the same OPD/code and balanced callback counts.

Callbacks are correlated through host-thread-local selected-import state and the existing `harness_guest_caller -> ppu_guest_call` forwarder. Asynchronous ticker calls run without that state and are not counted. No callback pointer, callback argument, return value or dispatch policy was changed. A `CALLBACK_RETURN` means the original `ppu_guest_call` completed its dispatch/trampoline and returned; it is not inferred merely from a downstream frame counter.

## Last completed sequence

**Last fully captured completed sequence: 50**, containing path **50**, display buffer **0**. Both summaries report `last_flip_return=50`, `last_path_return=50`. Its recent record has phase **4 (`IMPORT_RETURN`)**, callback entries/returns **1/1**, OPD `010A2D18`, code `000691B0`, and import result **0**. Paths 47-50 and their imports/callbacks all have completed records; buffers are 1, 0, 1, 0 respectively.

The same records persist at 170 seconds (log lines **1281255-1281258**). Thus no selected import or synchronous callback remains outstanding across the 135-second interval between summaries. The ring is not a continuous printed trace: sequence 50 is the last completion established by the final snapshot, not a claim that every possible event during the final ten seconds was recorded individually.

## Classification

**FLIP_BOUNDARY_UNRESOLVED**

The paired return evidence rules out a pending selected import or selected synchronous callback at the two observation points. The containing guest calls return as well. Startup return is not observed, and dispatcher entry is positive. A claim that the guest specifically stalls afterward would require separate downstream progress/return evidence that this scoped probe does not supply. This leaves the broader startup-progress question beyond the completed selected sequences without inventing a downstream fault or claiming that all guest threads stopped.

## Isolation / termination / original-data audit

Fresh experiment root: `F:/Games/MLB16PS3/build-mlb16-native-pinned/task023`. It has independent HDD0/HDD1/cache/temp/profile directories and four independent installed WAD/header seed copies, matching TASK-022R. Its disc mirror has 40 file-only links with real experiment directories; existing original files are locked against writes/deletion during preparation, boot and the full post-boot audit. The same native resolver and isolated write-canary gate passes before launch. No original directory is aliased; neither the TASK-022 nor TASK-022R original artifacts are cleaned.

The harness uses a fresh one-boot guard, a 180-second fail-safe and separate termination classification. It audits complete original-tree hashes, sizes, mtimes, attributes, identities and link counts. Experiment writes are expected and recorded separately; disc-file links are disarmed before releasing the original-file locks. No runtime writes are allowed in original game/RPCS3 trees.

**Termination: FAIL_SAFE_TIMEOUT**, after **180.703 seconds**. Exit code **1** is from external fail-safe termination, not a reported natural game exit or crash. Timeout alone is not used as a game-failure diagnosis.

**Original-data audit: PASS.** Complete before/after manifests match for **505 entries / 435 files / 34,330,559,625 hashed bytes**, including sizes, mtimes, attributes, file identities and link counts. The same original trees protected in TASK-022R remain unchanged, including its historical TASK-022 artifacts. All 40 temporary disc-file links are disarmed before the original locks are released. Experiment-only cache, shader-cache and log changes are expected and recorded separately.

Canonical configure/build/link passed again after the sole run with **all seven task diagnostic options OFF**. Normal diagnostic sources/wrappers are excluded. HEAD remains `2ebaa0a2cf98e02964fabf3a966e20f586fcde8b`; the toolkit remains clean at `bc5b3d30503169a025c88bd9e6195728fc8ad68d`, and the submodule pointer is unchanged. The prior 45 protected build inputs match. Canonical PM image 77 sources remain integrated; no legacy or `tmp/task*` inputs were used. No second boot was run.

## What is not proven

- A defect in `cellGcmSetPrepareFlip`, its callback, or any generic runtime primitive merely from the prior sampled return link.
- Gameplay or rendering success; timeout alone is not a game failure diagnosis.
- Any DXT/SPURS/DMA/scheduler relationship. Those paths and diagnostics are not investigated here.
- Identical scheduling under observation. No timing knob or game delay is changed, but instrumentation has ordinary observation overhead.
- That every possible flip call in other contexts returns, or that TASK-022R would reproduce identically. TASK-023 counts only the selected main-thread outer containing path/import; its 50 sequences are not directly interchangeable with TASK-022R's unfiltered `SetPrepareFlip` log count.
- A specific downstream guest stall or a permanent deadlock. Startup return remains unobserved, but dispatcher entry is positive and other runtime activity continues. This task does not instrument dispatcher return or investigate its body.

## Files changed

- `cmake/mlb16-pm-integration.cmake`: default-OFF TASK-023 diagnostic integration.
- `src/flip_boundary_diagnostic.cpp`: one bounded, project-scoped entry/return/sequence observer.
- `tools/run_task022r_boot.py`: parameterizes its trace-variable name and resolves its default isolation root at call time; TASK-022R defaults remain the same.
- `tools/run_task023_boot.py`: selects a fresh TASK-023 root and trace variable while reusing the TASK-022R isolation/audit lifecycle.
- `docs/TASK-023-FLIP-CALLBACK-BOUNDARY.md`: this report.

No semantic fix, generic-runtime edit, generated-source edit, staging, commit or push. Existing investigation files are retained. Game-derived files, installed copies, logs, linked disassembly and compiled probes remain outside Git.

## Next smallest step

Move the bounded investigation to **the continuation after the completed `00066530` sequence and the observed `000136F8` dispatcher invocation**. Establish the dispatch caller and entry/return progression before selecting a downstream subsystem. Keep this sandbox/audit model; do not change GCM/callback semantics or reopen DXT/SPURS investigation. No semantic change is justified by the selected flip-boundary evidence.

## How to verify

Inspect this task's `static-callback.json`, `build-results.json`, `build-gate.json`, linked wrapper/disassembly records, isolation gates, `observations.json`, `boot-result.json`, `preservation.json`, original before/after manifests, `mirror-unmounted.json`, `restore-results.json` and `final-verification.json`. Preserve the one-boot guard. Verify all seven diagnostic options are OFF in the canonical cache, the normal target excludes their sources/wrappers, staging is empty, and the toolkit remains clean at its pin. Verification of the saved result requires no additional boot.
