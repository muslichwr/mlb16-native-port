# Completed authorized migration - PRE-TASK-018

Status: migrated on 2026-10-23. The preparation report below remains historical evidence; its pending authorization text and proposed root-level path are superseded by this addendum.

## Summary

GitHub fork: https://github.com/muslichwr/ps3recomp.git (parent: sp00nznet/ps3recomp).
Submodule: `third_party/ps3recomp`, pinned to `bc5b3d30503169a025c88bd9e6195728fc8ad68d`.

## Fork commit history and upstream/base relationship

Exact base: `a6d76c895f77eb30b90a806c830544e61821b77f` (`v0.17.0`).
1. `0f8bf4bad8cd7ba26a98e05aa3afe3dc33cf5422`: PPU loader CLANG64 accommodation only.
2. `bc5b3d30503169a025c88bd9e6195728fc8ad68d`: interpreter policy-exit correction plus the original synthetic regression, copied byte-for-byte.

Published only `refs/heads/mlb16-runtime` after showing both commits/ref and passing push dry-run. No squash or force push. Normal complete fork clone retains an upstream remote. The old external checkout remains untouched.

## Evidence and patch/provenance status

Both runtime postimages match the approved patches. The combined diff contains exactly the two runtime files and original test. Policy source equals upstream; TASK-017 is not in the persistent commits. All existing patch hashes remain unchanged. The manifest records the exact fork tip and submodule path.

Optional diagnostic from parent root: `git -C third_party/ps3recomp apply --check ../../patches/spurs-pm-pipe-all-lane-diagnostic.patch`, then apply if separately wanted. Do not reapply persistent patches.

Future isolated CMake configurations use `-DPS3RECOMP_DIR=F:/Games/MLB16PS3/mlb16-port/third_party/ps3recomp`. No build configuration/cache was changed in this migration.

## Fresh reconstruction verification

PASSED: a fresh local clone of the committed parent initialized the submodule from GitHub. Gitlink, lock and submodule HEAD agree; both fresh checkouts are clean. All 850 original upstream files are byte-identical to the persistent reconstruction; the extra synthetic test is byte-identical to its original fixture. The optional diagnostic passes apply-check without being applied. All 400 protected artifacts and old external source files are unchanged. Verification details are local in tmp/ps3recomp-checkpoint/migration-result.json. Parent checkpoint is committed locally; parent main has not been pushed.

## Files changed

Parent scope is dependency gitlink/metadata, checkpoint documentation/audit, patch provenance, verifier and LF attributes. Unrelated historical task documents, startup code and local tests are excluded from the parent commit. Existing 400 build artifacts are checked against the preservation manifest.

## Uncertainties and next smallest step

Loader remains a platform accommodation with its prior validation limits. No new runtime behavior or game validation is claimed. Next work requires a separate assignment; no descriptor/producer investigation was performed.

BOOT TEST: TIDAK
TASK-018: BELUM DIMULAI

---

# ps3recomp dependency checkpoint

Date: **2026-10-23**. Status: **PREPARED; fork/submodule migration awaits authorization.** No SPURS debugging, boot, build, staging, commit, push, remote creation, or GitHub repository creation was performed.

## Historical preparation summary

Every currently modified toolkit source file has complete, separately recoverable patch coverage. Starting with the exact upstream revision and applying the three existing patches reproduces **all 850 tracked files byte-for-byte**, including the three modifications. A second verified profile contains only the two persistent changes. The old external checkout is no longer the only local recovery path for its source modifications.

This is not yet a version-controlled fork migration. The patches and new checkpoint files must be committed, the fork commits must be published, and a real submodule gitlink must be recorded before a clean remote clone can initialize the proposed dependency. The manifest explicitly leaves the future fork URL/commit null and `submodule.recorded=false`; there is no placeholder `.gitmodules` or misleading gitlink.

Proposed topology:

```text
mlb16-native-port.git                  main port repository
  dependencies/ps3recomp.lock.json     upstream provenance + fork commit + patch hashes
  ps3recomp/                          submodule at the exact persistent fork commit
  patches/                            recovery patches + optional TASK-017 diagnostic
  scripts/verify-ps3recomp-checkpoint.py

user/project ps3recomp fork
  upstream v0.17.0 / a6d76c895f77eb30b90a806c830544e61821b77f
    -> separate CLANG64 compatibility commit
    -> separate interpreter policy-exit correction + synthetic test commit

F:/Games/MLB16PS3/ps3recomp-source      preserved old experiment; no longer canonical
F:/Games/MLB16PS3/build-*              preserved existing build artifacts
```

Keep TASK-017 as an optional patch, rather than making title diagnostics part of the default persistent fork tip. Applying that patch locally reconstructs the audited experiment; reversing it returns to the persistent dependency. No arbitrary runtime source file is vendored into this port repository.

## Evidence

### Audit recorded before project edits

The complete initial machine-readable audit is [`PS3RECOMP_CHECKPOINT_AUDIT.json`](PS3RECOMP_CHECKPOINT_AUDIT.json). A local copy and full initial tracked diff are under `F:/Games/MLB16PS3/tmp/ps3recomp-checkpoint/{audit-before.json,tracked-before.patch}`.

| Property | Observed state |
| --- | --- |
| Port remote | `origin https://github.com/muslichwr/mlb16-native-port.git` for fetch/push |
| Port branch / HEAD | `main` / `f80d45558f4783aaddc9de0e077802f57fedf7dd` |
| Initially tracked port files | `.gitignore`, `README.md`, `docs/STATUS.md`, `patches/ppu-loader-clang64.patch` |
| Port index | Empty staged diff; other prior documents, tests, original startup glue and two patches are untracked (exact list in audit) |
| Toolkit remote | `origin https://github.com/sp00nznet/ps3recomp.git` for fetch/push; no fork/upstream alternate remote |
| Toolkit branch | Detached HEAD; no current branch |
| Toolkit HEAD / tag | `a6d76c895f77eb30b90a806c830544e61821b77f` / `v0.17.0` |
| Toolkit history | Shallow checkout; existing object pack approximately 3.81 MiB |
| Toolkit staged diff | Empty |
| Modified toolkit files | `runtime/ppu/ppu_loader.cpp`, `runtime/spu/spu_interp.c`, `runtime/spu/spurs_policy.c` |
| Untracked toolkit file | `runtime/ppu/ppu_loader.cpp.before-clang64` |
| Ignored toolkit files | Only two `tools/__pycache__/*.pyc` files; no ignored source files |

### Exact changes and classification

| File / patch | Exact local change | Classification and destination |
| --- | --- | --- |
| `runtime/ppu/ppu_loader.cpp` / `ppu-loader-clang64.patch` | In `samp_stack_guest`, replace `__try`/`__except` stack read with checked `ReadProcessMemory(GetCurrentProcess(), ...)`; change the `ppu_resv_break` forward declaration from external C linkage to `static inline` | Persistent CLANG64 build accommodation, in its own fork commit. Preserve its workaround/review status; this checkpoint does not establish universal portability or perform a clean rebuild. |
| `runtime/spu/spu_interp.c` / `spu-interpreter-policy-exit.patch` | At interpreter loop entry, intercept `policy_mode && pc == SPURS_PM_EXIT_TO_KERNEL_LS`, update the step counter, invoke existing indirect service dispatch, and return if no nonlocal halt occurs | Generic runtime correction, in its own fork commit with the original synthetic policy-exit regression. TASK-014/017 provide prior regression evidence; no runtime fix is introduced here. |
| `runtime/spu/spurs_policy.c` / `spurs-pm-pipe-all-lane-diagnostic.patch` | Remove lane-0 observation gate; reserve first three observations per workload ID atomically; add lane/instance identity; serialize complete single-line probe output using existing CRT locks | Temporary TASK-017 diagnostic, retained as a separate optional patch; exclude from persistent fork commits. |
| `runtime/ppu/ppu_loader.cpp.before-clang64` | Byte-identical to the loader in the exact upstream base | Redundant recovery copy. Fully reconstructable from the upstream Git object; no new source patch/commit required. Leave the existing file untouched. |

All three patch files were already present before this checkpoint; none was silently regenerated. Their SHA-256 values, upstream file hashes, and modified file hashes are recorded in [`../dependencies/ps3recomp.lock.json`](../dependencies/ps3recomp.lock.json). There are **no uncovered source changes**. The interpreter and diagnostic patches are currently untracked in the main repository, so they are recoverable locally but not yet durable in a published commit.

### Reconstruction and protected-state verification

Using local Git objects only, exported the exact base to new directories under `F:/Games/MLB16PS3/tmp/ps3recomp-checkpoint`, without copying the dirty checkout. All three patches pass `git apply --check` and apply successfully. The resulting 850 files match the audited checkout byte-for-byte. The persistent-only profile also verifies all 850 files against upstream blob identities plus its two expected modified-file hashes.

The committed-base export must use `git -c core.autocrlf=false archive`: the first attempt inherited `core.autocrlf=true` and exported CRLF text, so byte comparison failed. Inspection confirmed line-ending differences; repeating with LF produced zero mismatches. This was a verification-command correction, not a source change. A read of `HEAD:.gitattributes` reported that no such file exists; this is why the verifier explicitly selects LF conversion behavior. GitHub CLI `gh` is not available on PATH; no installation was attempted.

The reusable verifier reads the manifest, checks patch/base hashes before applying, writes only to a new child of the workspace's `tmp` directory, validates every reconstructed file, and optionally byte-compares an existing checkout. It never clones, initializes Git, changes remotes, stages, commits, builds, or boots. It refuses existing output directories rather than deleting or overwriting them.

The port's new `.gitattributes` keeps patch files, dependency JSON and Python scripts in LF form, so raw patch hashes remain stable after Windows clones with `core.autocrlf=true`.

All **400 existing build artifacts** selected at audit time are protected by size/mtime manifests, with SHA-256 additionally recorded for PPU objects, runtime archives, executables and build configuration files. This includes both existing build trees, all 30 PPU objects, and the saved experimental executables. Final comparison results are local in `tmp/ps3recomp-checkpoint/protected-after.json`. Toolkit source hashes, existing patch hashes, both repositories' HEADs/remotes, and both index contents are also checked against the initial audit.

### Build reference transition

The external `mlb16-native/CMakeLists.txt` already accepts `PS3RECOMP_DIR` for runtime, scaffold and tooling paths. No source edit or existing-cache rewrite is necessary. After the real submodule is recorded and initialized, future builds can select it with `-DPS3RECOMP_DIR=F:/Games/MLB16PS3/mlb16-port/ps3recomp`.

For a future isolated configure, use the established CLANG64 environment, a **new** output directory such as `F:/Games/MLB16PS3/build-mlb16-pinned`, the existing `RECOMP_DIR`, and explicit `MLB16_PREBUILT_DIR=F:/Games/MLB16PS3/build-objects`. Preserve `PORT_ENABLE_SPU=ON`, empty `PORT_SPU_INPUTS`, the existing `SPU_DIR`, and existing FFmpeg/Vulkan options. The runner's existing `prebuilt-ppu.cmake` requires exactly 30 objects and sets `RECOMP_SOURCES` to those external objects. Do not use the old successful build directory for the transition.

No configure was executed. This describes dependency source selection only: reproducing TASK-017's experimental executable also requires the existing local PM object/startup link arrangement, which is outside this dependency checkpoint. A source-complete dependency is not a source-complete game build without locally supplied game inputs and generated artifacts.

### Prepared migration commands — NOT EXECUTED

The first action requiring authorization is to **create or select the dedicated fork and confirm its URL**, then authorize the scoped commits, pushes, and submodule staging below. Proposed owner, inferred from the port's remote: `muslichwr`; proposed URL: `https://github.com/muslichwr/ps3recomp.git`. This URL has not been checked for existence or ownership. Create the fork through GitHub's upstream fork action, retaining upstream provenance; do not create an unrelated repository with copied files.

Once that URL and Git actions are explicitly authorized, these are the prepared **PowerShell** operations. `g` stops on any Git failure. Use a fresh fork working directory; preserve the old checkout.

```powershell
function g { & git @args; if ($LASTEXITCODE -ne 0) { throw 'Git command failed' } }
$portRoot = 'F:/Games/MLB16PS3/mlb16-port'
$forkWork = 'F:/Games/MLB16PS3/tmp/ps3recomp-fork-prep'
$forkUrl = 'https://github.com/muslichwr/ps3recomp.git' # CONFIRM before execution
$base = 'a6d76c895f77eb30b90a806c830544e61821b77f'
if (Test-Path -LiteralPath $forkWork) { throw 'Choose a fresh fork working directory' }
g -c core.autocrlf=false clone $forkUrl $forkWork
g -C $forkWork config core.autocrlf false
g -C $forkWork remote add upstream 'https://github.com/sp00nznet/ps3recomp.git'
g -C $forkWork fetch upstream
g -C $forkWork switch --create mlb16-runtime $base

g -C $forkWork apply --check "$portRoot/patches/ppu-loader-clang64.patch"
g -C $forkWork apply "$portRoot/patches/ppu-loader-clang64.patch"
g -C $forkWork add -- runtime/ppu/ppu_loader.cpp
g -C $forkWork diff --cached --check
g -C $forkWork diff --cached --stat
g -C $forkWork commit -m 'runtime: support checked stack reads and internal reservation declaration on CLANG64'

g -C $forkWork apply --check "$portRoot/patches/spu-interpreter-policy-exit.patch"
g -C $forkWork apply "$portRoot/patches/spu-interpreter-policy-exit.patch"
$testTarget = "$forkWork/runtime/spu/tests/test_spurs_interpreter_exit.c"
if (Test-Path -LiteralPath $testTarget) { throw 'Review existing upstream test before copying' }
Copy-Item -LiteralPath "$portRoot/tests/test_spurs_interpreter_exit.c" -Destination $testTarget
g -C $forkWork add -- runtime/spu/spu_interp.c runtime/spu/tests/test_spurs_interpreter_exit.c
g -C $forkWork diff --cached --check
g -C $forkWork diff --cached --stat
g -C $forkWork commit -m 'spu: intercept policy exit before interpreting service local store'

g -C $forkWork merge-base --is-ancestor $base HEAD
$runtimeTip = (g -C $forkWork rev-parse HEAD).Trim()
g -C $forkWork diff --stat $base $runtimeTip
g -C $forkWork status --short
# Review the exact two runtime changes plus original synthetic test before publication.
g -C $forkWork push --set-upstream origin mlb16-runtime

if (Test-Path -LiteralPath "$portRoot/ps3recomp") { throw 'Review existing dependency path first' }
g -c core.autocrlf=false -C $portRoot submodule add $forkUrl ps3recomp
g -C "$portRoot/ps3recomp" config core.autocrlf false
g -C "$portRoot/ps3recomp" fetch origin mlb16-runtime
g -C "$portRoot/ps3recomp" checkout --detach $runtimeTip
if ((g -C "$portRoot/ps3recomp" rev-parse HEAD).Trim() -ne $runtimeTip) {
    throw 'Submodule commit mismatch'
}

$lockPath = "$portRoot/dependencies/ps3recomp.lock.json"
$lockData = Get-Content -LiteralPath $lockPath -Raw | ConvertFrom-Json
$lockData.state = 'pinned'
$lockData.fork.url = $forkUrl
$lockData.fork.commit = $runtimeTip
$lockData.submodule.recorded = $true
$lockData.migration_note = 'Persistent fixes are in the pinned fork commit; retain base patches for recovery and TASK-017 as an optional diagnostic overlay.'
$lockText = ($lockData | ConvertTo-Json -Depth 20) + "`n"
[IO.File]::WriteAllText($lockPath, $lockText, [Text.UTF8Encoding]::new($false))

# Explicit allowlist; never git add . or add generated/game inputs.
g -C $portRoot add -- .gitattributes .gitmodules ps3recomp dependencies/ps3recomp.lock.json `
    docs/PS3RECOMP_CHECKPOINT_AUDIT.json docs/PS3RECOMP_DEPENDENCY_CHECKPOINT.md `
    docs/PROJECT_STATE.md docs/TASK-017-ALL-LANE-SPURS-TRACE.md `
    AGENTS.md docs/TASK-001-SPU.md docs/SPU_AUDIT_001.md `
    patches/README.md patches/ppu-loader-clang64.patch `
    patches/spu-interpreter-policy-exit.patch patches/spurs-pm-pipe-all-lane-diagnostic.patch `
    scripts/verify-ps3recomp-checkpoint.py `
    tests/test_spurs_interpreter_exit.c tests/test_spu_interrupt_roundtrip.c
g -C $portRoot diff --cached --stat
g -C $portRoot ls-files --stage -- ps3recomp
# Require mode 160000 and object ID exactly $runtimeTip; inspect the entire staged allowlist.
# No game-derived data, code, object, executable, or boot log may be staged.
g -C $portRoot commit -m 'build: pin ps3recomp fork and preserve runtime checkpoint'
g -C $portRoot push origin main
```

The exact fork tip cannot be stated before its commits exist. Capture `$runtimeTip` as above and record that full SHA both in the lock and parent gitlink; never pin only the tag or branch name. The staged-path review must also reject any unexpected already-staged path introduced after this audit. Mark the checkpoint status as migrated only after the operations and fresh-clone verification succeed; this report's current status remains an audit record until then.

After publication, a fresh checkout is reconstructed with `git -c core.autocrlf=false clone --recurse-submodules https://github.com/muslichwr/mlb16-native-port.git <new-directory>`, or `git -c core.autocrlf=false submodule update --init --recursive` inside an existing clean clone. The explicit LF setting matches this checkpoint's byte-level verification. Verify the initialized submodule HEAD equals `fork.commit` in the lock and the gitlink. The old external checkout is not an input to those commands. For the temporary diagnostic, run `git -C ps3recomp apply --check ../patches/spurs-pm-pipe-all-lane-diagnostic.patch`, then apply it; do not apply the two persistent patches again. Reverse only the diagnostic to recover a clean pinned submodule.

## Uncertainties

- Fork URL/ownership, new commit IDs, external publication, and clean remote clone/submodule initialization are pending authorization and are **not verified**. No fork or submodule has been created locally or externally.
- There is no local commit containing the three toolkit changes. Upstream plus the hash-checked patch profiles is the verified interim source recipe; it becomes durable version-controlled recovery only after the appropriate main-repository commit is authorized.
- Persistent classification concerns placement and intended maintenance, not a claim that every change is broadly validated. The loader accommodation retains its documented workaround status.
- A full historical upstream ancestry is absent locally because the old checkout is shallow. Preserve the exact base now, and use a normal complete fork clone for migration; do not rewrite the old checkout to manufacture ancestry.
- The external runner and game-derived build inputs remain separate local artifacts. Dependency reproducibility alone does not imply a distributable or playable game build.

## Recommended next smallest step

Authorize creation/selection of the fork URL and the explicitly scoped fork commits/push, submodule addition, and main checkpoint commit/push above. These permissions are required by the user's checkpoint instruction: **“Do not stage, commit, push, create remotes, or create GitHub repositories unless explicitly authorized by the user.”** Nothing in this checkpoint authorizes resuming TASK-018.

## How to verify

The following **PowerShell** checks were executed successfully. Output directories were new; choose new names if repeating.

```powershell
py -3 -B scripts/verify-ps3recomp-checkpoint.py `
    --source F:/Games/MLB16PS3/ps3recomp-source `
    --output F:/Games/MLB16PS3/tmp/ps3recomp-checkpoint/verified-task017 `
    --profile task017 --compare F:/Games/MLB16PS3/ps3recomp-source

py -3 -B scripts/verify-ps3recomp-checkpoint.py `
    --source F:/Games/MLB16PS3/ps3recomp-source `
    --output F:/Games/MLB16PS3/tmp/ps3recomp-checkpoint/verified-persistent `
    --profile persistent
```

Each reports 850 verified files; the first applies three patches and byte-compares the external checkout; the second applies two patches. Existing-output refusal was also checked without altering that directory. No new runtime behavior is implemented, so no runtime regression build or game boot is needed for this checkpoint. Prior TASK-017 runtime regression results remain historical evidence.

Final checks compare protected-file manifests and initial source/index/HEAD/remote snapshots, validate JSON and patch hashes, inspect newly created paths, and run whitespace checks on new documentation/script files. No game-derived file is a checkpoint output in the port repository. Git status/index checks do not constitute staging.

## Files changed

Added:

- `.gitattributes` — stable LF checkout for recovery patches and dependency metadata/scripts.
- `dependencies/ps3recomp.lock.json` — explicit upstream base, patch hashes/profiles, pending fork/submodule state.
- `docs/PS3RECOMP_CHECKPOINT_AUDIT.json` — exact source-control state recorded before edits.
- `docs/PS3RECOMP_DEPENDENCY_CHECKPOINT.md` — this report and prepared authorization-gated migration commands.
- `patches/README.md` — permanent/temporary patch inventory and application rules.
- `scripts/verify-ps3recomp-checkpoint.py` — offline reconstruction verifier.

Updated only the opening checkpoint pointer in `docs/PROJECT_STATE.md`; the historical baseline remains intact. Existing patches, original tests, startup source, runtime checkout, build references/cache, build outputs, remotes, indexes, and Git history were not modified. Local-only audit exports/reconstructions/manifests are outside this repository under `tmp/ps3recomp-checkpoint`.

**Stop: migration prepared pending explicit Git/external authorization; TASK-018 not started.**
