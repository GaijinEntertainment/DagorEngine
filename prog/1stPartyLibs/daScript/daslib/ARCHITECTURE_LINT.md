# daslib architecture notes - the lint family

Companion to `ARCHITECTURE.md` in this folder; section numbers are unique across the family.

## 1. perf_lint

- **Two parallel loop spaces.** `loop_depth`/`in_closure` (+ `VarStackEntry.depth`) is the
  contract for every loop rule: any closure body is deferred, hence not "in" the loop.
  PERF031 needs the opposite for inline block arguments (`peek_data(s) $(d) { ... }` runs
  in place), so `inline_aware_loop_depth`/`in_deferred` (+ `inline_aware_depth`) count
  loops inside inline block-argument bodies as real loop levels and defer only lambdas,
  local functions, generators, and generated blocks. `inline_block_entry_depth` records
  the loop depth at each inline-block entry because a `return` there unwinds only to that
  depth: loops at or below it keep re-invoking the block, so only barrier 0 proves the
  body runs once.
- **Closure-guard placement is load-bearing, per rule.** Four idioms coexist -
  `in_closure > 0` early return, `in_closure == 0` gate, `in_deferred == 0` gate, no guard -
  and the guard's POSITION inside a visitor method encodes which rules are meaningful
  inside a lambda. PERF001 deliberately has no guard: `loop_depth` never advances inside
  closures, so its loop check is already scoped; syntactic rules (PERF003, PERF021) fire
  anywhere because the pattern is wrong regardless of where it runs. Moving a guard by two
  statements changes which rules fire.
- **`perf024_skip_iptr` is a one-slot handshake** between `preVisitExprCallArgument`
  (classifies an annotated-arg use as safe, stashes the variable's identity) and the next
  `preVisitExprVar` (consumes the stash instead of disqualifying). It is sound because the
  classifier accepts only a bare `ExprVar` after one `ExprRef2Value` peel, so the very
  next variable the walker reaches is that argument's own. Widening the peel set breaks
  the adjacency and mis-applies the stash.
- **`add_ptr_ref` is the qmacro-splice marker.** PERF023 rests on `apply_qrules` wrapping
  every `$e(...)` tag in `add_ptr_ref(...)`; matching the name suffices because the
  templates_boost wrapper is its only producer post-expansion. Its depth tracking fires
  before the `in_closure` gate - the rules block is itself a closure.
- **Hot-path walk (PERF026-028).** `HotCache` keeps per-function scan results sliced into
  two flat arrays; `sinks_span_by_fn` doubles as the presence key and the two tables are
  strictly co-populated (one insert site each, adjacent). The visited set keys on
  (callee hash, `nearest_root_file_site` pointer): reaching one callee from two root-file
  call sites must report twice, so a function is re-walked once per distinct anchor -
  a cost taken for per-call-site diagnostics. Generated/generic frames re-anchor to the
  caller side because their own `at` points into the template. A call to any function
  with a `@scratch` parameter prunes that callee's whole subtree from the walk - the prune
  is deliberately TRANSITIVE, removing the callee's whole subtree and not just the sink,
  because narrowing the prune to the sink buries scratch-heavy callers in deep-resize
  findings on buffers the helper family sizes correctly.
- **PERF030 release credits are consume-once**: a `delete x` licenses exactly one
  subsequent `x <- ...`, then the warning re-arms; the lowered delete family is matched
  by name in its four spellings (`_::finalize`, `finalize`, ``builtin`finalize``,
  ``builtin`finalize_dim``).
- **`"$"` in module allowlists is the builtin module's name** - `Module_BuiltIn` registers
  as `Module("$")` (`src/builtin/module_builtin.cpp`), so macro-time reflection reports
  core builtins under `"$"` beside `"builtin"` and `"strings"`.
- **`Function.tempStringResult` marks a FRESH per-call string allocation** - never a view
  or a passthrough (`include/daScript/ast/ast.h`). A temp-string builtin (`to_char`,
  `repeat`) on a hot path IS heap traffic; the PERF026 string-returning catch-all
  deliberately has no temporary exemption. PERF020's `_type.flags.temporary` check is a
  different axis: a `#`-typed VALUE being cast, where `string(x#)` is a load-bearing
  temporary-to-permanent clone.
- **PERF021's tail-argument comparison passes `require_pure = false`** - workhorse-cast
  tails are flags and literals after the injected Fake* skips, so purity has nothing to
  protect there; the default protects rules that collapse duplicated subexpressions from
  changing evaluation counts.
- **`report_key` bit layout**: line in the low 20 bits, column shifted above it, XOR'd with
  the FNV-64-prime-mixed `fileInfo` pointer and the rule-code hash. Lines past 2^20 alias -
  accepted; a collision silently drops one warning.
- **`parse_range_leg` operator polarity**: `upper_op` is the var-on-left operator marking
  the upper-bound leg (`<=` closed-in-range, `>` open-out-of-range), `lower_op` the lower
  (`>=` / `<`). Yoda (const-on-left) forms flip, so the const-on-left arm tests against
  `lower_op` - the `bounds_above` assignments differ between the two arms by design.
- **PERF022 is a three-function protocol over one stack.** `perf022_push_sentinel` pushes a
  frame in `preVisitExprFor` unconditionally, so the stack stays lock-step with for-loop
  nesting; `preVisitExprForBody` may flip the frame's `matched`; `perf022_pop_and_check`
  fires when a frame survives matched-and-undisqualified - at the for-loop's `at`, the
  actionable line a reader rewrites, not the push site.
- **A generic-body exemption exists exactly where the remedy is not instantiation-safe.**
  PERF020's fix (delete the cast) is wrong for the sibling instantiations of the same
  source line, so it bails on `fromGeneric`; PERF019/PERF021 rewrites (fuse under one cast,
  hoist over the ternary) stay valid for every instantiation, so they fire everywhere.

## 2. lint_config

- **`.lint_config` is parsed once per process.** `[lint_macro]` apply() fires once per
  module in the require chain, so an uncached read re-parses the repo file N times per
  compilation; the file is repo policy and cannot change mid-compilation, so
  `ensure_cache_loaded` splits it once into off/on/path-exclude caches.
- **`LINT019` in a line's nolint list exempts the WHOLE line from stale-checking** -
  chosen tradeoff: the in-tree escape hatch is `nolint:PERF006,LINT019` on lines whose
  code fires only in downstream compiles; per-code vouching would need new syntax.

## 3. lint

- **`lint_error` reports only nodes whose lineinfo belongs to the current function's own
  file.** An inliner splice or macro-manufactured node from another file gets its
  diagnostics from that file's own compile; reporting it here would fail a module on
  idioms it cannot edit.
- **LINT022 collects references from three sources because none alone sees them all**:
  the ordinary walk (resolved pointers); a latched second walk over generic bodies
  (`visit(prog)` skips generics, and uninferred bodies carry only spelled alias names);
  and declarations plus string literals (signatures and name-based lookups leave only a
  literal). Self-reference is pre-inserted so it cannot keep a private type alive.
  Functions are checked only when inlining is off - inlining splices small private
  callees away and leaves a body whose only callers were inlined away, indistinguishable
  from dead code. Public symbols, lifecycle/macro hooks, virtual methods, and generic
  bodies are skipped: their callers are invisible to lint.
- **Argument rules skip what the author does not own, and converge leaf-first.**
  LINT012/013/014/023/029 stay silent on class methods, `[extern]` stubs, `finalize`
  overloads, and address-taken functions; LINT014 also skips a parameter whose mutability
  a callee demands - the leaf is flagged first and each fix re-exposes the next caller.
  A returned `var` argument keeps its `var` (a non-copyable result cannot move from
  const); a used `_name` parameter is never LINT004'd (interface and keyword-clash names).
  A variable a DISCARDED `static_if` arm names is marked used by the fold (`InferTypes::visit(ExprIfThenElse)`
  walks the dropped arm first), so the module-gated shape - the module arm uses every argument, the `else`
  skips - reads clean on a box without the module; `lint012_static_if_arm.das` pins it.
- **LINT023 reads the store/use stream in order.** A use disqualifies a candidate only
  once a store has been seen, so a read that precedes every store leaves the write dead -
  the shape that hides a cleared-but-not-returned handle. `lint023_deferred_depth` covers
  the placements whose execution order is not the source order: a closure, lambda or
  generator body, and any loop body, where a read is taken to follow the store. A `label`
  or a `goto` anywhere in the function sets `flow_jumps`, shared with LINT010, and both rules
  report nothing there at all: a forward jump skips the store that read as an overwrite and a
  backward one places a read after a store above it, so neither source-order test has ground
  to stand on. `lint023_deferred_depth` is deliberately NOT `branch_depth`, which also counts
  `if` and `try/catch`: a conditional
  reorders nothing, and the read that exposes the dead write sits inside one - folding the
  two counters together makes the rule silent on the shape it exists to catch.
- **LINT029 ships default-off and is advisory.** `seed_default_disabled` carries it beside
  STYLE005, so a tree sees it only once a file arms it (`options _lint = "LINT029"`) or the
  repo config turns it on. The findings are a reminder to prefer a returned result, not a
  defect report, so skipping one is a normal outcome rather than a suppression to justify.
- **LINT029 fires per argument straight from the compiler's access flags**
  (`access_ref` / `access_info_pass_mutable` - the same evidence LINT014 reads in
  reverse) and shares LINT014's return-erase. Receiver position exempts nothing: a
  mutated struct is a finding wherever it sits, and only a struct whose every field
  access yields a pointer or a handle stands down - there `var` is what keeps the
  CONTAINED handle non-const. The rule stands down wholesale when the program source
  sits in any `daslib/` folder (`lint029_source_exempt`, `daslib/lint_config.das`):
  library code lives on the builder/state idiom (`var self`, `var writer`), so the
  purity contract binds application code only.
- **LINT030 keeps one frame per call on a stack and reads mentions off the ordinary walk.**
  `preVisitExprCall` pushes the frame, `preVisitExprCallArgument` advances its slot, and
  `preVisitExprVar` records every variable read into the slot being walked - only at the
  frame's own closure depth, since a lambda argument's body runs later. On `visitExprCall` the
  frame pops, each argument that is a call is asked which mentioned variables it writes through
  a mutable by-reference parameter (a bare variable in a `var` slot of `&`, array, table or
  struct type) that the callee writes during the call - `Lint030WriteScan` reads the callee's
  body once for the write marks the compiler left on that parameter, discounting an address
  taken inside a `return`, since a callee that hands out a pointer touches nothing before the
  caller uses it, discounting a pointer the parameter holds read out or reached through, since
  the compiler marks a mutable pointer's source and a pointee's write alike while neither moves
  the parameter (an assignment to the pointer field itself still counts), and deciding the mark
  on a call argument by that callee's own body for that slot, since the compiler marks every
  writable argument of a call whose callee modifies any argument; a callee with no body, a
  builtin or an extern, is read as writing - and the
  frame's mentions flow into the parent frame's current slot, so a read buried in a deeper
  call still counts for the outer argument list. A mention that is the outer call's own
  by-reference argument - the variable, or a chain of plain fields rooted at it, since a field
  behind a pointer is a value a write can repoint - is not a read: the callee sees the final
  state whatever the order. The rule fires once per writing argument, on the outer call. It ships
  default-off in the SDK beside LINT029 (`seed_default_disabled`); `.lint_config` turns it on here.
- **LINT031 follows the write target to its root instead of reading the compiler's marks.**
  The compiler marks a parameter written on `addr` and on a reassigned local pointer copy
  alike (the `access_ref` the JIT's readonly guard trusts, over-approximate by design), so
  the rule walks the target itself: through carriers (a cast, ref-to-value, `addr`, an `intptr`
  call, a pointer-arithmetic call whose result aliases its first argument) and into operands
  (deref, field, index, swizzle), following a local pointer, a reference local or an `intptr`
  result to its initializer - a write to the local pointer itself never reaches a parameter,
  since the chase returns one only when a deref, field or index was crossed. Below the target,
  a node that yields a pointer, or a field or index that yields an integer, is a load of an
  address and ends the chase - a deref that yields an integer is the pointee itself and
  continues: what a loaded address points at is not the parameter's memory, which is LLVM's
  `readonly` contract. A call
  slot is a write when `lint031_callee_writes` says so: a builtin by name only
  (`lint031_builtin_writers` - both pointers of a memory builtin register as `void *`, and the
  compiler marks every pointer handed to a modifying builtin, `temp_array`'s view pointer
  included), a das function by `Lint031BodyScan` over its body - the rule's own write sites and
  chase with that parameter as the candidate - memoized per (mangled-name hash, slot) in
  `lint031_writers`, a slot mid-scan reading as not writing; the memo (and LINT030's
  `lint030_writers`) is cleared when a program's visit starts, since the lint runner, the MCP
  server and the LSP keep one process across programs and an edited body keeps its mangled
  name. The chase runs before the callee scan, so a body is read only for an argument rooted
  at a const parameter. An `implicit` parameter type
  waives the const contract (`archive.das`'s `read(bytes : void? implicit)`) and is skipped.
- **Closure bodies are per-rule, not global.** LINT010 counts a closure body as a branch
  (it may run later or never - writes inside must not kill outside stores, reads inside
  must not keep an outside init live); LINT021 counts the same body as an escape - a
  captured 64-bit local is never a narrow.
- **`expr_equal` compares `describe(a) == describe(b)`, never pointers** - every
  Expression has exactly one parent, so sibling pointer equality is effectively never
  true. LINT007 excludes `ExprConst*` operands: identical constants are macro-splice
  residue, not copy-paste typos.
- **LINT015 fires on `+` and `-` only** - the only operators both binary and unary, so a
  split continuation line re-parses as a valid discarded unary statement; every other
  operator produces a loud parse error instead.
- **LINT023's store set extends the compiler's.** `under_clone` marks only bare-variable
  `=`/`:=` targets; `<-`, swizzle LHS, and statement-level `++`/`--` are marked by the
  visitor. A pointer parameter stays a candidate: a write through the pointee reaches the
  caller and counts as a use, not a store.
- **The 64-bit family fires only where the overflow hazard survives.** LINT021 treats
  only unbounded-top math as transparent (`min`/`clamp`/`sign` cap the value - deliberate
  saturation); LINT024 requires a call among the product's leaves - that is the unbounded
  factor, a product of plain locals is tile geometry.
- **`canVisitWithAliasSubexpression` returns false on purpose** - `assume` keeps the
  template block at the definition site and clones it per use; the block is never invoked
  there, so descending would report its arguments unused.
- **C++ mirror pairs**: `lint022_calls_may_be_inlined` <-> `Program::patchInline` (the
  optimizer switch itself is read through the bound `Program.getOptimize`, never respelled); `stale_scan_line` <->
  `rtti_is_nolint_suppressed`; `is_inline_temp_name` <-> `INLINE_TEMP_PREFIX`;
  `lint017_is_32bit_parse`'s name set and `lint017_parse_remedy`'s twin spelling -
  `to_int` and `to_uint` over a string are the 32-bit parses, `to_int64` and `to_uint64`
  the 64-bit twins the remedy names - <-> the `fast_to_int`, `fast_to_uint`,
  `fast_to_int64` and `fast_to_uint64` bindings in `Module_Strings`
  (`src/builtin/module_builtin_string.cpp`), each registered over `("value", "hex")` with
  `arg_init` supplying `false` for `hex`. That default is why a one-argument site parses
  decimal and why the remedy repeats the site's own `hex` argument instead of spelling a
  base of its own. `lint031_builtin_writers` <-> the `memcpy` and `memset8`, `memset16`,
  `memset32`, `memset64`, `memset128` bindings in `Module_BuiltIn`
  (`src/builtin/module_builtin_runtime.cpp`), each registered `modifyArgumentAndExternal`
  over `void *` for both pointers - the name is the only thing that says which one is
  written. Nothing fails when one side moves alone.

## 4. style_lint

- **Lint runs under policies that skip const-folding and leave some types unset.** Take a
  variable's type from `Variable._type`, not an `ExprVar`'s; expect `T('c')` to stay an
  `ExprCall`; expect `-1` as an `ExprOp1` over a constant; when a type is missing, bias
  conservative ("needs unsafe").
- **Rules whose forms are identical in the AST read the source span instead** - `defer <|`,
  generators, STYLE003's `$()`, STYLE028's three invoke spellings. STYLE005 uses LineInfo
  identity: the parser gives braceless and postfix forms a synthetic block sharing the
  terminator's LineInfo, so `blk.at != inner.at` <=> the user wrote braces. Both of these
  tests - the source-span read and the LineInfo identity check - are the rule's only
  evidence for the distinction, and both break silently when lowering or LineInfo
  assignment changes.
- **STYLE024/025 mirror the compiler's unsafe rules with a deliberate asymmetry: in doubt,
  mark "needs unsafe".** A false "needs" costs a missed note; a false "redundant" tells
  the user to delete a wrap the compiler requires. `let v & = unsafe(expr)` is the one
  recorded exception - the wrap is required by the reference binding, not the inner
  expression - and a block whose only unsafe need is such a binding is not redundant
  either (the frame's let-ref mark holds STYLE024 off). STYLE025 narrows only when the
  single statement has an expression form.
- **A require counts as used when its symbols cannot be traced, not only when seen.**
  Generic instances attribute to `fromGeneric._module`; bare names in never-instantiated
  generic bodies keep their providers alive; re-export chains are followed; a module
  providing any compile-time macro or lifecycle hook is used - it leaves no AST reference.
  STYLE029 fires only when exactly one re-exported module is used: two or more is an
  aggregation facade.
- **Metric rules attribute code to the module that can fix it.** STYLE037/038 walk the
  `fromGeneric` chain to its root (`[template]` generics instantiate INTO the caller);
  generated carriers stay unmetered; closures are not length-checked separately - the
  host trips first.
- **STYLE041 is sound because of a reference count, not the structural scan.** The scan
  classifies writes, plumb reads, and the single consuming `if`, arming the candidate with
  that count; `preVisitExprVar` counts every reference in the function, and the warning
  fires only when the counts match. Anything the scan does not descend into therefore
  rejects the candidate instead of being assumed harmless.
- **STYLE036 keys on a flag the compiler clears.** Infer strips a substitution contract
  when a generic binds; one still set on a resolved cast target was never consumed - but
  an `auto`/unresolved-alias target has simply not been substituted yet, hence the
  `isAutoOrAlias` bail.
- **STYLE039 requires non-ASCII in both the value and its source span** - escape-spelled
  bytes in ASCII source are log-safe, and span-slicing keeps a neighbouring comment's
  typography from firing.
- **The unsafe-frame walk**: `preVisitExpression` pushes one frame per expression node,
  `visitExpression` pops it - the popped frame is the subtree's summary, stored in
  `unsafeExprs` for `visitExprUnsafe` (STYLE024 vs STYLE025), propagated to the parent
  slot, and fired as STYLE024 when an `unsafe(...)` target's subtree had count 0. The
  balance panic in each entry point is the guard against a `canVisit*` override skipping a
  subtree the frame walk still counts.
- **Two compiler regimes run this visitor.** The `[lint_macro]` path runs inside a normal
  compile (const-folding ON - `T('c')` folds to a typed const); the standalone runner and
  MCP path run under lint policies (folding OFF - `T('c')` stays an `ExprCall`, `-1` stays
  an `ExprOp1`). Shape-sensitive rules carry both arms (`numeric_char_cast_target`'s
  ExprCall branch + source-span re-parse); deleting the "redundant" second arm kills the
  rule in the other regime.
- **`expr.canShadow` on a for-variable exempts it from LINT002/003/004** - the flag marks
  macro-generated loops, and lint does not judge names the user never spelled.
- **The `expect` probe scans the whole file while `lint-skip-file` is header-capped** -
  an `expect` directive anywhere makes the file a compile-error fixture (its position is
  semantic to dastest), whereas `lint-skip-file` deeper than the header would let quoted
  prose unlint a file.
- **The `blk.list` index rescans are O(block^2) by choice** - the per-statement visitor
  callback carries no index, and a shared index stack would have to push/pop with nested
  blocks; block statement lists are short.
- **Threshold provenance**: STYLE038's 80 ~ p97 of this tree's function lengths; STYLE041
  caps report-arg recursion at 8 and fails closed (deeper is unknown shape);
  `LINT_SKIP_HEADER_LINES = 16` exists so prose quoting the directive cannot unlint a
  file; STYLE037's 20 and `note_type`'s depth 24 have no recorded derivation.
- **C++ mirror pairs**: the STYLE024/025 unsafe map <-> infer's `unsafe_*` rules;
  `style036_inert_contract` <-> infer's contract clearing. Nothing fails when one side
  moves alone.

### 4.1 STYLE029/030 over a require group {#style-lint-require-group}

- **The source line at a declaration's (file, line, column) key, read for the `[...]`
  spelling after the keyword and its guard, is what tells a group apart from a require written
  by hand.** A group's members share the group line's key, but a group with one member has one
  declaration, exactly like a plain require, so the key alone cannot say; both rules judge
  members and then keep, suppress and report per key.
- **A declaration's verdict is settled only after every member sharing its key has been
  judged**, so `st030_report_requires` records verdicts in one walk and reports them in a
  later one. Reporting inside the deciding walk would fire on a member judged ahead of the
  used member that keeps the line.
