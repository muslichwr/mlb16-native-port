# TASK-026 Full-Content Baseline Plan

**Goal:** Compare three independent full-installation boots at ordinary-runtime observability, without a subsystem investigation or guest probe.

**Architecture:** One host-only harness reuses original-file protection, native resolver and real-directory/file-only disc mirror checks. All three installed trees are physical copies of the complete verified BCUS01089_R source. Freeze the normal executable, seed and launch environment before any boot.

**Scope:** `tools/run_task026_baseline.py`, `tools/analyze_task026_baseline.py`, `tools/compare_task026_outcomes.py`, this plan and `docs/TASK-026-FULL-CONTENT-BASELINE.md`. All game-derived artifacts stay under canonical-build/task026, outside Git. No changes to runtime, generated code, CMake integration or existing diagnostics.

## Gates and sequence

- [x] Snapshot checkpoint, existing repository files and protected build inputs. Build/link the normal canonical executable once; retain a reversible prebuild executable copy. Verify every task diagnostic OFF and record the resulting executable/configuration hashes.
- [x] Under original-file locks, hash the complete installation and all protected original trees. Freeze the environment. Make three fresh independent full-tree copies; preserve seed file mtimes and empty directories. Reject reparse points and hardlink aliases.
- [x] Before each launch, compare the complete seed copy, executable/configuration hashes, normalized full environment and initial sandbox specification. Require the native resolver, original write-denial and isolated write-canary gates.
- [x] Run each sandbox exactly once, sequentially, with a 180-second fail-safe. Never rebuild or reuse a preceding run's files. Hash/audit every original tree after each process is gone; disarm all disc-file links before protection is released.
- [x] Record sandbox content/metadata mutations, SYS.MD5/cache results, dialog observations and stable ordinary runtime events. Derive semantic signatures from event presence/results, never time, line order, host IDs, descriptor values or volume. Treat missing damaged records as uncertain.
- [x] Compare signatures conservatively, verify all unchanged source/build inputs, and write one supported classification. Preserve one-boot guards and all experiment evidence. No staging, commit or push.

No synthetic guest/runtime test is introduced: there is no semantic change. Harness verification consists of prelaunch content/environment/isolation gates and post-run full preservation audits. An isolation/equivalence failure stops dependent launches.
