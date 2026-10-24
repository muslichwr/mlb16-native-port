# Workspace Boundaries and Canonical Layout

This document establishes the authoritative directory structure, ownership, and usage policies for the MLB The Show 16 (BCUS-01089) native PC port workspace on `F:\Games\MLB16PS3`.

---

## 1. Directory Classification and Ownership

Every relevant directory in `F:\Games\MLB16PS3` is classified into exactly one category:

| Path | Classification | Ownership / Description | Allowed use | Forbidden use |
| --- | --- | --- | --- | --- |
| `F:\Games\MLB16PS3\mlb16-port` | `CANONICAL_SOURCE` | Primary Git repository for original code, tools, docs, patches, and build orchestration. | Edit files within active task scope. Track project history. | Never commit game data, raw logs, lifted PPU code, or binaries. |
| `F:\Games\MLB16PS3\mlb16-port\third_party\ps3recomp` | `CANONICAL_DEPENDENCY` | Authoritative ps3recomp dependency checkout (pinned fork). Changes require branch commit on fork and repin. | Authoritative dependency for headers, runtime library, and tools. Read-only in regular tasks. | Never make ad-hoc detached-HEAD edits. Never point builds to external checkouts or create another independent source-of-truth. |
| `F:\Games\MLB16PS3\build-mlb16-native-pinned` | `CANONICAL_BUILD` | The ONLY canonical build directory and experiment staging tree for native PC compilation. | Active CMake/Ninja build target (`MyGameRecomp.exe`). Isolated experiment roots (e.g. `task026/`). | Never commit contents to Git. Do not delete or clobber without preserving rollback records. |
| `F:\Games\MLB16PS3\recompiled-bounded` | `GENERATED` | Lifted PPU C++ translation (30 source units + `ppu_hle_nids.cpp`). | Read-only input for the CMake build. | Never edit, format, or stage generated PPU source files into Git. Do not re-lift during ordinary tasks. |
| `F:\Games\MLB16PS3\build-objects` | `GENERATED` | 30 prebuilt PPU object files (`ppu_recomp_000.o` .. `029.o`). | Linked into `MyGameRecomp.exe` via `prebuilt-ppu.cmake`. | Never modify, overwrite, or delete prebuilt objects. |
| `F:\Games\MLB16PS3\game` | `IMMUTABLE_INPUT` | Decrypted retail game assets, `EBOOT.ELF`, and disc filesystem (`PS3_GAME`). | Read-only input for execution, analysis, and VFS disc emulation. | Strictly immutable. NEVER write to, modify, patch, or delete game files. Never commit to Git. |
| `F:\Games\MLB16PS3\rpcs3emu` | `REFERENCE` | RPCS3 emulator installation, logs, and reference/oracle traces. | Read-only oracle data for comparing behavior against native execution. | Never expose as writable native runtime state. Never use as native runner destination. |
| `F:\Games\MLB16PS3\analysis` | `EXPERIMENT_OUTPUT` | Static analysis dumps, extracted SPU ELFs, and disassembly scratch. | Read-only analysis scratchpad. | Never stage or commit into Git. |
| `F:\Games\MLB16PS3\logs` | `EXPERIMENT_OUTPUT` | Historical build logs and boot execution traces. | Read-only reference for diagnostics. Document findings in `docs/`. | Never commit bulk log dumps to Git. |
| `F:\Games\MLB16PS3\tmp` | `SCRATCH` | Scratch directory for disposable test scripts, temporary compiles, and non-canonical runs. | Temporary scratch work. | Disposable. Never treat as source-of-truth or permanent storage. |
| `F:\Games\MLB16PS3\mlb16-native` | `REFERENCE` | Active runner template and CMake project input. Durable runner integration lives in mlb16-port. | Read-only runner input; integrated via `mlb16-port/cmake/mlb16-pm-integration.cmake`. | Not the durable source-of-truth for MLB16-specific changes. Edits must be documented via patches in `mlb16-port/patches/`. |
| `F:\Games\MLB16PS3\ps3recomp-source` | `LEGACY_DO_NOT_USE` | Frozen legacy external `ps3recomp` checkout. | Preserved frozen rollback evidence only. | STRICTLY FORBIDDEN for new builds, tests, scripts, or includes. |
| `F:\Games\MLB16PS3\build-mlb16-native` | `LEGACY_DO_NOT_USE` | Historical known-good build root tied to `ps3recomp-source`. | Frozen rollback baseline. | Do not rebuild, reconfigure, or modify. |
| `F:\Games\MLB16PS3\build-runtime-clang64` | `LEGACY_DO_NOT_USE` | Historical standalone runtime build directory. | Historical reference only. | Do not use for new builds or tests. |
| `F:\Games\MLB16PS3\smoke-recompiled` | `LEGACY_DO_NOT_USE` | Historical preliminary PPU recompiled output. | Historical reference only. | Do not use. Replaced by `recompiled-bounded`. |
| `F:\Games\MLB16PS3\ps3recomp-v0.17.0-win-x64` | `LEGACY_DO_NOT_USE` | Upstream binary release archive. | Historical reference only. | Do not use. |

---

## 2. Hard Workspace Rules

1. **Single Source of Truth**: `F:\Games\MLB16PS3\mlb16-port` is the sole source-controlled project repository.
2. **Submodule Dependency**: `F:\Games\MLB16PS3\mlb16-port\third_party\ps3recomp` is the ONLY canonical `ps3recomp` dependency. All builds, scripts, and includes must resolve here.
3. **ps3recomp Submodule Policy**: No ad-hoc detached-HEAD edits. Persistent changes must be committed to the `muslichwr/ps3recomp` fork on a named branch, verified there, and then `mlb16-port` must repin the submodule to that commit. Never create another independent `ps3recomp` source-of-truth.
4. **Runner Integration Ownership**: `mlb16-native` is an active runner and build input, but is not the durable source-of-truth for MLB16-specific changes. Durable runner integration remains represented from `mlb16-port` (e.g. `cmake/mlb16-pm-integration.cmake` and reviewable patches in `patches/`).
5. **No Legacy Checkout Access**: `ps3recomp-source` is strictly `LEGACY_DO_NOT_USE`. New scripts and CMake commands must never reference this path.
6. **Canonical Build Directory**: `F:\Games\MLB16PS3\build-mlb16-native-pinned` is the active build target. `build-mlb16-native` is frozen rollback evidence.
7. **Game Data Immutability**: `F:\Games\MLB16PS3\game` is strictly `IMMUTABLE_INPUT`.
8. **RPCS3 Isolation**: `F:\Games\MLB16PS3\rpcs3emu` is reference/oracle data only. It must never be targeted by native runtime writes or symlinked as active native state.
9. **Isolated Runtime Writes**: Any test, boot, or benchmark that performs filesystem writes (e.g., HDD0, cache, temp) must operate in an isolated disposable sandbox under `F:` (e.g. `build-mlb16-native-pinned/<experiment>` or `tmp/<experiment>`), never against canonical sources or game trees.
10. **Git Commit Boundaries**: No game files, decrypted ELFs, `.WAD`, `.sdat`, `.o` objects, `.exe` binaries, or raw logs may ever be committed to Git.

---

## 3. Toolchain and Build Standards

- **Host Environment**: MSYS2 CLANG64 installed under `D:\APP\msys64`.
- **Compilers**: `clang` and `clang++` (version 23.1.3 or current CLANG64 package).
- **Build System**: `cmake` (version 3.31+) with `Ninja` generator.
- **Canonical Configure Command**:
  ```powershell
  $env:PATH = "D:\APP\msys64\clang64\bin;D:\APP\msys64\usr\bin;" + $env:PATH
  $env:MSYSTEM = "CLANG64"
  & "D:\APP\msys64\clang64\bin\cmake.exe" `
      -B "F:/Games/MLB16PS3/build-mlb16-native-pinned" `
      -S "F:/Games/MLB16PS3/mlb16-native" `
      -G Ninja `
      -DCMAKE_BUILD_TYPE=Release `
      -DCMAKE_C_COMPILER=clang `
      -DCMAKE_CXX_COMPILER=clang++ `
      -DPS3RECOMP_DIR="F:/Games/MLB16PS3/mlb16-port/third_party/ps3recomp" `
      -DRECOMP_DIR="F:/Games/MLB16PS3/recompiled-bounded"
  ```
- **Canonical Build Command**:
  ```powershell
  & "D:\APP\msys64\clang64\bin\ninja.exe" -C "F:/Games/MLB16PS3/build-mlb16-native-pinned"
  ```
- **PPU Object Reuse**: `prebuilt-ppu.cmake` reuses the 30 object files from `build-objects`. Do not inadvertently invoke PPU recompilation.
- **PM Integration**: The raw SPU PM lift (`src/spu/image0077/`) and startup wrapper (`src/spu_pm_startup.c`) are automatically linked via `cmake/mlb16-pm-integration.cmake`.
