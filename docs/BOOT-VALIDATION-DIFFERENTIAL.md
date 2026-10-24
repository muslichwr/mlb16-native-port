# Native / RPCS3 boot validation differential

## Summary

The user's known control is successful RPCS3 operation with this game content, including RTTS gameplay. The corruption dialog is treated as a failed native validation result, not a diagnosis of damaged original game data.

**Confirmed differential:** the default native HDD mount selects a separate, shorter installed `MLBPS3_GD.WAD`. It is an exact **463,339,520-byte prefix** of the **4,915,365,438-byte** disc and RPCS3 installed copies. The complete disc and RPCS3 installed WADs are byte-identical. All three `.HDR` files are byte-identical.

**Earliest reproduced failing predicate:** the WAD-length equality comparison after return link `00016FF0` in `00016D54`. The preceding header-length and 16-byte comparisons pass on these files. This is a read-only filesystem replay plus static predicate reconstruction, **not a new capture of that guest branch during boot**. Historical logs correlate the file-open sequence and dialog but do not contain the compared guest length registers or resolved host paths.

No game boot, data replacement, deletion, repair, semantic change, staging, commit or push was performed. The tools do not modify the normal runner or enable its diagnostics.

**BOOT TEST: TIDAK.** Existing logs, static code, full read-only byte comparison and standalone native filesystem replays supplied the differential evidence.

## Evidence and scope

Repository checkpoint on entry: `2ebaa0a2cf98e02964fabf3a966e20f586fcde8b` (`investigate DXT boot expectation`), clean. The pinned toolkit remains clean at `bc5b3d30503169a025c88bd9e6195728fc8ad68d`.

Evidence levels:

1. **Historical boot observation:** TASK-020 log `F:/Games/MLB16PS3/logs/mlb16-task020-boot-expectation-20261023T180828Z.log`, lines 522–529, opens the two GD headers, disc GD WAD, and HDD GD WAD, then reports the error dialog. Its two 16-byte header reads returned 16 bytes with identical first four bytes. TASK-019 also recorded the same error. No new diagnosis of original-file corruption follows from the message.
2. **Current content observation:** full-file hashing and streaming comparison of all six selected files, using `rb` only. The disc and RPCS3 WAD hashes match, and every byte present in the native installed WAD matches their prefix.
3. **Native API replay:** standalone `tools/validation_file_replay.cpp` includes the unchanged pinned `runtime/ppu/ppu_fs.cpp`, calls its actual open/stat/fstat/seek/read/close functions, and records the selected host paths and guest-visible results. It runs independently of the game.
4. **Static guest condition:** the lifted `00016D54` compares header lengths, 16 header bytes, and then WAD lengths. No generated source was changed or copied into the repository.
5. **RPCS3 comparison:** `rpcs3emu/config/games.yml` maps `BCUS01089` to this same disc tree. An inspected RPCS3 log mount record maps `/dev_hdd0` to `F:/Games/MLB16PS3/rpcs3emu/dev_hdd0/`. Later reads of the live RPCS3 log were denied by its writer. The successful RTTS run is the user's control; this investigation did not execute or instrument RPCS3.

## Exact virtual paths and selected host files

For compactness in the table:

- `D = F:/Games/MLB16PS3/game/MLB The Show 16 (USA)`
- `R = F:/Games/MLB16PS3/rpcs3emu/dev_hdd0`

| Virtual path | Default native replay host path | Replay using RPCS3's existing HDD files |
| --- | --- | --- |
| `/dev_hdd0/game/BCUS01089_R/USRDIR/MLBPS3_GD.WAD.HDR` | `D/game/BCUS01089_R/USRDIR/MLBPS3_GD.WAD.HDR` | `R/game/BCUS01089_R/USRDIR/MLBPS3_GD.WAD.HDR` |
| `/dev_bdvd/PS3_GAME/USRDIR/MLBPS3_GD.WAD.HDR` | `D/PS3_GAME/USRDIR/MLBPS3_GD.WAD.HDR` | Same disc path |
| `/dev_bdvd/PS3_GAME/USRDIR/MLBPS3_GD.WAD` | `D/PS3_GAME/USRDIR/MLBPS3_GD.WAD` | Same disc path |
| `/dev_hdd0/game/BCUS01089_R/USRDIR/MLBPS3_GD.WAD` | `D/game/BCUS01089_R/USRDIR/MLBPS3_GD.WAD` | `R/game/BCUS01089_R/USRDIR/MLBPS3_GD.WAD` |

The native mapping is established by `third_party/ps3recomp/runtime/ppu/ppu_fs.cpp:131–203`: `/dev_hdd0/` uses `PS3_HDD0_ROOT` when set; otherwise that mount prefix is stripped and the suffix is appended to `ppu_vfs_root`. The TASK-020 command sets the disc VFS root and does not explicitly set `PS3_HDD0_ROOT`. The current shell has no HDD override. Its historical inherited value was not separately saved, so the precise historical host path remains an inference; the table's replay paths were directly recorded.

Two separate replay processes were used because the native resolver caches the HDD override. The second child alone received `PS3_HDD0_ROOT=R`. This is an **offline comparison against the existing RPCS3 files**, not a persisted configuration change, game boot, or measurement of RPCS3's syscall implementation. Inherited filesystem diagnostic/cap/truncation variables were removed from both replay environments.

## First validation boundary

Static references are to `F:/Games/MLB16PS3/recompiled-bounded/ppu_recomp_000.cpp`.

| Step | Source / return links | Expected condition | Native files | RPCS3 HDD files with the same disc |
| --- | --- | --- | --- | --- |
| Open GD headers read-only | `00016D54`, lines 7415–7445; `00016E44`, `00016E84` | Both file objects available; no intervening error state | Existing boot logs record successful opens; standalone opens return 0 | Standalone opens return 0 |
| Compare header lengths | Lines 7451–7468; `00077B84`, links `00016EBC`, `00016ED0` | Equal lengths | `176552 == 176552`: pass | Same: pass |
| Read first 16 bytes | Lines 7469–7489; links `00016EFC`, `00016F34` | Read 16 bytes from each newly opened header | Offset 0, requested 16, actual 16 | Same |
| Compare header bytes | Lines 7497–7516, loop `00016F64` | All 16 bytes equal | Pass; full headers also match | Pass |
| Obtain WAD lengths | Lines 7526–7542; `0007C174`, links `00016FD8`, `00016FF0` | Selected HDD and disc WAD lengths equal | **`463339520 != 4915365438`: fail** | `4915365438 == 4915365438`: pass |
| Propagate result | Lines 7543–7574 | Preserve the success flag only when comparisons pass | Reconstructed result 0 | This data-dependent path can retain success; earlier feature/state guards are not simulated |

`0007C174` resolves a path, selects a file device, and invokes virtual slot `+38` (lines 120380–120476). `00077B84` invokes an opened file object's slot `+28` (lines 115311–115322). These are the values compared by the validator; their actual historical guest returns were not logged. The replay measures the pinned filesystem's metadata independently rather than assuming these virtual calls ran exactly as reconstructed.

The enclosing Utility PPU Thread function `00018478` calls `00016D54` with return link `00018E88`, saves its result, and tests it at lines 9693–9695. A zero result can select `00018F64`, which prepares the corrupt-Game-Data message, writes the shared error byte, and calls the dialog with return link `00018FC8` (lines 9731–9756). This supplies a concrete static relationship to the observed dialog; the exact historical call edge is not newly instrumented.

No DXT descriptor, DMA, scheduler, append or eligibility path was revisited.

## API results, offsets, bytes and metadata

All replay `cellFsOpen` calls use flags **0 (`O_RDONLY`)**. All selected opens, stat/fstat calls, seeks, reads and closes returned **`00000000`**. An EOF read returning zero bytes is a successful read result, not an open failure or an invented read error.

| Selected file | `cellFsStat` and `cellFsFstat` size | Initial read: requested / actual | Initial 16 bytes |
| --- | --- | --- | --- |
| Any of the three GD headers | `176552` (`0x2B1A8`) | Offset 0: `16 / 16` | `0116BF20 AC35FB42 70B2C8D3 653BB905` |
| Native HDD GD WAD | `463339520` (`0x1B9E0000`) | Offset 0: `16 / 16` | `49464630 4D4F444C 00000000 C0220000` |
| Disc or RPCS3 HDD GD WAD | `4915365438` (`0x124FA863E`) | Offset 0: `16 / 16` | Same WAD prefix |

The replay also records the full **52-byte CellFsStat** representation: mode `000081B6`, uid/gid/timestamps zero, size as an eight-byte big-endian word at `+24`, block size `200` at `+2C`. The differing size bytes are `000000001B9E0000` versus `0000000124FA863E`. The complete raw structures remain in the small local replay logs. Header prefixes are not assigned undocumented semantic field names.

The actual validation reads 16 header bytes; its final WAD condition concerns **whole-file length metadata**, with no WAD payload offset/length request at that comparison. To distinguish a short selected file from 32-bit offset truncation, the replay additionally performed the following probes. These extra offsets are diagnostic requests, **not claimed title requests**:

| Requested seek / read | Seek result | Native HDD WAD | Disc / RPCS3 HDD WAD |
| --- | --- | --- | --- |
| SET `463339520`, read 16 | Return 0; reported position exactly `463339520` | Return 0, **0 bytes** at its EOF | Return 0, **16 bytes**: `0080321BAB112B00A07AB64C0811A0AF` |
| SET `4294967296` (4 GiB), read 16 | Return 0; reported position exactly `4294967296` | Return 0, **0 bytes**, beyond its EOF | Return 0, **16 bytes**: `CAA93FA05D3C0000000BB4E21ED2675E` |

Thus the pinned PPU filesystem replay preserves the tested 64-bit sizes and seek positions. This does not prove all guest libc/device layers or concurrent boot I/O are correct. It does rule out a need to truncate the expected archive size merely because it exceeds 4 GiB at this measured API boundary.

## Full content comparison and preservation

| File | SHA-256 / comparison |
| --- | --- |
| All three `MLBPS3_GD.WAD.HDR` files | `595c1e4d1bf7a58084354341e6731ddde34e40a1d509ba2db1026411d0795f6a` |
| Complete disc and RPCS3 HDD WADs | `5a1e3e002f8c5429ea92ce455d7f0ff02a505b15e6f2cddcefdf67acf92e64d1` |
| Native HDD WAD | `306665d38c8368b950813910e0ecb00f2f8cf41f76af5d09693a59f1688200b2`; exact prefix of both complete WADs |

The native installed WAD's mtime is `2026-10-23T10:50:22.827466Z`; the disc and RPCS3 installed copies have the matching original mtime `2016-02-28T12:42:05Z`. Timestamps alone do not establish what created or shortened the native copy.

Initial replay metadata/header hashes were unchanged. Full content audits before and after confirmation replays verify all six file hashes, sizes and mtimes are unchanged. All audit inputs use `rb`; replay file opens use `O_RDONLY`. No original files were copied, repaired, deleted or replaced. No content was uploaded or staged.

The standalone C++ probe has two bounded dependency adapters: raw, existing, non-NPD files pass through the EDAT resolver seam unchanged; the VFS fallback seam verifies the selected path exists and refuses available alternative layouts. The ordinary pinned helpers would leave these audited inputs unchanged. Other dependency seams abort if unexpectedly used. The probe is not evidence for encrypted-file handling, fallback layouts, unknown mounts, a running lifted validator, or RPCS3 API execution.

## Uncertainties / what is not proven

- **Not proven:** the exact historical guest length values, first executed failure edge, or the historical inherited HDD override. The earliest failing condition is established for the current default-mount replay and reconstructed validator, not by retrospectively inventing a register trace.
- **Not proven:** who produced the shorter installed copy, why it stopped at this length, or whether a port copy/install operation was interrupted. An incomplete native installation is a hypothesis about its origin, not an identified runtime defect.
- **Not proven:** that redirecting the native runner to the intact installed copy would reach gameplay or fix black rendering. No redirecting game boot was run.
- RPCS3's successful gameplay is user-provided control evidence. The same complete content is verified locally, but exact RPCS3 per-call offsets/results were not captured. The control-file replay is identified separately throughout.
- Existing boot logs can interleave records and reuse file descriptors. Current replay descriptor 3 is local to the standalone process and is not equated with historical descriptors 7/8.

## Recommended next smallest step

Close the remaining **execution-correlation** gap at `00016D54 -> 00016FF0 -> 00018E88/00018FC8`: record the two guest lengths and the result at the actual validation boundary, with the selected host paths and all writes/deletes to original game data prevented by the experiment's design. Keep the measurement bounded; do not bypass the check or force a success value.

First make the installed-data mount used by the experiment explicit. The intact RPCS3 installed copy is a read-only comparison source, not a repair target. Do not copy it over the native file or expose the RPCS3 installation to an uncontrolled write-capable boot. A later experiment should distinguish selection of the shorter native cache from a wrong value returned by a guest/runtime layer. No semantic fix is implemented here.

## How to verify

Local artifacts live under `F:/Games/MLB16PS3/build-mlb16-native-pinned/validation-differential/`:

- `compile-result.json` / `compile.log`: Clang standalone build command, exit 0. Only the probe was compiled; no generated PPU objects or normal runner were rebuilt.
- `replay-results.json`, `native.stdout.log`, `rpcs3_files.stdout.log`: exact replay arguments, child HDD override, successful results, host paths and bytes. Both exited 0.
- `content-audit.json`, `audit-result.json`: full-file hashes, exact prefix comparison and audit command, exit 0.
- `content-audit-before-confirmation.json`, `content-audit-after-confirmation.json`, `preservation-verification.json`: identical content and metadata across confirmation replays.
- `final-verification.json`: checkpoint, clean toolkit pin, empty staging area, diagnostic defaults, canonical executable fingerprint and preserved build inputs.

PowerShell compile command (from the repository):

```powershell
& D:/APP/msys64/clang64/bin/clang++.exe -std=gnu++17 -O2 `
  -I F:/Games/MLB16PS3/recompiled-bounded `
  -I third_party/ps3recomp/include -I third_party/ps3recomp/runtime/ppu `
  tools/validation_file_replay.cpp `
  -o F:/Games/MLB16PS3/build-mlb16-native-pinned/validation-differential/validation_file_replay.exe
```

The executable takes the disc VFS root as its only argument. Run it in a child process with filesystem diagnostic variables cleared; omit the HDD override for the default native case, and set it only in the comparison child for the existing RPCS3 HDD files. Exact invocations/environment filtering are recorded in `replay-results.json` and `replay-environment.json`. Neither invocation boots the game.

Full read-only content audit:

```powershell
py -3 -B tools/audit_validation_content.py `
  --disc 'F:/Games/MLB16PS3/game/MLB The Show 16 (USA)/PS3_GAME/USRDIR' `
  --native-hdd 'F:/Games/MLB16PS3/game/MLB The Show 16 (USA)/game/BCUS01089_R/USRDIR' `
  --rpcs3-hdd 'F:/Games/MLB16PS3/rpcs3emu/dev_hdd0/game/BCUS01089_R/USRDIR'
```

`git diff --check`, `git status --short`, `git diff --cached --name-only`, toolkit status/pin, and inspection of canonical `CMakeCache.txt` / `build.ninja` verify repository/build isolation. All TASK-018/019/020 diagnostics remain OFF.

## Files changed

- `tools/validation_file_replay.cpp`: original standalone, read-only native filesystem probe; absent from the normal game target.
- `tools/audit_validation_content.py`: original streaming, read-only hash/prefix comparison tool.
- `docs/BOOT-VALIDATION-DIFFERENTIAL.md`: this evidence report.

No runtime, toolkit, generated code, CMake integration, game data or previous investigation report was edited. No staging, commit or push.
