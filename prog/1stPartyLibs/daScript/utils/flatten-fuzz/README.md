# flatten-fuzz

A metamorphic differential fuzzer for [`daslib/flatten`](../../daslib/flatten.das).

It generates random daslang functions, compiles them **in-process** (the
integrated compiler — no subprocess), and checks the flattened twin against the
pristine original over many inputs — random integers, or (with `--bool`) the
**complete bool truth table**:

```
f(x) == f_flat(x)   for all generated f, for all x
```

`[flatten] def f` leaves `f` untouched (real control flow) and emits the
branchless `f_flat` twin, so the un-flattened `f` is a **free, independent ground
truth** that never trusts flatten. The fuzzer is, in effect, an auto-generated,
randomized, at-scale [`tests/flatten/test_flatten_basic.das`](../../tests/flatten/test_flatten_basic.das).

## Run

```
daslang utils/flatten-fuzz/main.das -- [flags]
```

| flag | meaning |
|---|---|
| `-s, --seed N` | base RNG seed (default 1) |
| `-u, --units N` | function pairs per batch (default 20) |
| `-n, --inputs N` | random inputs per unit (default 40) |
| `-d, --depth N` | max nesting / expression depth (default 3) |
| `-b, --batches N` | run seeds `seed .. seed+N-1` (default 1) |
| `-f, --strict-fold` | enable the post-compile fold-residual oracle (`flatten_fold_residuals`) + const-density bias |
| `-k, --keep` | keep the generated temp source (don't delete) |
| `-L, --log-infer` | emit `options log_infer_passes` in generated source (dump the compiler's per-infer-pass AST) |
| `-B, --bool` | pure-bool domain: bool params/locals/return, **exhaustive** 2³ truth-table oracle (ignores `-n`/`--inputs`) |
| `-?, --show-help` | help |

`--log-infer` makes the fuzzer a probe for the **compiler** too, not just flatten:
the per-pass AST dump exposes infer non-convergence / oscillation on
flatten-generated code. The in-process compiler routes that log into the
compile-result string (not stdout), so the tool surfaces it explicitly.

Exit code is non-zero if any batch **fails** (a value mismatch or a compile
failure); the offending unit is bisected out and dumped as a standalone
reproducer (`flatten_fuzz_fail_seed<S>_unit<N>.das`). Fold residuals are
**warnings**, not failures (see below), and dump as `flatten_fuzz_residual_*`.

## Two oracles

- **Value-differential** (default) — `f(x) == f_flat(x)`. Catches semantic
  divergence in the mask machinery (early-return / break / continue / nested
  loops / inlining). This is the **only hard gate**.
- **Structural** (`--strict-fold`) — biases the grammar toward constants, then
  walks each compiled twin with `flatten_fold_residuals` and reports any
  const-foldable survivor. Because the units are plain `[flatten]` — whose fold
  fixpoint already converged — a residual in the *final* compiled twin
  necessarily materialized **after** the macro settled: the compiler's own later
  const-prop proved an operand constant (e.g. `x*k` with `k` proven `1` → `x*1`),
  exposing an identity flatten can no longer reach. So these are **non-fatal
  warnings** — missed-fold *opportunities* (grist for moving more const-fold into
  flatten), not flatten correctness bugs. A per-run summary tallies them.

## Domains

Two value domains, selected by `--bool`:

- **Integer** (default) — random `int` inputs sampled per unit; the differential
  is *exact* (no float epsilon). Trap-free: no `/`, `%`, or array indexing; shift
  counts masked to `& 31` (predication executes *both* arms, so a trapping op in
  an untaken branch would panic the twin but not the original — inherent to
  predication, and the shader backends flatten targets are trap-free anyway).
- **Bool** (`--bool`) — every value is `bool` (params, locals, return), the
  grammar is bool algebra (`&& || == != !`), and the oracle is **exhaustive**: a
  3-bool-input function has only 2³ = 8 possible inputs, so each generated
  function is verified over its *complete* input space, not a sample. Bool is
  also where flatten's machinery lives — the live / break / continue masks are
  bools — so bool data stresses the mask algebra head-on, and bool loop bodies
  (often pure dead computation) readily DSE to empty, exercising paths integers
  don't. Trap-free for free (no division / overflow / shift UB).

## Design

- **Mask-biased grammar** — if/elif/else, early return, loops over
  `range`/`urange`/const-array sources (single-source and parallel multi-source
  `for (a, b in xs, ys)`, unrolled in lockstep), break/continue, nested loops,
  helper inlining, compound assignment. That combinatorial surface is where
  flatten bugs live.
- **Pure-functional** — params + locals + return value; no globals (yet).
- **Domain-agnostic harness** — the in-process compile → simulate → invoke →
  bisect → reproducer-dump pipeline is identical for both domains; a bool
  candidate marshals through an `int` driver (`!= 0` in, `? 1 : 0` out) so the
  differential machinery never changes.

## Found

- The inlined-callee-local shadowing bug (`inline_call` not uniquifying callee
  locals across multiple inline sites) — fixed in the same change that added
  this tool.
- Unbounded loop unrolling: deeply-nested constant `for` loops unroll
  multiplicatively (`range(20)^4` = 160k body copies) with no expansion cap, so
  the flattened twin blows the heap to multiple GB. **Fixed** — `lower_for` now
  caps the product of nested iteration counts (`FLATTEN_MAX_UNROLL`, default 4096;
  `[flatten(max_unroll=N)]` overrides), bailing with a clean error before cloning.
- A deterministic compiler crash (the SIGSEGV/SIGBUS whose faulting address was
  always ASCII bytes of a flatten-generated name like `__flat_loop` / `v46_0`).
  Root cause was **not** in flatten: an incomplete 64-bit migration of the core
  shoe allocator (`MemoryModel::Deck`) — `total * size` (entry count × element
  size) overflowed `uint32_t` once a string-heap size-class grew past 4 GB, so
  `das_aligned_alloc16(0)` returned a degenerate pointer the next string
  overwrote. **Fixed** by widening the allocator's byte/count fields to 64-bit
  (regression: `tests-cpp/big/memory_model_4gb`). flatten amplified it via a
  per-`ExprVar` `"{name}"` string built inside the O(n²) opt-pass tree walks;
  that churn is now allocation-free (`das_string` compares). Default depth is 3.
- *(bool domain)* A compile-time SIGSEGV: an empty-body `for`-over-array loop
  inside a pure, foldable-result function null-derefs when the const-folder
  (`RunFolding`) simulates and evaluates it — `SimNode_ForGoodArray1<1>::eval`
  reads `list[0]` with `list == null` (no `total == 0` guard for an empty body).
  Surfaced on the very first `--bool` sweep (the integer domain never hit it; bool
  loop bodies DSE to empty and bool functions are pure + const-arg + foldable —
  exactly what `RunFolding` const-folds). **Fix in a follow-up** — guard
  `total == 0` in the for-node `eval` family.
