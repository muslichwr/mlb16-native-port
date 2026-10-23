# TASK-018 — DXTJob descriptor publication / producer path

Date: 2026-10-23. Investigation and bounded diagnostic only.

## Summary

**Earliest proven failure classification: NO_FAILURE_PROVEN.**

The DXTJob initializer executed and deliberately initialized an empty descriptor region. The sampled consumer fetched the correct first entry from that region: RAM and LS both contained two zero words, followed by the expected EMPTY return. No DXTJob append was observed at the intercepted producer boundary. That is evidence about the observed path, not proof of a missing required job: a request requiring DXT work before those reads has not been established, and inter-object linker wrappers do not cover every possible call.

**BOOT TEST: YA.** Exactly one controlled diagnostic boot, 45 seconds, timeout exit 124. Diagnostic compile/link and all 16 existing synthetic regression invocations passed using the pinned toolkit. The diagnostic was subsequently disabled and the canonical target relinked successfully. No semantic fix was made.

**TASK-018: COMPLETE.** The consumer and a concrete producer/publication boundary are mapped below; the remaining upstream request question is a follow-up investigation, not a confirmed defect.

## Evidence

Baseline main HEAD was and remains `5af6700a8d73e50bdaa0829ed891a539ca3f18ca`. Pinned `third_party/ps3recomp` HEAD was and remains `bc5b3d30503169a025c88bd9e6195728fc8ad68d`; its working tree stayed clean.

Source references below use these roots:

- **PM:** `F:/Games/MLB16PS3/mlb16-port/src/spu/image0077/spu_recomp.c`.
- **PPU:** `F:/Games/MLB16PS3/recompiled-bounded/`; each cited generated file was read locally, never edited or copied into this report.
- **SDK:** `F:/Games/MLB16PS3/mlb16-port/third_party/ps3recomp/`.
- **Artifacts:** `F:/Games/MLB16PS3/build-mlb16-native-pinned/task018/`.
- **Boot log:** `F:/Games/MLB16PS3/logs/mlb16-task018-dxt-publication-20261023T154742Z.log`.

Read `AGENTS.md`, `docs/PROJECT_STATE.md`, `docs/TASK-001-SPU.md`, the explicit TASK-018 assignment, `docs/TASK-017-ALL-LANE-SPURS-TRACE.md`, and the TASK-017.6 boot script/log. No standalone TASK-015/016 report was present in repository docs. The TASK-016 pipeline log and TASK-017 observations were available. The active TASK-018 authorization supplies the diagnostic/build/boot scope; the historical TASK-001 restrictions are not an instruction to resume TASK-001.

## DXTJob consumer path

1. **Workload identity and dispatch.** DXTJob's registration is traced through PPU `func_0006E650`, `ppu_recomp_000.cpp:107835`, then `func_009F98B8`, `ppu_recomp_018.cpp:134665`. Name setup passes `Wws_Job` and the control object's name; the add-workload argument contains the workload EA. SDK `libs/spurs/cellSpurs.c:1644` resolves the fingerprint; lines 1733–1768 select an eligible workload and obtain its argument from the instance's workload-info record. The worker calls `spu_run_policy_module` at line 1695. Runtime evidence identifies SPURS `01233C00`, workload ID **0**, workload EA **16023900**.

2. **Entry and LS argument.** SDK `runtime/spu/spurs_policy.c:239` loads the raw PM at LS `0xA00` on first residency. Lines 258–288 initialize kernel context, lane and workload ID, and put the workload argument in r4's preferred doubleword. PM entry `0xA00` transfers to `0x2B80`. PM lines 2237–2257 extract the low 32-bit workload EA into LS `0x13F0`, initialize `nextLoad` at `0x12C0`, and invalidate the descriptor-page cache at `0x14A0`. LS `0x1330` obtains the SPU lane from kernel LS `0x1C8` through helper `0x3610` (PM lines 2960–2963).

3. **Selection attempt.** With `loadState`'s preferred halfword zero, `0x2C90` tests the signed pipeline slot at `0x12C0`; zero permits `0x3020` (PM lines 2308–2314). `0x3020` passes the workload EA and lane to helper **`0x23A8`**, return link r6 (`0x3030`). An empty first attempt calls the policy-selection service through `0x3820` and retries the same helper (`0x3060`). This explains two descriptor fetches in an ordinary empty PM invocation.

4. **Header and descriptor page.** `0x23A8` GETLLARs 128 bytes from the workload EA into LS **`0x80..0xFF`** (`0x23BC`, PM lines 1700–1716). The current slot/group selector is the word at workload `+0x08`. It selects a 16-byte cursor row at workload `+0x40 + 16*g` (LS `0xC0 + 16*g`) and a descriptor base at workload `+0x30 + 4*g`. In the observed DXT state, `g=3`, so those are workload `+0x70` and `+0x3C` respectively. **Workload ID 0, group selector 3, lane number, descriptor index, and pipeline slot are different quantities.**

5. **Address calculation.** The low halfword of the selected cursor row's first word supplies the candidate descriptor index `i`. The helper computes `entryEA = base + 8*i`, `pageEA = entryEA & ~127`, `pageOffset = entryEA & 127`. It compares the current workload/header identity and cached page EA (LS `0x14B0`, `0x14A0`), fetching when either comparison misses. In the samples, `i=0`, `base=entryEA=pageEA=16023980`, `pageOffset=0`.

6. **LS `0x2544`.** The branch at `0x2534` programs a 128-byte GET, command **`0x40`**, tag **8**, destination LS **`0xD00`**. The command instruction is **`0x2544`**, PM line **1810**. It caches the page EA/header, waits for tag status, then goes to `0x2460`. Repeated transfers here are descriptor-selection reads. They do not independently establish a DMA-completion failure.

7. **EMPTY classification and claim.** At `0x2464` the helper loads the selected LS quadword and rotates it by the entry offset. At **`0x2484`** the preferred low halfword equal to **0** branches to **`0x250C`**, invalidating LS `0x14A0`/`0x14D0` and returning through r6 without a cursor PUTLLC (PM lines 1748–1796). Type **1** can update the cursor row, issue PUTLLC `0xB4` at `0x24A4`, and return only after successful reservation status; type **3** follows the cursor-advance/retry route, type **4** changes header/slot state, and another nonzero type takes a dependency-check route. These are branch descriptions, not inferred application structure names.

8. **Returned selection and pipeline.** `0x3020` stores the result descriptor at LS **`0x1420`**, index/result row at **`0x1410`**, and auxiliary result at **`0x1430`**. At `0x3080` it accepts only preferred halfword **1**; otherwise it returns at `0x31A4` (PM lines 2550–2579, 2649–2652). Acceptance writes `loadState`, assigns a load slot and advances the three-slot `nextLoad` ring. The observed zero descriptor never takes those writes. A zero `nextLoad` is a pipeline slot, not a count of produced jobs.

## Descriptor / structure map

Names here are descriptive labels grounded in loads/stores, not recovered C++ type names. `C` is the PPU control object; `W` is the 128-byte shared workload header; `B` is a descriptor region; all RAM words are big-endian.

| Storage | Observed value / role | Initialization or writer evidence |
| --- | --- | --- |
| Global EA `01113A0C` | Pointer to an outer object; outer `+4` points to `C` | PPU `0006E78C`, `ppu_recomp_000.cpp:107922`; request path reads it at 108091–108106 |
| `C` | `16023890`, PPU-side control object | `009FAAB4` entry observation |
| `C+00` | `W=16023900` | `009F9D54`, `ppu_recomp_018.cpp:135006` |
| `C+04`, `C+08` | SPURS EA `01233C00`, workload ID `0` | `009F98B8` stores SPURS pointer and passes `C+8` as ID output |
| `C+10` | Name storage used for workload registration | `009F9D54` calls `00A3E81C`; `009F98B8` passes `C+10` to name HLE |
| `C+20` | `B=W+80=16023980` | `009FAAB4` -> `009FA590`, `ppu_recomp_018.cpp:136016`, 136024, 135605 |
| `C+24` | Capacity `0x210` eight-byte entries | `009FA590`: `(4352-128)/8`; byte region length is `0x1080` |
| `C+28` | PPU append count, initialized `0` | `009FA590:135603`; incremented by `009FA42C:135494` |
| `C+2C`, `C+30` | Owner/control reference, slot `3` | `009FA464:135503`; caller `009F9C00` |
| `W+00`, `W+04` | PM EA `01057600`, aligned PM size `2F00` | `009F9D54` calls `009FB130/138`; inspected ELF TOC values `010CD80C=01057600`, `010CD810=2EC0`, latter rounded to 128 |
| `W+08`, `W+0C` | Group selector `3`, companion word `FFFFFFFF` | `009FAAB4:136031–136037`; group selector consumed at PM `0x23E0` |
| `W+10+4*g` | Per-group word initialized to `0` (`g=3` => `+1C`) | `009F9C00:134934`; not itself a selected descriptor |
| `W+20+4*g` | Region capacity minus one (`g=3` => `+2C=20F`) | `009F9C00:134937–134938`; **not proof of pending work** |
| `W+30+4*g` | Descriptor base (`g=3` => `+3C=16023980`) | `009F9C00:134927–134936`; consumed at PM `0x23DC` and rotated by `4*g` |
| `W+40+16*g` | Cursor row (`g=3` => `+70`) | PPU `009F9610` initializes halfwords `[0,0,FFFF,FFFF,FFFF,FFFF,FFFF,FFFF]`; PM selection can update via PUTLLC |
| Cursor row `+00`, `+02` | High-halfword progress value and low-halfword candidate index | PM `0x241C..0x2468`: low halfword yields `i`; row update prepares incremented index/progress and a lane halfword. Exact application names unproven |
| `B+8*i +00` | First descriptor word; low 16 bits are the dispatch type | Producer `009FA42C` stores the whole eight-byte descriptor; PM `0x2484` branches on type zero |
| `B+8*i +04` | Second descriptor word; used as a pointer by later paths | PM rotates by four bytes; dependency path masks it to 16-byte alignment. Job command-load path uses it after type-1 acceptance |
| LS `0080..00FF` | Atomic snapshot of `W` | GETLLAR in PM `0x23BC` |
| LS `00C0+16*g` | Selected cursor row inside that snapshot | PM `0x23FC` |
| LS `0D00..0D7F` | Cached 128-byte descriptor page | GET issued at `0x2544` |
| LS `13F0` | Workload EA | Entry `0x2B80`, r4 argument extraction |
| LS `14A0`, `14B0`, `14D0` | Cached descriptor-page EA, workload/header identity, auxiliary/dependency cache key | Entry invalidates `14A0`; PM `0x2534` populates page/header; `0x250C` invalidates page/dependency caches |
| LS `1410`, `1420`, `1430` | Returned index/row, descriptor, auxiliary result | PM `0x3030` and `0x3060` |
| LS `12A0` preferred low halfword | `loadState=0` in samples | Raw PM image initializes zero; type-1 acceptance writes it at PM line 2583; later stage transitions at 1668, 2395, 2669 |
| LS `12C0` preferred word | `nextLoad=0`, pipeline ring slot | Raw image zero; entry explicitly zeroes; accepted load advances modulo 3 at PM lines 2595–2609 |
| LS `12D0`, `12E0`, `12F0` preferred words | Load/run/store slots `FFFFFFFF` | Raw image contains `FFFFFFFF`; acceptance sets load at 2596; run transition at 2663–2667; store transition at 2721 |
| LS `1300` preferred word | Last-store slot `2` | Raw image contains `2`; entry explicitly writes `2` |

Initial LS data was checked by reading only the relevant words from the local ELF's mapped raw PM range: guest EA `01057600`, load LS `0A00`, offset `LS-0A00`. The first ELF PT_LOAD maps file offset 0 to guest `10000`. This was a local read, not extraction, regeneration or a new game-data artifact.

## Producer path

**Initialization before consumption is proven.** `func_0006E650` allocates the header/descriptor block, supplies the string at guest EA `00F2E974` (`DXTJob`), and calls `009FAAB4` at return link `0006E6F4`. The initializer constructs the shared header, initializes a descriptor-control block at `C+20`, binds that block to group 3, zeroes the descriptor region, and initializes the cursor row. It precedes registration through `009F98B8` at return link `0006E754`.

The descriptor zeroing is explicit: `009FA590` and `009FA3F4` clear the region in 128-byte steps (`ppu_recomp_018.cpp:135597`, 135468). An initial zero descriptor therefore has a known initialization source; it does not require a DMA or synchronization hypothesis.

**Request and append path, statically established:**

- Outer request adapters `0006EBAC` / `0006EBF0` call **`0006E988`** (`ppu_recomp_000.cpp:108236`, 108256). Cross-object callers include return links `007F6CD8`, `007F6CF0`, `0082D380`, `0082D478`, `009DE134`, `009E0900`, `009E0918`, `009E7BC4` in PPU files 015/018.
- `0006E988` reads the global outer pointer, returns early if it is null, tests alignment/size inputs, and skips its submission loop when the remaining count is not positive (`ppu_recomp_000.cpp:108091–108178`). Those predicates were inspected, not changed or observed dynamically.
- For an eligible batch it calls `00A55540` to construct an eight-byte descriptor, then loads the DXT control pointer from outer `+4` and calls **`009FA818`** with that descriptor in r5, at return link **`0006EB44`** (`ppu_recomp_000.cpp:108187–108206`).
- `00A55540` calls `00A55040`; the successful builder path ends through `00DA440C`, which writes the command pointer at descriptor `+4`, fills size/bits, and sets the halfword at descriptor **`+2` to 1** (`ppu_recomp_019.cpp:87620`, 87592–87598; `ppu_recomp_025.cpp:134277–134297`). This identifies how an actual job becomes a type-1 descriptor without assuming any DXT request occurred.
- `009FA818` passes `C+20` and the eight-byte value to **`009FA42C`** (`ppu_recomp_018.cpp:135794–135817`). That helper obtains `i` from `C+28`, base from `C+20`, computes **`B+8*i`**, increments the local count, writes the descriptor with `vm_write64`, and fences (`135486–135499`). It returns `i+1` through the surrounding marker structure.

No production invocation on this path was observed in the bounded window. Its initializers and registration did execute.

## Publication boundary

The smallest useful publication boundary is the **`009FA818` -> `009FA42C` append**, paired with the PM fetch of the same eight-byte address. The PPU-local count increment at `C+28` is distinct from the descriptor's visible type halfword. The consumer reads the descriptor itself; the examined selection helper does not use `C+28` as a work-availability count.

The static transition is zero/sentinel descriptor -> eight-byte store containing type 1 -> PM's descriptor GET -> type test -> successful atomic cursor claim -> load-stage state change. Cursor initialization/publication and the SPURS runnable fields are additional boundaries, not interchangeable with the descriptor store. The selected row already exists at registration. SDK scheduling permits periodic idle probes even with no ready count/signal (`cellSpurs.c:1743–1748`), so repeated PM dispatch alone does not prove an application request is outstanding.

The project diagnostic observes init, registration, append, reset and the focused consumer GET/return. It does not instrument the scheduler or other workloads. The optional append record includes original caller, control/workload/ID, destination/index, input descriptor, RAM before/after, local count before/after and cursor row after. No append record fired, so no nonzero publication transition was measured.

## Static findings

- PPU `C+20` and shared `W+3C` are established by the same constructor/binding path. Both point to `W+80` in the observed configuration. No static wrong-destination finding was established.
- The old probe's comment suggesting `W+20` versus `W+80` is not authoritative structure evidence. Selection follows the group-specific base table; the observed `W+20` is not a descriptor.
- All sampled EMPTY pipeline values have an initialization path. The absence of load/run/store indices is consistent with the zero-type early return.
- The type-1 acceptance/claim and pipeline gates remain untouched. Their correctness for nonempty work has not been demonstrated by this experiment.

## Runtime diagnostic findings

Ten complete `[task018]` records were emitted. Local `observations.json` preserves just those records and line numbers; the bulk log remains outside Git.

| Log line(s) | Observation |
| --- | --- |
| 1, 72 | Startup verifies fingerprint `75BFE68AE96D7577`, image 77, entry `A00`; workload 0 resolves to that image |
| 51–52 | Init executed: `C=16023890`, `W=16023900`, allocation `4352` bytes, base `16023980`, capacity-minus-one `20F`, append count 0, cursor first words `00000000,FFFFFFFF`, descriptor `0,0` |
| 73 | Registration returned SPURS `01233C00`, wid 0, same workload/base |
| 88, 95, 133 | First three actual descriptor GETs: EA `16023980`, LS `D00`, offset/index 0; RAM before and LS after both `00000000,00000000`; cursor snapshot `00000000,FFFFFFFF`; append count 0 |
| 105, 143, 161 | First three PM returns: image 77, lane 0, halt `9C0`, index 0, descriptor/type 0, `loadState=0`, `nextLoad=0`, load/run/store `FFFFFFFF`; append count 0 |
| 56807 | Snapshot 35 seconds after initialization: **0 intercepted appends**, 0 intercepted resets, 2,245 PM entries, 4,489 descriptor GET attempts |

The last counts include an entry/GET that may not return: counters increment before original calls. This distinction explains why the totals are not necessarily an exact 2:1 ratio.

Policy END logs include `exited=1` at `9C0`. The unchanged repeated-DMA guard later halts at `2544` for DXTJob and the two other registered workloads; the canonical TASK-017.6 log already contains the same three guard halts and three `[pm-end]` records. This investigation neither changes nor validates that guard's interpretation of idle polling.

D3D12 initialized, the known unmapped FIFO jump `10464648` occurred, and timeout returned **124**, measured **45.656 seconds**. No new fatal/assert/crash/access-violation message was found. Image-0004's known async miss did **not** appear in this run; this is a coverage difference, not evidence that the miss was fixed. Visual window contents were not independently inspected. Broader boot-progress equivalence is not claimed.

## Earliest proven failure classification

**NO_FAILURE_PROVEN.**

The earliest established state is intentional empty initialization, followed by a correctly addressed zero fetch. Zero producer calls at the intercepted boundary is an observation, but `PRODUCER_NOT_REACHED` as a failure requires evidence that expected work should have reached it. That prerequisite is missing. In addition, linker wrapping covers unresolved references between objects; same-object calls and alternate direct stores are outside its completeness claim. The DXT submit call at `0006EB44` was verified in the linked executable to target the wrapper, but this does not establish whole-program producer coverage.

No produced-empty descriptor, wrong destination, omitted publication after a known descriptor write, or nonzero producer/consumer disagreement was captured. None of those failure classes is uniquely supported.

## What is NOT proven

- That any DXT request was required during the sampled boot interval, or that `0006E988` executed or passed its guards.
- That no alternate/same-object producer or unsampled later write exists. The 35-second summary is not a process-exit snapshot.
- Correct consumption of type-1 jobs, memory ordering under concurrent publication, or all cursor/slot transitions.
- A common cause with MeshJobs0/MeshJobs1, a rendering cause, playability, or a semantic fix.

## Files changed

Repository changes, unstaged:

- `cmake/mlb16-pm-integration.cmake`: default-OFF `MLB16_TASK018_DIAGNOSTIC` option adds one original observation source and six linker wrappers.
- `src/dxtjob_publication_diagnostic.cpp`: original, process-opt-in glue (`MLB16_TASK018_TRACE=1`); first three producer/consumer records and one 35-second count snapshot. It forwards original calls once with unchanged arguments and returns; it makes no guest-memory writes.
- `docs/TASK-018-DXTJOB-PUBLICATION.md`: this report.

Local artifacts: configure/build and restore logs, command/result JSON, four synthetic executables/results, protected-input manifests, metadata, local disassembly, observation records and boot script under the canonical build's `task018/`; one boot log under `logs/`. No new build uses `tmp/task*`, the rollback toolkit, or the rollback build.

The canonical executable was relinked for the diagnostic and relinked again with the diagnostic **OFF**. Its final hash differs from the pre-task executable after relinking; byte-identical executable restoration is not claimed. Input hashes confirm the 30 PPU objects plus HLE object, PM source/header and startup source were unchanged. The PM source was neither regenerated nor edited. SDK source was never instrumented. No staging, commit, push, game-data upload, installation, legacy-tree edit or semantic runtime change occurred.

## Uncertainties

Guest RAM reads are ordinary individual atomic word reads, not an atomic multiword snapshot. The RAM-before and LS-after comparison surrounds the original GET; a concurrent writer could change RAM between them. The LS page and post-return descriptor records are separate observations. Their matching zeros establish the sampled result only. The monitor thread and wrapper overhead can affect scheduling; the ten extra lines do not establish timing equivalence to the prior boot. Logging holds stdout/stderr locks only for emission, not during guest execution.

The high-level request entry and its early-return predicates were not instrumented because the chosen first experiment targeted the concrete append/publication boundary. Exact application type names and the entire lifecycle of every group field remain unproven.

## Recommended next smallest step

Read-only trace of the **DXT request eligibility path into `0006EBAC` / `0006EBF0` -> `0006E988`**, starting with the cross-object callers listed above. Establish one concrete request that this boot should make, its dimensions/count, global outer/control pointer, and the predicate that admits or skips it. Only if that identifies an expected request should a follow-up add a first-few entry/return observation of that specific request path. Success is a proven request -> guard outcome -> append (or proven skipped append) correlation. No queue, DMA, scheduler, descriptor or guard fix is justified yet.

## How to verify

All commands were PowerShell unless the saved boot script explicitly uses MSYS2 CLANG64 Bash. Read-only inspection used `Get-Content`, `rg -n`, `rg --files`, `git status --short`, `git rev-parse HEAD`, Python file parsing, `llvm-nm`, and `llvm-objdump`. Source searches focused on the cited symbols, LS offsets, prior `pm-pipe` records and DXT registration. Initial probes for root `CMakeLists.txt`, unpadded `ppu_recomp_0.cpp`, a TASK-017.6 result JSON and several literal PowerShell wildcard paths failed; the actual CMake entry, zero-padded files, saved boot script/log and directory-aware `rg` searches supplied the evidence. No missing file's content was assumed.

The decisive commands and their actual arguments/results are saved locally:

1. `task018/configure-result.json`: `cmake.exe -S F:/Games/MLB16PS3/mlb16-native -B F:/Games/MLB16PS3/build-mlb16-native-pinned -DMLB16_TASK018_DIAGNOSTIC=ON`, exit 0.
2. `task018/build-result.json`: `cmake.exe --build F:/Games/MLB16PS3/build-mlb16-native-pinned --target MyGameRecomp --parallel 2`, exit 0; one diagnostic compilation and link, no PPU object rebuild.
3. `task018/regression-builds.json`: exact four Clang argument arrays, with every SDK input redirected to the pinned submodule and every output to the canonical build. Flags `-std=gnu17 -O1 -UNDEBUG`, existing CPU fixture/runtime source list, `-pthread -lm`; all four exit 0. Arrays were derived from the prior saved recipe, never executed against its legacy paths.
4. `task018/regression-results.json`: all **16/16 exit 0**: policy exit, 14-case machine-status fixture, interrupt round-trip, and 13 interpreter handoff modes. Both step-cap modes supply the required `SPU_STEPCAP=2` and `YDKJ_SPU_TRACE=64`. Overrides were cleared per child before applying each test's documented settings.
5. `task018/build-gate.json` and `linked-boundaries-disassembly.txt`: canonical PM/startup still linked, image-77 registration retained, no legacy SDK or `tmp/task*` build input; actual DXT init/register/append calls target the wrappers. The baseline `protected-before.json` covers 34 input files.
6. `task018/boot-command.json`, `boot.sh`, `boot-result.json`: one `timeout -k 5s 45s` invocation of the canonical executable with the local ELF and VFS root. Child environment removes inherited `SPU_`, `SPURS_`, `YDKJ_`, `LBP_`, `GH3_`, `MLB16_TASK018_` overrides, then enables only `MLB16_TASK018_TRACE=1` and `SPURS_PM_TRACE=1`. The prior broad `SPURS_PM_PIPE` dump is omitted because the focused probe supplies its DXT fields. Both enabled variables were unset afterward. Exit 124, empty shell stderr; no game process remained.
7. `task018/observations.json`: require exactly 10 complete focused records, exactly three FETCH and three PM_RETURN records, matching workload/destination/zero words, initialization before consumption, and the 35-second zero-append snapshot.
8. `task018/restore-results.json`: configure with `-DMLB16_TASK018_DIAGNOSTIC=OFF` and build the same canonical target; both exit 0. The final Ninja link excludes the diagnostic source/wrappers and retains PM/startup/pinned SDK. A fresh non-diagnostic boot was not run.
9. Final `git diff --check`, main/submodule status, protected-input hash comparison and build-input checks verify the remaining reviewable delta. No commit or pin change is needed to inspect it.

To reproduce a future authorized diagnostic, use steps 1–6 with a new unique local log name. To disable it, step 8 is sufficient; that rollback was already performed. The default-OFF source can remain for review without affecting the canonical target. To remove the experiment entirely later, remove only the named diagnostic option/source/report after review; do not reset unrelated files or touch legacy trees.
