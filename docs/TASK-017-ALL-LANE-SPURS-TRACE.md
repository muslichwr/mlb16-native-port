# TASK-017 — All-lane SPURS pipeline trace

Date: 2026-10-23. **Stopped after TASK-017.** One controlled boot; diagnostic change only. No runtime fix or playability claim.

## Summary

Exactly three coherent observations were captured for each of DXTJob, MeshJobs0, and MeshJobs1. All nine returned `jobHeader=0x00000000`, type `cmd=0`, and selected `jobIndex=0`. None had an active load/run/store index. **All three classify as EMPTY for these sampled selection results.** There is no sampled nonzero descriptor supporting REJECTED, and no sampled pipeline change supporting ADVANCING.

This establishes a shared empty-selection signature, not a shared underlying cause or proof that the PPU never publishes work. Post-run workload-memory reads are not DMA-time snapshots.

The isolated object/executable build passed. Four existing regression binaries rebuilt successfully; all 16 regression invocations passed with their required settings. The boot exited **124** through `timeout -k 5s 45s`, with measured shell duration **45.500 s**. Both diagnostic overrides were unset afterward.

## Evidence

### Scope and prior evidence

Read `AGENTS.md`, `docs/PROJECT_STATE.md`, `docs/TASK-001-SPU.md`, the existing audit, the current policy probe, and saved TASK-014 build/test/boot artifacts. No standalone TASK-014–016 findings documents exist under the repository's `docs/`; no TASK-015 artifact was found in the inspected `tmp`/log inventory. The saved TASK-014 policy-exit results and TASK-016 pipeline log were the available prior evidence. The older TASK-001 restrictions do not define this explicitly authorized TASK-017 diagnostic/build/boot assignment.

TASK-014's synthetic fixture demonstrates the interpreter's policy-exit contract. TASK-016's log (`logs/mlb16-task016-pipeline-20261023T135423Z-0096fa.log`, e.g. lines 138–149) shows lane-0 DXTJob's zero descriptor and post-run memory, but its first observation is interleaved. The current probe's lane-0 condition excludes Mesh observations when those workloads run on other lanes.

### Diagnostic change

Toolkit checkout: `F:/Games/MLB16PS3/ps3recomp-source`, HEAD `a6d76c895f77eb30b90a806c830544e61821b77f` (already locally modified).

Only `runtime/spu/spurs_policy.c` was edited for TASK-017:

- Remove the `spu_num == 0` gate from `SPURS_PM_PIPE`.
- Replace thread-local budgets with a shared atomic, saturating three-invocation budget per workload ID. Reserve the ordinal before PM dispatch. This run has one SPURS instance; the diagnostic budget is process-wide by ID, not a reusable per-instance accounting facility.
- Include `wid`, `spu`, and `spurs` in every record. Resolve workload names through registration evidence and workload data, rather than hardcoding game names into runtime source.
- Retain all existing probe fields and the ten workload-memory rows. Emit them on one line, holding the CRT locks for stdout and stderr only during emission. Both streams share the boot-log destination; this reuses the `_lock_file`/`flockfile` pattern already present in `runtime/ppu/ppu_fs.cpp`.
- Preserve the PM call, its arguments, all execution decisions, and existing repeated-DMA diagnostics. No DMA, queue, scheduler, PM, guard, RSX, or image-0004 change.

The source diff is separately preserved in `patches/spurs-pm-pipe-all-lane-diagnostic.patch` (35 insertions, 14 deletions). Reverse-apply dry-run succeeds in the toolkit checkout. Existing interpreter/loader changes were preserved.

### Build and regression results

All temporary artifacts are under `F:/Games/MLB16PS3/tmp/task017-all-lane`.

| Operation | Result |
| --- | --- |
| Compile `spurs_policy.c.obj` with saved runtime flags (`gnu17`, SSE4.1, no strict aliasing, FP contraction off) | Exit 0; one warning on existing `s_flow_arm_ms`, set but unused |
| Link `MyGameRecomp-all-lane.exe` | Exit 0; no diagnostics |
| Rebuild existing `exit`, `machine`, `handoff`, `irq` CPU regression binaries | Four exit-0 builds; no compiler diagnostics |
| Policy exit regression | Exit 0: direct service, full interpreter, ordinary non-policy behavior |
| Machine-status regression | Exit 0: 14 cases |
| Interrupt round-trip regression | Exit 0: handler, continuation, register restore, event/list acknowledgment |
| Interpreter handoff | 13 exit-0 modes: `top`, `top-zero`, `nested-return`, `nested-return-unlifted`, `nested-calls`, `stop-zero`, `stop-code`, `halt`, `nested-stop`, `smc`, `pure-interpreter`, `stepcap`, `negative-stepcap` |

The first `stepcap` invocation failed with exit `3221226505` because the test command supplied `SPU_STEPCAP=2` without `YDKJ_SPU_TRACE`. Source inspection confirmed the cap condition requires both. With `SPU_STEPCAP=2 YDKJ_SPU_TRACE=64`, both cap modes pass; all 16 invocations were then run successfully. The failed invocation is retained as `handoff-stepcap-missing-trace-result.txt`. No runtime source was changed to address this command error.

Build command arrays and working directory are saved in `build-commands.json`; regression compilation arguments are in `regression-builds.json`; exact test arguments, overrides, and statuses are in `regression-results.json`. The experimental link reuses TASK-014's explicit interpreter object, the existing PM/startup objects, the existing runtime archive, runner objects, and all 30 PPU objects. It adds the new policy object before the archive, overriding only that archive member. The archive itself was not rebuilt or replaced.

SHA-256 plus size and modification-time checks passed for all **34 protected files**: all 30 PPU objects, `build-mlb16-native/MyGameRecomp.exe`, the TASK-014 experimental executable, the original runtime archive, and `spu_interp.c`. Baseline and final manifests are `protected-before.json` and `protected-after.json`. The new executable's SHA-256 is `a018a720bbce1b6b1d6d83a5a5ee6694341de66b94e72247cda9ec237792403b`.

The broad upstream `run_tests.sh` was not invoked: it runs the lifter and writes generated fixtures into the toolkit, conflicting with this task's no-new-lifting scope. The existing CPU regressions above were used instead. They do not establish whole-game correctness; the boot additionally verifies observation count, lane coverage, and record completeness.

### Controlled boot

Full local log: `F:/Games/MLB16PS3/logs/mlb16-task017-all-lane-20261023T140321Z-c4c808.log`.

The following **MSYS2 CLANG64 Bash** body was executed once through `D:/APP/msys64/usr/bin/bash.exe --noprofile --norc -c`. This records the completed command; it is not a request for another boot.

```bash
cd /f/Games/MLB16PS3/build-mlb16-native || exit 2
export PATH=/clang64/bin:/usr/bin:$PATH
export MSYSTEM=CLANG64
export TEMP=F:/Games/MLB16PS3/tmp/task017-all-lane TMP=F:/Games/MLB16PS3/tmp/task017-all-lane
export PS3_VFS_ROOT='F:/Games/MLB16PS3/game/MLB The Show 16 (USA)'
export SPURS_PM_PIPE=1
export SPURS_PM_TRACE=1
timeout -k 5s 45s /f/Games/MLB16PS3/tmp/task017-all-lane/MyGameRecomp-all-lane.exe 'F:/Games/MLB16PS3/game/EBOOT.ELF' > /f/Games/MLB16PS3/logs/mlb16-task017-all-lane-20261023T140321Z-c4c808.log 2>&1
boot_rc=$?
unset SPURS_PM_PIPE SPURS_PM_TRACE
printf 'boot_exit=%s SPURS_PM_PIPE=%s SPURS_PM_TRACE=%s\n' "$boot_rc" "${SPURS_PM_PIPE-unset}" "${SPURS_PM_TRACE-unset}"
exit "$boot_rc"
```

Inherited `SPU_`, `SPURS_`, `YDKJ_`, `LBP_`, and `GH3_` overrides were removed from the child environment before starting the shell. The shell returned `boot_exit=124 SPURS_PM_PIPE=unset SPURS_PM_TRACE=unset`, with empty shell stderr. `boot.sh`, `boot-command.json`, and `boot-result.json` preserve the exact invocation and result. No experimental process remained afterward. Timeout is not proof of a crash or successful rendering.

### Exactly the first three observations per workload

The shared ordinal is reserved at invocation entry across lanes. All nine records are complete single lines, each containing all ten `jl+00` through `jl+90` rows. No additional `[pm-pipe]` record exists. Full extracted records remain local in `tmp/task017-all-lane/observations.json`; the table below summarizes them without copying the bulk boot log into Git.

**Fields common to every row:** SPURS instance `01233C00`; descriptor `jobHeader=00000000`, type `cmd=0` (empty under the existing probe convention); selected `jobIndex=0`; `loadState=0->0`; `nextLoad=0`; `loadJob=runJob=storeJob=FFFFFFFF`; `lastStore=2`; halt PC `009C0`. Thus neither descriptor/index nor pipeline state changes across the three observations of any workload. `first=1` means a lane's resident context was initialized; a different lane can legitimately have `first=1` at a later workload-wide ordinal.

| Workload / ID | Observation | SPU lane | first | eaWL | Log line | Classification |
| --- | ---: | ---: | ---: | --- | ---: | --- |
| DXTJob / 0 | 1 | 0 | 1 | `16023900` | 93 | EMPTY |
| DXTJob / 0 | 2 | 1 | 1 | `16023900` | 118 | EMPTY |
| DXTJob / 0 | 3 | 1 | 0 | `16023900` | 147 | EMPTY |
| MeshJobs0 / 1 | 1 | 0 | 1 | `16034B00` | 116 | EMPTY |
| MeshJobs0 / 1 | 2 | 0 | 0 | `16034B00` | 137 | EMPTY |
| MeshJobs0 / 1 | 3 | 0 | 0 | `16034B00` | 163 | EMPTY |
| MeshJobs1 / 2 | 1 | 2 | 1 | `16040B00` | 140 | EMPTY |
| MeshJobs1 / 2 | 2 | 2 | 0 | `16040B00` | 177 | EMPTY |
| MeshJobs1 / 2 | 3 | 1 | 1 | `16040B00` | 197 | EMPTY |

**Workload-memory fields already exposed by the probe:** values below are identical across observations 1, 2, and 3 for that workload. Offsets are from `eaWL`; values are big-endian words. The known workload-name/data mapping is consistent with registration in this log (e.g. line 92 for MeshJobs0), prior registration evidence, and all nine `eaWL` values. Some registration lines remain interleaved because only the requested pipeline probe was changed.

| Workload | `+1C` | `+2C` | `+3C` / correlated DMA source | `+80..+9F` |
| --- | --- | --- | --- | --- |
| DXTJob | `00000000` | `0000020F` | `16023980` | Eight zero words |
| MeshJobs0 | `00000001` | `000017EF` | `16034B80` | Eight zero words |
| MeshJobs1 | `00000001` | `000017EF` | `16040B80` | Eight zero words |

Other common rows: `+00` is `01057600 00002F00 00000003 FFFFFFFF`; `+10`, `+20`, and `+30` each have three initial zero words before their differing fourth word above; each of `+40`, `+50`, `+60`, and `+70` is `00000000 FFFFFFFF FFFFFFFF FFFFFFFF`. The nonzero Mesh `+1C` and `+2C` values are workload metadata in this probe, not a nonzero selected descriptor. Their presence alone cannot justify REJECTED.

**Repeated-DMA correlation:** the unchanged diagnostics contain at least 26 clean `cmd=40, lsa=00D00, ea=16023980, size=80, tag=8` lines, 28 for `ea=16034B80`, and 29 for `ea=16040B80` (other DMA messages are interleaved). First clean examples are lines 77, 128, and 130 respectively; repeated examples follow at 135, 131, and 133. These addresses match the workload-specific `+3C` values. Repeated fetching plus unchanged empty selected results is observed; exact contents at each DMA instant are not captured by these post-run records.

## Uncertainties

- EMPTY describes the first three returned selections per workload, not every queue entry or later dispatch. This sampling provides no nonzero descriptor whose claim or pipeline transition can be identified as failing.
- The RAM rows are read after each PM invocation and can differ from earlier DMA-time contents. Even within a record they are ordinary sequential reads, not an atomic guest-memory snapshot. Stream locks protect output coherence only.
- The probe reports its existing cached/result fields after PM return; it does not instrument the exact instruction that selects or rejects a descriptor. A zero result does not prove why the producer/consumer arrived at emptiness.
- The cap is workload-wide, not three observations per lane. MeshJobs0's first three invocations all happened on lane 0; other workloads demonstrate observation on lanes 1 and 2. Removing the restriction does not force scheduling onto every lane.
- The underlying cause of emptiness, later publication behavior, image-0004 execution, and rendering remain unresolved. No shared root cause or runtime fix was confirmed.

## Recommended next smallest step

Perform a **read-only inspection of the existing PPU producer/publication path for the first MeshJobs0 descriptor**, following its workload-data pointer to the region fetched at `eaWL+0x80`. Identify the actual descriptor location and publication condition; compare that location with the observed zero selected result. This is justified by MeshJobs0's three empty selections despite nonzero workload metadata. It requires no runtime fix, fresh lift, or additional boot. Do not assume the metadata words themselves represent available work.

## How to verify

- Inspect `tmp/task017-all-lane/build-commands.json`, build diagnostics, `regression-build-results.json`, and `regression-results.json`: two experimental build exits and four regression-build exits are 0; all 16 final test exits are 0. The cap modes explicitly set both required variables.
- Parse the existing unique log, selecting `[pm-pipe]`: require exactly nine full lines, per-ID ordinals `{1,2,3}`, one prefix per line, and ten complete `jl+XX` groups per line. Compare the fields with the table above. This check passed.
- Inspect `boot-result.json` for exit 124 and environment cleanup. Compare the protected manifests for all 34 unchanged files. These checks passed.
- **PowerShell**, diagnostic rollback dry-run (passed): `git -C F:/Games/MLB16PS3/ps3recomp-source apply --check --reverse F:/Games/MLB16PS3/mlb16-port/patches/spurs-pm-pipe-all-lane-diagnostic.patch`. `git diff --check -- runtime/spu/spurs_policy.c` also passed, with Git's CRLF conversion advisory.
- To roll back only this diagnostic in a future authorized operation, use the same command without `--check`, then select the preserved TASK-014 executable. No rollback or rebuild was performed after this run. Do not reverse the independent interpreter patch or overwrite unrelated toolkit edits.

Tooling limitations/errors: the initial reverse-check attempt used `--directory=../ps3recomp-source`, which Git rejected as an invalid path; the corrected `git -C` command above passed. A PowerShell `rg` attempt with literal `*.json` path arguments was rejected; source inspection and Python log parsing supplied the needed evidence. Neither error altered runtime files.

Checking the patch as a newly added text file reports its single-space blank context line as trailing whitespace; that space is unified-diff syntax. The applied runtime source and this report pass their whitespace checks, and the patch passes Git's reverse-application check.

## Files changed

- Edited externally: `F:/Games/MLB16PS3/ps3recomp-source/runtime/spu/spurs_policy.c` — temporary diagnostic only.
- Added in this repository: `patches/spurs-pm-pipe-all-lane-diagnostic.patch` and this report.
- Created local-only artifacts: the `tmp/task017-all-lane` baseline, manifests, object, isolated executable, regression executables, command/result files, diagnostics, and extracted observation records; one unique log under `logs`. The linker was given an import-library output option but did not create an import library.

No PPU object rebuild, game-code generation/lifting, game-file change, modification of the successful build, staging, commit, push, download, or external upload was performed. No game bytes, generated game code, binaries, or bulk boot logs were added to this repository.
