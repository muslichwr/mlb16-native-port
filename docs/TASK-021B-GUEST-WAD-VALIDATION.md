# TASK-021B - Actual guest WAD-length validation

## Summary

**Classification: GUEST_WAD_LENGTH_MISMATCH_CONFIRMED.**

One controlled, 45-second canonical boot measured the running guest's HDD WAD length as **463,339,520** and disc WAD length as **4,915,365,438**. The values match the offline differential. The equality test fails, the validator's flag changes from 1 to 0, `00016D54` returns 0 to `00018E88`, and the same guest thread executes the corrupt-Game-Data dialog call with return link `00018FC8`.

The experiment explicitly mounted the existing short native HDD copy. This establishes the selected native installation's length mismatch, not physical corruption of the complete original content. The user's successful RPCS3/RTTS run remains the control. No semantic fix or data repair was performed.

**BOOT TEST: YA**

**TASK-021B: COMPLETE**

## Evidence

Checkpoint: `2ebaa0a2cf98e02964fabf3a966e20f586fcde8b`. Toolkit unchanged and clean at `bc5b3d30503169a025c88bd9e6195728fc8ad68d`.

The local experiment directory is `F:/Games/MLB16PS3/build-mlb16-native-pinned/task021b/`. Raw boot output remains there, outside Git, in `boot-20261023T184942Z.log`. All ten diagnostic lines concern invocation **id=1**, guest **tid=7**. Boot duration recorded by the harness was **45.547 seconds**, with normal timeout exit **124**. There was no second boot.

| Log line | Observation |
| --- | --- |
| 513 | Validator entry, caller return link `00018E88`; explicit disc and HDD roots recorded. |
| 514, 516 | Resolved HDD/disc header opens both succeed. |
| 518-519 | Existing filesystem logging records two requested 16-byte header reads, each returning 16. |
| 520-522 | Disc WAD host open succeeds; `0007C174` returns `4915365438` at `00016FD8`. |
| 523-525 | HDD WAD host open succeeds; `0007C174` returns `463339520` at `00016FF0`; saved `r25=4915365438`, `r27=1`, equality false. |
| 526 | Actual downstream mismatch/cleanup edge at `0001702C`, `r27=0`. |
| 527 | `00016D54` returns 0 to `00018E88`, both lengths captured. |
| 528 | Caller successfully reopens the existing HDD WAD, before its result-dependent error branch. |
| 529 | Same-thread `00018FC8` dialog call; recorded validator result 0, error byte `011B7A02=1`. |
| 530, 532 | Existing dialog implementation logs the corrupt-Game-Data message. |

`observations.json` contains the ten bounded probe records and their line numbers. Static references below are to the unchanged local `F:/Games/MLB16PS3/recompiled-bounded/ppu_recomp_000.cpp`; generated code was neither edited nor copied into this repository.

## Resolved paths

Both child-process mount variables were set explicitly, for this process only:

```text
PS3_VFS_ROOT=F:/Games/MLB16PS3/game/MLB The Show 16 (USA)
PS3_HDD0_ROOT=F:/Games/MLB16PS3/game/MLB The Show 16 (USA)
```

Here `D` denotes exactly `F:/Games/MLB16PS3/game/MLB The Show 16 (USA)`.

| Guest virtual path | Actual resolved host path |
| --- | --- |
| `/dev_hdd0/game/BCUS01089_R/USRDIR/MLBPS3_GD.WAD.HDR` | `D/game/BCUS01089_R/USRDIR/MLBPS3_GD.WAD.HDR` |
| `/dev_bdvd/PS3_GAME/USRDIR/MLBPS3_GD.WAD.HDR` | `D/PS3_GAME/USRDIR/MLBPS3_GD.WAD.HDR` |
| `/dev_bdvd/PS3_GAME/USRDIR/MLBPS3_GD.WAD` | `D/PS3_GAME/USRDIR/MLBPS3_GD.WAD` |
| `/dev_hdd0/game/BCUS01089_R/USRDIR/MLBPS3_GD.WAD` | `D/game/BCUS01089_R/USRDIR/MLBPS3_GD.WAD` |

These are observed host `open` arguments correlated with the existing virtual-path open records, not merely paths predicted from configuration. Each host open uses flags `0x8000` (`O_BINARY | O_RDONLY`) and returns a valid descriptor (7 or 8); guest file descriptors in the adjacent filesystem records also indicate success. Nonzero logged `errno` after a successful open is retained host state, not an open failure. The probe preserves it.

The RPCS3 installed WAD at `F:/Games/MLB16PS3/rpcs3emu/dev_hdd0/game/BCUS01089_R/USRDIR/MLBPS3_GD.WAD` was protected and hashed as a comparison source; it was **not mounted into this boot**.

## Guest lengths and validation result

| Operand | Boundary | Decimal | Hex |
| --- | --- | ---: | --- |
| Disc WAD length | `0007C174` return at `00016FD8` | 4915365438 | `0x124FA863E` |
| HDD WAD length | `0007C174` return at `00016FF0` | 463339520 | `0x1B9E0000` |
| Saved disc operand | `r25` at HDD length return | 4915365438 | `0x124FA863E` |

The original instructions at lines 7542-7544 compare the full 64-bit `r3` and `r25`, preserve `r27` only on equality, and otherwise assign `r27=0`. Immediately before those instructions, the live operands differ and **`r27=1`**. The actual subsequent cleanup call at return link `0001702C` observes **`r27=0`**. This distinguishes the WAD-length rejection from a failure already accumulated during the header checks.

The recorded `equal_to_r25=0` at the HDD return is an observation of those operands, and the next guest boundary confirms the executed mismatch path. The disc-return record's older `r25` is not yet the saved disc operand. Likewise, the cleanup record's full CR snapshot includes later comparisons and is not presented as the original length comparison's CR field.

The final WAD predicate uses whole-file length metadata; it requests no WAD payload offset or byte range. This probe did not add read/seek/stat tracing or repeat the standalone filesystem replay. Its measurement is the actual length returned through the guest device/helper layer to the validator.

## Error-path correlation

`00016D54` returns **0** to `00018E88`. The caller saves that result in `r29`, reopens/closes the installed WAD, then tests the saved result (lines 9693-9695). The zero-result branch selects `00018F64`.

The title itself writes error byte `011B7A02=1` and calls `00F2610C` (`cellMsgDialogOpen2`) with return link **`00018FC8`**. At that call the observer sees the same completed validation invocation, result 0, and flag 1. The original dialog receives:

```text
Game Data corrupt. Please exit the game and delete this Game Data.
```

The observer does not alter the comparison, return value, cleanup, error byte, or dialog arguments. The wording is the title's response to this installed/disc mismatch; it is not an independent diagnosis of damage to the disc or RPCS3 installation.

## Protection and build verification

`tools/run_task021b_boot.py` held **354 Windows handles**, covering every existing file and directory beneath `F:/Games/MLB16PS3/game` and the existing RPCS3 `dev_hdd0/game/BCUS01089_R` installation. Handles use `OPEN_EXISTING`, `GENERIC_READ`, and `FILE_SHARE_READ` only. Incompatible write/delete opens therefore cannot modify those original files; directory handles also prevent their removal/renaming. No file attributes or ACLs were changed.

Before boot, access-only `OPEN_EXISTING` checks for write and DELETE access on all three WAD copies failed with Windows **32 (`ERROR_SHARING_VIOLATION`)**. These checks did not request creation, truncation, writing, or deletion. Ordinary reads and full hashes succeeded under protection. A separate protection-only check passed before the boot; it launched no game process.

Protection remained held until the timed-out child was gone and after verification. Before/after inventories (paths, sizes, mtimes) match across all 354 entries, and full SHA-256/size/mtime comparisons match across all six selected WAD/header files. No new files appeared in the protected trees. Holding directory handles is not a blanket prohibition on creating new children; the inventory comparison separately verifies none were created in this run. No game data was copied, replaced, truncated, deleted, or repaired.

The single project diagnostic is `src/wad_validation_diagnostic.cpp`. The default-OFF CMake option is **`MLB16_TASK021B_DIAGNOSTIC`**; logging additionally requires **`MLB16_TASK021B_TRACE=1`**. It excludes simultaneous TASK-018/019/020 diagnostics. Its four guest-function wrappers and selected host-open observer form one validation probe: the first two validations, at most eight matching opens per validation, and first two matching dialog calls. Every original is forwarded exactly once; guest memory and context are only observed. In this boot only one validation produced ten diagnostic lines.

Canonical configure/build/link passed with the probe ON. Linked function-table pointers for all four guest boundaries resolve to their wrappers; disassembly confirms the original validator's direct length and cleanup calls reach the wrappers. No prebuilt PPU object was recompiled. PM source/image 77 integration remains present: log line 1 verifies registration; line 97 resolves image 77. D3D12 initializes. There are no fatal/assert/crash markers in the captured log. The known RSX FIFO-jump message was not recorded in this window; that absence is not a fix claim.

After the sole boot, canonical configure/build/link passed again with **all four task diagnostics OFF**. The normal target excludes their diagnostic sources/wrappers. Protected original build inputs match the prior 45-file manifest; the toolkit remains clean at its pin. No generic runtime changes were made, so generic-runtime synthetic regressions were not applicable. No legacy build/runtime or `tmp/task*` inputs were used.

The restored executable was freshly relinked, with SHA-256 `ced9105352a3b46e9dedfeb9cc6ad20ecdc0666a3cbe91a22ccf700475854de5`. It is not byte-identical to the earlier executable fingerprint; verification here establishes the normal input/option set and successful link, not binary identity with the pre-experiment file.

Artifacts in the local experiment directory: `build-results.json`, `build-gate.json`, `function-table-coverage.json`, `validator-linked-disassembly.txt`, `protection-check.json`, `boot-started.json`, `boot-result.json`, `observations.json`, `data-before.json`, `data-after.json`, `content-before.json`, `content-after.json`, `preservation.json`, `restore-results.json`, and `final-verification.json`. Full logs and linked disassembly stay outside Git.

## Classification

**GUEST_WAD_LENGTH_MISMATCH_CONFIRMED**

The actual guest values match [the earlier offline differential](BOOT-VALIDATION-DIFFERENTIAL.md), and the actual rejection/result/dialog chain is now captured.

## What is not proven / uncertainties

- What produced the short native installed copy, or why its contents end at this length.
- Physical corruption of the complete original game data. The prior full audit instead found identical disc/RPCS3 WADs, identical headers, and the native HDD file to be their exact prefix.
- That an intact installed-data mount would reach gameplay, fix rendering, or resolve every other native runtime issue. No alternate-mount game boot was run.
- An actual RPCS3 syscall/register trace. Successful RPCS3 gameplay is the user's control; its existing installed files are the read-only byte comparison source.
- Later cleanup/install behavior without protection. Original-file writes/deletes were prohibited for this experiment; no validation or return was forced. No data-changing cleanup failure is recorded.
- Anything new about DXT, DMA, queues, descriptor publication, or scheduler semantics. Their diagnostics remained OFF and none of those paths was investigated.

## Files changed

TASK-021B changes only:

- `cmake/mlb16-pm-integration.cmake`: default-OFF, mutually exclusive validation diagnostic integration.
- `src/wad_validation_diagnostic.cpp`: bounded project observer; no semantic fixes.
- `tools/run_task021b_boot.py`: protection, single-boot guard, explicit mounts, timeout and preservation checks.
- `docs/TASK-021B-GUEST-WAD-VALIDATION.md`: this report.

The three pre-existing untracked files (`docs/BOOT-VALIDATION-DIFFERENTIAL.md`, `tools/audit_validation_content.py`, `tools/validation_file_replay.cpp`) are preserved unchanged. The protection harness uses the existing read-only audit helper. No staging, commit or push; HEAD and the submodule pointer are unchanged.

## Next smallest step / how to verify

In a separately assigned experiment, compare validation/startup progress with an explicit, protected mount to the existing intact installation. Keep both existing WADs unchanged, observe the same validator result, and make no claim about gameplay in advance. This task does not perform that experiment or repair the shorter installation.

To verify this result without another boot, inspect `observations.json` and boot log lines 513-532, then `boot-result.json`, `preservation.json`, build/restore results and `final-verification.json`. Check `CMakeCache.txt` for all task diagnostics OFF and `build.ninja` for the absence of diagnostic object/linker inputs. `git diff --check`, `git diff --cached --name-only`, and toolkit status/pin verify repository isolation.

`py -3 -B tools/run_task021b_boot.py --check-only` only performs protection/read-only audit checks. The harness refuses a second boot once its existing `boot-started.json` is present; do not remove that guard to repeat this experiment. Reversal of the diagnostic requires only keeping its option OFF; no game data rollback is needed.
