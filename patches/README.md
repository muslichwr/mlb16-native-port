# ps3recomp patch inventory

Exact base: upstream `sp00nznet/ps3recomp` tag `v0.17.0`, commit
`a6d76c895f77eb30b90a806c830544e61821b77f`.

The dependency manifest is [`../dependencies/ps3recomp.lock.json`](../dependencies/ps3recomp.lock.json).
The [checkpoint report](../docs/PS3RECOMP_DEPENDENCY_CHECKPOINT.md) records the
audit, verification, and completed authorized migration. These patches contain
original toolkit changes only; they do not contain lifted game code.

| Patch | Purpose | Long-term destination |
| --- | --- | --- |
| `ppu-loader-clang64.patch` | Replace stack-probe SEH with checked `ReadProcessMemory`; match the internal `ppu_resv_break` declaration | Separate persistent platform-compatibility commit in the fork. Preserve its current workaround status; do not claim general validation. |
| `spu-interpreter-policy-exit.patch` | Intercept the policy-exit service before decoding LS as instructions | Separate generic runtime-correction commit in the fork, with the original synthetic policy-exit regression |
| `spurs-pm-pipe-all-lane-diagnostic.patch` | TASK-017 workload-wide, three-record all-lane observation | Optional temporary diagnostic patch, excluded from the persistent fork tip |

Apply in table order to the exact base with LF checkout. The `persistent`
profile selects the first two; `task017` selects all three. Once the persistent
fork commits exist, their changes must **not** be applied again. Only apply the
diagnostic patch to reproduce TASK-017. All three patches are retained as
recovery/provenance records until a later explicit cleanup decision.

The untracked `runtime/ppu/ppu_loader.cpp.before-clang64` in the old checkout is
byte-identical to the base revision's loader. It needs no extra patch or fork
commit. Two ignored Python caches are disposable and contain no source changes.

Persistent changes are pinned at `third_party/ps3recomp`, commit `bc5b3d30503169a025c88bd9e6195728fc8ad68d`.
