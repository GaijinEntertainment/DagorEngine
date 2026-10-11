# Simulate headers - architecture notes

## The hot-path model

The interpreter's throughput is a handful of functions: the `SimNode::eval*` methods
(`simulate_nodes.h`, `runtime_*.h`, the extern-call nodes in `interop.h`), their AOT twins
in `aot.h`, and the dispatchers
`Context::callOrFastcall` / `callWithCopyOnReturn` / `invoke` / `invokeEx` (`simulate.h`).
Work added there runs once per evaluated node, so a single load, branch, or counter taxes
every program on every step. Amortized container work - growth in `src/simulate/runtime_array.cpp`
(repo root) / `runtime_table.h`, reached from eval nodes but running once per capacity change - is outside
the hot set; its cost is judged against the allocate/copy/rehash it rides.

## Table key hashing

A table key hashes on every rail as the interpreter's table node hashes it. A builtin key type -
`heap.h`'s `makeTableKeyValueNode` list: scalars, the vectors, the ranges, strings, pointers -
hashes as itself, `hash_function(context, key)`: `hash_uint32`/`hash_uint64` for the scalar and
range specializations, `hash_blockz64` for strings, `hash_block64` over `sizeof(key)` bytes
otherwise (8 for an `int2`, 12 for a `float3`). A handled key - a `ManagedValueAnnotation<OT>`
such as `Time` or a module's id type - hashes as its workhorse, `WrapType<OT>::type`, because
that is the das value type the annotation declares (`makeValueType`) and what the compiler casts
the key to before the node hashes it; `EntityId` hashes as an `int32`, `BigEntityId` as a
`vec4f`. The interpreter's table nodes (`runtime_table_nodes.h`), the JIT helpers
(`src/builtin/module_jit.cpp`, repo root), the JSON scanner (`src/simulate/json_scan.cpp`, repo
root), rtti and the C API call `hash_function` on the key type they hold. `KeyHash`
(`runtime_table.h`) is the same hash for the callers that hold a key of a C++ type - AOT's
`TTable`, the `__builtin_table_*` templates in `aot.h`, and the rehash a grow performs on every
stored non-string key (a string table reuses its stored hashes): it takes the workhorse detour
for a handled type and hashes a builtin type as itself, telling them apart by
`WrapsBuiltinValue` (`cast.h`, beside `WrapType`'s primary; a translation unit that sees the
vec4f detour from `jit_abi.h` sees the marks too, and one that sees neither hashes raw either way). The detour on a
builtin vector or range would hash a `vec4f`'s 16 bytes against the node's 8 or 12, and the raw
bytes of a handled type narrower than its workhorse (an 8-byte `ImVec2` wrapped to `vec4f`)
would miss the node's 16; either way every such key moves to another bucket at the first grow.
Non-string tables are open-addressed from their first slot (only string keys pack linearly up
to 8), so a disagreement shows on a one-key table as much as on a large one.

`KeyCompare` (`runtime_table.h`) answers the other half of a lookup, and it compares the bytes
`KeyHash` hashed: `KeyCompare<vec4f>` is a bit compare, `v_cmp_eqi`. It takes the workhorse
detour for every key type whose `WrapType` names one, builtin vector and handled type alike, and
unlike the hash it has no width to get wrong - both sides widen through the same
`prune<vec4f, PT>`, which zero-fills the padding on every one of its paths. What it must not do
is compare as float, because an integer key's bytes are not a float: every `int2` component in
[-8388607, -1] spells a NaN, NaN is not equal to itself, and such a key therefore inserts (nothing
compares equal, so nothing dedups) and is then unreachable to every lookup, `key_exists` and
erase, on any platform. Bytes small in magnitude spell denormals, which a host built with FTZ/DAZ
reads as zero, so two such keys merge wherever they meet in the probe sequence. A float key pays
for the bit compare with the `0.0 == -0.0` tie, which its two distinct hashes never honoured
anyway. `tests/language/table_vector_keys.das` covers both patterns.

## Function and global lookup

A `Context` finds a function or a global by mangled-name hash and by plain name through one
object each, `functionLookup` and `variableLookup` (`name_lookup.h`, shared between a context and
its forks and clones the way the functions array is). A simulated program builds them:
`Program::buildMNLookup` / `buildGMNLookup` insert every entry and seal once, and the sealed blob is
owned by the object and freed with it. A standalone exe and a standalone AOT context adopt them:
the emitter (`modules/dasLLVM/daslib/llvm_exe.das` and `daslib/aot_standalone.das`, repo root)
builds the same object at code-generation time through the `name_lookup_*` builtins of the
`rtti_core` module, writes the sealed arrays into the artifact as constant data in the word layout
the header pins, and the generated constructor hands that `StaticTable` to `adopt` - nothing is
built or allocated at startup and the object owns nothing. The standalone C++ constructor also
verifies every global's runtime offset against the emitted table, since the emitter computes those
offsets with `InitGlobalVariable`'s rule rather than reading them back. An insert or an adopt after
the seal stops the program, and a seal that finds two entries on one mangled-name hash, or two
different names on one name hash, fails and names both entries - the same footing the runtime
already gives every 64-bit string hash.

The seal builds two perfect hashes (compress-hash-displace over the distinct keys, five keys per
bucket, five percent empty slots), so a lookup is one probe and one 64-bit compare with no
collision chain: `fnByMangledName` and `globalOffsetByMangledName` - the latter on the hot path of
every global access by hash in all three tiers - read one entry, and a by-name lookup hashes the
string, reads one slot, and walks the same-name chain the seal linked in function-index order.
`findFunction(name, isUnique)` answers from the head's link, `findFunctions` is the walk, and a
missing key of either kind answers `NOT_FOUND` / `-1` without touching a name string. The
entry's `value` is what the hash probe hands back - a function index, a global's byte offset -
and `index` is the position in `functions` / `globalVariables`, which is what the by-name API
returns. Measured against `das_hash_map` on 8 000 keys the hash probe is 2.2 ns against 5.8 and
the by-name probe 25 ns against 41, in one eighth and one half the memory; sealing both hashes
for 8 000 entries takes single-digit milliseconds, paid once per `Program::simulate` and never at
the startup of an artifact that adopts. A fresh object, and one whose seal failed, points at a
static empty table and answers every probe with a miss, so a context that never simulated, a
compile that failed at the seal, and a standalone exe between its creation and its adopt all
answer `NOT_FOUND` rather than reading through a null pointer.

## The fastcall depth guard {#fastcall-depth-guard}

A fastcall function (`Function::fastCall`, decided in `src/ast/ast_allocate_stack.cpp`, repo
root) pushes no das stack frame: its body is a single expression evaluated straight from the
caller's node, so a call chain of fastcall functions grows only the native C++ stack. A
regular call fails cleanly at `stack.push` with `stack overflow while calling` once the das
stack is spent; unbounded fastcall recursion runs to the native guard page instead, which is a
process crash no `recover` sees. The guard is `CodeOfPolicies::max_fast_call_depth` (also
`options max_fast_call_depth`), mirrored into `Context::maxFastCallDepth` by
`Program::simulate` and `Context::setup`, and copied into every clone. When it is nonzero,
`Function::makeSimNode` (`src/ast/ast_simulate.cpp`, repo root) emits
`SimNode_FastCallChecked<N>` in place of `SimNode_FastCall<N>`: the same node with
`Context::enterCheckedFastCall` before the body - increment `fastCallDepth`, panic past the cap
- and a decrement after. The fused one- and two-argument shapes are a second family registered
under `"FastCallChecked"` (`src/simulate/simulate_fusion_call1.cpp` / `call2.cpp`, repo root),
so a checked call keeps its superinstructions; the unchecked `"FastCall"` family never gains
the counter, and a program with the cap at zero emits only the unchecked `FastCall` nodes -
the guard costs the unprotected call path nothing.

The counter is a plain integer the panic path does not unwind: a panic is a longjmp in a
build without C++ exceptions, so every frame between the throw and the handler skips its
decrement. Each handler that restores `abiArg` after a caught panic - `SimNode_TryCatch`, its
debugger twin, `das_try_recover` for AOT, `jit_try_recover` (`src/builtin/jit_runtime.cpp`,
repo root) and the `evalWithCatch` / `runWithCatch` family
(`src/simulate/simulate_exceptions.cpp`, repo root) - restores `fastCallDepth` to the value it
held at the `try`, and `Context::restart` zeroes it, so a recovered overflow leaves no drift.
Calls that resolve at runtime - function pointers, lambdas, class methods through
`SimNode_InvokeFn` and its kin - push a regular frame and are bounded by the das stack
already; `Context::callOrFastcall`, the entry AOT and JIT code use to call back into an
interpreted function, stays frameless and uncounted, and the native code around it has no
guard of its own either.

## Sanctioned hot-path additions

The ledger the checklist's hot-path rules route to. Each entry: what was added, where, why
correctness required it, and the alternative that was rejected.

- **`SetMod` on `SimPolicy_IntBin`** (`sim_policy.h`) - one compare-and-branch on the
  `a == INTMIN && b == -1` pair, matching the guard `Div`, `SetDiv` and `Mod` on the same
  template already carry. Correctness required it because x86 `idiv` faults on that pair
  rather than wrapping: without the override `SimPolicy_Int64` inherited
  `SimPolicy_Bin::SetMod`, whose only guard is `b == 0`, so `LONG_MIN %= -1` killed the
  process with SIGFPE in every build, and `INT_MIN %= -1` did the same wherever
  `DAS_FAST_INTEGER_MOD` is 0 - which `platform.h` sets for any host built `-ffast-math`.
  Rejected alternative: giving `SimPolicy_Int64` the double-division form `SimPolicy_Int`
  uses under `DAS_FAST_INTEGER_MOD`, which cannot hold an int64 exactly and is the reason
  that path is already disabled under relaxed math.

- **CRT scalar transcendentals** (`sim_policy.h`) - the scalar float arms of `Exp`, `Exp2`,
  `Log2` and `Pow` call the CRT; the `vec4f` arms stay on the vecmath polynomials, where
  four lanes amortize the setup. The lane trick's `v_set_x`/`v_extract_x` round-trip is a
  partial-register dependency chain scalar codegen does not break, so for these arms the
  CRT call is cheaper than the inlined polynomial - that is the rejected alternative. `Log2`
  is the one arm the swap does not speed up: it trades the `v_log2_est_p5` estimate for the
  exact answer the JIT already computes, so interp, AOT and JIT agree. `log`, `sin`, `cos`
  and `tan` stay on the lane, which is cheaper for them.

- **`v_pow_signed`** (`sim_policy.h`) - vector `Pow` xors back the sign `v_log2_est_p5` drops:
  the odd bit of `trunc(y)` shifted to bit 31, anded with x. `pow_est` is the same without the
  xor, which is `GLSLstd450.Pow` - undefined for a negative base, as GLSL leaves it.

- **The fastcall depth counter's save and restore at every panic handler** - one 32-bit load
  of `Context::fastCallDepth` at the entry of `SimNode_TryCatch::eval`, its debugger twin,
  `das_try_recover` (`src/simulate/simulate_exceptions.cpp`, repo root), `jit_try_recover`
  (`src/builtin/jit_runtime.cpp`, repo root) and the `evalWithCatch` / `runWithCatch` family,
  held across the `setjmp` or `try`, and one store on the catch path, cap set or not.
  Correctness requires it because a panic is a longjmp that runs no frame's epilogue, so the
  checked node's decrement is skipped for every frame between the throw and the handler, and
  the handler is the only place the counter can be put back (`ARCHITECTURE.md#fastcall-depth-guard`).
  Rejected alternatives: a save guarded on `maxFastCallDepth != 0` is a branch on the same
  path for the same load; an RAII guard runs no destructor under longjmp.

- **`das_ordered2`** (`aot.h`) - a two-member aggregate the AOT emitter wraps around any
  binary op whose operands are not both side-effect-free, because braced aggregate init is
  the C++ construct that guarantees left-to-right evaluation; a plain call argument list or
  binary operator is unsequenced, and the interpreter and JIT both evaluate left-then-right.
  Optimized builds flatten the wrapper to nothing; an unoptimized AOT build pays a copy of
  both operand values
  plus an immediately-invoked lambda frame per wrapped op. Ops whose policy operands need a
  ref cast decline the wrapper and keep the plain unordered emission. Rejected alternative:
  hoisting operands to named temporaries in the emitter, which needs statement-position
  rewriting the textual visitor cannot do inside an expression.

- **Member `fn` on the extern-call nodes** (`interop.h`: `SimNode_ExtFuncCall`,
  `SimNode_ExtFuncCallAndCopyOrMove`, `SimNode_ExtFuncCallRef`) - the function pointer is
  a runtime member so all binds of one signature share a single node instantiation, which
  is what shrinks module binaries (bind-heavy shared modules measure -5..-24%) and halves
  per-bind compile time. Cost per evaluated extern call: one member load plus an indirect
  call in place of a direct, potentially inlined one - measured at 3-4.5% on loops that
  are nothing but extern calls, noise on real programs (dasProfile lane medians >=1.00).
  Binds that need the old profile opt into the NTTP flavor (`addExternInline` ->
  `SimNode_ExtFuncCallInline`; the modules `src/builtin/REVIEW.md` names bind that
  way by policy). The cmres and ref flavors have no NTTP twin yet, so those
  binds pay the indirect call unconditionally. Cross-slot and lattice typed reads route
  through the base as `cast<Carrier>::to(this->eval(ctx))` - a second virtual dispatch
  plus a vec4f round-trip on paths that are rare by construction (the compiler emits the
  matching slot; only uint-index reads and value reinterpret land here). Rejected
  alternative: keeping the per-function NTTP matrix everywhere - master's shape, ~24KB of
  object code and ~11ms of compile per bind, mostly duplicated typed-eval stubs.

- **Typed evals on the function-address const nodes** (`simulate_nodes.h`:
  `SimNode_FuncConstValue`, `SimNode_FuncConstValueMnh`) - `evalPtr`, `evalInt64` and
  `evalUInt64` route through the base as `cast<CTYPE>::to(eval(context))`, a second virtual
  dispatch plus a vec4f round-trip. Correctness requires them because without them those
  three slots assert and return 0, which in a build with `DAS_NO_ASSERTIONS` is a silent null:
  `reinterpret<uint64>(@@fn)` written on the address-of expression is typed uint64, so every
  store of it - a local, a field, an array element, an arithmetic operand, a return - reads the
  uint64 slot, while JIT and AOT already answer the address. The slots widen by value:
  `cast<SimFunction *>::from` fills a whole vec4f, so a 32-bit host reads the pointer
  zero-extended. The same cast on a `function` parameter or local is a different path:
  `reinterpret` relabels memory at the target's width, and on a 32-bit host that reads four
  bytes past a 4-byte slot. The inliner turns a parameter into exactly such a local. The
  portable handle spelling is `intptr`, which switches on `sizeof`. Rejected alternative:
  rejecting the cast during inference, which would break the reverse spelling and the two
  tiers that already answer correctly.
