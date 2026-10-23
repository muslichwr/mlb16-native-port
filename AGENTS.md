# Agent instructions — MLB The Show 16 native port

## Purpose and scope

This repository (`mlb16-port`) holds original build orchestration, reviewable patches, documentation, and reproducible diagnostics for an experimental Windows x64 port of MLB The Show 16 (PS3, BCUS-01089) using ps3recomp v0.17.0. It is **not** a redistribution of the game or a claim of playability.

Read `docs/PROJECT_STATE.md` and the currently assigned task document **before changing files**. The first assigned task is the investigation in `docs/TASK-001-SPU.md` and is **read-only outside this repository**. Documentation review does not authorize starting that investigation or implementing SPU changes; wait for further instructions after the review.

## Local workspace (Windows paths)

| Location | Use | Default agent permission |
| --- | --- | --- |
| `F:\Games\MLB16PS3\mlb16-port` | This Git repository: original scripts, docs, patches | Edit only files within assigned task scope |
| `F:\Games\MLB16PS3\ps3recomp-source` | ps3recomp source, tools and runtime (separate Git checkout) | Read only for TASK-001 |
| `F:\Games\MLB16PS3\mlb16-native` | Native runner template and CMake project | Read only for TASK-001 |
| `F:\Games\MLB16PS3\analysis\spu-images` | Five locally extracted SPU ELF images | Read only; never stage or copy into Git |
| `F:\Games\MLB16PS3\game` | Locally owned game executable and assets | Read only; never stage or copy into Git |
| `F:\Games\MLB16PS3\logs` | Local build/boot traces | Read only; summarize findings in docs, not full log dumps |
| `F:\Games\MLB16PS3\recompiled-bounded` | Generated lifted PPU C++ | Read only; never stage or edit generated output |
| `F:\Games\MLB16PS3\build-objects` | 30 prebuilt PPU objects | Do not modify |
| `F:\Games\MLB16PS3\build-mlb16-native` | Native runner build output | Do not modify in TASK-001 |

The saved VS Code multi-root workspace is `F:\Games\MLB16PS3\mlb16-port\mlb16-port.code-workspace`; its folders are `.` and `..`. **VS Code folder visibility is not proof that a file belongs in this Git repository.** The active Git checkout for project commits is `mlb16-port`, not `ps3recomp-source`.

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
