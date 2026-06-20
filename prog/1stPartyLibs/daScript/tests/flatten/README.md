# tests/flatten

Tests for `daslib/flatten` — the `[flatten]` macro that rewrites a function into a
branchless, call-free **twin** (`<name>_flat`) for GPU-shader-emulator backends that
have no call primitive and no dynamic branch.

## Files

- `test_flatten_basic.das` — **differential equivalence**. Each `[flatten]` function is
  run against its generated `<name>_flat` twin over an input grid; the flattened twin
  must compute identical results. Covers: tail return, early-return guards, nested
  if/else, local mutation, transitive inlining (`a→b→c`), nested-expression call-lifting,
  and whitelisted builtins.
- `test_flatten_errors.das` — **supported-subset boundaries**. Compiles each `_err_*.das`
  fixture and asserts the specific loud `flatten:` error fires (recursion, loops,
  non-whitelisted builtin) rather than silently emitting wrong output.
- `_err_*.das` — compile-failure fixtures (the `_` prefix keeps the runner from executing
  them as standalone tests).

## Caveat: the interpreter cannot see the GPU hazard

daslang's ternary `cond ? a : b` **short-circuits** (it lowers to a real runtime branch,
`SimNode_IfThenElse`). So the differential tests prove *logical equivalence* only — they
do **not** exercise the branchless "both arms always execute" behavior of a real PS2.0-class
backend, where a predicated `arr[i]` / division on the dead path still executes.

That hazard is held off by the **builtin whitelist** (only pure, branch-safe builtins may
survive into flattened output) plus the deliberate decision to be *permissive* on user-code
indexing/division. Do **not** "tighten" anything here thinking the differential tests cover
hardware safety — they don't, by construction.
