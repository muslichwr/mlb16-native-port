# TASK-001 — SPU inventory and workload mapping audit

Date: **2026-10-23**. Status: **COMPLETE — read-only investigation**. No SPU program was lifted, extracted, built, or executed. No fix or playability claim is made.

## 1. Executive finding

**Confirmed:** The registered PM range is a raw, non-ELF byte range already present in the PPU executable. Guest EA `0x01057600`, length `12032` (`0x2F00`), maps through the PPU ELF's first loadable segment to EBOOT file range `[0x01047600, 0x0104A500)`. Its toolkit fingerprint is `0x75BFE68AE96D7577`, matching the clean `MeshJobs0` PM-miss log. Its first 96 bytes also match the existing PM diagnostic, compared locally without reproducing them here.

That file range precedes the first extracted ELF, which starts at file offset `0x0104A800`. The PM is neither a complete one of the five ELFs nor a contiguous byte range inside any of them. None of their full-image or individual PT_LOAD fingerprints matches the PM fingerprint. Merely registering those five images would therefore not satisfy this PM lookup with the inspected generator/runtime contract.

`DXTJob`, `MeshJobs0`, and `MeshJobs1` register the same PM EA and length but distinct workload data. They share the registered backing range, not evidence of three different PM images. Only the `MeshJobs0` miss has a clean complete fingerprint line; the other two miss messages are interleaved with other threads' output. Full runtime byte identity across all three lookup times is not proved by the log.

A separate task creation references guest ELF EA `0x01174C00`; its async miss fingerprint and length match **image 0004**. This establishes a relationship for that task, not for the three named PM workloads. The other four extracted images have no demonstrated runtime consumer in the evidence inspected.

**Unknown:** The raw PM's full control flow, required indirect/interrupt entries, execution correctness, downstream job-module inventory, and the primary cause of the black screen. The missing registered PM is a demonstrated execution gate in the inspected runtime, not a demonstrated sole rendering blocker. No confirmed fix exists from this audit.

## 2. SPU inventory

Evidence: existing files under `F:\Games\MLB16PS3\analysis\spu-images`; read-only Python `struct` parsing and the existing `build_spu_workloads.py` pure sizing/hash functions. All five are **ELF32, big-endian, EM_SPU = 23**. Each file compares byte-for-byte equal to the EBOOT slice at its filename's stated file offset. Full-image hash extents equal file sizes.

The table lists **every PT_LOAD**. Segment tuples are `(ELF-relative file offset, LS vaddr, filesz, memsz, flags)` in hex. `p_paddr == p_vaddr` for every listed segment; alignment is `0x80`. Flags use ELF R=4, W=2, X=1. BSS is the `memsz - filesz` tail; ranges are half-open.

| Existing filename | EBOOT file offset | Bytes | ELF entry (LS) | PT_LOAD tuples | Full-image fingerprint |
| --- | --- | ---: | --- | --- | --- |
| `spu_0000_at_0104A800.elf` | `0x0104A800` | 58,748 | `0x4080` | `(0x80, 0x4000, 0xDAB0, 0xDB40, 7)` | `8906376EBDE56DA3` |
| `spu_0001_at_01058D80.elf` | `0x01058D80` | 48,588 | `0x4080` | `(0x80, 0x4000, 0xB3D0, 0xB420, 7)` | `5050B00C6273A739` |
| `spu_0002_at_01064B80.elf` | `0x01064B80` | 106,652 | `0x15A88` | `(0x80, 0x4000, 0x19410, 0x1B600, 7)` | `26C919CD14D8C325` |
| `spu_0003_at_0107EC80.elf` | `0x0107EC80` | 58,520 | `0xD850` | `(0x80, 0x4000, 0xD720, 0xDEC0, 7)` | `2613521356764C82` |
| `spu_0004_at_01164C00.elf` | `0x01164C00` | 110,560 | `0x3050` | `(0x100, 0x3000, 0x1A7B0, 0x1A7B0, 5)`; `(0x1A900, 0x1D800, 0x20, 0x370, 6)` | `5917A156CE973C9B` |

Total: **383,068 bytes**. All segment file ranges fit their existing files, all LS memory ranges fit the 256 KiB local store, and every ELF entry lies inside its executable segment's initialized range. This is metadata consistency, not execution validation.

Individual PT_LOAD file-byte fingerprints, in table order:

| Image | Executable segment | Additional data segment |
| --- | --- | --- |
| 0000 | `BC3D43DB0D49A627` | — |
| 0001 | `A64437622385B616` | — |
| 0002 | `6A8060963A840E70` | — |
| 0003 | `4681C1BA4D435F04` | — |
| 0004 | `7CD16B0144474314` | `DBB3035FF316A2BB` |

No game bytes, disassembly, generated game source, or binary attachments are included in this report.

## 3. Runtime mapping trace

Source references below are relative to `F:\Games\MLB16PS3\ps3recomp-source`, with line numbers from the inspected checkout. These describe current source, not proof that every path was exercised by the historical executable. Prior TASK-000 environment/build findings were reused; no environment rebuild or repeat toolchain check was performed.

1. **Guest attributes and workload storage:** `libs/spurs/cellSpurs.c:1422–1434` defines the big-endian attribute layout: PM EA at `+0x08`, PM length at `+0x0C`, workload data at `+0x10`. `cellSpursAddWorkloadWithAttribute`, lines `1460–1474` and `1531–1535`, decodes and forwards these values. `cellSpursAddWorkload`, lines `1348–1409`, stores the PM as a guest EA, length and data in `s_workloads`, publishes guest workload info, and returns a workload ID. Registration of a workload is not registration of lifted SPU code.
2. **Lifted-program contract:** `runtime/spu/spu_workload.h:35,48–59` specifies `void (*)(spu_context*)` entries and `spu_workload_register_img(fp, fn, image_id, name)`. `spu_workload.c:59–105` ignores null entries, updates duplicate fingerprint registrations, limits the registry to 256 entries, and finds entries by **64-bit fingerprint equality alone**. Names, guest EAs, ELF entry points, and lengths are not separate lookup keys; length determines the bytes hashed upstream.
3. **Exact hash:** `spu_workload.c:32–40` starts at decimal **1469598103934665603**, XORs each byte, multiplies by **1099511628211**, and wraps modulo `2^64`. The toolkit calls this FNV-1a-64, but its initial value differs from the conventional FNV-1a-64 basis; use the toolkit's exact function. `tools/build_spu_workloads.py:39–73` uses that same basis, sizes the ELF including tables/sections, and can hash its executable segment separately.
4. **Populating both registries:** `tools/build_spu_workloads.py:192–227` emits calls to `spu_begin_image(id)`, the prefixed `spu_recomp_register()`, and `spu_workload_register_img()` for the full ELF and executable segment. The optional constructor invokes registration before guest execution. `runtime/spu/spu_channels.c:1392–1420` registers local-store functions under the current image ID; line `1517` looks up a lifted address using the context's image ID. A correct workload hash alone does not populate the image's indirect-branch table. The inspected generator does not automatically register the raw PM identified here merely because five ELFs exist.
5. **Exact PM-miss condition:** `libs/spurs/cellSpurs.c:1644–1675`, `spurs_resolve_pm`, hashes **`vm_base + (uint32_t)w->pm`, for exactly `w->sizePm` bytes**. It calls `spu_workload_find_img`; a null function sets `resolved = -1` and prints `PM NOT LIFTED`. A nonzero cached resolution bypasses subsequent lookup. `spurs_scheduler_forget` at `1825–1827`, called on workload allocation at `1366`, clears that cache. Installing a registration after a cached miss would not itself retry it. This diagnostic does not mean an ELF parser rejected the PM, and does not establish whether an unregistered lift exists elsewhere.
6. **PM dispatch gate and execution ABI:** `cellSpurs.c:1733–1766` checks workload state, lane priority and contention, resolves the PM, and **continues without dispatch when resolution fails**. On a hit, it passes the translated PM pointer, length and workload data to a worker; `1682–1696` calls `spu_run_policy_module`. `runtime/spu/spurs_policy.c:155–160,233–241` selects the image ID, enables policy mode, and copies raw PM bytes to LS `0xA00` on initial entry/reset. Lines `258–288` populate the SPURS context and entry registers, including workload data in r4; line `394` calls `spu_run_with_halt(entry, ctx)`. The implied initialized PM LS interval here is `[0xA00, 0x3900)`. This runtime convention is distinct from an ELF `e_entry`; the raw PM has no ELF entry header.
7. **Separate ELF task path:** `cellSpurs.c:1119–1129` translates the task ELF EA, obtains `spu_elf_image_size`, and calls `spu_workload_dispatch_async`. `spu_workload.c:1297–1331` hashes the full supplied range and returns 0 on an unregistered image. The ELF loader at `118–155` loads PT_LOAD bytes at their LS vaddrs and zeroes BSS. This is distinct from the PM raw-load path. The synchronous ELF dispatch at `199–237` is not the path responsible for the named workloads' PM-miss diagnostic.

The saved `logs/mlb16-configure-prebuilt.log:11–12` reports no lifted SPU files and reuse of 30 PPU objects, consistent with the misses. Toolkit prose is not sufficient on its own: `docs/SPU_LIFTER.md:24–43` retains historical fallback wording, whereas `docs/SPU_FALLBACK.md:3–12` directs new ports to lifting. The actual resolver code establishes the PM execution gate; no fallback, synchronization bypass, or success stub was proposed or installed.

## 4. Address/fingerprint analysis

### PPU mapping and raw PM

The local PPU ELF has two nonempty PT_LOADs:

| EBOOT file offset | Guest vaddr | File bytes | Memory bytes |
| --- | --- | --- | --- |
| `0x00000000` | `0x00010000` | `0x0108FE68` | `0x0108FE68` |
| `0x01090000` | `0x010A0000` | `0x000EFC00` | `0x00460808` |

Three further PT_LOAD headers have zero file and memory sizes and supply no mapping. The loader explicitly skips zero-memory segments and copies nonempty segments using `p_offset` and `p_vaddr` (`runtime/ppu/ppu_loader.cpp:3445–3468`). Within a file-backed segment:

`file_offset = p_offset + (guest_EA - p_vaddr)`.

Thus the PM maps to `0 + (0x01057600 - 0x10000) = 0x01047600`, not file offset `0x01057600`. Its raw file interval ends at `0x0104A500`, leaving `0x300` bytes before image 0000's ELF header at `0x0104A800`. A full contiguous search for this PM byte sequence inside each extracted file returned no occurrence. Its start is not ELF magic and it does not have the job-module extractor's `C0DEC0DE` marker at relative `+0x20`.

The hash of the mapped on-disk interval matches the clean runtime fingerprint, and the logged initial 96 bytes match directly. These support identifying the static PM source range; a non-cryptographic fingerprint and partial diagnostic do not prove every live byte stayed unchanged throughout boot. No runtime PM dump was identified by the filename searches described below.

### Three named workloads

Evidence is `F:\Games\MLB16PS3\logs\mlb16-boot-01.log`:

| Workload | ID | PM EA / length | Workload data | Log evidence |
| --- | --- | --- | --- | --- |
| `Wws_Job/DXTJob` | 0 | `0x01057600` / `0x2F00` | `0x16023900` | name/attribute/registration at 48,54,62; miss at 65 is interleaved |
| `Wws_Job/MeshJobs0` | 1 | same | `0x16034B00` | 77,83,84; clean miss at 87: `75BFE68AE96D7577`, 12032 bytes |
| `Wws_Job/MeshJobs1` | 2 | same | `0x16040B00` | 90,96,97; miss at 100 is interleaved |

The shared pointer/length denotes common backing storage. The distinct data values are arguments consumed by the PM ABI, not alternate fingerprint keys or proof of specific job-code modules. It is reasonable to hypothesize a shared job-manager policy with different queues, but the queue formats and downstream payloads were not decoded. Names alone do not prove DXT/mesh job-module identities or successful processing.

### Image 0004's independent task relationship

Image 0004 is byte-identical to EBOOT file interval starting at `0x01164C00`, inside the second PPU PT_LOAD. Its mapped guest start is:

`0x010A0000 + (0x01164C00 - 0x01090000) = 0x01174C00`.

Boot log lines `395–403` show taskset creation, `_TaskAttributeInitialize(eaElf=0x01174C00)`, `CreateTask(id=0, entry=...01174C00)`, then async miss `5917A156CE973C9B size=110560`. Both fingerprint and size equal image 0004. The log field called `entry` here holds the guest ELF pointer; the ELF's actual SPU LS entry is `0x3050`. This task is not evidence that the `Wws_Job` PM resides in image 0004. Images 0000–0003 remain structurally inventoried but functionally unidentified.

### Existing WWS job-module candidates and extractor applicability

Filename searches found no `*jobmod*`, `*pm_wws*`, or `*spujob*` artifacts under the local `analysis`, `mlb16-native`, `tmp`, and `logs` roots, including hidden/ignored paths. The game inventory contained only these four `.sdat` files; no `patch.sdat` or `data.sdat` was found:

| Relative to `F:\Games\MLB16PS3\game\MLB The Show 16 (USA)` | Bytes | Header metadata |
| --- | ---: | --- |
| `PS3_GAME/USRDIR/SPECIALWAD.SDAT` | 10,717,968 | NPD; flags `0x0100003C` |
| `PS3_GAME/USRDIR/SPECIALWADHDR.SDAT` | 6,816 | NPD; flags `0x0100003C` |
| `game/BCUS01089_R/USRDIR/SPECIALWAD.SDAT` | 10,696,786 | neither NPD nor FSHb |
| `game/BCUS01089_R/USRDIR/SPECIALWADHDR.SDAT` | 6,504 | neither NPD nor FSHb |

`tools/extract_jobmods.py:115–144` defaults to `patch.sdat`/`data.sdat` but accepts explicit names through repeatable `--sdat`. Renaming or selecting the actual archives would not solve the format mismatch: `sdata_decrypt` at `27–40` rejects flags containing `0x20`, present in both disc files. Its main path accepts already-plaintext archives directly only with `FSHb` magic, absent in the two HDD files; they also fail its NPD test. Module discovery at `62–68,82–112` looks for a particular header/marker in inflated zlib streams. Therefore **this extractor is not directly applicable to the four observed inputs as implemented**. No decryption, decompression, or extraction was attempted. WWS modules may be packed in WAD/other containers or appear under other filenames; this search does not prove their absence from game assets. Their presence and format remain unknown.

The unresolved HLE NIDs and RSX unmapped FIFO jump remain contextual findings from PROJECT_STATE.md; they were not investigated as additional targets and are not asserted as causes here.

## 5. Next single experiment — proposed, not executed

**One bounded static disassembly of the raw PM's first 256 bytes at the runtime's LS base.** This tests whether the identified range has a coherent entry sequence and exposes initial direct branch targets before choosing lifting boundaries/entries. It requires no new extraction, source change, or game execution.

PowerShell command, to run only after a follow-up assignment:

```powershell
py -3 -B "F:\Games\MLB16PS3\ps3recomp-source\tools\spu_disasm.py" `
    "F:\Games\MLB16PS3\game\EBOOT.ELF" `
    --offset 0x01047600 --length 0x100 --base 0xA00
```

CLI behavior was verified by source inspection only (`tools/spu_disasm.py:669–697`): it reads the input, selects the specified file range, decodes it at the supplied base, and prints to stdout. This command has **not** been run.

**Expected output and acceptance:** 64 four-byte instruction rows covering LS `0xA00` through `0xAFC`. Assess the initial sequence and classify any direct branch targets against PM LS interval `[0xA00,0x3900)` and the runtime's documented kernel interface. Explicitly record unknown opcodes, indirect targets, or targets outside the PM; do not infer successful execution from decodability. The experiment succeeds as an investigation when entry plausibility and initial target constraints are documented, including negative results. It does not authorize a lift or demonstrate correct PM execution.

**Regression check:** Git status and the five input sizes/fingerprints remain unchanged; no outputs are redirected into this repository and no source/build configuration is altered. Keep any disassembly local and summarize only metadata/findings.

**Rollback:** None to source/runtime/builds; the command reads inputs and writes stdout only. Discard the local console output. This is the sole recommended next experiment; downstream implementations are deliberately unspecified.

## 6. Operations performed

All commands ran from `F:\Games\MLB16PS3\mlb16-port` using **Windows PowerShell**, with inline Python passed through a PowerShell here-string to `py -3 -B -`. `-B` prevented Python bytecode-cache writes. Inline inspection code was not saved as new tooling. External files were opened read-only. Only this report was created.

Commands actually executed, grouped by purpose:

- `Get-Content -Encoding UTF8 AGENTS.md,docs/PROJECT_STATE.md,docs/TASK-001-SPU.md` and `git status --short` established current scope and the initial untracked documentation/workspace files.
- `Get-Content` read `../ps3recomp-source/tools/{extract_spu_images,build_spu_workloads,extract_jobmods}.py`; `rg -n` located `PM NOT LIFTED`, fingerprint/registry symbols, attributes, PM dispatch, and loader copy conditions in `libs/spurs/{cellSpurs,spurs_pm,spurs_taskset}.c`, `runtime/spu/{spu_workload,spurs_policy,spu_channels}.c`, `runtime/spu/spu_workload.h`, and `runtime/ppu/ppu_loader.cpp`. The initial search in `spurs_pm.c` did not locate the diagnostic; `cellSpurs.c` did.
- `rg -n` read SPU/fallback references in the toolkit README and both SPU documents; `Get-Content .../docs/SPU_FALLBACK.md -Encoding UTF8 -TotalCount 45` and the corresponding `SPU_LIFTER.md` command with `-TotalCount 48` read their introductions. `rg -n 'add_argument|def main|if __name__'` inspected the lifter and disassembler CLIs. No tool's extraction/lifting main function or CLI help was executed.
- Python line-numbered source reads used `Path.read_text(encoding='utf-8').splitlines()` and bounded ranges. Relevant ranges inspected: `cellSpurs.c` 1270–1415,1420–1568,1625–1775,1818–1831,1105–1134; `spu_workload.c` 30–240,1297–1376; `spu_workload.h` 28–60; `spurs_policy.c` 1–90,155–166,185–288,388–401; `ppu_loader.cpp` 120–220 and 3438–3475; `build_spu_workloads.py` 192–231; `spu_lifter.py` 1116–1136; `spu_disasm.py` 669–701; `spu_channels.c` 1390–1420. An attempted range 245–280 in `build_spu_workloads.py` was beyond EOF and printed no source. Some large tool outputs were truncated; the cited registration, loader, and dispatch details were inspected through smaller follow-up reads or targeted `rg` output. The early loader range was unrelated; the actual mapping evidence is at 3445–3468.
- `rg -n '01057600|PM NOT LIFTED|WorkloadAttributeSetName|AddWorkload\(wid|AddWorkloadWA:|wid=2|workload|spu_workload' ../logs/mlb16-boot-01.log | Select-Object -First 45` inspected workload evidence; Python read log lines 387–405 for the separate task. `rg -n 'Lifted SPU|prebuilt PPU' ../logs/mlb16-configure-prebuilt.log` reused existing configure evidence.
- `rg --files ../game ../analysis ../mlb16-native -g '*.sdat' -g '*.SDAT' -g '*jobmod*' -g '*wws*' -g '*spu*'` identified candidates. Follow-ups used `rg --files --hidden --no-ignore ../analysis ../mlb16-native ../tmp ../logs -g '*jobmod*' -g '*pm_wws*' -g '*spujob*'` (no matches), and `rg --files --hidden --no-ignore ../game -g '*.sdat' -g '*.SDAT'` (the four files above). `Get-Item ../ps3recomp-source/tools/spu_disasm.py` confirmed the proposed existing tool's path.
- Inline Python metadata inspection read `../game/EBOOT.ELF` and the five existing SPU files into memory, parsed big-endian ELF32/ELF64 program headers with `struct.unpack_from`, imported only `fnv1a64`/`img_size` from the inspected `build_spu_workloads` module (its guarded main was not run), and printed header fields/segment metadata/hashes. For PPU PT_LOADs it computed `pmoff = p_offset + 0x01057600 - p_vaddr` when the entire 12032-byte range was file-backed. For each SPU file it compared `EBOOT[filename_offset:filename_offset+size] == image` and hashed full/segment file bytes. No binary was written.
- A second inline Python check read `EBOOT[0x1047600:0x104A500]`, tested non-ELF and marker metadata, converted the six already-logged 16-byte diagnostic rows to an in-memory byte sequence using `re.findall(r'\b[0-9A-F]{8}\b', line)` over `log[55:61]`, and compared those 96 bytes. `image.find(raw_pm)` returned `-1` for all five files. A `Path('../game').rglob('*')` suffix filter found the four SDAT files; only their first `0x90` bytes and sizes were inspected for format metadata. No archive payload was decrypted or inflated.
- The report was written using `apply_patch`. Final verification used `git diff --no-index -- /dev/null docs/SPU_AUDIT_001.md` (exit 1 means a new-file diff), `git diff --no-index --check -- /dev/null docs/SPU_AUDIT_001.md`, `git diff --exit-code`, `git diff --cached --exit-code`, and `git status --short`. Since the report is untracked, ordinary tracked-file diff alone would not review it.

**Results and limitations:** All five images, PM file mapping, source registration/dispatch conditions, and the documented log relationships were inspected successfully. No missing prerequisite blocked this audit. Absence of filename matches is limited to the stated roots/patterns. No complete runtime PM dump, execution test, downstream job-module identification, or rendering fix was obtained. No game, generated code, executable, runtime, build file, or existing Markdown file was modified. No staging, commit, push, installation, build, boot, lift, or extraction was performed. TASK-001 stops with this report.
