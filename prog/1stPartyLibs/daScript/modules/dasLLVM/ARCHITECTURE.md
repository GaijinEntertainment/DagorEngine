# dasLLVM Architecture

The design document `REVIEW.md` cites. Numbered sections are the stable reference targets;
usage and installation live in `README.md`, the debugger rail and its roadmap in `DEBUGGING.md`.
Companions: `ARCHITECTURE_TARGET_FEATURES.md` (CPU feature truth, the tier gates, the CPU
classes), `ARCHITECTURE_DEBUG_INFO.md` (the `--jit-debug` DWARF rail - sec.12),
`ARCHITECTURE_JIT_ENTRY.md` (the entry module, the emitter-free cache hit, the candidate-set key)
and `ARCHITECTURE_EXE.md` (the standalone exe's link decision and startup - sec.10).

## 1. The jit backend pipeline

`run_jit_linked` (`llvm_jit_link.das`) takes a DLL cache hit by itself - **hash**, **probe**,
**install**, **finalize** - and hands every other run to `run_jit` (`llvm_jit_run.das`) in the
emitter's macro context (`ARCHITECTURE_JIT_ENTRY.md`). `run_jit` drives one linear pipeline per
program: **hash** (the DLL key over the candidate set: per-function AOT hashes plus the hint
folds), **init** (engine + target machine), **declare** (LLVM function declarations for the jit
set), **probe** (open the cached DLL, compare per-function hashes), **irgen** (the das IR emitter
over every function), **optimize** (the LLVM pass pipeline at the requested level, plus the
opt-in IR dump and the post-opt verify), **emit+link** (artifact production - `write_artifact` in
`llvm_jit_common.das`, itself **emit-obj**, machine-code emission, then **link**; which artifact
is the caller's choice, a JIT DLL or an exe or, for `-lib`, a shared library or static archive
with the C header written beside it), **install** (resolve externs, instrument sim nodes), and
**finalize** (engine teardown / state install). On a hit irgen, optimize and emit+link read as zero.

### 1.1 The timing contract

Every phase above reports its wall time in the `LLVM JIT time:` breakdown printed under
`options log_compile_time` / `policies.log_compile_time` - the same option the front end uses
for its `compiler took` line. The contract exists because an untimed phase is invisible exactly
when someone is hunting where compile time went: a phase outside the breakdown makes the printed
numbers sum short of the always-on total, and the gap has no name. Nested totals are allowed - a
parent entry (emit+link) may cover steps a callee reports separately (emit-obj, link) - provided both
levels print. When a phase is split into finer steps, each step reports its own number; an aggregate
label silently absorbing new sub-steps breaks the contract the same way an untimed phase does.

Under `--jit-split-modules` (`run_split_codegen` in `llvm_jit_run.das`) the same labels map
differently: **declare** reads ~zero (declaration moves inside the partition loop), **irgen**
covers partitioning plus every partition's declare/irgen/ctor work, **optimize** carries the
`jit_par_emit_run` pool wall - per-job passes AND object emission interleave on the workers -
plus the post-optimize verifies and the partition teardown, and **emit+link** is the
`link_from_objects` link plus the DLL reopen only. The finer steps print per the contract:
one `LLVM JIT time: job {obj} passes ... emit ...` line per partition (from the pool, under the
same log option) and the `link ... (N objects)` line under emit+link. With the obj cache on
(`--jit-obj-cache`, default under split), cached partitions skip declare/irgen/optimize
entirely: the key fold and probe cost lands in **irgen**, the `obj cache - K/N partitions
cached` line prints unconditionally beside the split announce, and per-partition
`obj cache hit` lines print under the log option.

An `-exe` takes the same driver when its build passes `--jit-split-modules` (its default stays
one unit): the entry rides a partition of its own (`exe_main`), the partitions link once with no
obj cache, and the objects and the response file go with the link. Under `--jit-lto` the
partitions leave as bitcode, every function stamped with the cpu and features the target machine
would carry (`default_target_cpu_features` - lld's LTO backend builds its subtarget from the
module, not from a machine), and the link runs LTO (`lto_linker_args`: `/opt:lldlto=3` on
lld-link, `-flto` on a POSIX driver, where the plan links through `clang++` unless
`--jit-path-to-linker` names another - a GNU driver handles no bitcode). `DAS_JIT_PROBE_LTO` is
the DLL path's dev twin of that rail; the slots the partitions own and the LTO link's needs are `ARCHITECTURE_EXE.md` sec.3 and 4.

### 1.2 The codegen tier

Code that EMITS machine code - the surface whose changes bump `LLVM_JIT_CODEGEN_VERSION` -
is IR generation, target-machine setup, the `[llvm_code]` generator bodies, and the jit call
ABI: the generated function signatures, name scheme, prologue, and the externs the install
phase binds. The file set that carries this surface is `EMITTER_FILES` in
`tests-cpp/small/test_jit_emitter_pin.cpp` (repo root); the pin test makes every text change to one
of those files visible (re-pin `LLVM_JIT_EMITTER_HASH`), and the bump is owed when the emitted code for
identical inputs can differ - a comment, a nolint, or a same-value rewrite inside an emitter file re-pins without a bump.

The per-artifact entry emitters are OUTSIDE that surface: `llvm_exe.das` (a standalone exe's
`main`) and its `-lib` half (the C entry points and thunks) emit startup glue for artifacts
nothing content-addresses, so neither is in `EMITTER_FILES` and neither owes a version bump. Both
artifacts run in `LlvmJitMode.EXE`, which is what makes the exe startup shareable: the four
`emit_standalone_*` helpers in `llvm_exe.das` are the shared halves, split so a library can put the
process-global half behind a once guard and the per-context half behind its own catch boundary.

### 1.3 A library's runtime

`daslang -lib` emits an artifact that loads into a process it does not own; its runtime,
environment and shutdown rules are `ARCHITECTURE_LIB.md` sec. 1.3.

### 1.4 A constant make-array literal is constant data {#const-data-literal}

A make-array literal of four elements or more (`MIN_CONST_DATA_ELEMS`) whose elements are all numeric or
vector constants (`is_const_data_literal`: the scalar and two- or four-wide vector element types; a
three-wide vector's padding, a string's runtime pointer and a fixed-array element - told apart by its size -
stay on the store path) lowers to a private constant array and one `memcpy`, not a store per element; an
element the builder did not fold to a constant of the element type sends the whole literal to a store loop over the collected values. A SPIR-V kernel blob is a thousand `uint4`, and the per-element form put a
620k-line initializer in front of the optimizer and the backend: most of the one-unit exe's codegen wall.

## 2. Codegen identity - the DLL cache

Jit DLLs are content-addressed: `jit_dll_basename` (`llvm_jit_plan.das`) folds the candidate
set's per-function AOT hashes (`ARCHITECTURE_JIT_ENTRY.md` sec.2), `LLVM_JIT_CODEGEN_VERSION`, the
opt/size levels, prologue and debug-info flags, and the target triple - same inputs, same
filename, cache hit. AST-level changes therefore self-invalidate through the function hashes;
**emitter-level changes do not** - a change that alters generated machine code for identical
inputs (IR generation, target-machine setup, `[llvm_code]` generators, the jit ABI) is invisible
to the key and silently serves stale code from cache unless `LLVM_JIT_CODEGEN_VERSION` is bumped. Stamped `[llvm_code]` *arguments* are not
emitter-level: they fold into both cache keys per function (the hint folds), so a change that
merely re-selects which perm gets stamped - the `[tune]` machinery - self-invalidates with no
bump. "The jit call ABI" is the contract between the
generated code and the engine: the generated function signatures and name scheme
(`create_uid_nodes` / `get_dll_fn_name`), the prologue shape (`jit_emit_prologue`), the
`LlvmJitFlags`/`LlvmJitMode` inputs to the emitter, and the extern-resolution surface the install
phase binds (`ResolveExternVisitor`, `generate_llvm_code`, `instrument_jit`). A pinned `jit_output_path`
bypasses the content-addressed name entirely; its probe compares function hashes only, which is why
the summary line asserts the opt-level tag only when the tier is actually known.

### 2.1 The split obj cache - positional invalidation

Under `--jit-split-modules`, each per-module partition object is content-addressed too
(`--jit-obj-cache`, on by default under split): its key is the running fold of every module
hash up to and including its own - symbol names, per-function AOT hashes, and the JIT-only
hint folds - combined with `jit_env_salt`, the ONE helper both the DLL key and the partition
keys fold their config/environment inputs through (a component folded into one key but not
the other would let a config change link stale objects). The chained prefix makes
invalidation **positional**: module order is topological, so a change in module j re-keys
every partition from j on, while everything before j links its cached `.o` - the probe is
bare file existence, before any per-partition LLVM state is created, so a hit skips
declaration, irgen, and the optimize/emit pool outright. Three consequences for consumers:
**require order is the cache layout** - a module you edit often belongs as late in the
require chain as its dependencies allow (registration-only requires, like the dasLLAMA GPU
tiers, belong at the END of an umbrella, not in a root module everything depends on); a
rename-without-body change still re-keys, because the module hash folds symbol names, not
just function hashes; and **the cache holds exactly one generation** - the GC keep-set is
the current link set, so reverting an edit is an eviction, not a hit: the run after a revert
re-emits from the reverted module on, same as the edit did.

## 3. Overrides and their announces

The backend's override knobs - the escapes that change what a run compiles, tunes, or emits
beyond its defaults - are: `DAS_TUNE_POLICY` (replaces the declared/injected tune policy),
`DAS_TUNE_MODE` (grid/tuner compile modes, and `fat` - one clone per shipped CPU class in a
standalone exe; `ARCHITECTURE_TARGET_FEATURES.md` sec.11), `DAS_TUNE_FAT_CLASS` (pins the class a
fat exe runs at startup), `DAS_TUNE_MANIFEST` (pins the sidecar),
`DAS_TUNE_NOISE_CV` (recalibrates the tuner noise gate), `DAS_TUNE_NOISE_OVERRIDE` (mints
through a failing gate), `--tune` (forced re-mint), `--tune-only` / `DAS_TUNE_ONLY` (re-mints
only the named families; the policy guard arms it itself for a profile's residue),
`DAS_TUNE_CONTROL` (a supervisor's stop request - tuners abort between families), `--jit-obj-cache=0` (forces every split
partition to re-emit, bypassing the obj cache), `--jit-lto` (a split `-exe`'s partitions emit
bitcode and its link runs LTO), `DAS_JIT_PROBE_LTO` (the DLL path's twin: split partitions emit
bitcode and the link runs lld LTO - a dev probe artifact), `DAS_JIT_X64_FORCE_FEATURES` /
`DAS_JIT_ARM64_FORCE_FEATURES` (force CPU features past detection - emission, the cache keys,
and `cpu_supports`-based tune eligibility all follow), `DAS_JIT_BASELINE` (build for a CPU class
instead of the box - the machine, the gates, the tune ladder and the cache keys all follow;
`ARCHITECTURE_TARGET_FEATURES.md` sec.10), `--jit-debug` / `-g` (emit DWARF or CodeView debug
info and promote every argument to a stack slot for it; `ARCHITECTURE_DEBUG_INFO.md` sec.12), `--jit-sanitize` / `options jit_sanitize` (`address` runs LLVM's `asan` pass after the optimizer on functions marked `sanitize_address`; `undefined` runs `bounds-checking`; a sanitizer host defaults to its own, `none` to nothing; an exe or a library links with `-fsanitize=` for the host's sanitizers and the requested ones, through `clang++` unless a linker is named; an ASan host lowers every array push to the runtime call, which keeps the buffer's container annotation - `src/misc/ARCHITECTURE.md` sec.10), and the
runtime escape API `tune_suppress_mint(knob)` (a library `[init]` suppresses the auto/restart mint; the
caller passes the knob name it acts for). The announce contract: an override announces at the point it
CHANGES THE OUTCOME - at least one line naming the knob (its env spelling, or the caller-supplied knob
name for `tune_suppress_mint`); a set-but-inert override may stay silent, and per-scope or per-site
repeats are correct. A library that only exposes the override bit (a `*_overridden` query such as
`tune_noise_threshold_overridden`) discharges the contract when the announce lands at the consumer in
the same change. Verbosity knobs (`DAS_TUNE_VERBOSITY`) shape only how much is printed, not what runs -
they are not overrides under this contract.

Environment knobs load ONCE, at context init, into the `[EnvConfig]` structs `g_env_jit` /
`g_env_tune` (`llvm_env.das`) - a mid-process `setenv` changes nothing the backend reads.
In-process overrides therefore go through the tune setters (`tune_set_verbosity`,
`tune_set_noise_cv`, ...), which also arm spawned children by exporting the matching variable.

## 4. Host CPU feature truth on aarch64

Moved to `ARCHITECTURE_TARGET_FEATURES.md` sec.4, with sec.6 (the x64 tier gates) and sec.10
(CPU classes and `DAS_JIT_BASELINE`).

## 5. The tune sidecar is a module-cache dependency {#tune-sidecar-cache-pin}

`[tuned]` and `[tune_policy]` stamp a function's hints at macro time out of the tune sidecar,
and the module cache stores the stamped AST. A re-mint therefore has to invalidate the cached
record, or a later run serves stamps minted against the old sidecar until some source file
changes. The stamping paths (`tune_apply`, `tune_kernel_pick`) register the sidecar path with
`add_module_cache_dependency` through `pin_module_cache_dependency` before they read it; the
record carries the path with the file's byte size and content hash, and the reader re-validates both
before it trusts the payload. Content, not mtime: an app that rewrites its sidecar byte-identically on exit must not churn the cache.

The registration runs before the staleness gate, and for a path that does not exist yet,
because the mints that matter most produce no successful read - the first mint has no sidecar,
and a re-mint replaces one the gate rejected. An absent file registers as size -1 and hash 0,
which the next run's re-validation sees change. The pin sits beside the read, not inside
`read_manifest`: the runtime shares that reader (the box-profile pin at load, `tune_status`),
and a standalone exe binds every extern its functions name at startup, so a reader carrying the
`ast_core` extern would drag the compiler module into every exe - and a wasm cross-link, which
sees only the compiler-free runtime archive, has nothing to bind it to.

The shipped defaults profiles are the same kind of input: with no sidecar entry a kernel
stamps its class entry out of `<defaults>/<class>.tune-defaults.json`, so `pin_profile_chain`
registers every candidate on the class ladder it tries, existing or not - a profile that
appears, or is re-exported after a re-mint, must invalidate the stamps minted without it. The
staleness gate itself compares the sidecar's mtime with the running binary's, which no content
hash sees; the host closes that hole by keying its default module cache on the binary's
mtime and size, so a rebuild is a fresh cache rather than a hit on pre-rebuild stamps.

## 6. The x64 kernel-matrix tier gates

Moved to `ARCHITECTURE_TARGET_FEATURES.md` sec.6.

## 7. A constant-folded GEP is not an instruction {#gep-constant-fold}

`LLVMBuildGEP2` over a global with a constant index does not create an instruction - LLVM folds
it into a `ConstantExpr`, one shared object per distinct expression in the context. Any API that
casts a "just built" GEP to `GetElementPtrInst` therefore writes through the wrong type into the
constant's memory when the fold happened - `LLVMSetIsInBounds` was the instance that corrupted
the context (heap damage surfacing in `LLVMContextDispose` at teardown). The in-bounds form is
requested at build time (`LLVMBuildInBoundsGEP2`), which folds to an in-bounds `ConstantExpr`
correctly; the `llvm_boost` wrapper's `inbounds` default rides that builder.

## 8. The inline-polynomial rail {#vector-poly-rail}

`math::exp`, `sin`, `cos`, `tan`, `exp2`, `log2`, `log`, `pow` and the three hyperbolics on a
float VECTOR type are emitted as inline IR by the `build_vector_*` emitters in
`llvm_jit_intrin.das`. The default lowering is `@llvm.<op>.vNf32`, which scalarizes to N libm
calls on any target without a vector libm - all of ours. Each emitter replaces that call with
the SAME polynomial the interpreter and AOT already run (vecmath, `include/vecmath/`), written
as generic vector IR (`fmuladd`, `trunc`, `roundeven`, `fptosi.sat`, integer masks and selects)
that the backend lowers to one vector instruction apiece - so the three rails agree instead of
merely being close (sec.8.3 is the one family that cannot). sin and cos mirror `v_sincos`
(quadrant = round(x*2/pi), the two-constant Cody-Waite reduction, a degree-3-in-x^2 pair); tan
mirrors `v_tan` (4/pi octants, three reduction constants, its own minimax); exp2, log2, log and
pow mirror `v_exp2`, `v_log2_est_p5`, `v_log` and `v_pow`. Two gates: log2 and log take the
rail on every target (`vmath_vector_float`) - masks and Horner steps with no guard branch; on
x64 `log2(float4)` runs in 2.4 ns per vector against 15-20 ns for four scalarized `log2f` calls
(`log` 2.8 against 17-19) - and pow always did, riding that log2 plus exp2's clamp; the rest
is aarch64-only (`vmath_aarch64_poly_gate`) by measurement, since the range guards of `exp2`,
`sin`, `cos` and `tan` cost more on x64 than the scalar calls they replace. Scalar float and double keep the libm intrinsic on every target: libm is correctly
rounded and one scalar call carries no scalarization penalty. exp keeps its ggml polynomial
(3.9e-6 relative off the interpreter); routing it through the exp2 emitter would make it exact.

### 8.1 Fusion is part of the polynomial {#vector-poly-fusion}

A Horner step vecmath writes as `v_add(v_mul(..))` is emitted unfused - `vmath_poly_step`, an
fmul then an fadd - and only the chains vecmath writes as `v_madd` / `v_nmsub` go through
`vmath_fma`. The exp2 and log2 chains alternate sign heavily enough that one contracted step
moves the result by several ulp, so emitting `@llvm.fmuladd` for them costs bit-exact agreement
with the interpreter and AOT: measured over 200k lanes, unfused is identical and fused is up to
1.9e-6 apart. Where vecmath does fuse, fusion is load-bearing rather than optional - the sincos
Cody-Waite reduction rounds `x - qf*KC1` into noise at |x| ~ 1e5 without it.

The interpreter side of that bit-exactness is a precondition the emitters cannot enforce: it holds
while the host compiler does not contract vecmath's POLY macros itself. clang's default `-ffp-contract=on`
contracts only inside one source expression, so the inlined `v_add(v_mul(..))` pair stays two instructions;
GCC's default `fast` contracts across statements and would fuse them. CMake pins neither flag, so on a
GCC-built interpreter it is the vecmath rail that moves, not the emitted one.

NaN carries lane for lane on this rail, and that has to be built in: a clamp or a float-to-int
conversion written with ordered compares replaces a NaN lane with a number, and no accuracy bound
can see the substitution because a bound only reads lanes that produced a number. So `tanh` selects
its operand back over its `[-9,9]` clamp through `fcmp uno`, and the sincos quadrant and the tan
octant convert through `llvm.fptosi.sat` rather than `fptosi`, whose result for NaN and for
out-of-range input is poison. `exp` is the one member of the rail that still diverges on NaN: the
JIT answers NaN for `exp(NaN)` where the interpreter answers inf.

### 8.2 log2 is an estimate, and its specials are not IEEE {#vector-log2-estimate}

`build_vector_log2` mirrors vecmath's `v_log2_est_p5`: the exponent field gives the integer part,
the mantissa is forced into [1,2) and fed to a degree-5 minimax, and the `p*(m-1)` shape is what
makes log2(1) exactly 0. It is an ESTIMATE (3.7e-5 relative at worst, just below x == 1 where that
combine cancels) the interpreter and AOT have always used, so the JIT matching it is the point: the
tiers agreeing is the property that wins, and a better log2 goes into vecmath for every tier, never
into one rail. `math::log` is it scaled by ln2 and `math::pow` is `exp2(log2_est(x) * y)`, so both
inherit the error, pow amplified by |y|. The estimate reads the exponent through a mask and never
produces -inf or NaN: log2(0) is -127 and log2(-x) == log2(|x|); `pow_est(-2, 3)` is +8 for the
same reason, and `pow` puts the sign back for an odd integer exponent (`v_pow_signed`, the
emitter's XOR) to answer libm's -8 - all on every tier and target, since the three take the rail
everywhere; a caller that needs IEEE's -inf and NaN has the scalar `log2`.
`tests/llvm_vector_math.das` pins the bounds and the special values.

### 8.3 The hyperbolics have no interpreter twin {#vector-hyperbolic-divergence}

vecmath carries no vector sinh/cosh/tanh, so `SimPolicy` binds `vsinh`/`vcosh`/`vtanh`
(`aot_builtin_math.h`), which call libm once per lane. `build_vector_hyper` builds all three on
`build_vector_expf` instead, so on this one family the JIT diverges from interp and AOT by the
exp polynomial's error rather than agreeing with them - the opposite trade from every other
emitter on the rail, taken because the consumer (GELU over float4 rows) otherwise pays four
libm calls per vector. `tests/llvm_vector_math.das` asserts the size of that divergence.

## 9. The idot family's target lowerings

Moved to `ARCHITECTURE_TARGET_FEATURES.md`: sec.9 (the three lowerings, and why the relaxed-SIMD
dot is not one of them), which is where the target's feature string it reads already lives.

## 10. The standalone exe

Moved to `ARCHITECTURE_EXE.md`: sec.1 (the require-resolver rows) and sec.2 (a global
initializer's addresses are filled at startup).

## 11. A global's address is a memory(none) lookup at its use site

JIT code reaches a das global through `jit_get_global_mnh(mnh, ctx)` (`jit_get_shared_mnh` for a
shared one): the mangled-name hash is static, and the offset it names is added to the context's
globals base, which is what a cross-context call needs - the same JIT function runs on any context
of the program, each with its own base, so the address cannot be baked. The emitter
(`visitExprVar`, `daslib/llvm_jit.das`) emits the call where the variable is used and declares it
`memory(none)` (`daslib/llvm_jit_common.das`): the result depends on nothing but its arguments for
the lifetime of the context, so LLVM CSEs one use against a dominating one, hoists a loop's lookup
into the preheader, and - the call not being `speculatable` - never moves it ahead of a branch, so
a global written on one branch is looked up on that branch only. Every access to a global in a
function therefore shares one base pointer, which is what lets LLVM see `xs[j]` and `xs[j + 1]` as
adjacent. Under `options solid_context` the address is instead `context->globals + stackTop`,
computed once per function in the entry block.
