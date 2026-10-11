# Simulate Headers Code Review Checklist

**Read `REVIEW_COMMON.md` (repo root) first - its contract binds this checklist.** Architecture doc:
`ARCHITECTURE.md`. A layout change to a struct or class is a member added, removed, reordered,
renamed or retyped, or a base changed. A diff that makes a layout change to a public struct or
class under this folder, any struct declared in `debug_info.h` included, applies
`skills/internal/abi_break_sweep.md` too. A diff that changes the signature, return type,
overload set or existence of any name `daslib/aot_cpp.das` (repo root) writes into generated
code, or what that name does or returns, applies `daslib/REVIEW.md` too; so does a diff that
renames, retypes or removes a flag or field a function under `daslib/` (repo root) reads.
Checklist discovery walks changed paths only, so the C++ half never opens the daslib checklist
on its own. A diff that adds, reorders or retypes a
member of a C++ type das binds through an annotation applies `src/builtin/REVIEW.md` too, for
the same reason.

- **A diff that adds a field to `CodeOfPolicies` (`code_of_policies.h`) adds it to
  `DAS_MODULE_CACHE_POLICY_FIELDS` in `src/builtin/module_builtin_ast_serialize.cpp`, in the
  same change** - that list drives both the module-cache record's policy stream and the compare
  that refuses a record written under other policies, so a field missing from it is a policy the
  cache silently ignores.

- **A diff that adds a data member to a C++ type under this folder that das binds through an
  annotation appends it after that type's last member, or bumps `LLVM_JIT_CODEGEN_VERSION`
  (`modules/dasLLVM/daslib/llvm_jit_plan.das`) in the same change.** A cached JIT DLL binds the members it read by offset; a member inserted
  before them moves every later one under code that still uses the old offsets.

- **A diff that hashes a table key hashes a builtin key type - one in `heap.h`'s
  `makeTableKeyValueNode` list - as itself through `hash_function(context, key)` (`hash.h`),
  and a handled key type as the value type its annotation's `makeValueType()` returns.** A
  table grow rehashes every non-string key with `KeyHash` (`runtime_table.h`), so a site that
  hashes a key type differently loses every key of that type at the first grow.

- **A diff that changes `KeyHash` (`runtime_table.h`) or `WrapsBuiltinValue` (`cast.h`) states
  in its own PR description which key types change hash value.**

- **A diff that adds or changes a `cvt_*` inline in `aot.h` returns `vec4f` from it and gives it
  a name no other `cvt_*` has - never an overload.** `vec4f` is the SIMD register; the
  `vec2`/`vec3`/`vec4` types are structs of scalars, so a concrete return spills the lanes
  through `v_extract_*` and the next conversion reloads them, and an overload set fed a `vec4f`
  result is ambiguous - the emitter writes `cvt_pass(cvt_uint3(..))` for `x |> uint3 |> int3`.

- **A diff that makes the hot path cost more per evaluated expression in the build the repo
  ships is a defect.** The hot path is a `SimNode::eval*` method, any helper such a method
  calls on every evaluation, the dispatchers `Context::callOrFastcall` /
  `callWithCopyOnReturn` / `invoke` / `invokeEx` (`simulate.h`), or an AOT-side function or
  template under this folder that generated code runs for every evaluated expression. An added
  load, branch, call, copy, or counter, a direct call becoming indirect, a static dispatch
  becoming virtual, or an unboxed value becoming a boxed round-trip is that defect unless the
  PR names what shows the shipped build costs no more: its emitted codegen unchanged, or a
  measurement of the new code against the code it replaces - a diff cannot show optimized
  codegen. The baseline is what already answered correctly: a slot that returned a wrong
  constant costs more once it computes the right one, and that is not this defect.

- **A diff that adds work to the hot path - whether or not the shipped build flattens it -
  lands its entry under `ARCHITECTURE.md`'s sanctioned hot-path additions in the same diff:
  what was added, where, why correctness requires it, and the alternative that was rejected.**
  Replacing a hot-path body with code that performs the same per-evaluation operations - no
  load, branch, call, copy, or counter the old body did not have - and measures no slower on
  the build the repo ships is not added work. A body that gains one of those operations is
  added work, even at no measured cost. A change that costs more only under a relaxed-math or
  otherwise non-default compiler flag states which flavor and how much in its PR description.

- **A diff that makes a layout change to a `debug_info.h` struct states a per-consumer verdict
  (updated / no change needed / rebuild required) in its own PR description: for the rtti binding
  (`src/builtin/module_builtin_rtti.cpp`), for the AST serializer
  (`src/builtin/module_builtin_ast_serialize.cpp`), for the das-side readers of the struct,
  and for external-module rebuilds.**

- **A `debug_info.h` layout change re-pins to the new layout every assertion in
  `tests-cpp/small/test_debug_info_layout_pin.cpp` it makes false, and adds an `offsetof`
  pin for each field it adds** - a field that lands in tail padding leaves `sizeof`
  unchanged, so no other assertion in that file fails.

