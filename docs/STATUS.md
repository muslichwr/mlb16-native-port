# MLB The Show 16 — Native Windows Port

Title ID: BCUS-01089
Platform asal: PlayStation 3
Platform target: Windows x64
Toolchain: MSYS2 CLANG64 / Clang 23.1.3
Recompiler: ps3recomp v0.17.0

## Completed

- Decrypted ELF loading
- 38,495 function candidates discovered
- 39,516 PPU functions lifted
- 30/30 generated C++ object files compiled
- Runtime compiled using CLANG64
- HLE/NID table integrated
- Native executable linked successfully
- D3D12 initialization on GTX 1650 Ti
- Game archive files accessible through VFS

## Current State

Native executable starts but displays a black window.

Boot diagnostics:
- Three unresolved firmware NIDs
- SPURS workloads registered but not lifted
- DXTJob and MeshJobs workloads identified
- RSX FIFO encounters an unmapped IO jump
- No game menu yet

## Next Milestone

Extract and identify SPU workloads, integrate their
execution, and investigate RSX FIFO synchronization.

## Dependencies

ps3recomp v0.17.0
CMake + Ninja
Clang / MSYS2 CLANG64

Game files are not distributed by this repository.
