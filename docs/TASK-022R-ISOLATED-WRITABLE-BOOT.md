# TASK-022R - Isolated writable boot progress

## Summary

The isolated guest repeats the successful alternate-HDD GD validation: HDD and disc lengths are both **4,915,365,438**, and `00016D54` returns **1**. The previously failing SYS.MD5 create/write now succeeds with a non-null file object and a 16-byte write. No validation result, guest flag, or generic runtime behavior is forced.

The disposable experiment root is `F:/Games/MLB16PS3/build-mlb16-native-pinned/task022r`. The hard isolation gate passed before the sole launch, and the final full-content/metadata audit proves all original trees unchanged. Startup advances beyond its previous validation wait but does not finish; the next observed no-progress boundary is its transition/frame-submission path near `00066530 -> 00F25BCC`, with last return link `00066BE0` (`cellGcmSetPrepareFlip`). This localizes the next investigation; it does not establish a faulty HLE implementation or exact internal wait.

**Termination reason: FAIL_SAFE_TIMEOUT.** This is not classified as a game failure.

**BOOT TEST: YA**

**TASK-022R: COMPLETE**

## Filesystem isolation

All writable guest domains and working/temp directories are under the experiment root, denoted `E` below:

| Setting / domain | Selected experiment location |
| --- | --- |
| `PS3_VFS_ROOT`, disc mirror | `E/vfs` |
| `PS3_HDD0_ROOT`, `/dev_hdd0` | `E/hdd0` |
| `PS3_HDD1_ROOT`, `/dev_hdd1` | `E/hdd1` |
| `/dev_hdd1/cache/BCUS01089` | `E/hdd1/cache/BCUS01089` |
| Firmware fallback | `E/dev_flash` (empty) |
| Process working directory | `E` |
| TEMP/TMP and profile/AppData | `E/tmp`, `E/profile/...` |

Disc access uses file-only symbolic links to the existing disc content. All mirror **directories are real, independent directories inside E**; there are no junctions or directory symlinks into original trees. Every original file is held open with `GENERIC_READ`, `FILE_SHARE_READ`, `OPEN_EXISTING` for the entire run and preservation audit. Thus file-link targets remain read-only, while new sibling paths such as EDAT sidecars resolve beneath the mirror, not in the original source directory. No hardlinks are used.

Exactly four installed files are physically copied from the intact RPCS3 installation into `E/hdd0/game/BCUS01089_R/USRDIR`: GD WAD/header and EXT WAD/header, **10,158,864,840 bytes total**. Seed hashes match their sources, and destination file identities differ from their sources. No save data, SDATs, pre-existing `.dec` sidecars, or other installed files are seeded from RPCS3. Disc file links are separate from installed seeds.

The pinned PPU filesystem resolver is exercised by the standalone map-only probe before launch. It reports:

| Guest request | Resolved host argument |
| --- | --- |
| `/dev_hdd0/game/BCUS01089_R/USRDIR/MLBPS3_GD.WAD` | `E/hdd0/game/BCUS01089_R/USRDIR/MLBPS3_GD.WAD` |
| `/dev_bdvd/PS3_GAME/USRDIR/MLBPS3_GD.WAD` | `E/vfs/PS3_GAME/USRDIR/MLBPS3_GD.WAD` |
| `/dev_hdd1/cache/BCUS01089/MLBPS3_SYS.MD5` | `E/hdd1/cache/BCUS01089/MLBPS3_SYS.MD5` |

Preflight verifies every experiment directory and file link, independent seed copies, hash equality, and writable canaries under HDD0, HDD1/cache and the disc mirror's USRDIR. Access-only write opens against the original disc WAD and the mirror file link both fail with sharing error **32**, without writing or truncating anything. Original directory identities and file link counts are unchanged by preparation.

Static cross-check: `runtime/ppu/ppu_fs.cpp:131-203` and `runtime/syscalls/sys_fs.c:155-210` honor explicit HDD0/HDD1 roots. Legacy `libs/filesystem/cellFs.c` relative defaults resolve beneath the experiment working directory; its disc override uses the mirrored VFS root. `edat_resolve` forms its output as the supplied source path plus `.dec` (`libs/filesystem/edat.c:671+`), so mirror-source siblings stay within E. The main runner honors the explicit VFS root. No resolver is patched.

The preflight is repeated immediately before launch with all original locks held. The hard gate is recorded in `isolation-preflight.json` and `launch-isolation-gate.json`. No game boot occurs in `--prepare` or in the standalone resolver probe.

Original-file write protection depends on the held handles during the experiment; a file symlink alone is not a read-only mount. After the preservation audit, all **40 temporary disc-file links were individually verified and unlinked**, leaving every original target in place. `mirror-unmounted.json` and `experiment-final.json` record this disarming step. The current harness includes the same checked unmount in finalization; for this run it was invoked explicitly after the completed audit. The internal capability-test link targets only another experiment file and poses no original-data alias. The one-boot guard remains in place; do not launch or write through a disc mirror outside the protected harness.

## Validation result

`boot.log` lines 526-532 record the resolved disc/HDD open arguments, disc helper return at `00016FD8`, HDD helper return at `00016FF0`, and validator return **1** to `00018E88` on tid 7. Both lengths are **4915365438**, matching TASK-022's successful comparison. No targeted corrupt-data dialog has been observed in the recorded probe events.

## SYS.MD5 / cache behavior

The cache was initially empty. The existing `SysCacheMount` implementation, now given `PS3_HDD1_ROOT`, creates its title-specific directory (log line 551). The harness does not pre-create `cache/BCUS01089` or provide a checksum file.

- Line 552: first read-only SYS.MD5 open returns -1 / errno 2, consistent with an absent cache entry.
- Line 555: the unchanged write/create/truncate path succeeds, returning descriptor 8 at `E/hdd1/cache/BCUS01089/MLBPS3_SYS.MD5`.
- Lines 558-559: the `000172FC -> 000765E4` call at return link `00017530` receives **file object `01258B98`**, size 16, count 1; its write returns **16**.

The resulting SYS.MD5 file is 16 bytes and matches the first 16 bytes read from the original disc SYS header (`cache-verification.json`). The successful host open retains errno 2 from the earlier miss; its nonnegative descriptor and subsequent successful write establish success. This is an ordinary cache miss followed by successful creation, unlike TASK-022's failure of both opens and subsequent null object. No fake success or guard bypass was introduced. No null-read, null-write or null-call record occurs in the complete run.

## Startup / dispatcher progress

At both 35 and 170 seconds: validations 1, corrupt-data dialog calls 0, startup entries 1, startup returns 0, dispatcher entries 0. There is no startup-return or dispatcher-entry record elsewhere in the full log. The first two entries/returns would be logged regardless of the summary time.

At 170 seconds the main-thread context has LR `00066BE0`, SP `0FEFF730`, `r3=01234C08`, `r4=1`. The stack's relevant return links map to:

```text
00011CE8 --00013330--> 000117F4
          --00011984--> 00011614
          --00011748--> 000114B4
          --0001156C--> 00062C1C
          --00062C64--> 00066530
          --00066BE0--> 00F25BCC (NID 0B4B62D5, cellGcmSetPrepareFlip)
```

This is downstream of the earlier startup `00015948` call/return link `000129D0` seen in TASK-020. Thus this run establishes later startup execution, even though the enclosing `00011CE8` still has not returned. Utility tid 7 is sampled at return link `000185E8` with `r3=10000`, matching its ordinary command/poll loop's `009A2048` call. These are bounded, asynchronous observations, not an atomic cross-thread state dump.

Stack `sp+16` words can retain a prior callee's saved link: in particular `00066A00` corresponds to an earlier `0006FB18` call and is not treated as an additional currently active frame above the live `00066BE0`. Main CIA is 0 and is not used as a current-PC assertion.

## First blocking boundary

**First newly localized no-progress boundary: the startup transition/frame-submission chain above, around `00066530 -> cellGcmSetPrepareFlip`.** The late snapshot, absent startup completion and absent dispatcher entry locate progress here rather than at WAD validation or SYS.MD5 creation.

The complete log contains 805 `SetPrepareFlip` records; the last is line 16176 (buffer 1), before the 35-second summary. No further record appears through the 170-second snapshot or fail-safe. `cellGcmSetPrepareFlip` logs each admitted call without a trace-volume cap (`libs/video/cellGcmSys.c:2832-2858`). Its body updates display state and may synchronously invoke the guest flip handler. The live arguments at 170 seconds match this boundary. Ordinary stdout buffering/interleaving can affect the final visible records; a single late guest-context snapshot does not prove whether the host is blocked inside that callback, FIFO/display processing, or another adjacent operation. The exact earliest causal defect and a measured per-call duration remain unproven.

Static identity is verified against the actual generated registration, not misleading nearby SDK NID comments: `F:/Games/MLB16PS3/recompiled-bounded/ppu_hle_nids.cpp:1454` registers `0B4B62D5` as `cellGcmSetPrepareFlip`; `ppu_recomp_028.cpp:130663` supplies the import stub, and `ppu_recomp_000.cpp:98934` supplies the call at return link `00066BE0`. The saved stack links map to the startup chain at lines 1698, 1827, 1990, 3724 and 94454 of object 000's source. No generated code was changed or copied into Git.

Missing BOOT/CONFIG_MAIN.TXT, PATCH_GD and unseeded SPECIALWAD requests are also logged, but later audio/thread initialization demonstrates these initial misses alone do not establish the persistent blocking boundary. The minimal installed seed is intentionally not the full RPCS3 installation. No causal relationship between those misses and the late no-progress point is established. Existing SPU messages were not used to reopen DXT/DMA/scheduler investigation.

## Termination reason

**FAIL_SAFE_TIMEOUT** - exactly one game process was launched. Python's `wait(timeout=180)` reached its fail-safe and the harness terminated that process tree; duration is **180.625 seconds**, with post-termination exit code **1**. That code is the externally terminated process result, not an observed natural game exit or crash code. No fatal/assert marker is present in the log. A fail-safe timeout is an experiment termination reason, not a game failure diagnosis.

## Original-data preservation

The baseline includes all existing entries beneath `F:/Games/MLB16PS3/game` and the entire `F:/Games/MLB16PS3/rpcs3emu/dev_hdd0` tree, including TASK-022's previously created artifacts. They are not cleaned or altered. The initial manifest contains **505 entries / 435 files**, with full SHA-256 coverage of **34,330,559,625 bytes**, plus sizes, mtimes, attributes, inode identities and link counts.

**PASS:** `original-before.json` and `original-after.json` are exactly equal across every entry and every file hash. No original file or directory was added, removed, renamed, or changed in the recorded metadata/content. Original locks stayed held until the native process was gone and the final full-content audit finished. No original ACL, attribute, timestamp, file contents, or link count was deliberately changed; no cleanup was performed in the original trees.

Expected experiment mutations during the run are recorded separately in `experiment-difference.json`: the 16-byte SYS.MD5, cache `CACHE`/`UI_DOWNLOAD` directories, default `hdd0/game/BLES00000/USRDIR` directories, five driver/D3D shader-cache files beneath the redirected profile, boot log and diagnostic/audit JSON artifacts. No pre-launch experiment entries were removed during the boot. Post-audit, only the 40 temporary disc-file links were removed to disarm the mirror; original targets and the four independent installed copies remain. These changes do not constitute an original-data preservation failure.

Canonical configure/build/link passed before boot, with six linked guest-wrapper entries verified. Only TASK-022R was ON; the earlier diagnostic options stayed OFF. After the sole run, canonical configure/build/link passed again with **all six options OFF**, and normal diagnostic sources/wrappers are excluded. The pinned toolkit remains clean at `bc5b3d30503169a025c88bd9e6195728fc8ad68d`; the prior 45 protected build inputs are unchanged. Generic runtime code was not modified, so generic-runtime synthetic regressions were not applicable. The standalone resolver, write-access denial/canary checks, file identity/hash checks and full preservation audit supplied the relevant experiment verification.

## Files changed

- `cmake/mlb16-pm-integration.cmake`: default-OFF, mutually exclusive TASK-022R observer option.
- `src/wad_validation_diagnostic.cpp`: reuses bounded validation/startup counters, adds selected SYS.MD5 open/write observations and one late main/utility context/stack snapshot.
- `tools/validation_file_replay.cpp`: optional `--map-only` mode for the existing standalone, pinned PPU resolver probe; normal replay behavior preserved.
- `tools/run_task022r_boot.py`: disposable preparation, file-only disc mirror, four independent installed seeds, isolation gate, one launch with fail-safe, and full original-content preservation audit.
- `docs/TASK-022R-ISOLATED-WRITABLE-BOOT.md`: this report.

Game content, seed copies, compiled standalone probe, logs and manifests stay outside Git under E. No generated source, generic ps3recomp semantics, or legacy tree is modified. No staging, commit or push. Earlier investigation files and original TASK-022 artifacts are retained.

## Next smallest step

Investigate **entry/return and synchronous callback progress at `00066530 -> cellGcmSetPrepareFlip`**, using a separately assigned bounded observation that distinguishes the import's return, its flip callback's entry/return, and the containing startup transition's completion. First review this run's saved context/stack and unchanged callback code; do not infer that the GCM call is defective from one sampled LR. Keep the same isolated mounts/protection design and preserve this task's one-boot guard.

Do not infer gameplay, visible rendering success, deadlock, or a semantic fix from validation/cache success or the fail-safe. Dispatcher execution and full startup completion remain unproven; the precise internal blocking mechanism is the next task. Do not return to DXT internals or repair original content.

## How to verify

Inspect the saved isolation gates, `seeds.json`, `disc-file-links.json`, `boot-started.json`, `boot.log`, the bounded probe observations, termination record and original before/after manifests. Canonical build, linked wrapper coverage, toolkit status and final normal-build restoration are recorded under E. Preserve the one-boot guard; no second boot is needed to verify this run's evidence.
