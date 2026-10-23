# Project state — MLB The Show 16 native port

**Dependency checkpoint (2026-10-23):** See [PS3RECOMP_DEPENDENCY_CHECKPOINT.md](PS3RECOMP_DEPENDENCY_CHECKPOINT.md) for the audited toolkit state, verified recovery profiles, and completed fork/submodule migration. The dependency is third_party/ps3recomp at bc5b3d30503169a025c88bd9e6195728fc8ad68d. The content below is the historical documentation-review baseline; its TASK-001 next-priority text is not the current assignment. Do not resume SPURS debugging until separately assigned.

Status recorded: **2026-10-23**. Documentation review: **2026-10-23**, using local metadata, source/configuration, tool version commands, and existing logs only. No build, extraction, or boot was performed in this review. Historical observations are qualified below; this snapshot is not a guarantee that every system behaves correctly.

## Identity and environment

- Title: **MLB The Show 16** (USA), PS3 title ID **BCUS-01089**.
- Target: Windows x64 native executable via **ps3recomp v0.17.0**. Local `git describe --tags --always --dirty` reports `v0.17.0-dirty`; `runtime/ppu/ppu_loader.cpp` is modified and its `.before-clang64` backup is untracked. The root CMake project still declares runtime version `0.11.0`; that banner differs from the Git release tag.
- Host toolchain: **MSYS2 CLANG64**, Clang 23.1.3, CMake 4.4.4, Ninja 1.13.2 (verified with `--version`); installed MSYS2 under `D:\APP\msys64`.
- Shared local workspace: `F:\Games\MLB16PS3`; Git working tree root: `F:\Games\MLB16PS3\mlb16-port`.
- Previously reported boot-test hardware: Ryzen 5 4600H, GTX 1650 Ti, 16 GB RAM. The boot log confirms the GPU; CPU/RAM and the historical window appearance were not independently verified in this review.

## Verified milestones

| Area | Observed result |
| --- | --- |
| Decrypted PPU ELF | `game\EBOOT.ELF`, 18,364,328 bytes |
| ELF boot | 2 PT_LOAD segments; OPD entry `0x010A21C0`, code `0x0001C628`, TOC `0x010D5780` |
| PPU function analysis | 38,495 scanned candidates; 39,516 lifted functions (counts confirmed in `logs\find_functions.log` and `logs\ppu-lift-bounded.log`; wrapper breakdown not verified) |
| Generated PPU source | `recompiled-bounded`, 30 `ppu_recomp_*.cpp` files plus HLE NID table |
| Native PPU objects | All 30 prebuilt `.o` objects compiled |
| Runtime | ps3recomp runtime library compiled under CLANG64 |
| Native link | `build-mlb16-native\MyGameRecomp.exe` exists, 247,803,904 bytes (approximately 236.3 MiB); `logs\mlb16-native-build-v4.log` ends at linking, without a captured shell exit code |
| GPU boot | Boot log records GTX 1650 Ti and D3D12 resource/pipeline creation; window opening is a historical report |
| VFS | System/game archives opened, including `MLBPS3_SYS.WAD`, `MLBPS3_ROM.WAD`, `MLBPS3_GD.WAD` |
| Boot test | Previously reported black window and `timeout` exit **124 after 45 seconds**; duration/exit code are not captured in the inspected boot log/analysis. No fresh boot verification. A timeout alone does **not** prove a crash. |

## Current boot findings

- The initial executable was linked using manually compiled PPU object files and the native runner template. The saved configure/build logs report no lifted SPU sources. Current `CMakeCache.txt` has `PORT_ENABLE_SPU=ON`, empty `PORT_SPU_INPUTS`, and `SPU_DIR=F:/Games/MLB16PS3/mlb16-native/spu` (directory absent). `mlb16-native/prebuilt-ppu.cmake` replaces `RECOMP_SOURCES` with 30 external PPU objects; configuration must preserve that reuse.
- SPURS registered named `Wws_Job` workloads: `DXTJob`, `MeshJobs0`, `MeshJobs1`.
- Runtime logged `PM NOT LIFTED ... workload will not run`. Example observed PM guest address: `0x01057600`, length: `12032` bytes. **The relationship of this address to extracted ELF file offsets has not been established.**
- The RSX FIFO logged `FIFO JUMP to unmapped IO 0x10464648` and a resynchronization. The exact upstream cause is not yet established.
- Repeated flip and event-queue activity is observed. There is not yet sufficient evidence to label the full runtime deadlocked.
- Three unresolved HLE NIDs were identified by the toolkit's NID database:
  - `0x9DCBCB5D` — `cellSpurs::cellSpursAttributeEnableSystemWorkload`
  - `0x9638F766` — `cellHttp::cellHttpInitCookie`
  - `0xCE4374F6` — `cellGame::cellGamePatchCheck`
- None of these is independently confirmed as the primary cause of the black screen.

## Newly extracted embedded SPU ELF images

Previously recorded extraction command (not rerun in this review; the extractor source confirms the CLI syntax):

```powershell
py -3 "F:\Games\MLB16PS3\ps3recomp-source\tools\extract_spu_images.py" `
    "F:\Games\MLB16PS3\game\EBOOT.ELF" `
    --output "F:\Games\MLB16PS3\analysis\spu-images"
```

| File | Reported EBOOT file offset | Size (bytes) |
| --- | --- | ---: |
| `spu_0000_at_0104A800.elf` | `0x0104A800` | 58,748 |
| `spu_0001_at_01058D80.elf` | `0x01058D80` | 48,588 |
| `spu_0002_at_01064B80.elf` | `0x01064B80` | 106,652 |
| `spu_0003_at_0107EC80.elf` | `0x0107EC80` | 58,520 |
| `spu_0004_at_01164C00.elf` | `0x01164C00` | 110,560 |

Inventory verified with local file metadata: **5 images, 383,068 bytes**. A read of each 20-byte header confirms ELF32, big-endian, machine `23` (SPU); entry points, segments, and correspondence to EBOOT offsets remain for TASK-001. Offsets above are reported by filenames, not independently mapped in this review. These are local game-derived binaries and are excluded from the Git repository. Their presence does **not** prove the required WWS job modules have been lifted or registered.

## Known local paths and artifacts

- `ps3recomp-source\tools\extract_spu_images.py`: scans the decrypted PPU ELF for embedded SPU ELF images.
- `ps3recomp-source\tools\extract_jobmods.py`: separate extraction path for WWS job modules contained in SDATA; do not assume those archives exist for this title.
- `ps3recomp-source\tools\build_spu_workloads.py`: assists lifting/registering SPU images; inspect CLI and code before use.
- `mlb16-native\main.cpp`: accepts PPU ELF path as first CLI argument and supports `PS3_VFS_ROOT`.
- `logs\mlb16-boot-01.log`: first native boot log (local only).
- `logs\mlb16-boot-analysis-01.txt`: derived boot analysis (local only).

Existing repository documentation issues outside this review's three-file edit scope: `README.md` still says environment preparation and contains literal PowerShell/Git command text; `.gitignore` also contains stray PowerShell here-string text. `docs/STATUS.md` has an older next milestone. Treat this file and TASK-001 as the current scope/status references; Git ignore patterns are not a substitute for checking every staged path.

## Next priority

**TASK-001 — Audit SPU inventory and workload relationship.** Prerequisites are locally available for the read-only audit; it has not been performed or started as part of this documentation review. Determine how the five extracted embedded ELF images relate (or do not relate) to the SPURS `PM NOT LIFTED` entries. Produce an evidence-backed plan before lifting or modifying runtime files. See `docs/TASK-001-SPU.md`.
