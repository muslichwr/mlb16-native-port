# TASK-024 Dispatcher Observer Implementation Plan

> **For agentic workers:** Use inline execution for this bounded investigation; no delegation, commit or push is authorized.

**Goal:** Pair dispatcher/caller entry and return evidence and observe its immediate continuation.

**Architecture:** One project-only link observer forwards every original call once. Select only the startup dispatcher edge and the dispatcher's own virtual slots; do not trace their descendants. Reuse the TASK-023 isolation lifecycle in a fresh root.

**Tech Stack:** C++17, CMake/Ninja CLANG64, Python, existing pinned PPU objects.

**Spec:** User TASK-024 request and `docs/TASK-023-FLIP-CALLBACK-BOUNDARY.md`.

## Global constraints

No semantic changes, timing controls, generic toolkit edits, generated-code edits, DXT/SPURS investigation, staging, commit or push. At most one isolated boot, 180-second fail-safe. Protect and audit original trees. Diagnostic defaults OFF.

## Review focus

Verify same-object direct calls and indirect function-table entries reach wrappers. Restrict state-slot observation to the dispatcher's own stack frame. Absence of a return is window-bounded evidence. Preserve old task files and toolkit pin. Exclude every diagnostic after the experiment.

## Steps

- [x] Add `src/dispatcher_progress_diagnostic.cpp`, default-OFF CMake integration, and `tools/run_task024_boot.py` using the existing isolation helper.
- [x] Build/link only canonical pinned target, verify wrapper coverage and source scope, compile existing standalone isolation resolver. Do not rebuild generated PPU objects.
- [x] Prepare fresh `build-mlb16-native-pinned/task024`; require resolver/write-canary/original-lock gates before the sole boot. Save build/executable hash gate.
- [x] Run one 180-second boot; retain entry/return observations and complete original-tree audit. Distinguish timeout from crash.
- [x] Restore all diagnostic options OFF, build/link normally, verify toolkit/protected inputs/index; write TASK-024 report with one supported classification.

Verification uses linked call coverage, bounded live observations and the existing full preservation audit. No semantic fix is proposed or authorized.
