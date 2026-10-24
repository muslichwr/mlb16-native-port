# TASK-022 - Alternate-HDD validation comparison

## Summary

**Classification: ALT_HDD_VALIDATION_PASSES.** The running guest, using the same disc and an explicit RPCS3 HDD mount, observes both GD WAD lengths as **4,915,365,438** and `00016D54` returns **1** to `00018E88`. The targeted `00018FC8` corrupt-data dialog does not execute during the observed boot. Startup `00011CE8` does not return and dispatcher `000136F8` is not reached.

**The task's protection requirement was not fully met.** Windows sharing locks protected existing files, but did not prevent the runtime from creating new children. The post-boot gate detects two new decrypted SDAT sidecars, six new directories, and three existing directory mtime changes. All six original GD WAD/header hashes, sizes and mtimes are unchanged; no pre-existing file metadata changed or pre-existing file was removed. No cleanup or second boot was attempted.

**BOOT TEST: YA**

**TASK-022: INCOMPLETE** - the validation comparison is conclusive, but the no-data-modification gate failed.

## Evidence

Entry checkpoint remains `2ebaa0a2cf98e02964fabf3a966e20f586fcde8b`; the pinned toolkit remains clean at `bc5b3d30503169a025c88bd9e6195728fc8ad68d`. Earlier TASK-021B changes were already present and unstaged on entry.

The sole canonical boot used `timeout -k 5s 45s`, exited **124**, and took **45.532 seconds** according to the harness. It ran from `F:/Games/MLB16PS3/build-mlb16-native-pinned`, with the unchanged `game/EBOOT.ELF` and disc tree. There was no second boot.

Local artifacts are under `F:/Games/MLB16PS3/build-mlb16-native-pinned/task022/`. The raw log is `boot-20261023T190121Z.log`; it and all game-derived output remain outside Git. Only eight lines originate from the bounded TASK-022 observer. The ordinary runtime additionally produces substantial existing output, including repeated flip messages; broad tracing was not enabled.

| Log line | Observed fact |
| --- | --- |
| 27 | Startup `00011CE8` enters once, tid 1, caller return link `00011CB8`. |
| 516 | Validator enters once, tid 7, return link `00018E88`, explicit disc/HDD roots recorded. |
| 521, 523 | Disc host open succeeds; length helper returns `4915365438` at `00016FD8`. |
| 524, 526 | RPCS3 HDD host open succeeds; length helper returns `4915365438` at `00016FF0`. |
| 527 | Validator returns **1**, with both lengths observed. |
| 197021 | At 35 seconds: validations 1, corrupt dialog calls 0, startup entries 1, startup returns 0, dispatcher entries 0. |

There is no `STARTUP_RETURN`, `DISPATCH_ENTER`, or targeted `ERROR_PATH` event anywhere in the complete 45-second log. Their first occurrences would have logged even after the 35-second summary. Linked function-table entries for all five observed guest functions were checked against their wrappers before boot; direct length-helper calls in the original validator also resolve to the observer.

## Resolved paths

The child alone received these explicit settings:

```text
PS3_VFS_ROOT=F:/Games/MLB16PS3/game/MLB The Show 16 (USA)
PS3_HDD0_ROOT=F:/Games/MLB16PS3/rpcs3emu/dev_hdd0
```

| Guest virtual path | Observed host open argument |
| --- | --- |
| `/dev_hdd0/game/BCUS01089_R/USRDIR/MLBPS3_GD.WAD` | `F:/Games/MLB16PS3/rpcs3emu/dev_hdd0/game/BCUS01089_R/USRDIR/MLBPS3_GD.WAD` |
| `/dev_bdvd/PS3_GAME/USRDIR/MLBPS3_GD.WAD` | `F:/Games/MLB16PS3/game/MLB The Show 16 (USA)/PS3_GAME/USRDIR/MLBPS3_GD.WAD` |

Both host opens use `0x8000` (`O_BINARY | O_RDONLY`) and return valid descriptor 7. Adjacent existing filesystem records show successful guest opens. A nonzero retained `errno` on a successful host open is not an open error.

The shorter native HDD copy at `F:/Games/MLB16PS3/game/MLB The Show 16 (USA)/game/BCUS01089_R/USRDIR/MLBPS3_GD.WAD` was protected and audited, but was not selected by this mount. Neither WAD was copied, repaired, replaced or truncated.

## Guest lengths / validation result

| Observation | TASK-021B native HDD control | TASK-022 alternate HDD |
| --- | ---: | ---: |
| HDD GD WAD length | 463339520 | **4915365438** |
| Disc GD WAD length | 4915365438 | **4915365438** |
| `00016D54` return at `00018E88` | 0 | **1** |
| Target corrupt-data dialog at `00018FC8` | Executed | **Not observed in the full window** |

The length equality is therefore satisfied, and the original validator returns success without forcing its result or clearing any guest flag. This is actual guest execution rather than another offline filesystem replay. The complete original GD files and headers retain the hashes reported in [the differential](BOOT-VALIDATION-DIFFERENTIAL.md).

## Dialog correlation

The same guest validator caller used in [TASK-021B](TASK-021B-GUEST-WAD-VALIDATION.md) receives 1 on tid 7. The `00F2610C` observer specifically counts return link `00018FC8`; its count is zero at the bounded summary and no matching call appears later. This establishes absence of that particular corrupt-data call in this observed window, not universal absence of every dialog or later validation error.

## Startup / state-dispatch progress

Startup enters but does **not** return during the boot; dispatcher `000136F8` is **not reached**. Passing this validator does not prove completion of startup or exit from the earlier main-thread wait. This probe deliberately adds no live-context snapshots or arbitrary ancestor tracing.

The next observed boot work is on the Utility PPU thread after successful GD validation: EXT WAD validation/open activity appears at lines 529-534, followed by `SysCacheMount(id='BCUS01089')` at line 544. Existing logs then expose a concrete failed boundary:

```text
/dev_hdd1/cache/BCUS01089/MLBPS3_SYS.MD5
  -> F:/Games/MLB16PS3/game/MLB The Show 16 (USA)/cache/BCUS01089/MLBPS3_SYS.MD5
```

Its open fails first with guest flags `0x0` (line 545), then `0x241` (line 549, write/create/truncate request). The cache parent was absent from the pre-boot inventory. Later creation of cache directories is recorded by the post-boot inventory; the exact causal origin of the failed opens is not fully traced here.

At lines 555-564 the existing runtime reports a null file/object call at return link `00076504`, tid 7, and the stack chain `00018478+1050 -> 000172FC+234 -> 000765E4+B8`. No new stack or filesystem probe was added. These observations identify the next actively executing boundary to investigate; they do not prove the main thread advanced beyond its previous wait, that the cache failure alone explains all later behavior, or that protected write denial caused it. No semantic correction was attempted.

## Protection outcome - failed gate

The harness held **497** existing file/directory handles across both `F:/Games/MLB16PS3/game` and the **entire** `F:/Games/MLB16PS3/rpcs3emu/dev_hdd0` tree. Handles use `OPEN_EXISTING`, `GENERIC_READ`, `FILE_SHARE_READ` only. Access-only write/delete checks on all three original GD WADs fail with Windows sharing error 32. A protection-only preflight passed before the sole boot.

Those locks cannot prevent new child creation. The post-boot audit raises `RuntimeError: Preservation comparison failed`; its wrapper process exits 1, although the game boot itself exits 124. `preservation.json` records `inventory_unchanged=false` and `six_hashes_metadata_unchanged=true`. This failure is not suppressed or reported as a clean preservation result.

New files created by the runtime:

| Path | Size |
| --- | ---: |
| `F:/Games/MLB16PS3/rpcs3emu/dev_hdd0/game/BCUS01089_R/USRDIR/SPECIALWAD.SDAT.dec` | 10696786 |
| `F:/Games/MLB16PS3/rpcs3emu/dev_hdd0/game/BCUS01089_R/USRDIR/SPECIALWADHDR.SDAT.dec` | 6504 |

Existing EDAT logs at lines 655 and 659 explicitly record those sidecar writes. Their contents were not copied into this repository or inspected for this report.

New directories:

- `F:/Games/MLB16PS3/game/MLB The Show 16 (USA)/cache`
- `F:/Games/MLB16PS3/game/MLB The Show 16 (USA)/cache/BCUS01089`
- `F:/Games/MLB16PS3/game/MLB The Show 16 (USA)/cache/BCUS01089/CACHE`
- `F:/Games/MLB16PS3/game/MLB The Show 16 (USA)/cache/BCUS01089/UI_DOWNLOAD`
- `F:/Games/MLB16PS3/rpcs3emu/dev_hdd0/game/BLES00000`
- `F:/Games/MLB16PS3/rpcs3emu/dev_hdd0/game/BLES00000/USRDIR`

Three existing directories have changed mtimes: the disc/VFS root, RPCS3 `dev_hdd0/game`, and RPCS3 `BCUS01089_R/USRDIR`. `inventory-difference.json` records every added/changed entry and exact metadata. No pre-existing files were removed; metadata of all pre-existing files is unchanged. The six GD WAD/header full hashes, sizes and mtimes also match before/after. Original WADs remain intact, but this experiment cannot be called free of filesystem modifications.

No automatic cleanup, timestamp restoration, or retry was performed. The new sidecars/directories remain in place for review. Protection was released only after the timed-out native process was gone and post-boot checks had completed.

## Classification

**ALT_HDD_VALIDATION_PASSES**

This class describes the directly measured guest validation outcome. It does not override the failed protection gate or turn the task status into COMPLETE.

## Build / verification

- Canonical configure/build/link succeeded with only `MLB16_TASK022_DIAGNOSTIC=ON`; TASK-018/019/020/021B stayed OFF. One project observer source was compiled; prebuilt PPU objects were reused.
- TASK-022 is default OFF and mutually exclusive with the earlier diagnostic options. Its source-local build definition selects reduced validation/path logging plus first-two startup/dispatcher events and one 35-second counter summary. No descriptor/DMA/append/eligibility tracing or TASK-020 monitor is enabled.
- PM source/image 77 integration is preserved; registration is verified at boot line 1 and PM resolution is recorded. No toolkit changes, legacy build inputs, or `tmp/task*` inputs.
- After the single boot, canonical configure/build/link succeeded with **all five diagnostics OFF**. The normal target excludes diagnostic sources and wrappers. No second boot was run.
- Prior 45 protected build inputs are unchanged. The toolkit pin and submodule pointer are unchanged. No generic runtime modification or semantic fix was made; generic-runtime synthetic regressions are not applicable.
- No staging, commit or push. The earlier untracked diagnostic/report/tool files remain present. The original native HDD, disc and RPCS3 WAD files were never repaired or replaced.

## What is not proven / uncertainties

- Gameplay, rendering success, startup completion, or dispatcher execution.
- Exit from the exact main-thread wait observed in TASK-020. Validation passed, but the startup-return probe remains negative.
- A complete explanation of the subsequent cache-open/null-call behavior or a demonstrated semantic fix.
- Why the separate native HDD WAD was originally shorter. The alternate-mount comparison changes file selection only.
- That existing-file sharing locks provide a sufficient no-modification environment for future boots. This run demonstrates they do not prevent directory/sidecar creation.
- Original-content corruption or a defect in DXT/DMA/scheduler paths. None of those were investigated.

## Files changed

TASK-022 project changes:

- `cmake/mlb16-pm-integration.cmake`: adds the default-OFF TASK-022 mode and observation wrappers.
- `src/wad_validation_diagnostic.cpp`: reuses the existing observer with reduced TASK-022 logging and bounded startup/dispatcher counters; TASK-021B behavior remains behind its separate mode.
- `tools/run_task021b_boot.py`: adds `--task022`, a separate one-boot guard/output directory, explicit RPCS3 HDD selection, entire-HDD protection coverage and a pre-existing-runner check. Its original TASK-021B mode remains available.
- `docs/TASK-022-PROTECTED-ALTERNATE-HDD.md`: this report.

Outside the repository, the runtime-created files/directories listed above remain. The canonical executable was relinked twice; experiment logs/manifests stay under its `task022` directory. No generated source, original content, pinned toolkit, or legacy tree was edited by the agent.

## Next smallest step

First address the experiment's protection gap before any more boot: existing-file locks alone are insufficient. A separately reviewed containment design must also prevent or isolate new directories and EDAT sidecars. The current one-boot guard must remain in place; do not repeat this experiment or silently remove its new files.

With adequate protection established in a separately assigned task, investigate the observed `SysCacheMount -> /dev_hdd1/.../MLBPS3_SYS.MD5` open/create failure and caller failure handling at `000172FC -> 000765E4`. Start from the saved log and pre-boot inventory. Do not return to DXT or assume validation success establishes gameplay.

## How to verify

Use the existing `task022/observations.json`, log lines 516-549 and 555-564, `boot-result.json`, `preservation.json`, `inventory-difference.json`, and the before/after inventories and content audits. `build-results.json`, `build-gate.json`, linked coverage/disassembly, `restore-results.json`, and `final-verification.json` record build isolation and final state. Verify all diagnostic cache options are OFF, `git diff --check` succeeds, staging is empty, and the toolkit remains clean at its pin. No further boot is required to verify the recorded comparison or preservation failure.
