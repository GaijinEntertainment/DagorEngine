# dasLLVM architecture - target feature truth

Companion of `ARCHITECTURE.md` (contract: `../../ARCHITECTURE_COMMON.md`). Sections 4, 6 and 9 moved
here with their numbers - rules cite sections by number, and a number is never reused - and
section 10 continues the numbering, so the index reads in order. Each section is cited by the code
that embodies it.

## 4. Host CPU feature truth on aarch64 {#aarch64-feature-truth}

An aarch64 host target reads its CPU features from two sources, because neither answers alone.
`LLVMGetHostCPUFeatures` returns an EMPTY string on macOS - there the CPU name is meant to imply
the features - and a part this LLVM cannot name maps to the generic CPU, where SDOT and SMMLA
have no instruction to select and codegen aborts. `cpu_supports` reads the operating system
instead (sysctl / `AT_HWCAP` / `IsProcessorFeaturePresent`), so it answers for silicon LLVM has
never heard of. The tier gates (`init_jit_target_flags` - `g_target_arm64_dotprod`, `_i8mm`,
`_fullfp16`) and the target machine's feature string (`create_default_target_machine`) therefore
take the union of the two: an LLVM host-string hit OR a `cpu_supports` hit (fullfp16 additionally
reads darwin-arm64 as always-on - every Apple Silicon part has it). One asymmetry: the host rail's
machine string carries `+dotprod` unconditionally (every part the JIT has run on has it), while the
DotProd GATE probes like its siblings - on an ARMv8.0 host the gate declines and the `sdot4` family
compiles its fallback, whatever the string says. A cross-compile triple takes neither - only the force env - and so
does a generic-CPU standalone exe (one carrying no `[llvm_code]` kernel): its machine is the
ARMv8.0 baseline, which cannot select SDOT or SMMLA, so the DotProd and i8mm gates
(`g_target_arm64_dotprod`, `g_target_arm64_i8mm`) stay off there and every `aarch64_neon` call
that needs either compiles its daslang fallback body. The gates and the machine string are one
truth on both rails: a force-env feature raises the gate AND is appended to the generic machine.
The string reaches the code on three routes, one per engine: the DLL and exe rails build their
target machine from it (`create_default_target_machine`); the in-memory MCJIT - the engine every
run without a DLL cache ends on, a static daslang build always - is created through the C API
with no cpu and no feature string, so its subtarget is the generic one, and `stamp_host_target_attrs`
puts the same cpu and string on every defined function as `target-cpu` / `target-features`
attributes, which the per-function subtarget lookup honors (the route the fat-mode clones of
sec.11 already ride). A module the gates emitted SDOT into and the generic subtarget selects
aborts codegen with `Cannot select`; the attributes are what let the two agree.

The two ways a feature reaches the target machine's string license different things. A
detection-derived append - `+dotprod` always, `+i8mm` when `cpu_supports` confirms it - is
EXECUTION-safe: the silicon running this process really has the instruction. A
`DAS_JIT_ARM64_FORCE_FEATURES` append is EMISSION-only: it may name silicon this box does not
have, so the artifact is for another machine and executing it here traps.

## 6. The x64 kernel-matrix tier gates {#x64-tier-gates}

An x64 host target publishes eight boolean gates (`g_target_x64_*`), one per instruction tier the
kernel matrix and the tune grids select on: `avx2`, `f16c`, `vnni256` (256-bit VPDPBUSD by either
VEX AVX-VNNI or EVEX AVX512-VNNI+VL), `avx512bw` (zmm byte ops - BW, not merely F), `avx512vnni`
(zmm VPDPBUSD; implies bw, the sign trick around it is BW), `avx512vbmi` (VPERMI2B / VPERMB /
VPMULTISHIFTQB - the grid formats' symbol lattice), `vnniint8` (VEX VPDPBSSD, native s8 x s8),
and `amx` (both amx-tile and amx-int8; the per-process XTILEDATA grant is a separate runtime step
the family's own witness performs). `init_jit_target_flags` decides each from cpuid truth OR'd with
the `DAS_JIT_X64_FORCE_FEATURES` emission-only override; a cross triple or a generic target
(`host_features = false`) drops to forced-only truth - cpuid is not consulted, the force env is
the only tier source there.

The cpuid truth is `das_cpu_supports` (`src/builtin/module_builtin_runtime.cpp`), a hand-kept table
keyed by the LLVM target-feature spelling - so the force env and `llc -mattr` take the same names.
A tier feature usually lands as three parts: its cpuid line there, its name in
`TUNE_KNOWN_FEATURES` (`daslib/llvm_tune.das`, the profile fingerprint the `requires=` gates are
checked against), and - when the emitters branch on it - a `g_target_x64_*` gate. The cpuid line
is the load-bearing one: a name missing from the table answers false on every box, so the perm
that requires it declines everywhere and no error names the cause.

## 9. The idot family's target lowerings {#idot-lowerings}

The exact integer dots on the 8-bit lattice have three lowerings, picked by target: one
`@llvm.aarch64.neon.sdot` where the target has DotProd (`g_target_arm64_dotprod` - the host rail's
`+dotprod` append, or the force env on the generic rail), the SIMD128 form on a wasm target
(`idot_wasm_simd128`), generic widen-multiply IR everywhere else. The native arms exist because
neither backend produces them from the generic form: AArch64 expands it to zip/uzp/smull instead
of folding to SDOT, and the wasm backend runs it a fifth as fast. The wasm form is the ISA's two
halves of an int8 dot, `i16x8.extmul_{low,high}_i8x16_s` (what LLVM makes of `mul(sext, sext)`) and
`i32x4.extadd_pairwise_i16x8_s`; the pairwise sums land as byte pairs, and one even/odd shuffle-add
folds them into the quad lanes the generic form defines - exact for every int8 lane. An unsigned
first operand widens with ZExt rather than SExt, so the unsigned-by-signed form takes this same
path instead of the 16-lane generic IR; the i16 product still holds every value that pair can make
(255 * -128 = -32640), so the fold stays exact there too. The relaxed-SIMD dot
(`i32x4.relaxed_dot_i8x16_i7x16_add_s`) is NOT what the generic op lowers to, and sec.12 keeps
the feature out of the string: its second operand is 7-bit, so the sign trick that
would feed it (`dot(w, x) == dot(sign(x)*w, |x|)`) wraps at -128 in either operand and answers the
wrong sign there - an operand a caller can bound to 7 bits is a different op, not this one.

## 10. CPU classes and the baseline {#cpu-classes}

A CPU class is a feature set, never a box model: the set every box of the class carries, spelled
in LLVM's target-feature names. The classes are the rows of `JIT_CPU_CLASSES`
(`daslib/llvm_cpu_class.das`): `x86-base`, `x86-avx2`, `x86-vnni256`, `x86-vnni512`, `x86-amx`,
`arm-neon`, `arm-i8mm`. Each row carries the LLVM cpu its target machine schedules for and its
feature set, and the cpu is the arch's bare baseline (`x86-64`, `generic`) on every row: an LLVM
cpu level such as `x86-64-v3` implies features the row does not name (bmi2, lzcnt, movbe; `v4`
adds avx512cd/dq), the backend would emit them, and the membership test below would admit a box
without them. The set alone is what the backend enables and what cpuid is asked for.
The set is the fingerprint the class's shipped profile records (`skills/tune.md`, the shipped
defaults profiles), so a profile and the class it is keyed by state the same silicon. A box is
in a class when it carries every feature of the set; `tune_cpu_class()` names the highest class
this box is in, and `jit_cpu_class_ladder` is the one adoption ladder both the compile-time
profile walk and the runtime pick descend. M1 is `arm-neon` (it has no i8mm); M2 and later are
`arm-i8mm`. A vnni512 part without vbmi - the pre-Ice-Lake Xeons - is not in `x86-vnni512` and
runs as `x86-avx2`, because the set is the contract and a subset of it is not.

`DAS_JIT_BASELINE=<class>` builds for a class instead of the running box. It is one truth on
every rail - the DLL, the standalone exe, the AOT object: the class's set reads as forced
(`x64_forced_feature` / `arm64_forced_feature` answer it before the force env), so the tier
gates, the machine's feature string and the `requires=` eligibility of every `[tune_perm]` follow
the class; `init_jit_target_flags` drops to the generic rail whatever the caller asked;
`create_default_target_machine` names the class's cpu; `tune_cpu_class()` answers the class, so the
profile ladder starts at it; and `jit_env_salt` folds the class, so the artifact never serves a
host run. A kernel-carrying exe stops targeting the box (`exe_host_cpu` is false under a
baseline). The force env still appends after the class's set, as after detection. A baseline
never crosses architectures - `jit_baseline_error` refuses a class of another arch, and a name no
row carries, by name, before any IR is built. The variable is environment-only by construction:
the AST module cache keys on `DAS*`, and a flag read at JIT time would serve stamps minted for
another class.

At the runtime of a built artifact the variable is inert for dispatch: the class an exe runs on
is read from cpuid, never from an environment a build left behind.

## 11. The fat mode - one clone per class, picked from cpuid {#fat-mode}

`DAS_TUNE_MODE=fat` is the third `[tune]` mode (the other two: `skills/tune.md`). It is a
standalone-exe mode under a baseline: `stamp_fat` (`daslib/llvm_tune.das`) refuses a build that is
not `-exe` and one with no `DAS_JIT_BASELINE`. The ship set is every class of the baseline's
ladder that some `[tune_scope(defaults=)]` ships a profile for, at or above the baseline, plus
the baseline itself - most capable first, the baseline last; adding a profile file grows the
exe. For each `[tune]` function and each class of the ship set the apply adds a generated clone
`<name>__fat_<class>` (the class with `_` for `-`), stamped with the class profile's winner when
the grid carries it and the class's set passes its `requires=`, else the first `fallback=` entry
the class's set passes, else the reference body. Companions clone per class alongside, taking
their own profile entry when the class's set can run it and the main's pick otherwise. The
stamp carries one extra `[llvm_code]` argument, `tune_class`, which folds into the cache keys
with the rest.

The function's own body becomes the dispatch, generated as daslang: one arm per non-baseline
class, `if ((tune_fat_mask() & bit) != 0) return <clone>(args)`, most capable first, and the
unguarded baseline call last. The JIT lowers that to a load, a compare per arm and direct
calls - no function pointer, no `invoke`. A bit is the class's position on its arch's ladder
(`tune_fat_class_bit`); the mask starts at zero, so a kernel called before the `[init]` filled
it runs the baseline clone, which is always correct.

The mask is an LLVM-side word, not a das global: `tune_fat_mask()` and its setter are
`[llvm_code]` stubs whose generators (`daslib/llvm_tune_fat_word.das`) emit a load and a store
of one module global, `das.tune.fat_mask` (`tune_fat_built()` rides a second one). A das global
lives per context, and an engine's job contexts are pooled forks cloned with the init script
skipped (`set_jobque_fork_pool(keep, skipInit)`), so their copy would stay zero and every
kernel dispatched from a worker would run the baseline clone against planes the main context
laid out for its own class. Process memory has one copy, so a fork reads what the main
context's `[init]` stored, and the chain's guard is a plain load the backend hoists. The stubs'
das bodies (the das globals) serve the interpreter and AOT, where there is no fat exe. The
spelling `[llvm_code]` inside `llvm_tune` comes from `daslib/llvm_code_shell.das`, an
annotation-only module required non-publicly: `daslib/tune` carries the user-facing annotation
and requires `llvm_tune`, so `llvm_tune` cannot see it, and a program must never see both. The
shell is its own module rather than a corner of the generator module because `llvm_tune`'s
closure reaches every program with a `[tune_policy]`, JIT or not: pulling the LLVM bindings in
there applies their `[extern]` stubs to the `dasbind` builtin in a different set and order per
process, and the module cache reads that as a builtin hash drift and reparses.

The emitter (`try_llvm_code_function`, `daslib/llvm_jit.das`) reads `tune_class` off the
annotation and gives the clone's impl three things: the `target-cpu` and `target-features`
attributes of the class row, so the backend lowers this one function for the class inside a
module that targets the baseline; the tier gates swapped to the class's set for the span of the
generator call (`with_class_target_gates`, `daslib/llvm_jit_common.das`), so a generator that
branches on `g_target_x64_*` emits the class's form; and `noinline`. The last one is
load-bearing: x86's inliner refuses a callee whose features exceed the caller's, but AArch64's
admits one whose body carries no target intrinsic, and the inlined copy would then lower for
the baseline and the clone would be dead code.

The exe's status `[init]` (`emit_status_init`) first calls `tune_fat_init(baseline)`: it refuses
a box that lacks any feature of the baseline class by name, then sets the mask to every class
of the ladder the box is in - membership is cpuid only (`tune_fat_box_in_class`), never the
build's baseline or a force env - or to the one class `DAS_TUNE_FAT_CLASS` pins, refusing a pin
the box is not in. `tune_status()` lists one row per clone with its class (`klass`), and
`log_tune_status` marks the classes this box runs. A profile-covered class needs no sidecar and
no tuner: the exe carries no grid beyond the reference rows, and a re-tune is a new profile and
a rebuild.

## 12. The wasm target's feature string {#wasm-feature-string}

A wasm cross-compile gets `+simd128,+nontrapping-fptoint`, with `+atomics,+bulk-memory` appended
on the threaded rail (`wasm_target_features`). The string carries no `+relaxed-simd`, and the
reason is reach, not codegen: WebKit implements none of the relaxed opcodes, so one of them
anywhere in a module makes Safari and every iOS browser refuse the whole module at validation,
and the browser builds ship to every engine. What the feature would buy is `f32x4.relaxed_madd`,
the only multiply-add wasm has; without it a contracted multiply-add splits back into `f32x4.mul`
plus `f32x4.add`. Measured on the Pocket TTS page in Chrome the split form runs at the same
real-time factor as the fused one, so nothing is paid for the reach. The feature would also move
more than the multiply-add: with it on, the backend lowers an `nnan` float min/max on a vector
to `f32x4.relaxed_min` / `_max`, whose NaN and signed-zero answers are the engine's choice, and
the relaxed dot stays ruled out for the idot family by sec.9 above whatever the string says.
