# Agent instructions — MLB The Show 16 native port

## Purpose and scope

This repository (`mlb16-port`) holds original build orchestration, reviewable patches, documentation, and reproducible diagnostics for an experimental Windows x64 port of MLB The Show 16 (PS3, BCUS-01089) using ps3recomp v0.17.0. It is **not** a redistribution of the game or a claim of playability.

Read `docs/PROJECT_STATE.md` and the currently assigned task document **before changing files**. The first assigned task is the investigation in `docs/TASK-001-SPU.md` and is **read-only outside this repository**. Documentation review does not authorize starting that investigation or implementing SPU changes; wait for further instructions after the review.

## Local workspace (Windows paths)

| Location | Classification | Purpose and policy | Default agent permission |
| --- | --- | --- | --- |
| `F:\Games\MLB16PS3\mlb16-port` | `CANONICAL_SOURCE` | Main Git repository: original scripts, docs, patches, tests | Edit only files within assigned task scope |
| `F:\Games\MLB16PS3\mlb16-port\third_party\ps3recomp` | `CANONICAL_DEPENDENCY` | Authoritative ps3recomp submodule checkout (pinned fork). Changes require branch commit on fork and repin. | Read-only in regular tasks; changes require explicit authorization |
| `F:\Games\MLB16PS3\build-mlb16-native-pinned` | `CANONICAL_BUILD` | Canonical native build root (BUILD ONLY). Completed experiments must not reside permanently here. | Active build/link target; do not commit contents to Git |
| `F:\Games\MLB16PS3\recompiled-bounded` | `GENERATED` | Generated lifted PPU C++ code (30 units + NID table) | Read-only; never edit or stage generated output |
| `F:\Games\MLB16PS3\build-objects` | `GENERATED` | 30 prebuilt PPU object files (.o) reused by CMake | Read-only; do not modify or delete |
| `F:\Games\MLB16PS3\game` | `IMMUTABLE_INPUT` | Decrypted game executable (EBOOT.ELF) and disc assets | Read-only; never stage, copy into Git, or modify |
| `F:\Games\MLB16PS3\rpcs3emu` | `REFERENCE` | RPCS3 emulator and reference/oracle data | Read-only reference; never expose as writable native state |
| `F:\Games\MLB16PS3\experiments` | `SCRATCH` | Dedicated root for future disposable runtime experiment sandboxes | Disposable sandboxes; compacted after evidence audit |
| `F:\Games\MLB16PS3\analysis` | `EXPERIMENT_OUTPUT` | Local analysis dumps and extracted SPU ELFs | Read-only; never stage or copy into Git |
| `F:\Games\MLB16PS3\logs` | `EXPERIMENT_OUTPUT` | Local build and boot traces | Read-only; summarize findings in docs, not log dumps |
| `F:\Games\MLB16PS3\tmp` | `SCRATCH` | Scratch and temporary experiment workspaces | Disposable scratch; never treat as source-of-truth |
| `F:\Games\MLB16PS3\mlb16-native` | `REFERENCE` | Active runner and CMake project input. Durable runner integration lives in mlb16-port. | Read-only runner input; integrate via mlb16-port |
| `F:\Games\MLB16PS3\ps3recomp-source` | `LEGACY_DO_NOT_USE` | Preserved legacy external ps3recomp checkout | Do not use for new builds, tests, or investigations |
| `F:\Games\MLB16PS3\build-mlb16-native` | `LEGACY_DO_NOT_USE` | Preserved legacy rollback build tree | Frozen rollback baseline; do not rebuild or modify |
| `F:\Games\MLB16PS3\build-runtime-clang64` | `LEGACY_DO_NOT_USE` | Historical standalone runtime build tree | Legacy only; do not use |
| `F:\Games\MLB16PS3\smoke-recompiled` | `LEGACY_DO_NOT_USE` | Historical preliminary recompiled lift | Legacy only; do not use |
| `F:\Games\MLB16PS3\ps3recomp-v0.17.0-win-x64` | `LEGACY_DO_NOT_USE` | Upstream binary release archive | Legacy only; do not use |

The saved VS Code multi-root workspace is `F:\Games\MLB16PS3\mlb16-port\mlb16-port.code-workspace`; its folders are `.` and `..`. **VS Code folder visibility is not proof that a file belongs in this Git repository.** The active Git checkout for project commits is `mlb16-port`, not any external checkout.

## Hard workspace rules

1. `mlb16-port` is the single source-of-truth repository for this project.
2. `third_party/ps3recomp` is the ONLY canonical ps3recomp dependency checkout. All builds, headers, and tests must resolve against it.
3. `third_party/ps3recomp` policy: No ad-hoc detached-HEAD edits. Persistent ps3recomp changes must be committed to the `muslichwr/ps3recomp` fork on a named branch, verified there, and then `mlb16-port` must repin the submodule to that commit. Never create another independent ps3recomp source-of-truth.
4. `mlb16-native` is an active runner and build input, but is not the durable source-of-truth for MLB16-specific changes; durable runner integration remains represented from `mlb16-port` (e.g. `cmake/mlb16-pm-integration.cmake`).
5. `ps3recomp-source` is strictly `LEGACY_DO_NOT_USE`. It is preserved only as frozen rollback evidence; new tasks must never point compilers, scripts, or includes at it.
6. `build-mlb16-native-pinned` is the ONLY canonical build directory (BUILD ONLY). Completed experiment sandboxes are not permanent source-of-truth and must not reside permanently in the build root.
7. Future disposable runtime experiment sandboxes must use `F:\Games\MLB16PS3\experiments`. Only one active experiment needs full writable game copies at any time, and completed experiments must be compacted after an evidence-retention audit. Compact logs, JSON manifests, and witness files are durable evidence; bulk duplicated immutable game data is not.
8. `game` is strictly `IMMUTABLE_INPUT`. Never write to or mutate original game files.
9. `rpcs3emu` is strictly `REFERENCE` oracle data. Never write to RPCS3 directories or use them as active native runtime write destinations.
10. All runtime writes must be targeted to disposable experiment directories under `F:`, never against original game trees.
11. No game files, `.WAD`, `.sdat`, extracted binaries, `.o` objects, `.exe` executables, or raw logs may ever be committed to Git.

## Non-negotiable constraints

- Never add, copy, commit, or push game files, decrypted ELF/EBOOT, `.WAD`, `.sdat`, embedded SPU programs, generated PPU/SPU game code, game-derived object files, native game executables, or bulk boot logs.
- Do not upload game content or derived code to an external service. Source inspection should stay local.
- Preserve a reproducible path from a clean ps3recomp checkout to every proposed runtime change. Keep runtime changes as reviewable patch files in `patches/` when explicitly approved.
- Do not modify `ps3recomp-source`, `mlb16-native`, generated code, or the already successful build during TASK-001. Do not rebuild or boot the game during TASK-001 unless explicitly requested.
- Do not fake SPU job completion, stub out synchronization, discard RSX errors, or return unconditional success from unimplemented HLE functions to force progress.
- Do not install Visual Studio Community, move files to `C:`, or perform large downloads/installations without an explicit request. The working toolchain is MSYS2 CLANG64 installed under `D:\APP\msys64`.
- Do not run destructive Git commands (`reset --hard`, `clean -fdx`, force push), stage everything (`git add .`), commit, or push unless the user explicitly asks.
- Distinguish **observed facts**, **hypotheses**, and **confirmed fixes**. A successful link or black window is not proof of game playability.

## Engineering workflow

1. Read the active task, inspect existing sources and local documentation, and verify assumptions against actual files.
2. Write a short, evidence-based finding: exact file path, symbol, relevant log line, or command output. Do not assert undocumented behavior.
3. Propose the smallest independently verifiable change, along with success criteria and rollback steps.
4. Before modifying runtime code, request approval if the active task is read-only; only then implement a single scoped change and test it.
5. Keep scripts idempotent where possible; use `F:` for large build outputs and temporary data.
6. Report precisely which commands were run, what passed, what failed, and what remains unknown.

## Terminal and build conventions

- Windows PowerShell and **MSYS2 CLANG64 Bash** have different syntax; label terminal-specific commands.
- Existing native target: `MyGameRecomp`, output `F:\Games\MLB16PS3\build-mlb16-native\MyGameRecomp.exe`.
- The native runner is built with CMake/Ninja and Clang, not MSVC.
- Game PPU objects were compiled separately and reused by CMake; do not inadvertently compile the entire generated PPU tree again.
- A temporary workaround in `ppu_loader.cpp` addresses MinGW/Clang SEH incompatibility and an inconsistent `ppu_resv_break` declaration. The existing `patches/ppu-loader-clang64.patch` matches the inspected local source diff; it has not been validated by a clean rebuild in this documentation review. Preserve it for review rather than silently rewriting it.

## Required report format

For each task report: **Summary**, **Evidence**, **Uncertainties**, **Recommended next smallest step**, **How to verify**, **Files changed**. If a tool or file is unavailable, say so rather than guessing.
