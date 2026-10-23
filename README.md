@'
# MLB The Show 16 — Native PC Port Research

Target: BCUS-01089 (PlayStation 3)
Platform: Windows x86-64
Toolchain: ps3recomp v0.17.0

## Goal
Research native recompilation of MLB The Show 16
for Windows using ps3recomp.

## Current Status
Milestone 0: Environment preparation.

## Milestones
- [ ] Obtain and validate decrypted PPU ELF
- [ ] Analyze PPU functions and imports
- [ ] Recompile PPU executable
- [ ] Resolve HLE dependencies
- [ ] Reach game initialization
- [ ] Reach main menu
- [ ] Run gameplay
- [ ] Validate offline Road to the Show

## Legal
No proprietary game assets or executables are
included in this repository.
'@ | Set-Content README.md

git add README.md .gitignore
git commit -m "chore: initialize BCUS-01089 port research"