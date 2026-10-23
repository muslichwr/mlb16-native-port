# TASK-019 — DXTJob request eligibility path

Date: **2026-10-24** (Asia/Jakarta). Investigation only; no semantic fix.

## Summary

**Classification: REQUEST_NOT_REACHED.** The instrumented `0006EBAC`, `0006EBF0` and `0006E988` entries were not reached in the observed boot window. The selected upstream candidate, `009DDA00`, also had zero observed entries. There is no executed request whose inputs can be classified as validly or suspiciously rejected by an eligibility guard.

The 35-second snapshot found the global outer pointer and its DXT control pointer readable and nonzero: `01113A0C -> 16023870`, outer `+4 -> 16023890`. Their existence does not establish an outstanding DXT request. No builder, append or append-write call correlated with a request was observed.

**BOOT TEST: YA.** One controlled diagnostic boot: timeout exit **124**, measured **45.734 seconds**. The diagnostic compiled and linked in the canonical pinned build. It was then disabled and the canonical target relinked successfully. Generic runtime code and the pinned dependency were unchanged.

**TASK-019: COMPLETE.** The result is bounded non-execution evidence, not a defect finding or proof that DXT work should occur during this interval.

## Evidence

Main HEAD remains `5af6700a8d73e50bdaa0829ed891a539ca3f18ca`; the submodule remains clean at `bc5b3d30503169a025c88bd9e6195728fc8ad68d`. The three unstaged TASK-018 files present at task start were preserved. No staging, commit or push occurred.

Read `AGENTS.md`, `docs/PROJECT_STATE.md`, the active TASK-019 assignment, the TASK-018 report/source and the canonical build configuration. Historical TASK-001 instructions do not replace the explicitly assigned TASK-019 diagnostic/build/boot scope.

Paths used below:

- PPU sources: `F:/Games/MLB16PS3/recompiled-bounded/`, read locally only.
- Project: `F:/Games/MLB16PS3/mlb16-port/`.
- Toolkit: `F:/Games/MLB16PS3/mlb16-port/third_party/ps3recomp/`.
- Build/artifacts: `F:/Games/MLB16PS3/build-mlb16-native-pinned/task019/`.
- Boot log: `F:/Games/MLB16PS3/logs/mlb16-task019-dxt-request-20261023T174136Z.log` (UTC filename; start was October 24 locally).

The decisive source is `ppu_recomp_000.cpp:108074`, function `0006E988`; its adapters start at lines 108236 and 108256. No generated source or object was edited or rebuilt.

## Caller map

These labels describe fields/arguments only. No high-level game subsystem identity is assigned. In this table, `X` and `Y` are adapter arguments r4/r5, `k` is the enclosing loop index, and `M(O)` means the metadata pointer read from object `O+48`. The helper `004B762C` supplies source/destination pointers using selector arguments 5/6; the meaning of those selectors is not inferred.

| Return link | Enclosing function / source line | Adapter and inputs | Conditions controlling the call / startup relevance |
| --- | --- | --- | --- |
| `007F6CD8` | `007F6C00`, PPU 015:5137 | `0006EBF0`; source from selector 5 on entry r3 object, destination from selector 6 on entry r4 object; `X=M(dest)+00 >> k`, `Y=M(dest)+02 >> k` | Loop bound is the smaller metadata `+04` halfword of the two objects and must be positive. Destination metadata `+08 == 7` selects this adapter. No static proof of startup execution |
| `007F6CF0` | Same function, PPU 015:5144 | `0006EBAC`, same pointers/dimensions, flag r7=0 | Complementary `+08 != 7` branch of the same loop |
| `0082D380` | `0082D290`, PPU 015:65512 | `0006EBAC`, flag 0; selector 5 on entry object's `+80`, selector 6 on its `+88`; dimensions from `M(object+88)+00/+02`, shifted by `k` | Entry r4 low byte nonzero; both object pointers nonnull; minimum metadata `+04` bound positive. Conditional upstream caller `0084D10C` at return link `0084D430`; startup execution unproven |
| `0082D478` | Same function, PPU 015:65577 | Same pattern for object `+84` / `+8C` | Entry r5 low byte nonzero, both pointers nonnull, positive minimum bound |
| `009DE134` | `009DDA00`, PPU 018:105812 | `0006EBF0`; `X=512 >> k`, `Y=256 >> k`; selector 5 on a locally created object, selector 6 on entry object's `+1B4` | Entry mode r4 is neither 0 nor 11, entry r6 low byte nonzero, helper `0099B68C` returns nonzero, signed level bound from EA `01346070` is positive. This is the selected runtime candidate; no entry was observed |
| `009E0900` | `009E07CC`, PPU 018:108592 | `0006EBF0`; `X=entry r6`, `Y=entry r7`, destination=entry r4; source=entry r3 or the repacked buffer at `01348680` | Entry r8 format equals 7 and the admission conditions described below pass. Caller `009E09A0` supplies dimensions and metadata format at return link `009E0B64`; boot execution unproven |
| `009E0918` | Same function, PPU 018:108599 | `0006EBAC`, flag 0; same dimensions/source/destination | Entry format differs from 7; same admission conditions |
| `009E7BC4` | `009E70E8`, PPU 018:116355 | `0006EBAC`; source/destination from selector 5/6 calls; `X/Y` from returned object's `+60 -> metadata +48 -> halfwords +00/+02`; flag is whether a byte in the table rooted at `0114E894` is nonzero | A state counter is below 18 and its remainder modulo 3 is 2; metadata `+04` bound positive; `X != 2`. The enclosing function has many earlier conditions. A cross-object caller exists in `002AB7B0` at return link `002AB810`; execution of this request branch in the observed boot is not established |

For `009E07CC`, the r9 low-byte flag must be zero, `X` must be at most `0x800` and divisible by four, `Y` must be divisible by four, and the combination `X==4 && Y>4 && format==5` is routed away from the request adapters. Those alternatives call `00A51560`. When input pitch differs from `4*X`, it repacks rows first; a signed `Y*(4*X) > 0x400` returns before submission. These are caller-local predicates, distinct from `0006E988`'s guards (`ppu_recomp_018.cpp:108517–108618`).

All eight known adapter call sites are direct cross-object references. Source search found exactly two direct callers of `0006E988`, the adapters in PPU object 000, plus its function-table entry in PPU object 029. Both adapters call `0006E988` unconditionally after register rearrangement.

Adapter argument mapping:

| `0006E988` argument | `0006EBAC` | `0006EBF0` |
| --- | --- | --- |
| r3 source | Adapter r3 | Adapter r3 |
| r4 source format value | `0500AAE4` | `0500AAE4` |
| r5 X | Adapter r4 | Adapter r4 |
| r6 Y / remaining count | Adapter r5 | Adapter r5 |
| r7 destination | Adapter r6 | Adapter r6 |
| r8 destination format value | `0600AAE4` | `0800AAE4` |
| r9 preprocessing flag | Adapter r7 low byte | 0 |

## 0006E988 predicate table

The global source is a 32-bit guest load from **EA `01113A0C`**, formed by `lis 0x111` plus `0x3A0C` (PPU 000:108091–108096). `0006E78C` allocates/stores the outer pointer; its constructor `0006E650` stores the DXT control at outer `+4`. Teardown `0006E800` can clear the global. TASK-019 does not change these operations.

Let `W=(uint32_t)entry r5`, `H=(uint32_t)entry r6`, and `R` be `W` interpreted as signed 32-bit, divided by two with truncation toward zero only when entry r8 equals `0600AAE4`; otherwise `R=(int32_t)W`.

| Predicate / location | Input/source | Pass behavior | Fail behavior |
| --- | --- | --- | --- |
| Outer pointer nonzero, `0006E9D8` | `read32(01113A0C)` | Continue | Branch to return `0006EB70` |
| `H & 3 == 0`, `0006E9E4` | Entry r6, mask of its low two bits | Continue | Return `0006EB70` |
| `W & 3 == 0`, `0006E9F0` | Entry r5, same mask | Continue | Return `0006EB70` |
| Not `(R < 4 && H > 4 unsigned)`, `0006EA10..0006EA1C` | Adjusted row size and original count | Continue | Return `0006EB70` |
| Positive computed initial batch target, `0006EA20..0006EA38` | `B=((H >> 2)+2) & ~3` with 32-bit operations | Keep B when signed B>0 | Set B=4; **this is not rejection** |
| Optional preprocessing admission, `0006EA3C..0006EA6C` | Low-byte entry r9 nonzero, source-format r4=`0500AAE4`, H nonzero | Reverse W-word rows in place for H rows, then continue | Skip preprocessing and continue |
| `(int32_t)H > 0`, `0006EAD0..0006EADC` | Original count assigned to remaining count | Enter submission loop | Return `0006EB70` |
| Batch choice, `0006EAE0..0006EAF0` | Remaining count, B, four-slot budget | Use B normally | If remaining<B or this is the fourth slot, use all remaining; **not rejection** |
| Remaining count >0 after append, ending at `0006EB6C` | Subtract submitted batch count | Repeat | Return normally |

The first four rejecting checks and the signed-count check are the complete early-return set before descriptor construction in the inspected function. There is no separate source/destination pointer-alignment rejection here, no unconditional positive-W check, and **no explicit null check of the DXT control pointer at outer `+4`**. For example, W=0/H=4 is not rejected by the small-row/count predicate alone; this is a static boundary observation, not a defect finding about real inputs. Optional preprocessing precedes the positive-count check; arbitrary signed-negative inputs must not be treated as a demonstrated safe or valid request.

Submission-loop arguments (`ppu_recomp_000.cpp:108187–108206`):

- Before builder entry, reload outer from `01113A0C` and retain it in r21.
- `00A55540`: r3=`SP+80` output location; r4=current source; r5=original source format; r6=W; r7=current positive batch count; r8=current destination; r9=original destination format; r10=1; stack slot `SP+70`=0.
- After the builder, load its eight-byte output and read control from the retained outer `+4`.
- `009FA818`: r3=`SP+88` marker output, r4=control, r5=eight-byte descriptor. Return link is `0006EB44`.
- `009FA818` passes control `+20` and the descriptor to `009FA42C`. Its returned value is next index `i+1`, not the original zero-based index.
- Source advances by `batch * (4*W)`; destination advances by `batch * R`; remaining count decreases by batch. At most four positive batches are submitted for ordinary positive dimension inputs.

## Candidate request

The selected candidate was **`009DDA00`**, particularly the edge from `002A81EC` at return link **`002A85E8`** (`ppu_recomp_005.cpp:2621–2631`). That edge supplies:

- Object from guest global `0114E8EC`.
- Mode from `01346088`.
- Argument 5 from a halfword at `[read32(0113E618)+5A]`.
- Flag r6=1.

Other direct callers are `009DF7A4` at return link `009DFCE8` (mode 11, flag 0, excludes the request loop), and `009E56F0` at return link `009E59A0` (mode from `01346088`, flag 1, with its own outer-pointer/flag conditions).

This gives a concrete, testable candidate invocation rather than an assumption that DXTJob must always have work. If admitted, its first level would supply X=512, Y=256: positive and divisible by four, and not the small-row exclusion. But the candidate's own entry, mode/flag and positive level bound must first be observed. **It did not execute in this observation**, so those hypothetical dimensions are not runtime inputs and do not create a work expectation.

## Runtime observations

Exactly two coherent `[task019]` records were emitted:

| Log line | Observation |
| --- | --- |
| 1 | Probe armed before runner main, fixed 35-second snapshot; request `0006E988`, candidate `009DDA00`; old probes OFF |
| 16729 | `candidate_entries=0`, `candidate_adapter_calls=0`, `adapter_ac=0`, `adapter_f0=0`, `request_entries=0`, `builder_calls=0`, `append_calls=0`, `append_write_calls=0`; `outer_ok=1 outer=16023870 control_ok=1 control=16023890` |

No first-entry record appeared later in the completed 45-second log either. The numerical snapshot is explicitly bounded to 35 seconds; it is not a process-exit counter dump. A first entry at any point before termination would emit its own entry record under the fixed first-three budget.

This negative observation has verified coverage for the primary path:

1. A small original synthetic COFF fixture compiled and ran locally: a same-object call hit its wrapper, which forwarded the original once (`same_object_wrapped=1 value=11`). This was not a game boot.
2. Disassembly of the diagnostic executable verified **14 actual wrapped call sites**: all eight caller sites, both same-object adapter-to-`0006E988` calls, request-to-builder, request-to-append, append-to-helper, and the chosen `002A85E8` candidate edge.
3. Two additional disassembled edges from `009DF7A4` and `009E56F0` targeted the candidate wrapper.
4. Reading the linked PE's function-table entries confirmed that guest addresses `0006E988`, `0006EBAC`, `0006EBF0` point to the wrappers. Same-object coverage was checked on these artifacts, not assumed from a generic linker rule. TASK-018's conservative inter-object coverage limit was not silently reused as a TASK-019 guarantee.

The diagnostic forwards each original call once with the original context/arguments and writes no guest memory. If a request had occurred, first-three request IDs and thread-local nesting would have correlated adapter, entry-input predicate interpretation, builder calls, append calls and `i+1` returned by the actual append helper. The entry predicate is explicitly a snapshot interpretation, not an instruction-by-instruction branch probe. No actual guard outcome can be inferred from the zero-entry result.

The boot used only `MLB16_TASK019_TRACE=1`; broad `SPURS_PM_PIPE`, TASK-018 descriptor/DMA tracing and `SPURS_PM_TRACE` were disabled. No `[pm-pipe]` or `[task018]` record appeared. Startup verified PM image 77 and its fingerprint. D3D12 initialized; the known image-0004 async miss appeared. The unchanged repeated-DMA guard later produced the existing three halts at LS `2544`, as in prior canonical logs. No new fatal/assert/crash/access-violation message was found.

There was no intact `FIFO JUMP` record in this run; absence is not a fix. Rendering/window contents were not independently inspected. The timeout ended with exit 124, empty shell stderr, `MLB16_TASK019_TRACE=unset`, and no remaining `MyGameRecomp` process. Neither timeout nor lack of an error line proves whole-game correctness.

## Request-to-append correlation

No request ID was allocated because `0006E988` never entered. Consequently there is no executed eligibility decision, constructed descriptor, or correlated append to compare with TASK-018. The two investigations agree on an observed absence of production at their covered boundaries, but TASK-019 does not repeat the already-answered DMA/descriptor investigation or infer a shared cause with other workloads.

## Classification

**REQUEST_NOT_REACHED**, for the primary adapter/`0006E988` path during the observed boot interval.

This classification does **not** label non-execution a defect. No candidate invocation with an established work expectation ran. A skipped-validly or skipped-suspiciously conclusion would require an executing request and a decisive guard; neither exists here. The positive finding is coverage-backed absence of request entry, with nonzero initialization pointers separately observed.

## What is NOT proven

- That startup should reach `002A85E8`, `009DDA00`, any other listed caller, or a DXT request in this interval.
- Which earlier boot-sequencing branch, wait or application choice prevents the selected candidate from executing.
- An executing request's mode, dimensions, count, alignment, guard outcome or descriptor contents.
- A publication, queue, scheduler, DMA or workload-guard defect; a relationship to black rendering or playability.
- Request behavior after the timeout or under different user input/startup state.

## Files changed

TASK-019 repository delta, unstaged:

- `cmake/mlb16-pm-integration.cmake`: adds default-OFF `MLB16_TASK019_DIAGNOSTIC`, one original source and seven wrappers; refuses simultaneous TASK-018/019 probes.
- `src/dxtjob_request_diagnostic.cpp`: original, doubly opt-in, bounded request/candidate observation glue. No SPU/DMA/scheduler hooks.
- `docs/TASK-019-DXTJOB-REQUEST-ELIGIBILITY.md`: this report.

Existing TASK-018 source/report and its diagnostic option block remain preserved. Local `task019/` artifacts contain the coverage fixture, build/restore logs, command/result JSON, local disassembly, function-table metadata, observations and protected-input manifests. The unique bulk boot log remains outside Git.

The canonical executable was relinked for the experiment and relinked with **both task diagnostics OFF** afterward. Byte-identical restoration is not claimed. All **43 protected input files** match the pre-task hashes: 31 PPU/HLE objects, PM source/header, startup glue, TASK-018 source/report and seven inspected generated PPU source/header files. The pinned toolkit remained clean; legacy trees were not used or modified. No generic runtime code, game assets or generated code changed; no PPU regeneration/recompilation, installation, staging, commit, push or external upload occurred.

## Uncertainties

The selected upstream candidate was not reached, so a positive request lifecycle cannot be manufactured from this run. Zero request entries establishes non-execution of the verified primary path, not an application obligation to execute it. Alternate workloads or application operations were not surveyed.

The monitor's RAM reads are individual reads, not a simultaneous snapshot of global/outer/control. `VirtualQuery` prevents reading uncommitted observation pages and distinguishes unreadable from zero; it does not establish guest object lifetime. Wrapper/logging overhead and the monitor thread can affect scheduling. The added records are bounded, but timing equivalence to an uninstrumented boot is not claimed.

## Recommended next smallest step

Read-only reconstruction of boot sequencing into **`002A81EC -> 002A85E8 -> 009DDA00`**, identifying the first proven executing ancestor and the condition or wait separating it from the candidate. Check what creates/updates globals `0114E8EC` and `01346088` before the candidate edge. If a later diagnostic is necessary, observe that single ancestor-to-candidate boundary, not the DXT guards again. Success is a proven execution/branch correlation that either establishes a required candidate invocation or explains why no request is expected. No semantic fix is justified by TASK-019.

## How to verify

Inspection used PowerShell `Get-Content`, `rg -n`/`rg --files`, read-only Git commands, local Python parsing and MSYS2 CLANG64 `llvm-nm`/`llvm-objdump`. Commands and results that determine the outcome are saved under the canonical build's `task019/`:

1. `wrap-coverage-result.json`: exact compile command and synthetic execution result verifying same-object forwarding. Its source is original synthetic code; no game input is used.
2. `build-results.json`: `cmake.exe -S F:/Games/MLB16PS3/mlb16-native -B F:/Games/MLB16PS3/build-mlb16-native-pinned -DMLB16_TASK018_DIAGNOSTIC=OFF -DMLB16_TASK019_DIAGNOSTIC=ON`, then `cmake.exe --build F:/Games/MLB16PS3/build-mlb16-native-pinned --target MyGameRecomp --parallel 2`. Both exit 0; one diagnostic compile and link. No generic runtime edit occurred, so TASK-019's conditional persistent runtime-regression rebuild was not required and was not run; TASK-018's 16 passes remain historical evidence only.
3. `build-gate.json`, `linked-boundaries-disassembly.txt`, `candidate-caller-disassembly.txt`, `function-table-coverage.json`: verify actual call redirection and function-table targets, canonical PM/startup inclusion and image-77 registration, pinned SDK, and absence of legacy or `tmp/task*` build inputs. Default-OFF declaration is also checked in project CMake.
4. `boot-command.json`, `boot.sh`, `boot-result.json`: one **MSYS2 CLANG64 Bash** `timeout -k 5s 45s` execution of the canonical executable with the local ELF and VFS root. Inherited `SPU_`, `SPURS_`, `YDKJ_`, `LBP_`, `GH3_`, `MLB16_TASK018_`, `MLB16_TASK019_` overrides were removed before enabling only `MLB16_TASK019_TRACE=1`. Measured 45.734 seconds, exit 124; override unset afterward.
5. `observations.json`: exactly two complete focused records, ARM and WINDOW_35S; require zero candidate/adapters/request/build/append counters, readable nonzero pointers and no later first-entry record in the full local boot log. Do not interpret the zero downstream counters as evidence of a rejected executing request.
6. `restore-results.json`: same configure command with both task diagnostics OFF, then the same canonical build. Both exit 0; link restored without task wrappers. No second game boot was run.
7. Final verification checks protected hashes, diagnostic-free canonical link, unchanged HEAD/pin, clean SDK, no staged paths, and `git diff --check`. The only whitespace advisory is Git's existing LF-to-CRLF conversion notice.

To reproduce an authorized observation later, use steps 2–4 with a fresh log name. Rollback is the OFF configure/build in step 6, already performed. The default-OFF diagnostic remains reviewable; no runtime patch or dependency commit is needed.
