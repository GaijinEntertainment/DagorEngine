# dasLLVM Architecture - the standalone exe

Companion of `ARCHITECTURE.md` (sec.10 routes here). Contract: `ARCHITECTURE_COMMON.md` (repo
root). This document carries what `inject_main` (`daslib/llvm_exe.das`) decides for a standalone
exe at build time and what the exe does at startup before its init script runs.

## 1. The require-resolver rows

`inject_main` decides the exe's link - the runtime-only library, or the whole compiler library
when the program registers every builtin module, ships a dynamic module, or reaches a
compiler-lib module - and emits the `register_native_path` rows the host's module scan loaded
only under the whole-lib link, once, after that decision. The rows feed the compile-time
require resolver (`FsFileAccess::getModuleInfo`), which lives in the compiler library: a
runtime-only exe has no compiler and can never reach them, and every row it would carry is a
startup `jit_register_native_path_resolve` call - one exe-file lookup and one stat per row, for
every row the host scan loaded - and a build-machine path baked into the binary. A whole-lib exe
(`dastest.exe`, which compiles test files at run time) still carries every row, re-rooted at run
time the way dynamic modules are.

## 2. A global initializer's addresses are filled at startup

The exe emitter leaves every address global - a `@@fn` value, a handled type's `new`, the
table accessors - null and private, since a JIT-process address means nothing in the exe, and
`collect_external_functions` walks the used functions and the used global initializers at
startup to store the exe-resident address into each, registering the function with the context
on the way. The walk covers a promoted das module - one declared `module X shared`, which the
front end promotes and marks `builtIn` like a C++ module - because its initializers are program
code `init_globals` emits like any other; only the C++ modules, which have no das initializer to
walk, are skipped. A shared module's global that holds `@@fn` is the shape daslib's dispatch
tables and icon catalogs take, and a walk that skips such a module leaves the Func null: nothing
references the function, LLVM drops it, and the first invoke throws.
`tests/llvm_exe_shared_addr.das` holds the shape.

## 3. A split exe's accessor slots {#split-exe-slots}

An exe built with `--jit-split-modules` emits one partition a das module, codegen in parallel,
and gives the entry (`main`, the registration, `init_globals`) a partition of its own, with every
function's pair declared there so the entry names them and the link resolves each to its
partition. The accessor slots the startup walk of sec.2 fills - a builtin's address, a function
address taken in a body, a table method - live in the partitions that use them, so under a split
exe (`g_jit_split_exe`) a slot is one `weak_odr` null definition shared across the partitions:
weak, not linkonce, because a partition whose only local use the optimizer removed must still
carry the definition the entry stores into (the DLL split's linkonce slots carry a baked address
and may vanish); its load is never `invariant`, which would fold the null; and every slot name a
partition created lands in `g_split_glob_names` for the entry partition to declare before
`inject_main` runs (a slot the entry itself creates, and a slot it declared before creating, is
looked up by name, never added twice). Nothing in a split exe is exported: the wrappers the entry
calls are plain external, the slots carry no DLL storage class, and the per-function hash globals
of the DLL rail are not emitted, so the exe has no export table and no import library beside it.
The partitions link once with no obj cache, and the objects and the response file go with the
link, whether the link succeeded or not. `tests/llvm_exe_split_lto.das` holds the shape: a
body-level `@@`, a global table literal whose accessor a partition collected first, and the
announces of each form.

## 4. The split exe's LTO link {#split-exe-lto}

Under `--jit-lto` (`resolve_split_options`: a split exe for the host only, no `--jit-target` - no
cross-target arm has run the link; a build flag with no script pin, like the exe's split; `clang++`
as the driver on a POSIX box unless `--jit-path-to-linker` names another, since a GNU driver handles
no bitcode and a driver older than the bundled LLVM reads none of its bitcode - no POSIX driver is
bundled, so the exe tests gate their LTO cell on the PATH driver's major through `lto_driver_missing`;
a mingw host's default driver is already a clang) the partitions leave as bitcode
after the pool's optimize pass and no object emission (the pool skips a job with no object path),
the module triple the one the target machine carries (`host_jit_triple` where the host's toolchain
differs from LLVM's default), every function stamped with the cpu and features the target machine
would carry (`stamp_target_attrs` over `default_target_cpu_features` - lld's LTO backend builds its
subtarget from the module, not from a machine; a function already naming its cpu, a fat-mode clone,
keeps it), and the link runs regular LTO - one merged module, its optimization serial - at level 3,
the codegen level every exe pins (under LTO the linker is the codegen), with `lto_linker_args`:
`/opt:lldlto=3` through lld-link, `-flto -O3` through the POSIX driver. `DAS_JIT_PROBE_LTO` is the DLL path's dev twin of the rail and takes the same stamp
and the same object-free pool; it never reaches an exe.
