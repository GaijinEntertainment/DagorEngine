# JIT Debugging Infrastructure

State and roadmap for debugging jitted daslang. The goal is not "some debugging
capabilities" — the goal is a first-class debugging story for JIT-compiled das code:
post-mortem, live, and time-travel-adjacent. We control the JIT, which means we can
instrument anything, compile in anything, and emit any metadata we want.

## What ships today

- **`--jit-debug` / `policies.jit_debug_info`** — full debug-info rail (2026-07-19):
  - impl + wrapper functions get `DISubprogram`s; every expression carries a
    `DILocation` (file/line/column from the das AST `LineInfo`);
  - `DICompileUnit` per DIBuilder, `"Debug Info Version"` + `"CodeView"` module flags
    (CodeView gated on windows triples via `LLVMGetDefaultTargetTriple()` —
    `host_jit_triple()` is "" on MSVC hosts, it only disambiguates mingw arches);
  - lld-link gets `/DEBUG` → a real PDB lands beside the jitted DLL in
    `.jitted_scripts/`, auto-discovered by cdb/WinDbg/VS via the embedded path;
  - the in-process crash handler (`src/hal/crash_handler.cpp`) resolves jitted frames
    through that PDB with **function name + .das file:line** — `SymFromAddr` +
    `SymGetLineFromAddr64` were always called, they were just starved of data.
  - flag folds into the DLL cache hash; default path is byte-identical, no PDB.
- **`--jit-opt-level=0..3`** — IR pass pipeline honors it. (Codegen-side target
  machine in `write_dll` is still pinned at 3 — see roadmap.)
- **`--jit-stack`** (`emit_prologue`) — logical daslang stack frame per generated
  function/block, the Prologue path that makes `Context::getStackWalk()` meaningful
  under JIT.
- **Crash handler + WER bundles** — `CRASH:` banner, faulting address, walked stack
  with PDB-grade names/lines when `--jit-debug` was on; WER minidump; watchdog
  bundles dump + jitted DLL + `.map` + tune manifest per crash. `/MAP` is always on:
  map + retained COFF object are the fallback decode when there is no PDB.
- **das-stack-in-crash-handler** (`src/hal/project_specific_crash_handler.cpp`) —
  rbp-window scan for magic-validated `Context*`, prints `getStackWalk()`.
  Interpreter-only today: the frame filter matches `"SimNode"` symbols. See roadmap.
- **Hardware breakpoints** — already work. With PDB line info they compose into
  source-level breakpoints on jitted das code in any native debugger.

## Roadmap

### The shadow buffer (the endgame)

A `--jit-debug`-family flag that makes the JIT emit its own bookkeeping into a
**dedicated per-thread buffer**, allocated by the JIT runtime, address printed to the
log on creation (new thread → new buffer, address logged again).

- Enter function → push a packed record (file id : line / function index) into the
  buffer; exit → pop. `begin_section` / `end_section` semantics. Because we own
  codegen these are 2-3 inlined instructions (store + bump / decrement), not calls.
- Want more? Per-line mode: overwrite the top slot on every statement — one store.
- Want even more? Per-variable records: static side table (code-range → variable
  stack-slot descriptors, DWARF-shaped but ours) costs zero runtime; deeper mode
  spills live values into the buffer at probe points.
- Conditional compiled-in probes: watch expressions / assertions compiled into the
  jitted code only when the flag asks for them.

**Why a separate buffer is the load-bearing design decision:** stack corruption and
memory overwrites on the *main* stacks become irrelevant to post-mortem quality.
A smashed native stack kills the unwinder; a smashed das stack kills `getStackWalk`;
neither touches the shadow buffer — it is a separate allocation that keeps holding
the better truth. The crash handler (or an offline decoder) reads it regardless of
how mangled the crashing thread's state is. Corruption bugs are exactly the bugs
where the primary structures lie; the shadow buffer is the witness that doesn't.

- **Flight-recorder mode**: no-pop ring variant. Stack mode answers "where am I";
  ring mode answers "how did I get here" — the last N thousand line-crumbs *before*
  the fault. For corruption bugs (AV site downstream of the corruption) the history
  is worth more than the stack.
- **`WerRegisterMemoryBlock(buf, size)`** at buffer allocation → the shadow buffer
  rides inside every WER minidump automatically. Log line (address) + dump (content)
  + map/PDB (decode) = complete offline story even if the in-process handler never
  ran (stack overflow, handler reentry).

### Debug info, next levels

- **Variable info**: `DIBuilderCreateAutoVariable`/`CreateParameterVariable` +
  declare records, real `DISubroutineType`s. With `--jit-opt-level=0` → live locals
  in cdb/VS and in `!analyze`. The 24-arg fastcall ceiling (raised from 16 for
  `LLVMDIBuilderCreateCompileUnit`, 19 args) already covers `CreateStructType` /
  `CreateClassType` for real das type info; next bindings regen picks them up.
- **Inline-frame expansion in the crash handler**: PDB carries `S_INLINESITE`
  chains; `SymFromInlineContext`/`SymGetLineFromInlineContext` print the full
  inlined stack instead of the outermost frame only.
- **Block/lambda subprograms**: nested `make_block_function` visitors share the
  parent DIBuilder but emit no DISubprogram yet — block bodies have no line info.
- **O0 all the way down**: `write_dll` pins the codegen-side target machine at
  level 3; plumb the flag so `--jit-opt-level=0 --jit-debug` is a true debug build.
- **Debug artifacts coexisting**: the stale-artifact GC keeps only the current
  hash-basename, so toggling `--jit-debug` relinks every time. Keep both flavors.

### das-stack + shadow-stack in the crash handler

- Extend `das_crash_frame_filter` beyond `"SimNode"`: recognize jitted frames by
  module address range (loaded jitted DLLs) — or skip the scan entirely and read
  `Context*`/shadow buffer from a registry keyed by thread id.
- `--jit-stack` + the rbp-scan already gets close for JIT; the shadow buffer
  supersedes both with corruption immunity.

## Session log

- 2026-07-19 — `/DEBUG` + full CodeView line info shipped (`LLVM_JIT_CODEGEN_VERSION`
  0x45). Found en route: publics-only PDBs don't carry internal-linkage impl symbols
  (nearest-public misresolution — `handle_embeddings` vs the true `register_request`
  frame in the 2026-07-18 serving-crash bundles); impl-inlined-into-wrapper kills
  lines unless the wrapper has a DISubprogram; fastcall wrapper tables were the real
  16-arg ceiling behind the binder's "too many arguments" skips.
- 2026-07-19 (same night) — first live catch: the dasllama-server pressure corruption
  reproduced on gemma-4-E4B q8 at concurrency 1, and the CRASH block named the true
  jitted chain with .das lines (`register_request:831 <- handle_chat:1075 <-
  update_server:2034 <- main:699`) where the old trace had three wrong names. Dump
  confirmed the standing signature: `mov r10,[r9+28h]`, r9 = token-id-sized garbage
  (0x39d51; historical 0x1735, 0x7a) in a pointer slot, Context-relative loads right
  after. Remaining blind spot was the runtime DLL's nearest-EXPORT frames — fixed the
  root cause: Release now compiles /Z7 and links /DEBUG /OPT:REF /OPT:ICF
  (CMakeCommon.txt), so every future dump resolves runtime frames with C++ lines.
  Known cosmetic: jitted-frame paths print the relative dir twice
  (`utils/dasllama-server/utils/dasllama-server/...`) — DIFile directory + filename
  both carry it; fix in `get_debug_file_location_by_name`.
