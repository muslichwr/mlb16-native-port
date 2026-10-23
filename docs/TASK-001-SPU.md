# TASK-001 — Audit SPU images and SPURS workload mapping

**Status:** READY for the read-only audit; awaiting further instructions after documentation review

**Mode:** Investigation first; **read-only outside `mlb16-port`**  
**Goal:** Identify the smallest safe next step toward executing one currently unlifted MLB 16 SPU workload.

## Problem statement

MLB 16's native Windows runner successfully loads its PPU ELF, accesses WAD assets, and initializes D3D12, but remains on a black screen. Boot logs show `Wws_Job` workloads `DXTJob`, `MeshJobs0`, and `MeshJobs1` being registered while runtime reports `PM NOT LIFTED ... workload will not run`. Five embedded SPU ELFs have now been extracted into `F:\Games\MLB16PS3\analysis\spu-images`.

We do **not** yet know whether those five complete SPU images supply the referenced workload program, whether separate WWS job modules are needed, or whether the absent SPU execution is the sole blocker to rendering. Avoid guessing.

## Read first

1. `AGENTS.md` (current project Git root: `F:\Games\MLB16PS3\mlb16-port`; the toolkit is a separate, locally modified checkout)
2. `docs/PROJECT_STATE.md`
3. `ps3recomp-source/README.md` as the local SPU documentation entry point; locate further lifting/fallback documentation as needed rather than assuming paths exist
4. Source/CLI help for `extract_spu_images.py`, `build_spu_workloads.py`, `spu_lifter.py`, and `extract_jobmods.py` where present
5. Relevant SPURS/SPU registration and workload dispatch code in `ps3recomp-source`

## Investigation checklist

- [ ] Verify that all five extracted SPU ELF files exist locally; record **metadata only** (size, ELF machine/endianness, entry point, loadable sections/segments). Do not copy contents into this repo.
- [ ] Identify the runtime's exact SPU program fingerprint/registry matching mechanism and the required interfaces for registration and dispatch.
- [ ] Trace the `PM NOT LIFTED` diagnostic to its source-code condition. Record which fingerprint, length, entry point, or mapping is used. Treat runtime pointers, guest effective addresses, and EBOOT **file offsets as different address spaces** until a mapping is proven.
- [ ] Investigate guest PM address `0x01057600` and reported length `12032` from boot logs. Establish whether it matches a complete extracted ELF, an internal SPU program segment, or another job format. Show evidence for any conclusion.
- [ ] Inspect the `DXTJob`, `MeshJobs0`, `MeshJobs1` registrations. Determine whether they share a module or merely share a displayed PM pointer; do not infer identity from names alone.
- [ ] Look for locally present WWS job module sources (e.g. SDATA archives) and determine whether `extract_jobmods.py` is applicable. **Do not decrypt/extract more archives in this first task.**
- [ ] Distinguish confirmed causes from independent symptoms: unresolved HLE NIDs and RSX FIFO unmapped-JUMP may require later tasks.
- [ ] Recommend **one** minimal follow-up experiment/implementation with verification criteria and rollback steps.

## Scope boundaries for TASK-001

**Allowed once TASK-001 is assigned:** Read source, examine local binary metadata, inspect tool help after checking that it has no write side effects, parse existing logs, search within local workspace, write a concise Markdown findings report to `mlb16-port/docs/SPU_AUDIT_001.md`. Do not run lifting/extraction workflows to inspect their behavior. The documentation review has confirmed file presence/sizes and basic ELF headers only; the investigation checklist remains open.

**Not allowed without follow-up approval:** Re-extract binaries or run SPU lifting; modify source outside this repo; edit generated PPU code; run any recompilation/build or boot the game; patch SPURS/HLE/RSX; fake SPU job completion; change build configuration; push/commit to GitHub; introduce copyrighted content into tracked files.

## Deliverable

Create `docs/SPU_AUDIT_001.md` with:

1. **Executive finding** — what is proved, what remains unknown.
2. **SPU inventory** — one metadata row for each of five extracted ELFs, with evidence source.
3. **Runtime mapping trace** — code locations/conditions from SPU registration through `PM NOT LIFTED`.
4. **Address/fingerprint analysis** — explicitly separate guest EA, ELF file offset, image/segment ranges, and content fingerprint.
5. **Next single experiment** — exact command or source change proposed, expected output, regression check, rollback plan.
6. **Operations performed** — commands run, files read/created, errors or unavailable prerequisites.

Do not report a root cause as confirmed until the relevant program mapping and execution path are demonstrated. Do not claim the black screen is fixed without a repeatable boot test.

## Definition of done

- Each of the five image files is accounted for.
- The `PM NOT LIFTED` path and registration contract are referenced to source locations.
- The status of `DXTJob`/`MeshJobs0`/`MeshJobs1` is evidence-backed rather than inferred.
- A focused next action is specified with acceptance criteria.
- Only `docs/SPU_AUDIT_001.md` (and optionally corrections to the task documentation) is newly authored; working executable and game assets remain untouched.
