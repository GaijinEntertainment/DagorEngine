# daslib linq family Code Review Checklist

**A diff touching the linq family - `linq*.das`, `sql_*.das` - applies this checklist
together with `REVIEW.md`.** `REVIEW_COMMON.md` (repo root) binds this file too.
Architecture doc: `ARCHITECTURE_LINQ.md`.

**A fused emit that binds a terminator's default or compare argument lazily, more than once, or
below the top of the generated invoke, every `*_or_default` decs lane included, is a defect** -
bind it once at the top, even on paths that never use it, because the unfused `linq.das`
overload evaluates it that way.

**A fused emit whose empty-source behavior differs from the unfused `linq.das` overload's -
panicking where the unfused overload yields the default, or yielding a default where it
panics - is a defect.** A fused `first` over a prefilter buffer panics before it reaches
`min`/`max`, which return an uninitialized reference on an empty array.

**Never add `count` or `first` to the 2-arg reducer set (`is_bucket_reducer_call`) - it admits
only a reducer whose second argument is a selector.** `sum` / `min` / `max` / `average` take a
selector there; `count` / `long_count` take a predicate, and a predicate spliced into a
projection slot is silently wrong.

**Never gate the distinct lane's dedup insert on the terminator's `count(pred)` predicate - that
predicate gates the counter only.** `distinct[_by]` keeps the first occurrence per key, so an
insert under that predicate changes which occurrence represents the key. A `where_` filter
gating the whole per-element body is expected.

**A generated invoke that inserts into or erases from a table it probes by reference off
`unsafe(tab?[k])` is a defect** - the bind is a borrowed pointer into table storage, and a rehash
turns it into a use-after-free. This binds both the join probe (`build_join_probe_pieces`) and
the point-lookup probe (`try_table_point_lookup`).

**A diff that leaves `constant` unset on the table parameter type
`TableAdapter.invoke_param_type` returns is a defect.** That const proves the point-lookup
probe's borrowed bind survives the invoke; without it the bind is a use-after-free.

**A per-field walk over a row-usage set that is neither sorted first nor ordered by an external
declaration-order list is a defect** - a membership test over the same set needs no order.
`collect_row_usage` / `collect_decs_tup_usage` return names in `table<string>` key order, so an
unsorted walk makes the emitted AST a function of hash order instead of the chain, which
AST-shape tests and AOT semantic hashes both pin.

**Two emit sites reachable at the same `at` that share one `qn` prefix are a defect - give each
site its own prefix.** `qn` keys only on (prefix, at), so a shared prefix shadows a bind instead
of colliding loudly.

**A diff that changes one implementation of the composite-key ordering also changes the others,
in the same change** - `key_less` / `less_masked` (`linq.das`), which sort eagerly, and
`try_make_inline_cmp_keys` (`linq_fold_common.das`), which emits an inline `_::less` if-chain for
the same chain. All spell bit `i` of `mask` as "key `i` descending" (LSB = first key), flip
operand order for descending, and break ties in key order; one side changed alone makes a
spliced chain and its unfused fallback sort differently, with no error.

**A diff that changes the inline `_::less` if-chain's key cap or `less_masked`'s tuple overloads
changes both, in the same change.** A key tuple longer than the cap declines the splice and sorts
eagerly, so a cap raised without the matching overload leaves a spliced chain with no unfused twin
to agree with.

**Never reserve `n` in a `top_n*` lane over an iterator - let the bounded-heap fill phase grow
geometrically.** Cardinality is unknown there, so an `n` far above the element count allocates
the whole `n` upfront for no win.

**A linq call-macro that synthesizes the `_` block parameter without setting `can_shadow` on it
is a defect.** Chains nest, and the inner `_` has to shadow the outer one.

**Never move the `_sql` hand-off below `normalize_order_reverse` / `collapse_chained_*` or below
`if (empty(calls))`.** `_sql` re-expands the pristine expression on the next infer pass, and a
`[sql_table]` source with no chain ops at all is still SQL.

**A pattern row that emits when its captured `select` (the `srcsel` slot) cannot be wrapped into
a projected adapter is a defect - skip the row instead.** The row's emit assumes the projected
element, so running it on the un-projected adapter orders or dedups raw rows - a wrong result,
not a missed splice.

**A diff that reorders the registration calls in `register_all_linq_fold_rows` - the
`build_*_rows()` calls, the group call that registers the sources - without naming the chains
whose matching arm changes is a defect.** Rows land in `splice_patterns` in
call order, the walker takes the first match, and that registrar is the only place the order
exists.

**A sql_linq column-ref arm that accepts a `$e(recv).$f(field)` qmatch without first proving
`recv is ExprVar` is a defect.** A nested receiver otherwise matches under a foreign name and
emits a wrong column, with no error.

**A sql_linq computed-expression arm that runs without an `is_sql_renderable_scalar` gate ahead
of it is a defect.** That type gate routes a whole-row carry var to the clean row-object reject
instead of to a macro-time crash in `pred_to_sql`.

**A call to a SQL-returning helper that checks only one of `q.hadError` and `empty(frag)` is a
defect - check both.** Operator arms wrap an empty child into a non-empty fragment and several
helpers return `""` without setting `hadError`, so one check alone emits malformed SQL in place
of a diagnostic.

**A path from `analyze_chain` to `build_sql_string` / `collect_query_binds` that skips
`maybe_finalize_distinct_by_passthrough` or `maybe_wrap_take_before_aggregate` is a defect.**
Skipping the first drops a `_distinct_by` dedup with no error; skipping the second leaves
LIMIT/OFFSET on the aggregate.

**Never install a wrap over an already-populated `q.innerSql` or `q.innerBindExprs` - each takes
exactly one producer per query.** The wrap drops the earlier subquery and orphans its binds - a
bind/placeholder mismatch, with no error.

**A diff that adds a projection slot pushes one entry into every index-parallel projection
array - `selectCols`, `selectColAliases`, `selectColSqlFragments`, `selectColTypes`,
`projRecordNames` - in the same change.** `push_source_column` and `push_computed_proj_slot`
cover only part of the set and leave the rest to their caller; a partial push desyncs the SELECT
list from the row builder, with no error.

**A diff that adds a post-join `_.<alias>` consumer to sql_linq resolves the alias through
`find_projection_alias` + `render_projection_alias_sql`, and rejects a miss rather than
falling back to resolving the name against the base table.** After a join the alias lives in
the join projection snapshot, so base-table resolution answers with an unqualified
base-table column - a wrong column, with no error.

**Never pick a projection slot's SQL by `q.seenJoin` - pick it by which of that slot's own
entries is non-empty: SQL fragment first, then aliased column, then unqualified column.**

**A diff that adds a clause that can emit `?` also pushes that clause's binds at its SQL parse
position in `collect_query_binds`, in the same change.** `sql_to_frags_ex` re-scans the emitted
SQL text and pairs markers with that list in occurrence order, so a clause added to the emitter
alone binds every later placeholder to the wrong value, with no error.

**A generated SQL statement's column list and its bind function change in one edit.** Bind
indices are placeholder positions over bindable columns, never struct field positions; drift is
silently wrong data, not an error.

**A diff that adds or changes a `[sql_table]` helper pair registers its 2-arg form BEFORE the
1-arg form.** `find_struct_helper_fn` keeps the LAST match and the finish pass rewrites the
helper it returns, so a swapped pair attaches index DDL to the wrong overload.

**Never narrow or widen the linq_das clause-keyword test - a clause keyword is a whole word at
bracket nesting depth zero, not preceded by `.` and not preceded by a `>` that tails `|>`, `=>`
or `->`.** Narrowing lets an in-body aggregate parse as a `select` clause; widening the
exclusion to any `>` hides a clause keyword that legitimately follows a generic bracket or a
comparison.

**A diff that changes the token model in one substituting linq_das scanner - `substitute_idents`,
`mentions_ident`, `rewrite_group_var` - changes it in the others, in the same change** (the
shared model: `ARCHITECTURE_LINQ.md` sec. 13). A model change in one scanner desyncs
`mentions_ident` from the rewrite it gates, and the emitter then renames a parameter the
spliced projection still references.

**Giving `find_kw_depth0` the substituting scanners' interp-as-code model is a defect - it skips
a whole string literal, interpolation bodies included.** A clause keyword inside `"{...}"` must
never claim a stage terminal.

**A `parse_one_stage` that reads a `where` or `orderby` position before it resolves the stage
terminal - the earliest `select`/`group` at or after `start` - is a defect.** The terminal bounds
the stage, so scanning clauses first lets a later stage's `where`, past this stage's `into`,
be taken as this stage's terminal.

**Never widen the `group ... into g` rewrite past its two spellings - bare `g` -> `g._1` and
`g.key` -> `g._0` - leave every other `g.<member>` verbatim.** Raw tuple access then keeps working
and a bad member errors as itself; a wider rewrite turns `g._0` into `g._1._0`.

**Never `return call` from a `FromInMacro` reject, the not-yet-inferred-source arm included -
report `macro_error` and `return null` instead.** Returning the call reports ast-changed on every
pass and churns to the infer-pass cap (30507).

**Accepting a decs range slot out of canonical chain order - skip -> skip_while -> take_while ->
take, all after any `where_` - is a defect.**

**Emitting the decs range guards in any order other than take-cap -> skip counter -> skip_while
flag -> take_while break -> take bump is a defect** - that order mirrors the array side's
`wrap_with_ranges`. The bump is last so an element the while-guards rejected does not count
against `take(N)`.

**A predicate-driven decs range accepted with a select in its prefix is a defect** - its
predicate peels against the source tuple, so a select ahead of it changes the element the
predicate reads.

**Emitting a decs range counter or the take limit inside the archetype walk, rather than in the
prelude above it, is a defect.** That state is global to the query, and the user's `take(N)`
expression must evaluate exactly once.

**A diff that drops a decs component slot erases it from every parallel `ExprFor` vector -
`sources`, `iterators`, `iteratorsAt`, `iteratorsAka`, `iteratorsTags`, back-to-front - and
clears `iteratorVariables` so the typer rebuilds it, in the same change.** A vector left
un-erased pairs a surviving slot with the wrong component.

**Never route `long_count` through the decs `arch.size` shortcut - walk the entities instead.**
The shortcut accumulates `acc += int(arch.size)`, so it is int-safe only.

**Never derive `emit_decs_reverse_skip_into_tail`'s projection type from the peeled lambda body -
take it from the typer-resolved call type, and prove that type non-null before using it.**
`peel_lambda_rename_var`'s invoke fallback carries a null `_type` at macro stage.

**`emit_loop_or_count_lane_decs` that lets a `to_table` terminator reach the implicit-to_array
arm is a defect - decline `to_table` ahead of that arm.** decs has no to_table lane, so the
implicit arm emits an array for a table-typed expression.
