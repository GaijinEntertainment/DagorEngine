# dasLLVM Ledger

**Living document.** Planned work on the JIT, the AOT and standalone-exe cards, and the
cross-compile toolchain: what each entry costs today and what the change would do. An entry
leaves when it lands or is refuted.

## Entries

- **The split obj cache shares nothing between programs.** A partition's key is the running
  chain over every module before it (`ARCHITECTURE.md` sec.2.1) and its object sits in the
  program's own `.jitted_scripts/<namespace>/`, so two programs over one engine share no
  object: two dasLLAMA unit tests (`tests/test_batch_grid.das` and `tests/test_box_profile.das`,
  each a `-jit` child of `modules/dasLLAMA/tests/run.das -- --suite model-free`, the `.o` names
  listed from the namespace folder its log names) read 0 of 19 key-named objects in common, while their
  partitions' own content - each function's mangled name and semantic hash under
  `DAS_JIT_DUMP_HASHES=1` on the same two runs - is identical in 11 of 19 partitions carrying 524 of 757 functions,
  the tune-stamped kernel partition among them (about 12 s of O3 codegen in every file that
  reaches the engine, on a Threadripper 3990X at 16 lanes). No function carried two hashes; partitions differ only by which
  functions a program uses. After an engine edit each of the module's 146 test children
  re-emits the engine, 13 to 31 s apiece on that box. The change: key a partition by its own content - the
  semantic hash is what the AOT tier already binds stubs by, and partitions are separate LLVM
  modules - and keep the objects in one folder shared by every program, written to a temp name
  and renamed, swept by age instead of the per-program keep-set.

- **The vector `log` returns finite values for zero and negative inputs on an AVX2 box.**
  `tests/llvm_vector_math.das`'s `test_vector_log_special_values` reads `log(0) = -127` and
  `log(-x) = 2` where the scalar answers are `-inf` and NaN - the shape of an exponent-field
  extraction with no special-value select on that lane path (a Zen 2 3990X; CI's lane reads the
  cell green). Its own small change: the select over the special inputs in the vector rail's
  `log`, proven by the same cell on an AVX2 box.
- **The released `-exe`'s default form.** An `-exe` builds split (`--jit-split-modules`) and
  split with LTO (`--jit-lto`) on request; its default stays the one-unit form, which the records
  rig measures. The change: the rig re-profiles a released exe built one unit against one built
  split with LTO on the same GGUF, per board; if the served rates hold, the default flips, the
  release rig and the exe smoke build the fast form, and the build-and-debug skill's sentence on
  the released exe's form follows. Its own PR, after the split-and-LTO options land.
- **The rule-document splits the exe split arc's audit proposed.** The review-md audits of that
  arc read six rule documents against their contract and proposed splits and trims that are
  not that arc's to apply: `doc/REVIEW.md` (the trigger list), `tests/REVIEW.md` (the numbers
  criterion, where a cell is defined, the pinned-gate roster's home), `modules/dasLLAMA/REVIEW.md`
  (one rule's scope), `daslib/REVIEW.md` (two rules to split), `modules/dasLLVM/REVIEW.md` (the
  `host_jit_triple` wording, the inventory trigger, one split), and two guide lines in
  `skills/comment_style_hygiene.md`. The change: the tail of a later PR applies them one document
  at a time under the two-pass rule, Boris ruling on each removal.
- **Cross-target handled-type layouts from the target, not the host.** Today a cross-compiled
  exe bakes the host's `sizeof`/`offsetof` of every handled type (`BasicStructureAnnotation`
  fields, `TypeDecl::getSizeOf`/`getAlignOf` for `tHandle`), so any platform-sized member ahead
  of a das-visible field - a mutex (64 bytes on darwin, 40 under Linux libc++ and emscripten), a
  `struct stat` (144 against 104), a `std::function` (48 under Linux libc++ 19, 32 under
  emscripten's and darwin's) - shifts what the bundle reads; cases found and fixed by member
  order: `Context`, `FStat`, `Program`. `--jit-check-abi` finds a
  mismatch at the bundle's first launch, and only for the types the bundle links. The change:
  `build --wasm` runs a small emscripten program under node that walks every registered
  annotation (the check's own walk) and writes a per-toolchain table of size, alignment and
  field offsets; a program compiled with `--jit-target` answers the layout calls named above
  from that table. The hard part is per-Program dispatch: macro and tooling programs running on the host
  during the same compile keep host layouts (reached through the bound `g_Program`), the
  cross-target program takes the table, and struct layouts fix at inference, so the switch
  cannot be codegen-only. Sized at two to three days; the check stays as the proof and gate.
