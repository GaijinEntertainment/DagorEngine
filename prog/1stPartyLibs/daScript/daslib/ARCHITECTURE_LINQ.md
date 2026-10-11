# daslib architecture notes - the linq and sql families

Companion to `ARCHITECTURE.md` in this folder; section numbers are unique across the family.

## 11. linq

- **`top_n_by_with_cmp` exists for the fold splice and has no daslib call site.** When an
  order-by key body is pure and inlineable, `linq_fold_common` emits it with that body
  spliced into BOTH sides of the comparator - killing the per-comparison comparator dispatch
  and the per-side `key(v)` dispatch the key-lambda overloads pay. It is not a redundant
  twin of `top_n_by`.

## 12. linq_boost

- **`BucketLambdaStamper` exists because a bucket-surface lambda cannot infer on its own.**
  On the `group_by_lazy` element shape `tuple<K; array<E>>`, an untyped lambda in
  `<bind>._1 |> select/sum/...(<lambda>)` has nothing to bind against in the fully generic
  tier-2 parameters (error 30303); the chain type knows `E`, so the macro stamps the
  lambda's parameter before the rewrite - the same move as the outer-parameter injection in
  `visit()`.
- **`result_param1_type` is the join result-lambda auto-typing hook.** param0 is always TA;
  param1 is whatever the override returns - `_join` -> TB, `_group_join` -> a bare `array<TB>`
  with const/ref cleared to match the bucket type. `_left_join` / `_right_join` /
  `_full_outer_join` deliberately do not override: their result parameter is `$Option<T>` on
  one or both sides, so those result lambdas stay user-spelled.

## 13. linq_das

- **The substituting scanners share one token model.** `substitute_idents`,
  `mentions_ident` and `rewrite_group_var` all read it: plain `"..."` content is verbatim, a
  `{...}` interpolation body is code that is scanned and substituted, one level of nested
  string literal inside an interpolation is verbatim again, and identifier position
  classifies the same way everywhere - after-`.` field access and before-single-`=` labels
  are not references, with `is_label_position` the shared judge.
- **`JOIN_TI` is a spellable identifier on purpose.** `__`-prefixed names are reserved by
  the compiler, so the transparent-identifier carrier cannot be hidden outside the user's
  namespace and must stay a single plain token - hence the explicit rejection of a range
  variable named `linq_join_ti` instead of an unspellable name.
- **`build_src`'s two arms are a fusion decision, not a formatting one.** A fused source
  (chain head, join) is built as `from_in(...)` so `_fold` can route it; an UNFUSED operator
  argument - the uncorrelated multiple-`from`, whose `_cross_join` runs at tier-3
  passthrough - is passed as the bare parenthesized expression, so `each`'s
  `[unsafe_outside_of_for]` does not trip and the arrayxarray overload is selected.
- **The correlated-flatten collection selector borrows, it never copies.** `unsafe(each(tail))`
  is chosen over `to_sequence` (whose const-array overload clones per row) and over
  `where_`'s array overload (which materializes per row). Borrowing a TEMPORARY tail
  (`from x in [c.id]`, a function result) is safe because daslang heap-allocates arrays and
  does not finalize them at scope exit - the backing storage outlives the borrow and is
  reclaimed by GC.
- **A `FromInMacro` reject returns null so infer can stabilize.** Errors clear on every pass,
  so the not-yet-inferred-source arm's error survives only when the source never infers at
  all - that is what makes that arm a deferral rather than a final verdict.
- **`from_in` identifies typed sources by matching module + type NAME as strings**, not by
  requiring the provider modules: `sqlite_boost::SqlRunner` -> `select_from`, pugixml
  `xml_node` -> `from_xml_node`, `json::JsonValue?` -> `from_json` - linq_das stays
  dependency-free on sqlite/pugixml/json. Both node builders are `[unsafe_outside_of_for]`,
  so the emitted call carries the `unsafe` wrap a `_fold` chain requires. A table source is
  rejected with a fix message before those arms: it already carries its row shape, and the
  typed builders would all mis-fire.

## 14. linq_fold_common

- **group_by's per-key update is a dummy plus an address compare, not a `key_exists`
  probe.** `entry &= tab?[uk] ?? dummy` costs one hash op per element on hits;
  `addr(entry) == addr(dummy)` identifies the miss, which then fills the dummy, inserts it,
  and re-defaults it - the second hash op is paid on misses only. Average slots hold a
  `(sum, count)` accumulator and divide with no zero guard: sound because a key enters the
  table only on its first element.
- **`hoist_prelude` hand-builds a flat block on purpose.** The materializer prelude - the
  hoisted default-row declaration on the field-prune path - must be a SIBLING of the walk; a
  `$b(prelude)` splice nests it in its own scope, where the per-element reads cannot see it.
- **One `at` can reach two materializers.** A `group_by` chain closed by
  `first`/`order`/`distinct`/`take` emits through both the array-walk materializer and the
  handle materializer at the same source location, so `qn` - which keys on (prefix, at) -
  keeps the two sites' binds apart only while each site carries its own prefix.
- **An `ExprFor` the typer has not re-inferred carries an EMPTY `iteratorVariables`.** A
  decs bridge feeding a no-chain terminator (`from_decs_template(...).count()`) has no chain
  op to force the second inference pass, so the iterator names are recovered from the push
  tuple's values - each references its iter var by name under an `ExprRef2Value` wrap - and
  match what the loop binds once a later pass infers it.

## 15. linq_fold_sql

- **`extract_sql_source` walks the call's own `arguments[0]` spine, not linq_fold's
  flattened `top`** - the SQL DSL ops are not `linqCalls`, so `flatten_linq` stops at the
  first one and never reaches the `select_from` source. `sql_linq_loaded(prog)` then gates
  on `_sql` being in THIS program: a consumer can get `select_from` from a provider boost
  without `daslib/sql_linq`, and that chain must fall through to the in-memory path.

## 16. linq_fold_decs

- **Column pruning is a four-step pipeline over the `decs_tup` bind, and every step exists
  to reach bind elision.** Scan the body for `decs_tup.<field>` uses; if the bind is
  referenced as a WHOLE var, rewrite each whole-var use into a synthesized
  `(userName1 = iter1, ...)` literal - semantically identical - and re-scan; then flatten the
  surviving field reads to the iter vars and drop the bind entirely. Unused `get_ro` slots
  disappear along with one tuple-make plus N field reads per iteration. The unpruned bind
  stays only for whole-var refs that survive the rewrite.
- **The decs random-index fast paths rest on these source facts.** A plain `[decs_template]`
  field compiles to `get_ro` (an indexable array) while a default-init field compiles to
  `get_default_ro` - an iterator, not indexable - so `decs_can_random_index` bails and the
  walk stands. `get_ro` is `[unsafe_outside_of_for]`, so each cloned source AND the index
  read are marked `alwaysSafe` (mirroring `decs_boost`'s `append_index_lookup`).
  `for_each_archetype` visits archetypes in order and skips empty ones, so a no-predicate
  `last()` overwriting a result per archetype ends holding the global last in
  O(num_archetypes), and `reverse |> take(N)` collects only the tail with the boundary
  archetype's head indexed past rather than walked.
- **Trivial-select elision removes the copy a `_select(_.userName)` would emit.** The
  pruned inner-for already keeps that component's iter var, so the synthetic bind would
  flatten to a pure copy; renaming the chain's `finalBind` to the iter var skips the binding
  entirely. Detecting the shape needs TWO `ExprRef2Value` peels: the typer wraps both the
  projection root and the `ExprVar` inside the `ExprField`.
- **The decs join hook keeps hash-collect and probe inline rather than routing through
  `wrap_source_loop`** - that is what preserves the count-no-`where` fast path, which
  answers from bucket lengths instead of walking pairs. Its key lambdas are synthesized
  upstream by the LinqJoin macro as 1-arg single-return blocks, so the BLOCK's return type
  is the key type; a change to that synthesized shape breaks the decs join silently.

## 17. sql_linq

- **Equal phase numbers are deliberate.** `PHASE_ORDER_BY == PHASE_SELECT` - ORDER BY
  commutes with the projection in SQL (it may reference projected aliases or source
  columns) - and `PHASE_GROUP_BY == PHASE_DISTINCT == PHASE_SET_OP`. Giving either member
  its own number spuriously diverts the canonical `_group_by |> _order_by |> _select` into
  a nested SELECT.
- **Emitted SQL is a template over two in-band markers.** `?` is a bind, `\x01` a runtime
  identifier (a dynamic `_order_by` column name); `next_placeholder` scans outside
  single-quoted literals and always resumes just past a placeholder, so the scan starts
  with no quote open. `sql_to_frags_ex` pairs each marker with `orderedBinds` /
  `orderedInlineIds` in occurrence order and preserves a marker as text once its list is
  exhausted. Nothing reaches the text unquoted: a compile-time constant folds in with SQL
  doubling, a non-constant emits a runtime `sql_quote_id` / `sql_quote_lit` call, and every
  other runtime value stays a `?`.
- **Key contexts render with constants inlined because their fragment is re-used.** A
  computed `_group_by` / `_order_by` key fragment is emitted at several SQL positions
  (SELECT, GROUP BY, ORDER BY) while a placeholder can be bound only once, so
  `render_inlined_key_sql` sets `q.inlineConstants`, admits only `ExprConst*` (strings
  through `sql_quote_lit`), and rejects any key that pushed a bind.
- **`_distinct_by(K)` is dialect-routed on `caps.distinct_on`.** Without it, SQLite's
  bare-aggregate form `SELECT *, MIN/MAX(pk) FROM t GROUP BY K` keeps the `*` columns of
  the min-pk (or max-pk) row per K - SQLite-only semantics; strict engines reject bare
  columns beside GROUP BY. With it, `SELECT DISTINCT ON (K) * ... ORDER BY K, pk [DESC]` - a
  PostgreSQL extension DuckDB also implements. MIN/ASC is linq's "first row per K", MAX/DESC
  is `reverse() |> _distinct_by(K)` "last row per K" - both only while pk is monotonic with
  insertion order.
- **A join's `into` projection registry is snapshotted because the outer projection
  clobbers the live one.** `process_join_call` copies the live projection into
  `joinProjRecordNames` + the `joinSelectCol*` arrays, which survive
  `analyze_grouped_projection`'s clear-and-repopulate. Post-join `_.<alias>` resolves
  through that snapshot in every post-join consumer - `pred_to_sql`'s column-ref arm,
  `collect_one_order_key`, `push_group_key`, `try_translate_group_aggregate`,
  `try_translate_global_aggregate` - all through `find_projection_alias` +
  `render_projection_alias_sql`. A registry miss is rejected loudly everywhere alike: falling
  back to base-table resolution would leak the unqualified base-table namespace into post-join
  predicates.
- **`normalize_single_source_arg_names` exists for the linq_das front end.** `_sql`
  resolves a single source against the placeholder `_`, but LINQ-syntax lowering splices
  the user's range variable verbatim (`$(c) => c.field`), so a single-parameter chain-op
  lambda's bound variable is renamed to `_`. Gated on a pure single-source chain - join and
  set-op key lambdas bind distinct sources and are left alone.

## 33. sql_migrate

- **The audit table is provider-neutral by construction** (client-side epoch seconds,
  BIGINT); duplicate versions are caught in two layers because neither sees everything; the
  `struct_convert_field` overload set is a specificity ladder - deleting a "duplicate"
  silently re-routes conversions.

## 37. linq_fold module layout and the SourceAdapter contract

- **Charters.** `linq_fold` is dispatch only: source recognition (`try_splice_patterns`),
  adapter construction, the tier-2 `fold_linq_default` cascade, the `LinqFold` call macro,
  and the single registrar `register_all_linq_fold_rows`. `linq_fold_common` holds what no
  source can influence - the pattern-table types, the walker, the predicate library, the
  chain pre-passes, the generic emit lanes, the `splice_patterns` registry, and the abstract
  `SourceAdapter`. Each remaining file owns one source: `linq_fold_array` (array, zip,
  array-join), `linq_fold_decs`, `linq_fold_json`, `linq_fold_table`, and pugixml's
  `linq_fold_xml`. `linq_fold_sql` is the exception - it recognizes a `[sql_table]` source
  and hands the whole chain to the `_sql` macro, so it has no adapter and no emit. A source
  file requires `linq_fold_common` and never a sibling source.
- **`try_splice_patterns` tries recognizers in a fixed order** - sql, decs, the registered
  `linq_fold_sources` in registration order, json, table - and the array arm runs last with
  no recognizer of its own: it is what claims a chain nobody else claimed. A source outside
  daslib joins the `linq_fold_source` group from its descriptor and from its C++ module's
  constructor, and registers a
  `LinqFoldSourceRecognizer` from its `register_linq_fold_source` entry, which
  `register_all_linq_fold_rows` calls on every member; pugixml's `linq_fold_xml` is the one
  such source.
- **The adapter contract is four abstract methods; everything else on the base has a
  default.** `bind_name`, `element_type`, `wrap_source_loop` and `wrap_invoke` are abstract,
  so every source answers them. The per-operation hooks (`emit_loop_or_count`,
  `emit_reverse_skip_into_tail`, `emit_reverse_last_backward`, `emit_distinct_take_loop`,
  `build_group_by_adapter`, `emit_join_hook`) return null on the base, and a null sends the
  caller down its generic path. The capability methods (`can_group_by`, `can_join`,
  `can_reserve_by_length`, `has_own_loop_or_count_lane`, `supports_direct_return`,
  `effective_dispatch`, `defers_materialization`, `count_shortcut`, `invoke_param_type`,
  `name_prefix`) are the source answering questions about itself. daslang classes have no
  `is`/`as` downcast, so an emit fn holding a `SourceAdapter?` reaches source-specific
  behavior only through these methods - there is no place to write a per-source branch, and
  adding a source needs no edit to a central enum or switch.
- **The capability defaults are fail-closed, and `supports_direct_return` is the one that
  matters.** A wrong `true` emits a mid-loop `return` that escapes a nested-callback walk
  (decs's `for_each_archetype`); a wrong `false` only costs the slower state-var +
  find-stop + tail path.
- **`loop_source_expr` / `loop_source_name` is what lights up the array-shaped lanes.** The
  shared lanes spell their loop and their `length(...)` reads with the name, and read the
  expr for compile-time facts - the `type_has_length` reserve gate and the join's srcA
  element-type check. Array and table override both, xml and json override the name only,
  decs overrides neither.
- **`ProjectedSourceAdapter` absorbs a leading source projection.** When a row captures the
  optional `srcsel` slot (`source |> _select(f) |> order/distinct/take`), the dispatcher
  wraps the real adapter in this decorator: it binds `projName = f(rawElem)` on top of the
  per-element body and delegates loop and invoke to the inner adapter, so every emit sees the
  projected element unchanged. It leaves `loop_source_expr` / `loop_source_name` /
  `can_reserve_by_length` at the base defaults, which disables the source-direct fast paths -
  those would bypass the projection.
- **Rows are per operator family, not per source.** The seven `build_*_rows` fns
  (order_family, loop_or_count, reverse, distinct, group_by, join, zip) emplace 17 rows into
  the one `splice_patterns` table; only zip's builder lives outside `linq_fold_common`.
  Source gating happens inside a row - through a `RequiresPredicate` that asks the adapter
  (`can_group_by_source`, `can_join_source`, `array_source`, `non_array_source`) or through
  a hook that returns null. Registration order is match priority: the dispatcher walks the
  table top to bottom and the first row whose chain matches and whose emit returns non-null
  wins, so more specific chains are declared first. A row whose slot chain is a strict prefix
  of a later row's shadows that row (`chain_prefix_of`).
- **The walker consumes the whole call list or fails.** `match_pattern` walks the flattened
  calls left to right against the row's slots: `c_one` must match and advances both cursors,
  `c_opt` advances both on a match and skips the slot otherwise, `c_chain` matches greedily
  while the call is in the slot's name set and always succeeds - including with zero matches,
  which still creates the (empty) `captures.many` entry, so an emit fn can rely on the key
  existing. Any call left unconsumed after the last slot is a no-match. The `requires`
  predicates run last, against the populated captures.
- **`Captures.single_name` exists because a pre-pass renames the op without touching the
  AST.** `normalize_order_reverse` folds a trailing `reverse` into the order op by swapping
  the tuple's `LinqCall` record, leaving the `ExprCall` alone; deriving the name back from
  the `ExprCall` (`call_norm_name`) would silently undo the swap. The walker therefore
  records the `LinqCall.name` at match time, and any emit fn that cares about the
  post-normalize name reads `single_name`.
- **`alias_table` names the op-name groups a `m_alias` slot admits, and a missing name fails
  silently.** A terminator the lanes handle but `loop_terminator_family` omits never matches
  the terminator slot, so the chain leaves the splice and lands on the tier-2 cascade with no
  diagnostic - the symptom is a slow chain, not an error.
- **Predicates and emit fns are named module-level functions, referenced as
  `@@<RequiresPredicate> name` / `@@<EmitFn> name`.** An anonymous `@@(...)` lambda compiles
  to a `_localfunction_*` symbol the LLVM JIT pass cannot resolve; a named function has a
  stable address.
- **The table lane's key and value binds are asymmetric on purpose.** `keys(tab)` yields
  non-const elements (writable temp copies), so the key lanes rebind the loop var through a
  `let` before the body sees it; `values(tab)` over the const table param already yields
  `V& const` and binds directly. Keys are workhorse types, so the copy is free, and
  downstream `==const` composition (a `push_clone` of a bare projected key) needs the const.
