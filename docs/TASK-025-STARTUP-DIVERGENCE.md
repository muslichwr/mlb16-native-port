# TASK-025 - Saved TASK-023/024 startup divergence

## Summary

**Classification: STARTUP_DIVERGENCE_UNRESOLVED.** The saved runs share substantial startup activity, but their observers do not record the same guest call/return boundaries. They cannot establish the exact last shared completed startup call or the first execution divergence **before** dispatcher entry.

A common guest completion **lower bound** is recoverable: startup progresses past `00DB3DD4` at return link `000127C4` to the next semaphore-helper call at `000127EC`. Later matching file-operation results and startup requests are present in both runs, without sufficient thread/caller/return information to promote them to a later paired main-thread guest boundary.

The strongest comparable execution difference remains the dispatcher boundary itself: TASK-023 records entry once; TASK-024 records zero entries throughout its bounded run. Earlier TASK-023-only messages are candidate observations, not proof of the first absent execution in TASK-024. Neither guest control flow nor asynchronous timing/state is established as the cause.

**BOOT TEST: TIDAK**

**TASK-025: COMPLETE** (saved-artifact review complete; localization remains unresolved).

## Compared artifacts

Only saved evidence under `F:/Games/MLB16PS3/build-mlb16-native-pinned/task023/` and `task024/` was used to analyze execution:

- Both complete `boot.log` files and their `observations.json` extracts.
- `boot-started.json`, `boot-result.json`, `seeds.json`, both isolation gates, original before/after manifests and preservation records.
- `entry-context.json`, `build-gate.json`, `function-table-coverage.json`, protected-input manifests and final-verification records.
- TASK-023 `timing-controls.json`, `static-callback.json` and saved callback-forwarder/containing-path disassembly.
- TASK-024 `static-dispatch.json` and saved `00011CE8-linked-disassembly.txt`, including its call/continuation sequence. No new disassembly, build or generated-source inspection was performed.

Read first: `AGENTS.md`, historical `docs/PROJECT_STATE.md` checkpoint note, and the TASK-023/024 reports. No execution evidence from TASK-020, TASK-022R or another boot was substituted for the two compared runs.

### Normalization and evidentiary limits

Comparison keys are guest return link, guest thread ID plus entry OPD/name, API operation and stable arguments/results, virtual pathname, and observer boundary/counter identity. Experiment prefixes `task023`/`task024` are normalized to `<RUN>`. Absolute timestamps, host thread IDs, raw line positions and output volume are not progression keys. Repeated waits are compared by signature, not frequency.

File-descriptor numbers are transient handles, not universal object identities. Matching read results are kept as operation evidence with their printed fields; neither a global byte counter nor the first four bytes proves a guest control-flow decision. No file offset is inferred.

Ordinary stdout/stderr records are frequently interleaved inside tags, paths and numeric fields. Only intact result tuples are treated as exact matches. Removing a complete literal background message can help inspect context, but damaged fields are not guessed into completed records. Missing ordinary messages do not prove non-execution: the saved logs do not establish exhaustive, lossless guest-call coverage. The protected observer records and their validated hooks have stronger absence evidence.

Line references below are locators for verification, not an alignment between runs. Within-run observation order does not create a happens-before relationship between unrelated guest threads.

## Last shared boundary

**The exact last shared completed guest startup boundary is not recoverable.** Distinguish these levels of evidence:

| Level | Shared evidence | What it establishes |
| --- | --- | --- |
| Startup entry | Both observers log `00011CE8` entry once. TASK-024 additionally records thread 1, caller LR `00011CB8`, object `16023800`. | Common containing startup entry; neither observer records its return. TASK-023's entry record does not contain those extra fields. |
| Completed guest-call lower bound | Both logs report semaphore creation at LR `000127AC` (id 5, destination `169048D0`) and then LR `000127EC` (id 6, destination `169048F0`). | Saved startup disassembly places `009A1A08` at `127AC`, then `00DB3DD4` at `127C4`, then `009A1A08` at `127EC`. Reaching the latter call on this route requires the earlier calls and their trampoline drains to complete. Thus `127C4` is a common completion lower bound, inferred from continuation rather than a paired return log. |
| Later completed operations | Both logs have matching SYS/ROM and installed GD/EXT header-read results, cache-mount readiness, and the SYS-header 16-byte read with printed first word `7CCA8590`. | Common operation results later in the recorded startup activity; no paired return of their enclosing guest function or reliable main-thread identity. |
| Still later common activity | Both contain the intro-file request, audio-out query and subsequent matching read results. | TASK-024 is not proven to stop before the intro-file request. These messages do not locate an enclosing guest return. |

The semaphore records are at TASK-023 lines 466-467 and TASK-024 lines 465-466. The saved TASK-024 startup disassembly has the corresponding direct calls at lines **8161, 8210 and 8317**. The drains between them are present in the saved disassembly. This does **not** establish return from the second semaphore helper at `127EC`, nor claim that `127C4` was the true last shared completed call.

An intact late read-result match is:

| Field | TASK-023 | TASK-024 |
| --- | --- | --- |
| Printed fd | 7 | 7 |
| Requested / returned bytes | 65536 / 65536 | 65536 / 65536 |
| Printed first word | `CCA3667A` | `CCA3667A` |
| Printed cumulative total | 2510592 | 2510592 |
| Locator | line 1335 | line 50677 |

Later read text with first word `2DAC6A9B` is also present in both logs, but TASK-024's record is split/interleaved. It is not promoted to a fully intact paired tuple. None of these read records carries a guest caller LR, thread ID or enclosing-function return. Their order relative to other threads cannot identify the last shared main-thread return.

## First divergence

**No first pre-dispatch execution divergence is proven.** Three kinds of differences must remain separate:

| Observation | TASK-023 | TASK-024 | Interpretation |
| --- | --- | --- | --- |
| Selected flip/path/callback return records | 50 entries and 50 returns at both summaries; first three explicitly paired. | That probe was OFF. | Different observer coverage. Absence in TASK-024 cannot establish a missing flip, callback or containing-path return. |
| Ordinary syscall-caller message | `flag=100`, function text `0005F120`, LR `0005F160`, recorded before TASK-023 dispatch; one intact occurrence at line 808. | No recoverable corresponding signature. | Candidate message difference. It has no guest thread identity or paired return and is not a complete common execution trace. The printed native function offset/argument labels do not establish an earlier startup branch. |
| Ordinary late API request | `cellHttp CreateClient(id=0)` at line 34500, followed by unresolved-NID text `BDC07FD5`, before dispatch. | No recoverable `CreateClient`/`BDC07FD5` signature. | A concrete TASK-023-only recorded request, but no caller, returned result or complete absence guarantee. It does not justify choosing an HTTP subsystem or calling it the first execution divergence. |
| Dispatcher entry | Dedicated observer entry once; both summaries retain count 1. | Dedicated observer count 0 at 35 and 170 seconds; no entry anywhere in the complete output. | Comparable instrumented progress difference at dispatcher entry, not localization to an earlier branch. Both saved function-table coverage records validate the dispatcher hook. |

The intro request is not a valid missing-boundary candidate: TASK-023 has an intact `/dev_bdvd/PS3_GAME/USRDIR/PS3/UI/Movies/intro/MLB_INTRO.BIK` open record; TASK-024 contains the same filename and fd result in interleaved text at lines 49845-49847. Similarly, incomplete audio, module or thread-creation tags cannot establish skipped initialization merely because an intact tag search misses them.

TASK-024's additional caller-edge, dispatcher-return and continuation probes were not present in TASK-023. Its zero continuation count therefore cannot be combined with TASK-023's entry to construct a cross-process entry/return pair.

## Thread / state correlation

### Shared setup

The two saved `original-before.json` manifests are exactly equal. The two saved 45-input protected manifests are exactly equal. After normalizing only the experiment-root name, `seeds.json`, `launch-isolation-gate.json` and `boot-started.json` are exactly equal. Both use the same disc/installed content baseline, equivalent isolated mount design, the same entry ELF argument, 505 held original handles and a 180-second fail-safe.

This controls those recorded inputs; it does not prove every environment variable or asynchronous state was identical. The instrumented executable hashes differ, as expected for different observers. TASK-023 saves an explicit GCM-variable record; TASK-024 does not save an equivalent complete environment snapshot. Neither report authorizes attributing the divergence to observer overhead or timing alone.

### Waits and callbacks

- Both logs contain the timer signatures `200 us / LR 00067C54 / CIA 0` and `30 us / LR 00064E68 / CIA 0`. These are wait invocations, not paired completions or precise PC samples; these records do not identify the guest thread or a final active wait.
- Both contain main-worker semaphore wait/post activity using the corresponding ids 5/6. Their interleavings differ. A post record is not proof that a particular waiter resumed; the records do not identify a separating main-thread predicate.
- The recurring queue wait explicitly identifies **thread 8**, entry OPD **`010C0E48`**, LR **`00531B68`**, queue 2 and timeout 66666. TASK-024 names this `GEM PPU Thread`; TASK-023 records thread 8 starting at that same OPD. It must not be assigned to the main startup thread or used to diagnose its occupied boundary.
- Both register sysutil callback slots 0 and 1 at `010A2130` and `010C48A8`. TASK-023 proves selected synchronous flip callbacks return. TASK-024 has no equivalent callback-return probe. There is no common recorded callback outcome or flag read immediately before `1353C` that explains the split.
- Saved lightweight-mutex records show different worker/owner interleavings, including TASK-023's printed owner 1 at mutex `01234E78` and other owners in TASK-024. They are not synchronized observations of the same startup decision and supply no main-thread stack at the separation.

Both runs have startup entries/returns 1/0 at their two summaries. Neither saves the live main-thread LR/stack/state needed to identify its occupied earlier call at those points. TASK-023's selected flip records and TASK-024's dispatcher state fields do not fill that gap: the former cover a different boundary, and the latter were never captured because dispatch was not entered.

Both saved termination records are **FAIL_SAFE_TIMEOUT**, approximately 180.7 seconds. This defines the observation windows; it is not evidence of a natural exit, crash, permanent deadlock or a timing-dependent root cause.

## Classification

**STARTUP_DIVERGENCE_UNRESOLVED**

The exact last shared completed guest boundary and the first earlier divergent execution edge are not observable in these saved artifacts. There is a common completion lower bound and substantial later common operation evidence. There is a validated difference at dispatcher entry. The interval between those facts remains unlocalized.

No controlling guest value, branch outcome or callback/wait completion is recorded comparably at the separation. Differences in asynchronous interleaving are real observations, but do not establish timing dependence or causality. A late TASK-023-only API message does not uniquely identify a skipped guest branch in TASK-024.

## What is not proven / uncertainties

- Completion/non-completion of `000117F4` at startup return links `00013220`, `00013330` or `00013340` in both runs. These are present in the saved startup map, not paired runtime evidence.
- The actual caller or return of TASK-023's dispatcher invocation; its entry-only record remains insufficient to attribute it retrospectively.
- That the main thread waits on the prominently printed thread-8 queue, a callback, or a particular mutex.
- Whether a guest condition, missing completion, asynchronous state, incomplete logged environment, observer overhead or another input explains the split.
- A defect in HTTP, GCM/RSX, DXT/SPURS, filesystem semantics or any other downstream subsystem. No such investigation or fix was undertaken.
- Validation results or every file byte from matching short read prefixes alone; this task relies on the saved input/audit records for content comparability and does not rerun validation.

## Next smallest step

The saved-only exit is reached. A separately authorized follow-up should pair **startup's own call and continuation boundaries**, starting with `00011CE8 -> 000117F4`, discriminating its call sites by LR (`13220`, `13330`, `13340`), and relating their completion to the existing `1353C` dispatcher edge. This tests a known containing startup transition without selecting a descendant subsystem from a missing ordinary log message. It would need new runtime evidence; no probe, rebuild or boot is added in TASK-025.

Do not force a guard, return, callback or dispatch result. An exact separating predicate should be selected only after paired main-thread evidence narrows the interval.

## How to verify

Read both saved observation extracts and validated dispatcher function-table coverage. Match the two `127AC`/`127EC` semaphore records to the saved startup disassembly call sequence. Search complete logs for the exact read-result tuple above, the intro filename fragments, explicit thread-8 queue signature and TASK-023-only candidate messages. Compare normalized setup JSON and the saved original/protected manifests. Do not align by line count, wait frequency or timestamps, and do not convert damaged ordinary messages into negative execution evidence.

All analysis commands were read-only PowerShell/Python reads, comparisons and hashing. No game process, replay executable, compiler, linker, disassembler or boot harness was run. Original content was not accessed for fresh validation or modified. Prior artifacts and build inputs are preserved; all prior diagnostics remain OFF. No staging, commit or push.

## Files changed

Only `docs/TASK-025-STARTUP-DIVERGENCE.md` is added by TASK-025. The existing uncommitted investigation delta is retained.
