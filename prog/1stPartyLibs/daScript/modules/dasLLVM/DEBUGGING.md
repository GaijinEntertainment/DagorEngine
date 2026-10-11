# JIT Debugging Infrastructure

State and roadmap for debugging jitted daslang. The goal is not "some debugging
capabilities" - the goal is a first-class debugging story for JIT-compiled das code:
post-mortem, live, and time-travel-adjacent. We control the JIT, which means we can
instrument anything, compile in anything, and emit any metadata we want.

## What ships today

- **`--jit-debug` / `-g` / `policies.jit_debug_info`** - the source-level debug-info rail.
  How the metadata is shaped and why: `ARCHITECTURE_DEBUG_INFO.md` sec.12.
  - one `DICompileUnit` per generated module; impl, wrapper and every block/lambda body get a
    `DISubprogram` named as the das source names them, and every expression carries a
    `DILocation` (file/line/column from the das AST `LineInfo`);
  - every das local, loop variable and argument gets a `DILocalVariable` with a real `DIType`
    (structs and tuples with named members at their own offsets, enums with their entries,
    `string` as `char *`, vectors as lane arrays, `dim` as an array type), so a debugger reads
    and prints das values; each das block opens a `DILexicalBlock`;
  - `"Debug Info Version"` + `"CodeView"` module flags (CodeView gated on windows triples via
    `LLVMGetDefaultTargetTriple()` - `host_jit_triple()` is "" on MSVC hosts, it only
    disambiguates mingw arches);
  - lld-link gets `/DEBUG` -> a real PDB lands beside the jitted DLL in
    `.jitted_scripts/`, auto-discovered by cdb/WinDbg/VS via the embedded path;
  - the in-process crash handler (`src/hal/crash_handler.cpp`) resolves jitted frames
    through that PDB with **function name + .das file:line** - `SymFromAddr` +
    `SymGetLineFromAddr64` were always called, they were just starved of data.
  - flag folds into the DLL cache hash; default path is byte-identical, no PDB.

  On a linux or macOS box the jitted artifact is an ordinary `dlopen`ed shared object carrying
  DWARF, so lldb and gdb need no JIT protocol. What works, verified end to end under lldb and
  gdb: a breakpoint set by the das function name (`breakpoint set -n hot` / `break hot`),
  source listing of the `.das` file at the stop, a backtrace of das frames with argument
  values, `frame variable` / `info locals`, and expression evaluation over das locals
  (`expr n * 2`). Argument and local values read fully at `--jit-opt-level=0`; at the default
  O3 the breakpoints and frames still resolve - including into an inlined instance - while
  values read `<optimized out>` exactly as they do for an optimized C++ build.

  Two things a debugger does not see yet: a das GLOBAL (it lives at `context->globals +
  stackTop`, not in an LLVM global, so it needs a `DIGlobalVariableExpression` with a computed
  location), and the interpreted half of a mixed stack (an interpreter frame shows as the
  `SimNode` eval chain; `--jit-stack` plus `Context::getStackWalk()` is the parallel answer).
- **`--jit-opt-level=0..3`** - IR pass pipeline honors it, and the DLL path's
  codegen-side target machine follows it too (`write_dll` `codegen_opt_level`);
  `write_exe` / AOT-object emission deliberately stay at 3 (shipped artifacts).
- **`--jit-stack`** (`emit_prologue`) - logical daslang stack frame per generated
  function/block, the Prologue path that makes `Context::getStackWalk()` meaningful
  under JIT.
- **Crash handler + WER bundles** - `CRASH:` banner, faulting address, walked stack
  with PDB-grade names/lines when `--jit-debug` was on; WER minidump; watchdog
  bundles dump + jitted DLL + `.map` + tune manifest per crash. `/MAP` is always on:
  map + retained COFF object are the fallback decode when there is no PDB.
- **das-stack-in-crash-handler** (`src/hal/project_specific_crash_handler.cpp`) -
  rbp-window scan for magic-validated `Context*`, prints `getStackWalk()`.
  Interpreter-only today: the frame filter matches `"SimNode"` symbols. See roadmap.
- **Hardware breakpoints** - already work. With PDB line info they compose into
  source-level breakpoints on jitted das code in any native debugger.

## Roadmap

### Crash fixture for `[hint(unsafe_division_check)]`

The hint's ON state is observable only as a process death: a hinted `x / 0` dies with the CPU
trap (`0xC0000094` on Windows, `SIGFPE` on Linux) where the un-hinted twin panics, and on
ARM `sdiv` returns 0 with no trap at all. `tests/jit_tests/div_unsafe_hint.das` pins the
results and that the hint does not leak into the next function; the death itself wants a
subprocess fixture in the `dastest/tests/_isolated_fixture_crash/` shape with a per-platform
expectation.

### One-process many-engine-compile SIGSEGV in IPSCCP (unreproduced, 2026-08-15)

A single dastest process compiling all of `modules/dasLLAMA/tests` under `-jit` (the
invocation dasLLAMA's checklist forbids - its suites run through `run.das`) crashed once in
`llvm::SCCPInstVisitor::visitInsertValueInst` -> `getStructValueState` -> DenseMap insert,
under `IPSCCPPass` inside `runPasses`. `--isolated-mode` over the same corpus does not
reproduce, and the sanctioned invocations never enter the shape. Suspect cross-compile
state in the shared process (many huge JIT compiles back to back), not the IR of any one
program. Worth a dig if it recurs: repro would start from two-in-a-row engine compiles in
one process, watching LLVM context/heap reuse.

### `REVIEW.das`'s tracked-write scan - three blind spots

The gate matches a tracked directory name in the write line plus the initializers of the
`let`/`var` names that line mentions, against `TRACKED_FIXTURE_DIRS`. It therefore misses:
(a) a path a helper returns (`remove(fixture_dir())`) - the expansion traces declarations,
not return values; (b) a write two call levels down - a function containing a direct write is
marked as a sink, a wrapper that only calls a marked one is not, and the marking is per file,
so a helper in a sibling test file is invisible; (c) a tracked file directly under
`modules/dasLLVM/tests` rather than inside a listed subdirectory. Closing (c) needs
argument-position awareness: a naive "line names the tests dir" test fires on the sanctioned
`copy_file(<fixture under tests/>, <temp>)` idiom. Until then the narrowed `REVIEW.md` rule
carries the residual by hand.

### The shadow buffer (the endgame)

A `--jit-debug`-family flag that makes the JIT emit its own bookkeeping into a
**dedicated per-thread buffer**, allocated by the JIT runtime, address printed to the
log on creation (new thread -> new buffer, address logged again).

- Enter function -> push a packed record (file id : line / function index) into the
  buffer; exit -> pop. `begin_section` / `end_section` semantics. Because we own
  codegen these are 2-3 inlined instructions (store + bump / decrement), not calls.
- Want more? Per-line mode: overwrite the top slot on every statement - one store.
- Want even more? Per-variable records: static side table (code-range -> variable
  stack-slot descriptors, DWARF-shaped but ours) costs zero runtime; deeper mode
  spills live values into the buffer at probe points.
- Conditional compiled-in probes: watch expressions / assertions compiled into the
  jitted code only when the flag asks for them.

**Why a separate buffer is the load-bearing design decision:** stack corruption and
memory overwrites on the *main* stacks become irrelevant to post-mortem quality.
A smashed native stack kills the unwinder; a smashed das stack kills `getStackWalk`;
neither touches the shadow buffer - it is a separate allocation that keeps holding
the better truth. The crash handler (or an offline decoder) reads it regardless of
how mangled the crashing thread's state is. Corruption bugs are exactly the bugs
where the primary structures lie; the shadow buffer is the witness that doesn't.

- **Flight-recorder mode**: no-pop ring variant. Stack mode answers "where am I";
  ring mode answers "how did I get here" - the last N thousand line-crumbs *before*
  the fault. For corruption bugs (AV site downstream of the corruption) the history
  is worth more than the stack.
- **`WerRegisterMemoryBlock(buf, size)`** at buffer allocation -> the shadow buffer
  rides inside every WER minidump automatically. Log line (address) + dump (content)
  + map/PDB (decode) = complete offline story even if the in-process handler never
  ran (stack overflow, handler reentry).

### Debug info, next levels

- **Variable info**: `DIBuilderCreateAutoVariable`/`CreateParameterVariable` +
  declare records, real `DISubroutineType`s. With `--jit-opt-level=0` -> live locals
  in cdb/VS and in `!analyze`. The 24-arg fastcall ceiling (raised from 16 for
  `LLVMDIBuilderCreateCompileUnit`, 19 args) already covers `CreateStructType` /
  `CreateClassType` for real das type info; next bindings regen picks them up.
- **Inline-frame expansion in the crash handler**: PDB carries `S_INLINESITE`
  chains; `SymFromInlineContext`/`SymGetLineFromInlineContext` print the full
  inlined stack instead of the outermost frame only.
- **Block/lambda subprograms**: nested `make_block_function` visitors share the
  parent DIBuilder but emit no DISubprogram yet - block bodies have no line info.
- **Debug artifacts coexisting**: the stale-artifact GC keeps only the current
  hash-basename, so toggling `--jit-debug` relinks every time. Keep both flavors.

### das-stack + shadow-stack in the crash handler

- Extend `das_crash_frame_filter` beyond `"SimNode"`: recognize jitted frames by
  module address range (loaded jitted DLLs) - or skip the scan entirely and read
  `Context*`/shadow buffer from a registry keyed by thread id.
- `--jit-stack` + the rbp-scan already gets close for JIT; the shadow buffer
  supersedes both with corruption immunity.

## Session log

- 2026-07-19 - `/DEBUG` + full CodeView line info shipped (`LLVM_JIT_CODEGEN_VERSION`
  0x45). Found en route: publics-only PDBs don't carry internal-linkage impl symbols
  (nearest-public misresolution - `handle_embeddings` vs the true `register_request`
  frame in the 2026-07-18 serving-crash bundles); impl-inlined-into-wrapper kills
  lines unless the wrapper has a DISubprogram; fastcall wrapper tables were the real
  16-arg ceiling behind the binder's "too many arguments" skips.
- 2026-07-19 (same night) - first live catch: the dasllama-server pressure corruption
  reproduced on gemma-4-E4B q8 at concurrency 1, and the CRASH block named the true
  jitted chain with .das lines (`register_request:831 <- handle_chat:1075 <-
  update_server:2034 <- main:699`) where the old trace had three wrong names. Dump
  confirmed the standing signature: `mov r10,[r9+28h]`, r9 = token-id-sized garbage
  (0x39d51; historical 0x1735, 0x7a) in a pointer slot, Context-relative loads right
  after. Remaining blind spot was the runtime DLL's nearest-EXPORT frames - fixed the
  root cause: Release now compiles /Z7 and links /DEBUG /OPT:REF /OPT:ICF
  (CMakeCommon.txt), so every future dump resolves runtime frames with C++ lines.
  Cosmetic residue from that night, since fixed: jitted-frame paths printed the
  relative dir twice (`utils/dasllama-server/utils/dasllama-server/...`) because the
  DIFile directory and filename both carried it. A DIFile now carries the absolute
  path with an empty directory (`ARCHITECTURE_DEBUG_INFO.md` sec.12).
