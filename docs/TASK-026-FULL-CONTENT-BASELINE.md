# TASK-026 - Reproducible full-content baseline experiment

## Summary

**Classification: NATIVE_BOOT_NONDETERMINISTIC.** Three fresh, independent sandboxes started with identical complete installed-content manifests, the same normal executable, and equal normalized environment/configuration and initial guest-input manifests. Their audited post-boot installed-file states differ.

Run 1 retains the original installed-file lengths. Runs 2 and 3 change one seeded file from **9,026,120 bytes to 5,056 bytes**. This is positive filesystem evidence of a different persistent outcome, independent of timestamps, descriptors, host thread IDs, log order or volume. The earliest proven divergence **category** is **PERSISTENT_INSTALLED_FILE_STATE**. No subsystem, writer, guest branch or causal defect is identified.

All three runs terminate by the prescribed **FAIL_SAFE_TIMEOUT**, create the same correct 16-byte SYS.MD5, and have no recoverable corrupt-Game-Data dialog marker. Every original-tree audit passes. A timeout is an experiment termination reason, not a game-failure diagnosis.

**BOOT TEST: YA**

**TASK-026: COMPLETE**

## Baseline definition

Repository checkpoint: `a2cb0cd6bb8a989de2d4a421ef66c580f301f6c5`, clean at task entry. Pinned ps3recomp remains clean at `bc5b3d30503169a025c88bd9e6195728fc8ad68d`.

Immutable seed source:

`F:/Games/MLB16PS3/rpcs3emu/dev_hdd0/game/BCUS01089_R`

The **complete installation tree** is used: **280 files / 284 manifest entries / 10,573,845,294 bytes**, including the installation root and all subdirectories. No four-file subset is used. No unrelated RPCS3 HDD directories, home/save trees, profiles or other installations are seeded.

The source is held with original-file read-sharing locks during preparation and each boot/audit. Before every launch its full protected-tree manifest must equal the initial baseline. No original ACL, attribute, timestamp or content is deliberately changed to make it read-only. This is a verified immutable experiment input, not a repair of the source installation.

Fresh experiment roots are:

```text
F:/Games/MLB16PS3/build-mlb16-native-pinned/task026/run1
F:/Games/MLB16PS3/build-mlb16-native-pinned/task026/run2
F:/Games/MLB16PS3/build-mlb16-native-pinned/task026/run3
```

Each has a complete physical installation copy at `hdd0/game/BCUS01089_R`, an initially empty `hdd1/cache`, independent temp/profile/dev-flash roots, and its own disc mirror. Seed file contents, sizes, readonly bits and mtimes match. Empty directories and their seed metadata are retained. Full prelaunch rehashing verifies the copy. File identities differ from the source and from both other copies; link counts are one. No copy is seeded from a previous run.

The disc content and entry ELF are the same originals as earlier experiments. Real experiment directories and 40 file-only links provide the disc mirror; original files remain write/delete protected while the game runs. There are no original directory aliases or installed-file hardlinks. Native resolver checks demonstrate HDD0, disc write parents and HDD1/cache mappings inside the sandbox. Write-access opens to original WAD targets are denied with Windows error 32; writable canaries succeed only inside disposable roots.

## Seed / executable / environment equivalence

The normal canonical target is configured and linked **once before the experiment**, using `build-mlb16-native-pinned` and `third_party/ps3recomp`. The old normal executable is retained as `task026/prebuild-normal.exe`; moving that one build output aside makes Ninja perform a fresh link without recompiling generated PPU objects. The build log shows one link. It is not rebuilt between or after the three boots.

All eight TASK-018/019/020/021B/022/022R/023/024 diagnostic options are OFF. The normal target excludes their sources and guest wrappers. Task trace environment variables are absent. No TASK-026 guest observer or new runtime option exists. The existing canonical PM registration/readiness glue is unchanged, including its normal TASK-011 readiness banner; it is not a newly enabled guest probe.

| Frozen input | SHA-256 |
| --- | --- |
| Complete seed manifest, canonical JSON | `67d20f8b3ed805165665d8c050ad2e5243fb11778a1357caf8aed015e457de60` |
| Canonical `MyGameRecomp.exe` | `a023ae80c0f96fb3cf938d37dd12753bafa5fe33cf2dda2a007cc2a7024e42d1` |
| Normalized full environment/configuration | `0f667f86ec738aec564ddb86fb476e42fd5ad2507351d6a3c39c8e06b36ecc04` |
| Normalized initial guest inputs | `dfe4dcab20a264237afa4533b49b751185d7edacb563560ebe2ff22e828fcefa` |

Full process environment is frozen once, stored locally, and reused. Only sandbox-specific writable paths/CWD differ. Configuration includes the same executable/ELF arguments, 180-second fail-safe and seed digest. Each run regenerates and compares its normalized launch configuration before `Popen`. Full environment values remain local; they are not copied into this report or Git.

| Guest/process domain | Location relative to each run root |
| --- | --- |
| `/dev_hdd0` | `hdd0` |
| `/dev_hdd1`, including cache children | `hdd1` |
| Disc VFS | `vfs` |
| TEMP/TMP/TMPDIR | `tmp` |
| USERPROFILE, APPDATA, LOCALAPPDATA | `profile`, its Roaming and Local children |
| Dev-flash fallback | `dev_flash`, initially empty |
| Working directory | That run root |

Executable, canonical cache and Ninja configuration hashes are checked before and after every run. The exact executable hash also appears in all three launch and termination records. No timing controls or guest return/state values are forced.

## Run 1

- Prelaunch seed, executable/configuration, normalized environment, complete initial guest-input and isolation gates: **PASS**.
- Termination: **FAIL_SAFE_TIMEOUT**, **180.688 seconds**, external-kill exit code 1.
- Corrupt-Game-Data outcome: no corrupt-data text or `cellMsgDialog` marker observed. No validator guest-return probe is enabled, so an exact validation return value is not claimed.
- SYS.MD5/cache: initially absent; after the run, SYS.MD5 exists with **16 bytes**, SHA-256 `991150e52101810603f7d1c8582beda60d79fa260b91c95bdfc670db72287d80`, matching the disc SYS header's first 16 bytes. Cache directories are created inside this run only.
- Stable ordinary evidence: both startup semaphore return-link signatures, main-worker creation, SYS/GD/EXT header results, audio-port open/ready and screenshot-parameter request are recoverable. Intro filename and HTTP-client request are not recoverable; this does not prove their non-execution.
- Guest-domain mutations: **21 additions / 0 changed existing entries / 0 removals**. The only new regular file in HDD0/HDD1/VFS is SYS.MD5; remaining additions include directories and profile/driver-cache files. Seeded installation files remain unchanged.
- Original-tree audit: **PASS**. All 40 disc-file links are disarmed before locks are released.

## Run 2

- All prelaunch equivalence/isolation gates: **PASS**, using its untouched independent copy after Run 1's audit passed.
- Termination: **FAIL_SAFE_TIMEOUT**, **180.625 seconds**, external-kill exit code 1.
- No corrupt-data/dialog marker observed; the same correct 16-byte SYS.MD5 is created.
- The same nine common ordinary milestones are recoverable, plus the intro filename request and HTTP-client request. Neither is interpreted as a paired guest return or dispatcher-entry proof.
- Guest-domain mutations: **22 additions / 3 changed existing entries / 0 removals**. In the disposable installation, `USRDIR/ArenaLogoCache` changes from **9,026,120 to 5,056 bytes**. `DefaultProfiles` changes bytes at the same length; `DefaultAppearances` has only recorded metadata change. Those latter two differences are not used to classify nondeterminism. A new zero-byte title-cache file is also recorded and is not the classification basis.
- Original-tree audit: **PASS**; disc-file links disarmed. The original source `ArenaLogoCache` remains at its baseline size/hash.

## Run 3

- All prelaunch equivalence/isolation gates: **PASS**, using its own untouched copy after Run 2's audit passed.
- Termination: **FAIL_SAFE_TIMEOUT**, **180.641 seconds**, external-kill exit code 1.
- No corrupt-data/dialog marker observed; SYS.MD5 has the same correct 16 bytes/checksum.
- Recoverable ordinary milestone set equals Run 2's. The installed-file structural outcome also equals Run 2's: `ArenaLogoCache` ends at **5,056 bytes**. Same-size content/mtime changes are recorded separately and excluded from the structural comparison.
- Guest-domain mutations: **22 additions / 3 changed existing entries / 0 removals**; original-tree audit **PASS**, all disc-file links disarmed.

## Cross-run comparison

| Stable observation/outcome | Run 1 | Run 2 | Run 3 |
| --- | --- | --- | --- |
| Matched seed/executable/environment/input gates | PASS | PASS | PASS |
| Termination category | FAIL_SAFE_TIMEOUT | FAIL_SAFE_TIMEOUT | FAIL_SAFE_TIMEOUT |
| Corrupt-data/dialog marker | Not observed | Not observed | Not observed |
| SYS.MD5 size / expected checksum match | 16 / yes | 16 / yes | 16 / yes |
| Nine common ordinary milestones | Observed | Observed | Observed |
| Intro filename / HTTP request text | Not recoverable | Observed | Observed |
| Audited final seeded-file size at differing path | 9,026,120 | 5,056 | 5,056 |
| Original-data audit | PASS | PASS | PASS |

Ordinary-event signatures use stable semantic event presence/results, not count, timing or order. Their SHA-256 values are:

| Run | Normalized ordinary-log/milestone signature |
| --- | --- |
| 1 | `0b7446f05d7ca10b5fb5fd837f8c5d5fc8206af53b4c30fc8403313cb46c5d92` |
| 2 | `70a16564f3ba0b33d246379a9dc71fc8f45daf4a4a3b6991b3e65e0eb97c2f0e` |
| 3 | `70a16564f3ba0b33d246379a9dc71fc8f45daf4a4a3b6991b3e65e0eb97c2f0e` |

An unequal recoverable ordinary-event set alone is insufficient: missing interleaved messages remain unknown. The ordinary-log-only comparator deliberately does not treat that absence as proof of non-execution.

The full-outcome adjudicator additionally compares **presence/type/size of every original installed seed file**, using the complete post-termination manifests. It finds exactly one structural difference, without selecting a filename in advance. It ignores mtimes, same-size content-hash changes, new transient cache files, profiles and directory metadata. Thus the classification rests on a measured file-state difference, not the ordinary-log absence or timestamp differences. This host-only audit adjudicator does not change the frozen launcher, normalizer, executable or guest inputs.

Full combined signatures are `34f354387dc256c20ce26683ca26d5e7eafc3fbfc1cee0e1467ddb63dd7a1d2a` for Run 1 and `2345eb3a3f414bb1ac9b442c5bf89b2838c63ce9004177ef95bbe474fb6126e6` for Runs 2/3. These signatures include the installed-file structural outcome and the normalized ordinary observations; they are not a root-cause attribution.

## Classification

**NATIVE_BOOT_NONDETERMINISTIC**

Matched controlled inputs produce a proven difference in persistent guest file outcomes during the bounded boot experiment. This establishes non-reproducibility at this available observability level. It does not establish why the difference occurs or whether either outcome is incorrect.

## Earliest proven divergence category

**PERSISTENT_INSTALLED_FILE_STATE**

The file audits establish this category after common input gates and checksum/cache outcomes. They do not identify the exact chronological first divergent instruction, call, branch, writer or completion. The changed file's name is evidence for an output difference, not a selected subsystem to investigate. No subsystem probe or localization is performed.

## Evidence / original-data preservation

Evidence root: `F:/Games/MLB16PS3/build-mlb16-native-pinned/task026/`.

- Shared: `build-results.json`, build/configure logs, `build-gate.json`, `seed-manifest.json`, `seed-summary.json`, `prepared.json`, frozen/normalized environment/configuration, initial guest inputs, original baseline, host-tool hashes and comparison specification.
- Each `runN`: full seed manifest/identities, initial guest inputs, preflight/launch isolation gates, `launch-equivalence.json`, `boot-started.json`, `boot.log`, `boot-result.json`, complete original before/after manifests, preservation record, final guest inputs, sandbox mutations, ordinary/full-outcome signatures and checked mirror unmount.
- Comparison: `cross-run-comparison.json` for conservative ordinary observations; **`full-baseline-comparison.json` is the final combined classification**, supported by audited installed-file structural differences. `disk-comparison-method.json`, `comparison-invariance.json` and `final-verification.json` record method/verification details.

For **every run**, complete original before/after manifests equal the shared original baseline: **505 entries / 435 files / 34,330,559,625 hashed bytes**, covering the original game/disc/native HDD tree and complete original RPCS3 HDD tree. Equality includes content hashes, sizes, mtimes, attributes, file identities and link counts. Locks remain held until the child is gone and the audits finish. All **120 disc-file links** across three mirrors are disarmed safely; original targets are preserved.

The physical original installed file used as the difference witness is independently rechecked at the end against its baseline size/hash and remains unchanged. All differing writes are in disposable copies. Canonical configuration/executable hashes remain frozen, all 45 protected build inputs match, and the toolkit remains clean/pinned. No second build or fourth boot occurs.

## What is not proven / uncertainties

- Exact validation return values, startup `00011CE8` return or dispatcher `000136F8` entry/return. All task guest observers are OFF.
- Absence of every possible dialog merely from absent ordinary text. No corrupt-data/dialog marker is recoverable in these runs; that is the reported observation.
- Non-execution of intro/HTTP requests in Run 1. Ordinary-log absence alone is not the classification basis.
- A faulty file writer, corrupted original content, specific guest subsystem, semantic defect, race, or timing cause. A legitimate cache/state update can change file length; the task measures differing outcomes rather than declaring that update wrong.
- Whether a differing file represents completed serialization or an interrupted update at fail-safe termination. Both are distinct measured persistent outcomes under the same prescribed bounded experiment; no writer completion is inferred.
- Identical wall clock, scheduling, CPU/GPU state, driver internals or every external input. The controlled seed, executable and recorded environment/configuration match; guest time is not forced.
- That every hidden execution path is deterministic or nondeterministic, or that more time would converge the outcomes. Three observations do not establish a frequency or a permanent deadlock.
- Gameplay, rendering success or a relationship to black rendering. No visual success claim or subsystem investigation is made.

## Files changed

Only original experiment tooling/documentation is added to the repository:

- `tools/run_task026_baseline.py`: complete-copy preparation, frozen launch-input gates, three one-boot guards and original/sandbox audits; reuses the unchanged prior isolation helper.
- `tools/analyze_task026_baseline.py`: ordinary-event normalization, per-run signatures and conservative observation comparison.
- `tools/compare_task026_outcomes.py`: complete installed-file structural outcome comparison alongside ordinary signatures.
- `docs/superpowers/plans/2026-10-24-task026-baseline.md`: scoped experiment/check plan.
- `docs/TASK-026-FULL-CONTENT-BASELINE.md`: this report.

No runtime/generated code, CMake integration, generic toolkit or game original is modified. Installation copies, executable backup, all bulk logs, manifests and environment data stay outside Git. No binaries or game-derived files are added to the repository. HEAD and toolkit pin are preserved; staging is empty. No commit or push.

## Next smallest step

Use this verified **complete seed** and fresh independent copies for a separately scoped, coarse startup-progress correlation task. Pair main-thread startup stage boundaries before selecting a descendant subsystem. Preserve these three outcomes and the baseline hashes; the changed filename does not identify the responsible subsystem. No follow-up probe or additional boot is part of TASK-026.

## How to verify

PowerShell commands used: normal canonical CMake configure/build (exact arguments and exit codes in `build-results.json`), then `python tools/run_task026_baseline.py --prepare`, followed sequentially by `--run 1`, `--run 2`, `--run 3`. Comparison commands are `python tools/analyze_task026_baseline.py --spec` before the boots and `python tools/compare_task026_outcomes.py` after them. Do not repeat the boot commands against their one-boot guards.

Re-run only the host comparison to inspect saved outcomes. Verify matching launch gates and original manifests, frozen executable/configuration hashes, three seed identities, SYS.MD5 checksums, and final installed-file sizes from `final-guest-inputs.json`. The semantic-invariance check confirms selected milestones are unaffected by altered fd/host-ID/timestamp fields or reordered log lines. Read final-verification results, confirm every task option OFF, toolkit clean at its pin, unchanged source/build inputs and an empty index. No new boot is needed to verify these saved results.
