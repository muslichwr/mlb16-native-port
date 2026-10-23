# TASK-020 — DXT request expectation during the current boot

## Summary

**Classification: BOOT_SEQUENCE_NOT_REACHED.** The observed startup has not reached the state-dispatch stage that can call `000104AC -> 002A9300 -> 002A81EC -> 009DDA00`. No obligation to produce DXT work at this boot stage is established. Stop following DXT internals.

One original, bounded, default-OFF diagnostic observed the startup/state-dispatch boundary in one controlled 45-second boot. Startup `00011CE8` entered once and had not returned at the 35-second snapshot; `000136F8` had zero entries. The main-thread context and guest stack correlate with the callback-poll/sleep loop at `00016054` inside the continuation of `00015948`. The log separately records a corrupt-game-data error dialog. This error was already present in TASK-019's log.

Baseline: `bde43459f638e9728244c335501076f1fb60a0fd`. Pinned toolkit: `third_party/ps3recomp` at `bc5b3d30503169a025c88bd9e6195728fc8ad68d`, unchanged and clean. No semantic changes, staging, commit or push.

**BOOT TEST: YA**

**TASK-020: COMPLETE**

## Evidence

Read first: `AGENTS.md`, `docs/PROJECT_STATE.md`, TASK-018 and TASK-019 reports. Generated sources and the local ELF were inspected locally and remain outside the repository delta. Line references below are to `F:/Games/MLB16PS3/recompiled-bounded/`.

| Evidence | Location / result |
| --- | --- |
| Prior upstream non-execution | TASK-019: primary request and `009DDA00` candidate had zero entries. This does not establish a work expectation. |
| Startup already proven by prior log | TASK-019 log contains semaphore return links `000127AC` and `000127EC` inside `00011CE8`. |
| Startup to state dispatch | `ppu_recomp_000.cpp:3850–3858`: object virtual slot `+18`, return link `0001353C`. ELF metadata maps the manager vtable `0102EC98 +18` through OPD `010A1E98` to `000136F8`. |
| State update to candidate ancestor | `ppu_recomp_000.cpp:4007–4030`: current state from manager `+24`, then state virtual slots `+8` and `+C`. Three inspected vtables (`0102EC38`, `0102EC68`, `0102ED30`) have `+C -> OPD 010A1DA0 -> 000104AC`. These are static possibilities, not observed selected states. |
| Candidate ancestry | `ppu_recomp_000.cpp:581`, `ppu_recomp_005.cpp:3923,2631`: return links `000105A0`, `002A98D0`, `002A85E8`. |
| Earlier occupied startup call | `ppu_recomp_000.cpp:3120`: `00011CE8` calls `00015948`, return link `000129D0`, before state dispatch. |
| Wait | `ppu_recomp_028.cpp:132542–132554`, also overlapping continuation `ppu_recomp_029.cpp:29572–29584`: poll byte `011B7A02`; while nonzero, call `00F25FEC`, then syscall `8D` with `r3=2710` (10,000 microseconds), then recheck. |

The continuation is emitted both within `func_00015A50` in unit 028 and `func_00015B94` in unit 029. The parent `00015948` tail-transfers through generated trampoline continuations. Those generated boundaries must not be mistaken for independent application calls or extra guest stack frames.

## Executing ancestor

**Nearest proven executing common ancestor on this reconstructed route: `00011CE8`.** Its currently occupied child is `00015948` and its continuations; the child must return before the later state-dispatch call.

The static route is:

`00011370 -> 00011BA8 -> 00011CE8 -> [startup completes] -> 000136F8 -> [selected state +C] -> 000104AC -> 002A9300 -> 002A81EC -> 009DDA00`.

The first two indirect edges resolve through the manager's ELF vtable. `00011BA8` invokes `00011CE8` with return link `00011CB8`. The boot entry/CRT eventually calls `00011370` with return link `0001C8B4`; no high-level name is assigned to the selected state objects.

At 35 seconds, the probe recorded:

| Observation | Value |
| --- | --- |
| Startup entries / returns | `1 / 0` |
| State-dispatch entries / returns | `0 / 0` |
| Manager / manager `+24` current-state pointer | `16023800 / 00000000`, readable |
| Live main-thread ID | `1` |
| LR / SP / r3 | `00016058 / 0FEFFA20 / 00002710` |
| Parent stack link | `0FEFFB00 +10 -> 000129D0` |
| Startup caller stack link | `0FEFFCF0 +10 -> 00011CB8` |
| Further saved return links | `00011400`, `0001C8B4`, `0001C648` |

LR `00016058` is the return link of the callback pump in the identified wait, and `r3=2710` matches its subsequent sleep syscall. This supports an occupied error wait; it is not a precise program-counter sample. CIA was `0`, so it provides no instruction-location evidence. The saved LR in the youngest frame (`00015DBC`) can be stale from a returned callee; it is not proof that the logging function is active.

The zero dispatch count is numerically sampled at 35 seconds. No first-entry record appears anywhere later in the completed 45-second log. Its linked guest function-table entry was verified to point to the wrapper, so indirect dispatch coverage is established. No new DXT entry/descriptor/DMA hooks were enabled.

## Separating branch/state

The **earliest proven occupied separator on this route** is the wait in the `00015948` startup operation, before `00011CE8` reaches `0001353C`.

| Condition | Storage / source | Behavior |
| --- | --- | --- |
| Pending/error byte nonzero | Main-memory byte `011B7A02`, loaded via `r25=011B0000` in the continuation | At `00016048` enter, and after each sleep repeat, `00016054`: callback pump plus 10 ms sleep. Zero leaves at `00016074`. |
| Current state exists | Manager `+24` | `000136F8` returns if zero. Constructor `00011A24` initializes it to zero (`ppu_recomp_000.cpp:2044`); `00010E84` later selects and stores it (`:1539`); dispatch updates it at `:4086`. The constructor value is still zero at the snapshot. |
| Candidate ancestor's object field exists | `000104AC` entry object `+4` | Zero returns before `002A9300` (`ppu_recomp_000.cpp:527–533`). Nonzero permits the later call. This guard was not sampled executing. |
| Candidate initialization path selected | Byte `012D8F38` | Nonzero in `002A9300` branches to `002A98D4`, bypassing `002A81EC`; zero takes the longer path toward it (`ppu_recomp_005.cpp:3628–3632,3921–3935`). Not reached in this experiment. |

The diagnostic did **not** directly read byte `011B7A02`: it was identified after the one permitted boot from the stack/LR and static code. Its nonzero wait condition is an inference from that correlation, not a recorded byte value or an assertion that it remained constant for all 45 seconds. No second probe or boot was added to strengthen that inference.

## Controlling-value producer

Static stores establish where the wait byte can become nonzero:

- In the main startup continuation, immediately before the dialog import call with return link `00016040`, the code stores **1** to `011B7A02` (`ppu_recomp_028.cpp:132531–132538`; overlapping unit 029 `:29561–29568`). Arguments are type `80`, a prepared message pointer, and zero callback/userdata/extra argument. The syscall loop then waits for this byte to become zero.
- `00018478`, the entry code of the log-named **Utility PPU Thread** (OPD `010A2098`), also stores **1** to this same byte before error-dialog calls: `ppu_recomp_000.cpp:9425,9610,9632,9753`. Return links include `00018D98`, `00018DFC` and `00018FC8`. These are multiple possible producer sites, not proof of which store executed.
- The byte lies in the ELF's zero-fill portion of the second PT_LOAD segment; it is not initialized as a nonzero file-backed datum. No direct zero store using the literal `+7A02` was found in the inspected lifted output. This search is not an exhaustive alias/write proof.

`00F25FEC` imports NID `189A74DA`, resolved by the pinned local NID tool as `cellSysutilCheckCallback`; `00F2610C` imports `7603D3DB`, resolved as `cellMsgDialogOpen2`. Original NID lookup commands are included under How to verify. No toolkit implementation was changed.

The runtime evidence separately shows the title reporting corrupt Game Data after file access under `BCUS01089_R`, followed by `cellMsgDialog` opening type `80` and reporting an automatic no-button response. The diagnostic does not uniquely attribute that dialog or the byte's last writer to one of the static sites above. A displayed corruption message is not proof that physical game content is corrupt: mapping, API results, validation inputs and title logic remain possible explanations. No files were deleted or repaired.

### Four requested globals

| EA | Snapshot | Proven static role / writer |
| --- | --- | --- |
| `0114E8EC` | `0`, readable | Object argument loaded into r3 for `002A85E8`. In `009E2CF8`, result of `00D9ACF8` is published here after return link `009E3740` (`ppu_recomp_018.cpp:111681–111684`); its alternate branch clears it (`:112019`). `009E45D8` initializes it to zero (`:112695`), and `009E28C0` clears it after an indirect destruction call (`:110732`). No constructor failure is inferred from this snapshot. |
| `01346088` | `0`, readable | Selected mode argument. `002A81EC` stores its computed mode at convergence `002A8568`, optionally overrides it with 5 (`ppu_recomp_005.cpp:2600,2618`), then reads it for the candidate. `009DDA00` also stores its input mode on one path (`ppu_recomp_018.cpp:105370`). Neither mode selection nor the candidate is required merely because this word exists. |
| `0113E618` | `0`, readable | Metadata pointer used by the candidate edge to read an unsigned halfword at `+5A`. `008CA788` publishes `[entry_r3 +0] +28` here (`ppu_recomp_016.cpp:94182,94224–94227`), after earlier initialization work. A zero pointer does not prove initialization was required or attempted during this window. |
| `01346070` | `6`, readable | Later request-loop bound, already mapped in TASK-019. `009E0158` computes it from two converted floats at static addresses `009E0150` and `009E0154`: single-precision division, integer conversion, minus 2 (`ppu_recomp_018.cpp:108438–108453`). No runtime hook was added to that path. A positive bound alone does not make a request obligatory. |

The four values show that some earlier initialization occurred while the candidate's object/metadata are absent. They do not prove a DXT guard or publication defect. The byte word at `012D8F38` was also read as zero; it cannot explain candidate non-execution when its enclosing path has not been reached.

## Runtime observations

Local-only artifacts: `F:/Games/MLB16PS3/build-mlb16-native-pinned/task020/`.

Log: `F:/Games/MLB16PS3/logs/mlb16-task020-boot-expectation-20261023T180828Z.log`.

| Log lines | Evidence |
| --- | --- |
| 1, 28 | Diagnostic armed; startup entry with caller LR `00011CB8`, manager `16023800`. |
| 2, 103, 119 | Canonical PM fingerprint `75BFE68AE96D7577`, image 77 registration/resolution. Workload-0 resolution at line 67 is interleaved, so its full text is not treated as intact. |
| 232 | D3D12 initialization completes. |
| 513 | Known async dispatch miss fingerprint `5917A156CE973C9B`, split across lines 512–513. |
| 518–528 | Content checks and local HDD/disc header/WAD accesses precede the error message. These accesses do not establish which comparison failed. |
| 529–533 | Corrupt-game-data dialog and no-button response. TASK-019 log line 534 already contained the same diagnostic message. |
| 33615 | Known unmapped FIFO jump to `10464648`; not a new regression. |
| 125883–125904 | Bounded 35-second counts, globals, live context and stack observations. |

Boot exited **124 after 45.813 seconds**, with empty shell stderr and `MLB16_TASK020_TRACE=unset`; no remaining runner process. There were **16** focused `[task020]` records. No TASK-018/019 or `[pm-pipe]` records appeared. No new fatal/assert/access-violation/exception text was found; absence of text does not prove correctness. Rendering/window contents were not independently inspected. Different bulk log volume and SPU scheduling compared with prior runs are not classified as fixes or regressions.

## Classification

**BOOT_SEQUENCE_NOT_REACHED.** The observed startup is occupied before state dispatch. An error wait is correlated with that occupied operation, and no candidate-state invocation with a proven DXT-work expectation exists. `DXT_EXPECTED_BUT_BLOCKED` would require a separately established work obligation; this evidence does not supply one.

This completes the DXT investigation exit gate. TASK-018's `NO_FAILURE_PROVEN` and TASK-019's `REQUEST_NOT_REACHED` are preserved. No finding is extrapolated to MeshJobs0 or MeshJobs1.

## What is not proven / uncertainties

- The exact producer invocation/last write or directly measured byte value at `011B7A02`; whether any alias later clears it.
- The exact failed content-validation comparison or whether its inputs, virtual paths, file contents, API results or title decisions are incorrect.
- That startup should leave this error state under the observed conditions, or should produce DXT requests while it persists.
- Continuous instruction location or duration at one loop: the live context/RAM/stack reads are asynchronous and can race execution. Saved LR slots may be stale; CIA was uninformative.
- Behavior beyond the bounded window, other boot conditions or user input; a cause of black rendering, whole-game liveness or playability.

## Files changed

- `cmake/mlb16-pm-integration.cmake`: default-OFF `MLB16_TASK020_DIAGNOSTIC`; one original source and two wrappers; refuses combining it with either prior probe.
- `src/dxtjob_boot_diagnostic.cpp`: original read-only boundary wrappers and one 35-second observer; first-two entry/return budgets, at most 12 stack rows. Requires `MLB16_TASK020_TRACE=1` in addition to the build option. No guest/context writes or DXT internals.
- `docs/TASK-020-DXTJOB-BOOT-EXPECTATION.md`: this report.

Configure/compile/link passed in the canonical pinned build. Only the project diagnostic source compiled; the PPU objects and generic runtime did not rebuild. The canonical PM source/startup glue remained included. No legacy toolkit or `tmp/task*` build input was used. Function-table coverage for both wrappers was verified in the diagnostic PE before boot.

After the single boot, all three task options were configured OFF and the canonical executable linked successfully again. Normal `build.ninja` excludes all three diagnostic sources and PPU wrapper flags. All **45 protected input hashes** match the pre-task manifest (31 prebuilt PPU/HLE objects, PM/startup source and header, both prior sources/reports, seven initially inspected generated units). Unit 028 was additionally inspected read-only after boot and its current hash was recorded separately. Byte-identical executable restoration is not claimed. The toolkit remains clean at its pin; no generic-runtime delta exists, so the generic-runtime regression gate was not triggered.

Diagnostics are reversible by keeping the option OFF, or by removing the new option block and source in a later authorized change. No source-control rollback, staging or commit was performed.

## Next smallest step

Investigate the **actively executing boot content-validation/error-dialog boundary**, starting with `00015948`'s `011B7A02` wait and the Utility PPU Thread's `00018478` producer. Correlate the specific validation result before the observed corrupt-game-data message with its local VFS inputs and the flag store. Establish whether that boot error is an expected response to the supplied content or a port boundary defect. Keep original content read-only; do not clear the flag, force success, delete data or force DXT submission.

Success is one proven validation-result -> error-state transition. That is the next independent task, not work implemented here. Do not continue tracing arbitrary DXT ancestors or re-enable its descriptor/DMA/eligibility probes. If that boundary cannot be correlated, reassess overall boot progress rather than returning to DXT internals.

## How to verify

PowerShell read-only checks: `git diff --check`, `git status --short`, `git diff --cached --name-only`, `git rev-parse HEAD`, and toolkit `git status --short` / `git rev-parse HEAD`.

Local artifact verification:

1. `build-results.json`, `preboot-build.log`, `build-gate.json`: configure/build exit 0 using `D:/APP/msys64/clang64/bin/cmake.exe`, source runner `F:/Games/MLB16PS3/mlb16-native`, canonical build `F:/Games/MLB16PS3/build-mlb16-native-pinned`, TASK-018/019 OFF, TASK-020 ON. Existing runner template was read-only.
2. `function-table-coverage.json`: guest entries `00011CE8` and `000136F8` resolve to their actual linked wrappers; local PE parsing and `llvm-nm` supplied the addresses.
3. `boot-command.json`, `boot-result.json`, `observations.json`: exact child command/environment policy, timeout/result, line-numbered focused records. MSYS2 CLANG64 Bash used `timeout -k 5s 45s`. Child environment removed inherited `SPU_`, `SPURS_`, `YDKJ_`, `LBP_`, `GH3_`, `MLB16_TASK*` variables before enabling only TASK-020 tracing.
4. `restore-results.json` and restore logs: all task diagnostics OFF; configure/build exit 0. Inspect canonical cache and `build.ninja` to verify normal exclusion.
5. `protected-before.json`, `protected-after.json`, `final-verification.json`: preservation and final repository/build gates. Full logs, ELF, derived disassembly and binaries stay outside Git.
6. Pinned local name lookup, without creating Python cache files: `py -3 -B third_party/ps3recomp/tools/nid_database.py --name cellSysutilCheckCallback` and the same command with `--name cellMsgDialogOpen2`.

No additional boot is needed to verify the saved investigation result. A future rerun requires a separate authorized controlled-boot task.
