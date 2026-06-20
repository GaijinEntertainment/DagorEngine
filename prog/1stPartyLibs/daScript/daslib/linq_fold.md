# linq_fold.das refactor — masterplan

Living document. Update **Status** + **Decision log** as phases ship.

## Status

- [x] **PR A** — Foundation + first migrations (plan_reverse, plan_distinct) — branch `bbatkin/linq-fold-patterns-foundation`
- [x] **PR B1** — KR-1 closure (`collapse_chained_wheres`) + `c_chain` cardinality + `Captures` wrapper struct + `plan_loop_or_count` migration — branch `bbatkin/linq-fold-pattern-table-prb` (PR #2881 merged)
- [x] **PR B2** — `plan_order_family` migration (4 emit archetypes + 5 rows) + `Captures.single_name` parallel-table extension — branch `bbatkin/linq-fold-pattern-table-prb2` (PR #2883 merged)
- [x] **PR C** — SourceAdapter widening + 4 decs planner migrations (plan_decs_reverse / _distinct / _order_family / _unroll) — branch `bbatkin/linq-fold-pattern-table-prc`
- [x] **PR D1** — Group-by migrations: `plan_group_by` + `plan_decs_group_by` → 2 pattern rows + 1 shared emit fn delegating to `plan_group_by_core` (unchanged sub-codegen). `emit_reducer_branches` 13-arm if/elif → `mk_reducer_*` data table. Partial GroupBy↔SourceAdapter reconciliation via `to_source_adapter()` projection. Branch `bbatkin/linq-fold-pattern-table-prd1`
- [x] **PR D2** (partial) — `plan_zip` + `plan_decs_join` migrations; SourceAdapter widening to `Array | Decs | DecsJoin | Zip`; `plan_decs_join` thin stub gains `collapse_chained_wheres` pre-pass. Branch `bbatkin/linq-fold-pattern-table-prd2`
- [x] **PR D3** — Full GroupBySourceAdapter melt (deferred from PR D2). Struct + 3 helpers (`to_source_adapter`, `adapter_emit_source_loop`, `adapter_finalize_emission`) deleted; `plan_group_by_core` takes `SourceAdapter` directly; `emit_group_by` builds `SourceAdapter` per source-shape. Branch `bbatkin/linq-fold-pattern-table-prd3`. The PR D2 C3 regression root cause turned out to be a one-character typo: `resBindName := qn(...)` in a named-arg struct constructor silently dropped the field (the `:=` move-assign syntax is invalid in named-arg constructor position — must be `=`); changing the operator fixed the test cleanly. Closes the per-planner-stub migration.
- [x] **PR E** — Final collapse: 7 per-plan tables → 1 `splice_patterns` (commit 1); 12 stub fns + 12-line LinqFold.visit cascade → 1 `try_splice_patterns` dispatcher (commit 2). Per-row predicates (`array_source` / `decs_source` / `decs_join_invariants`) handle adapter selection; dispatcher walks the table twice (Decs adapter if bridge present, then Array). Pre-passes run once at the top — safe because `normalize_order_reverse` / `collapse_chained_*` are no-ops on chains that don't trigger them. Branch `bbatkin/linq-fold-pattern-table-pre`. **Closes the per-planner-stub refactor.**

## Phase F — emission-lane unification (post-PR-E)

Match-side is fully uniform after PR E (every row is `SplicePattern { name, chain, requires, emit }` walked by `try_splice_patterns`). Emit-side is the inverse: 22 leaf `emit_*` functions, each hand-builds its own `qmacro_block` scaffold. detect_duplicates: zero pairs among them cluster above 0.7 similarity. Structural sameness is invisible mechanically because every var name, op snippet, and decs-vs-array splice is hand-typed.

Inventory: **19 of 22 leaf emit fns fit one of two clean shapes** — 11 Terminator (`var acc=init; for(...){update}; return acc`), 7 FoldArray (`var buf; for(...){push}; tail; return buf`), 3 Passthrough (no for-loop). Two are genuinely multi-pass (`emit_decs_join`, `emit_decs_reverse_skip_into_tail`); one is no-entity-walk (`emit_decs_count_archsize`). Plan: 5 PRs introducing `TerminatorSpec` / `FoldArraySpec` / `PassthroughSpec` + per-lane fns; ~-900 LOC total.

- [x] **PR F1** — Terminator lane + migrate the 2 array-side orchestrators (`emit_accumulator_lane` + `emit_early_exit_lane`). `TerminatorSpec` struct + `emit_terminator_lane` fn + `build_lane_adapter` helper. Orchestrators now build a `TerminatorSpec` (op-specific prologue + perElement + tail) and delegate scaffold to the lane fn. `finalize_lane_emission` orphaned + deleted (now via `adapter_wrap_invoke`). AST parity verified byte-identical on `test_linq_fold_loop_or_count.das` + `test_linq_aggregation.das`. Branch `bbatkin/linq-fold-emission-lanes-pr1`.

- [x] **PR F2** — Migrate 4 array-side Terminator leaves: `emit_counter_lane`, `emit_reverse_counter`, `emit_streaming_min`, `emit_reverse_walk_overwrite_scalar`. Spec extended with `outElemType` + `wrapIter` slots (threaded to `adapter_wrap_invoke`); previous orchestrators continue to pass null/false by struct default. `emit_counter_lane` keeps signature (still called from `emit_loop_or_count_lane`) but body collapses to a thin `TerminatorSpec` builder + `build_lane_adapter` call; range PRELUDE state-decls move into spec.prologue (caller already wraps in-loop range checks). AST parity verified byte-identical on counter / reverse-counter / streaming-min / reverse-walk chains. 3 decs leaves (`emit_decs_min_max_by`, `emit_decs_element_at`, `emit_decs_walk_lane`) deferred to PR F3 since they need the WalkMode (`for_each_archetype_find`) decs-adapter extension that PR F3's orchestrator migration introduces. Branch `bbatkin/linq-fold-emission-lanes-pr2`.

- [x] **PR F3** — Decs Terminator lane + migrate 2 decs orchestrators + 3 decs leaves (deferred from F2). Choice: **parallel lane** rather than full SourceAdapter unification — `DecsDispatch` 4-tag variant (`Each` / `Find` / `FindReturn` / `FindReturnNegated`) + `DecsTerminatorSpec` (preludeStmts, perElement, tailStmts, retType, dispatch) + `emit_decs_terminator_lane(spec, bridge, chainInfo, rangeInfo, calls, intermediateEnd, at)` owns: range-prelude (`decs_range_prelude`), per-element range wrap (`wrap_inner_for_with_decs_ranges`), chain wrap (`wrap_decs_chain`), inner-for build (`build_decs_inner_for_pruned`), 4-way for_each_archetype{,_find} dispatch, invoke wrap, force_at/generated. 5 decs fns migrate: `emit_decs_accumulator` + `emit_decs_early_exit` (orchestrators) + `emit_decs_min_max_by` + `emit_decs_walk_lane` + `emit_decs_element_at` (leaves). `emit_decs_count_archsize` stays bespoke (no-entity-walk shape). `emit_terminator_lane` (array side) unchanged. Net **-52 LOC** in linq_fold.das (-193 / +141). AST parity verified byte-identical on 6 chains covering all 4 dispatch modes (count Each, _select+sum Each, take.count Find, _where._any FindReturn, _all FindReturnNegated, _min_by Each). Branch `bbatkin/linq-fold-emission-lanes-pr3`.

- [x] **PR F4** — FoldArray lane + migrate 5 fns. New surface: `DistinctGateKind` variant (`NonKey` / `KeyLam`) + `DistinctGateSpec` (dset/dkey names, bindName, bufElemType) + `FoldArraySpec` (prologue / bufDecl / postBufDecl / reserve / preCond+whereCond / distinctGate / perElementPush / tailStmts / outElemType / wrapIter) + `emit_fold_array_lane` (composes perElement: distinct gate INNERMOST → whereCond → preCondStmts; assembles bufDecl → dsetDecl → postBufDecl → reserve → source-loop → tailStmts) + `build_terminal_select_tail` helper (raw array tails: `[return <- buf]` or `[var outBuf; reserve; for-loop; return <- outBuf]`). `DecsTerminatorSpec` extended with `wrapIter` knob (routed through `finalize_decs_emission`). Migrations: `emit_array_lane` (deleted, sole caller `emit_loop_or_count_lane` array arm builds FoldArraySpec inline); `emit_decs_to_array` (reuses `emit_decs_terminator_lane` per D-NEW-1 — saves ~50 LOC of parallel decs scaffold); `emit_reverse_buffer_inplace` (decs skip-into-tail fast path preserved); `emit_fused_prefilter` (4 tail arms hand-emitted: first / first_or_default / bare order / order+take); `emit_bounded_heap` (postBufDecl slot holds outBuf+early-return-on-takeN-zero). Stays bespoke per D6: `emit_reverse_backward_index_walk`, `emit_reverse_backward_walk_dset_gate` (both bypass adapter_wrap_source_loop). Bugfix surfaced during fused_prefilter migration: lane composition initially had distinct gate OUTSIDE whereCond, breaking `where+distinct` chains (dset recorded rows that whereCond rejected); fixed to distinct gate INSIDE whereCond. Net **+23 LOC** in linq_fold.das (+258 / -235) — roughly break-even: ~+118 LOC for the lane+specs+helpers offsets ~-95 LOC across the 5 migrations. Structural win: 5 emit fns are now thin spec-builders instead of hand-rolled qmacro scaffolds; future Passthrough PR F5 benefits since FoldArray sits next to it. Branch `bbatkin/linq-fold-emission-lanes-pr4`.

- [x] **PR F4.1** — FoldArray closure (D6 revisit) + post-F4 cleanup. With the F4 lane battle-tested, the D6 deferral premise no longer holds: the two reverse-walk fns are 30+80 LOC of nearly duplicated scaffold that the lane already owns. New surface: `customSourceLoop : Expression?` field on `FoldArraySpec` (when non-null, replaces `adapter_wrap_source_loop` at the source-loop step) + `compose_fold_per_element(spec)` helper (extracted from the lane so customSourceLoop callers can wrap the exact body the lane would have wrapped, in their own outer-loop shape — for D6's backward integer-index walks). Lane still owns prologue/bufDecl/dsetDecl/postBufDecl/reserve/tail/invoke wrap; only the source-loop step is overridable. Migrations: `emit_reverse_backward_index_walk` (no distinctGate, perElementPush is inline `src[len-1-k]`); `emit_reverse_backward_walk_dset_gate` (DistinctGateSpec populated, element bind moves to preCondStmts so lane composes preCond → gate → push identically to the bespoke version). Both fns now thin spec-builders, byte-equivalent emission. Cleanup sweep bundled: delete orphaned `is_buffer_required_op` (zero callers since pre-F4 pattern-table migration) + trim 4 stale-prose comment blocks per CLAUDE.md hygiene. Net **+12 LOC** in linq_fold.das (+77 / -65) — infrastructure overhead (~+15 for hook + helper signature) and spec-builder boilerplate offset most of the migration savings; the win is structural (zero bespoke FoldArray fns remain). Bench refresh: all cells within 8% of F4 baseline, drift verified as bench-suite ordering noise via isolated re-runs. PR F5 (Passthrough lane: `emit_length_shortcut` + `emit_any_empty_shortcut` + `emit_decs_count_archsize`, ~46 LOC total) remains independently doable later — likely net-LOC-neutral, not bundled here. Branch `bbatkin/linq-fold-foldarray-customsourceloop`.

- [x] **Zip-lane unification + count/long_count merge** (post-G3c, branch `bbatkin/linq-fold-zip-lane-unify`). With G3c (PR #2948) un-pinning `emit_counter_lane` / `emit_fold_array_lane` into `linq_fold_common`, `emit_zip`'s last two bespoke paths — counter (`count`/`long_count`) and buffer (`to_array`), ~130 LOC duplicating those lanes — now ride them. Three commits: (1) **internalize the `count_shortcut` applicability gate** so every source (array/decs/zip) answers `null → walk` itself and the caller is source-agnostic (was: `ArrayAdapter` relied on an external `type_has_length(top)` gate at the call site; `DecsAdapter` already gated internally); (2) **extend `emit_counter_lane` with `isLong`** (int64 acc) and route `long_count` through it behind one `counting` predicate (`COUNTER || (ACCUMULATOR && long_count)`) used at every counting-lane site — retiring the count/long_count lane split on array + zip; sum/min/max/average stay on the accumulator lane; (3) **add `ZipAdapter.count_shortcut`** (O(1) `min(len a, len b)`, int for count / int64 for long_count) and **collapse `emit_zip`** — counter → `emit_counter_lane(isLong)`, buffer → `emit_fold_array_lane(FoldArraySpec)`. `emit_zip` now has zero inline terminator emission (parse → accumulator → early-exit → count-shortcut → counter lane → buffer lane); a future change to those lanes no longer needs a parallel zip edit. Behavior-preserving (emission equivalent, not byte-identical): all zip + count/long_count bench cells within thermal noise with identical alloc profiles (`long_count_aggregate` m3f 4.1/0.3, `zip_count_pred`/`zip_dot_product` m3f 0.1, `zip_reverse_to_array` m3f 4.6). `emit_accumulator_lane`'s `long_count` arm is left in place (decs may still use it; now unreachable from array/zip, removal pending a decs check).

- [x] **PR F4.2** — Close decs-side post-take-where splice gap (Theme 2 5c on decs). The `benchmarks/sql/take_where_count` row exposed a 354× outlier: m4 = 35.4 ns/op vs m3f = 0.1 ns/op INTERP (10.4 vs 0.0 JIT). Root cause: `emit_loop_or_count_lane_decs` was explicitly cascading on `post_take_where` capture ("no decs lane fn yet"), so `take(N) |> _where(P) |> <term>` chains fell through to tier-2 unspliced `from_decs` (~3.5 ns/elem of iterator overhead × N). Array side has owned this shape since Theme 2 PR #2852 via `postTakeWhereCond` slot in `emit_accumulator_lane` / `emit_early_exit_lane` / the buffered tail path. Decs fix mirrors the array-side gate at a single chokepoint: new `postTakeWhereCond : Expression?` field on `DecsRangeInfo` populated in `emit_loop_or_count_lane_decs` (peeled against `chainInfo.finalBind`, simpler than array-side seenSelect split because `finalBind` already collapses to tupName / selected name / elided iter var), consumed in `emit_decs_terminator_lane` by wrapping `spec.perElement` BEFORE `wrap_inner_for_with_decs_ranges` — the gate lands AFTER takeC++ and BEFORE op-specific work, identical semantics to array. Because all 6 emit_decs_* paths (accumulator/early_exit/walk/to_array/min_max_by/element_at) route through the shared lane, the splice fires uniformly on every terminator with zero per-emit-fn signature change. 7 new decs tests under `test_linq_fold_theme2_trailing_where.das` cover count/sum/first/to_array lanes plus head-where+gate combo, select-before-take peel, and skip+post-take cross-shapes (1689 → 1703 tests/linq). Bench: take_where_count m4 35.4 → 0.1 ns/op INTERP (354× → 1×); other lanes within suite-ordering noise. Net **+19 LOC** in linq_fold.das (+22 / -3). Branch `bbatkin/linq-fold-decs-post-take-where`.

## Phase G — class-based source adapters (file split prep)

Match-side (Phase E) and emit-side (Phase F) are uniform. Phase G makes the **source side** open: replace the closed `variant SourceAdapter` with a class hierarchy so each data source (array, decs, future XML via dasPUGIXML) lives in its own file, and linq_fold keeps only the pattern-matching engine. The staged plan is the G1–G4 checklist below.

- [x] **PR G1** (`de0851b62`) — `variant SourceAdapter` → abstract `class SourceAdapter` + 5 subclasses (`ArrayAdapter` / `DecsAdapter` / `DecsJoinAdapter` / `ZipAdapter` / `ArrayJoinAdapter`). The 4 dispatch fns (`adapter_bind_name` / `adapter_element_type` / `adapter_wrap_source_loop` / `adapter_wrap_invoke`) → virtual methods; `EmitCtx.src : SourceAdapter?`. Source-branching emit-fn sites (`ctx.src is Array/Decs`) → virtual calls via a transitional `kind()` enum + getters. All in linq_fold.das. Behavior-identical; verified across ~550 linq/decs tests. Branch `bbatkin/linq-fold-adapter-extraction`.
- [x] **PR G2** — File split, done in 5 commits:
  - **G2a** (`490b95947`) — virtualize the 5 decs-path entry sites through `SourceAdapter` hook methods (`emit_loop_or_count` / `emit_reverse_skip_into_tail` / `emit_distinct_take_loop` / `build_group_by_adapter` / `emit_decs_join_hook`, default-null on base, overridden by `DecsAdapter`); drop the `decsBridge()`/`decsTupName()` getters so the base no longer references `DecsBridgeShape`.
  - **G2b** (`f6966eae0`) — consolidate the 8 per-family `register_*_rows` `[_macro]`s into plain `[macro_function] build_*_rows()` + one engine `[_macro] register_all_linq_fold_rows` calling them in definition order (separate modules run `[_macro]`s in separate macro contexts that can't coordinate; verified `macroInit` runs in definition order so first-match priority is preserved).
  - **G2c** (`ede0ed9d7`) — extract `linq_fold_common.das` (`module linq_fold_common shared public`): kernel types, matchers, walker, predicates, chain transforms, the `splice_patterns` registry, the abstract `SourceAdapter` base, the adapter-pure generic lanes, and `DecsBridgeShape`/`extract_decs_bridge` (AST-only shape detection, no ECS coupling).
  - **G2d** (`22a1b3fbd`) — extract `linq_fold_array.das` (`module linq_fold_array shared public`, requires common): `ArrayAdapter`/`ZipAdapter`/`ArrayJoinAdapter` + the lanes that construct concrete adapters + array row-builders.
  - **G2e** (`11141555d`) — extract `linq_fold_decs.das` (`module linq_fold_decs shared public`, requires common): `DecsAdapter`/`DecsJoinAdapter`, the decs-bridge visitors, the decs terminator scaffold, and the `emit_decs_*` lanes.

  Final engine `linq_fold.das` is dispatch-only — `fold_linq_default`, `try_splice_patterns` (constructs `new ArrayAdapter`/`new DecsAdapter`), the `LinqFold` macro, and `register_all_linq_fold_rows` — and `require`s all three sub-modules `public`, so `linq_boost → linq_fold` API is unchanged. DAG: `common ← array`, `common ← decs`, `{common, array, decs} ← linq_fold`. No cycles. Behavior-identical at every step (no emit change, no benchmark refresh).
- **PR G3** — Unify the decs parallel terminator scaffold onto the generic lane. **Split into a bounded merge (done) + a full unification (planned):**
  - [x] **G3a** (`f991ff251`) — `count_shortcut(opName, at)` capability on `SourceAdapter` (array `length`/`long_length`, decs `count`→`Σ arch.size` / `long_count`→null-must-walk). Both bare-count call sites route through `ctx.src->count_shortcut(...)`. Emission byte-identical.
  - [x] **G3b** (`11022b0a0`) — extract `build_accumulator_spec` + `AccumulatorSpec` into common; `emit_accumulator_lane` (array) and `emit_decs_accumulator` (decs) share the per-op prologue/return/retType (they already shared `build_accumulator_perelement_stmts`). Array byte-identical; decs `long_count` prologue gains an explicit `: int64` (runtime-identical).
  - [x] **G3c (lane relocation — `1a3ee19d5` un-pin + pure-move follow-up).** The four source-generic emit lanes (`emit_counter_lane` / `emit_accumulator_lane` / `emit_early_exit_lane` / `emit_loop_or_count_lane` + `build_loop_or_count_rows`) were pinned to `linq_fold_array` only because they resolved their adapter via `build_lane_adapter`, which hard-constructs `new ArrayAdapter` / `new ZipAdapter` — a compile dependency on the array-file adapter classes. **Commit 1 (un-pin):** callers pass an explicit resolved adapter; the lanes take a single `var adapter : SourceAdapter?` (dropping the parallel `topExprs`/`srcNames` arrays + `adapterOverride` that existed only to feed `build_lane_adapter`). `emit_loop_or_count_lane` passes `ctx.src` directly — `loop_or_count_general` has no leading-select slot so `ctx.src` is never a `ProjectedSourceAdapter`, and decs short-circuits via `has_own_loop_or_count_lane` before these lanes, so only `ArrayAdapter` / `XmlAdapter` reach them; `emit_zip` hoists its `ZipAdapter` above the lane dispatch. Deletes `build_lane_adapter` + the now-dead `can_ride_array_lane` capability (base + `XmlAdapter` override). **Commit 2 (relocate):** the four lanes + `build_loop_or_count_rows` move verbatim into `linq_fold_common` (646/646 pure cut-paste). Behavior-preserving throughout — emitted AST byte-identical, validated by AOT semantic-hash identity (zero `error[50101]` across linq/pugixml/decs/sqlite) + `benchmarks/sql` within thermal noise with identical alloc profiles. `linq_fold_array` now holds only the array/zip/join adapter classes + their source-specific emits (`emit_zip` / `emit_array_join` / row-builders), so an XML/json/future-source PR no longer touches it. This unblocks G3d: the generic lanes now live where decs can fold onto them.
  - [x] **G3d (decs FULL UNIFICATION).** The whole `emit_decs_*` terminator scaffold + `emit_decs_terminator_lane` driver + `DecsTerminatorSpec` are deleted; the decs source now rides the generic `emit_{terminator,counter,accumulator,early_exit,fold_array}` lanes through `DecsAdapter`. How it landed:
    1. **`LoopDispatch`** (`Each`/`Find`/`FindReturn`/`FindReturnNegated`) promoted into common + `supports_direct_return()` capability (array/zip/join + xml/xml-join `true` = mid-loop `return X` exits the invoke; decs/decs-join `false` = nested `for_each_archetype` callback needs state-var + find-stop + tail). Loop seam became `wrap_source_loop(loopShape : LoopDispatch; var body; at)` — array/zip/xml frame an unconditional loop and ignore `loopShape`; decs maps it onto `for_each_archetype{,_find}`. Added `effective_dispatch(base)` (decs upgrades `Each`→`Find` when `take`/`take_while` is present, since the range guard's `return true` only works in a find bool-lambda).
    2. **The decs walk lives entirely in `DecsAdapter.wrap_source_loop`** (driven by `chainInfo`/`calls`/`intermediateEnd`/`rangeInfo` fields set by the dispatcher): in-loop ranges (`wrap_inner_for_with_decs_ranges`) + chain-binding (`wrap_decs_chain`) + column pruning (`build_decs_inner_for_pruned`) + range prelude + `for_each{,_find}` framing. It hands the generic lanes a body referencing one bind (`bind_name`) and passes **empty** where/binds/range fields, so the lanes' existing `wrap_with_ranges`/`append_ranges_prelude` no-op for decs — no separate adapter range hook was needed.
    3. **Each terminator family collapsed in its own commit:** accumulator (`count`/`long_count`→`emit_counter_lane`, `sum`/`min`/`max`/`average`→`emit_accumulator_lane`); `to_array`→`emit_fold_array_lane`; `last`/`single`/`aggregate`/`single_or_default`/`element_at`→`emit_early_exit_lane` (the lane gained a per-op `!supports_direct_return` state path); `first`/`first_or_default`/`any`/`all`/`contains`→`emit_early_exit_lane` via `LoopDispatch` (`FindReturn`/`FindReturnNegated` no-range fast paths, `Find`+found-state under `take`/`take_while`); `min_by`/`max_by`→`emit_terminator_lane` (Each+tail). `emit_decs_count_archsize`/`emit_decs_reverse_skip_into_tail`/`emit_decs_join_impl` stay as decs-specific adapter hooks (genuinely source-coupled).
    4. **Bundled dead `long_count` cleanup:** with all sources on the counter lane, `emit_accumulator_lane`'s `long_count` side-effect bind + `build_accumulator_spec`/`build_accumulator_perelement_stmts`'s count/long_count arms were dead; removed.
    Behavior-preserving (not byte-identical for decs): full `tests/` green on INTERP+JIT+AOT; `benchmarks/sql` decs/array/zip/xml cells within thermal noise with identical alloc profiles. The array/zip/xml seam commits (`LoopDispatch` threading) are AST byte-identical.
- [ ] **PR G4** (separate) — `linq_fold_xml.das` validates the interface (dasPUGIXML source; field access via `operator .` overloads).

### Module layout & adapter contract

The adapter is an abstract `class SourceAdapter` (`[macro_interface]`, so every method is macro-callable — no per-method `[macro_function]`). One subclass per data source carries that source's data as fields. The contract is four virtual methods:

- `bind_name(at) : string` — per-element bind name.
- `element_type() : TypeDeclPtr` — source element type.
- `wrap_source_loop(loopShape : LoopDispatch; var body; at) : Expression?` — emit the per-element iteration (array `for`, decs `for_each_archetype{,_find}`, zip lockstep, joins hash+probe). `loopShape` is the loop-framing knob consumed only by nested-callback sources (decs); direct-return sources frame an unconditional loop and ignore it.
- `wrap_invoke(var stmts; retType; wrapIter; at) : Expression?` — outer invoke binding sources as params.

Emit fns hold a `SourceAdapter?` (via `EmitCtx.src` or an `adapter` local) and call these virtually. **daslang classes have no `is`/`as` downcast** (variant-only), so source-specific data is never pulled off a base pointer by downcasting — it goes through virtual methods. Beyond the 4 dispatch methods the base also declares 6 default-null **per-operation hook methods** (`emit_loop_or_count` / `emit_reverse_skip_into_tail` / `emit_reverse_last_backward` / `emit_distinct_take_loop` / `build_group_by_adapter` / `emit_join_hook`) that the owning source overrides; the generic lane falls back to its inline (array) body when the hook returns null. (`XmlAdapter` overrides the two reverse hooks with a **backward DOM walk** — `last_child`/`previous_sibling`, both O(1) in pugixml: `emit_reverse_skip_into_tail` collects only the last N children for `reverse |> take(N)` (m5f `reverse_take` 88.9 → 0.0 ns/op), and `emit_reverse_last_backward` returns the last element in one step for a no-predicate `last()` / `reverse |> first`. Predicated `[where] |> last` stays on the forward walk — reverse DOM traversal is ~2× cache-hostile per node, profiled — and the named 3-arg `from_xml_node` form falls back to the buffer path since pugixml has no last-named-child primitive.) (`emit_join_hook` is the standalone-join dispatch: the single `join_general` pattern's thin `emit_join` routes to it, so each source supplies its own join body — array `for`+2-param invoke, decs `for_each_archetype`, XML field-pruned DOM walk — with no parallel per-source join pattern.) It also declares **capability methods** the source answers about itself — `can_group_by` / `can_join` / `can_reserve_by_length` / `has_own_loop_or_count_lane` (bool, default false) and `name_prefix` (string) — which replaced the old `kind() : AdapterKind` enum + per-site switches, so a new source only implements the methods (no central enum to extend). The `can_group_by` / `can_join` capabilities are queried from the `can_group_by_source` / `can_join_source` `RequiresPredicate`s (which thread the adapter), so the single `group_by` / `join_general` pattern admits any capable source and the adapter's `build_group_by_adapter` / `emit_join_hook` does the source/srcb-shape gating (null → tier-2). Two transitional getters remain — `arrayTop()`/`arraySrcName()` (default null/"" on base, overridden by `ArrayAdapter`); the decs-specific getters were removed in G2a so the base (and thus `linq_fold_common`) is free of `DecsAdapter`/ECS coupling. One decorator subclass lives in `linq_fold_common`: `ProjectedSourceAdapter` wraps any inner adapter to absorb a leading `_select(f)` source projection (the `srcsel` slot) — it binds `projName = f(rawElem)` atop the per-element body and delegates `wrap_source_loop`/`wrap_invoke`/`name_prefix` to the inner adapter, leaving the base no-op `arrayTop`/`arraySrcName`/`can_reserve_by_length` so source-direct fast paths (which would bypass the projection) stay disabled. This lets order/distinct splices fuse over `source |> _select(f) |> …` for any source.

**Realized module layout (post-G3d):** `linq_fold_common` (kernel + abstract base + adapter-pure generic lanes — terminator/fold-array plus the source-generic loop_or_count / counter / accumulator / early-exit lanes, with `LoopDispatch` + the per-op `!supports_direct_return` state path that lets nested-callback sources ride the early-exit lane — + `splice_patterns` + `DecsBridgeShape`/`extract_decs_bridge`) ← `linq_fold_array` (Array/Zip/ArrayJoin adapters + the zip/join emit `emit_zip`/`emit_array_join` + array row-builders) and `linq_fold_decs` (Decs/DecsJoin adapters + decs-bridge visitors + the decs dispatcher `emit_loop_or_count_lane_decs` + the decs-specific hooks `emit_decs_count_archsize`/`emit_decs_reverse_skip_into_tail`/`emit_decs_join_impl`/`emit_decs_min_max_by` — the parallel terminator scaffold is gone, decs rides the generic lanes via `DecsAdapter`); the engine `linq_fold` requires all three and holds only the dispatcher + the `LinqFold` macro + the single `register_all_linq_fold_rows`. Adding a source = a new `linq_fold_<src>.das` subclass module + one `require` + one `build_<src>_rows()` call in the engine registrar.

## Goal

Split `_fold` splice machinery into two layers:

- **Pattern recognition** — declared via a data table (`splice_patterns`). Each row names a chain shape, a list of requires-predicates, and a target emit fn.
- **Code generation** — reusable emit archetypes parameterized by a `SourceAdapter` (array / decs / decs-find, plus future zip / decs-join variants).

Today's 13 `plan_*` functions (~3300 LOC across the file) re-implement the same boilerplate: `flatten_linq → declare 8-30 tracking flags → giant for-loop with if/elif on op name → ad-hoc co-occurrence guards → giant final bail → emit dispatch`. 70-80% of plan_* code is recognition state + co-occurrence checking, not codegen. Adding a splice arm means patching 1-3 plan_* cores at 5-8 sites each, hunting through if/elif walls.

End state: adding an arm = adding a row to `splice_patterns`. Coverage gaps surface as missing rows, not as negative bails buried at the bottom of a function.

Estimated savings: **~-1750 LOC** across the 4 PRs (~-25% of the file).

## Today's situation (input to the refactor)

13 `plan_*` functions cover all splice cases. From the census:

| Plan | LOC | Anchor | Emit shapes |
|---|---|---|---|
| plan_order_family | 543 | `order*` | 11 (3 archetypes × variants) |
| plan_loop_or_count | 208 | terminator dispatch | 6 (already lane-factored) |
| plan_reverse | 284 | `reverse` | 5 |
| plan_distinct | 223 | `distinct*` | 1 archetype × 7 returns |
| plan_group_by_core | 364 | (pre-stripped contract) | 8 + 12-arm reducer table |
| plan_group_by | 62 | `group_by_lazy` | delegate |
| plan_decs_unroll | 61 | decs + terminator | 7 (already dispatcher) |
| plan_decs_group_by | 103 | `group_by_lazy` + decs | delegate |
| plan_decs_order_family | 384 | `order*` + decs | ~12 (near-mirror of array sibling) |
| plan_decs_reverse | 288 | `reverse` + decs | 4 (near-mirror) |
| plan_decs_distinct | 254 | `distinct*` + decs | 1 × 4 × 2 (near-mirror) |
| plan_decs_join | 189 | `join` + 2 decs bridges | 1 × 4 (no array sibling) |
| plan_zip | 352 | `zip` | 8 |

Decs-side plans are near-mirrors of their array-side siblings modulo source-loop wrap. `GroupBySourceAdapter` (existing) is the proof-case: `plan_group_by_core` is fully source-agnostic, with the array-side and decs-side wrappers each ~60-100 LOC.

## Grammar kernel (lives inline at top of linq_fold.das)

Visibility follows the rule in **Tests / exports philosophy** below: default `private`, public only what walker tests must name (`Slot` / `SlotMatcher` / `SlotCardinality` + their `m_*`/`c_*` constructors, `SplicePattern`, `Captures` / `EmitCtx` / `EmitFn` typedefs, `chain_prefix_of`, `check_pattern_table_reachable`, `alias_table`). The snippet below omits the `private` keyword for readability; the implementation in `linq_fold.das` carries it on everything not in that public list.

```das
variant SourceAdapter {
    Array : tuple<Expression?; string>            // (top, srcName) — PR A
    Decs  : tuple<DecsBridgeShape?; string>       // (bridge, tupName) — PR C
    // PR D widens: Zip(...), DecsJoin(...)
}

variant SlotMatcher {
    literal      : string                       // exact name match
    one_of       : array<string>                // any-of name set
    alias        : string                       // named group looked up in alias_table
}

variant SlotCardinality {
    one          : void?                        // required, exactly 1
    optional     : void?                        // 0 or 1
    chain        : void?                        // 0 or more (greedy); captures as array<ExprCall?> via Captures.many — PR B1
}

struct Slot {
    matcher      : SlotMatcher
    cardinality  : SlotCardinality
    capture_name : string = ""                  // "" = don't capture
    arity        : int = -1                     // -1 = any; positive = require N args
}

// PR B1 — Captures is a wrapper struct: `single` for c_one/c_opt slots, `many` for c_chain slots.
// PR B2 — `single_name` parallels `single`, stores the LinqCall.name at capture time. Load-bearing for
// plan_order_family where `normalize_order_reverse` mutates LinqCall.name without rewriting ExprCall.func.
struct Captures {
    single      : table<string; ExprCall?>
    single_name : table<string; string>
    many        : table<string; array<ExprCall?>>
}

variant MatchResult {
    no_match : void?                            // daslang-idiomatic Option<Captures>
    matched  : Captures
}

typedef RequiresPredicate = function<(var c : Captures; var top : Expression?; src : SourceAdapter?) : bool>

// Fold-time context passed to every emit archetype. Carries the peeled source expression, source adapter,
// and the outer `_fold(...)` expression's iterator-ness (drives `buffer_return` wrap).
struct EmitCtx {
    top              : Expression?              // peel_each'd; stubs pre-clone per invoke
    src              : SourceAdapter
    expr_is_iterator : bool
}

typedef EmitFn = function<(var c : Captures; var ctx : EmitCtx; at : LineInfo) : Expression?>

struct SplicePattern {
    name     : string                           // for debug / lint diagnostics
    chain    : array<Slot>
    requires : array<RequiresPredicate>         // all must hold
    emit     : EmitFn
}

var plan_reverse_patterns : array<SplicePattern>
var plan_distinct_patterns : array<SplicePattern>
var plan_loop_or_count_patterns : array<SplicePattern>   // PR B1
var splice_patterns : array<SplicePattern>     // PR D: collapsed from per-plan tables; first match wins
```

Predicates and emit archetypes are NAMED module-level `def` functions wrapped at use sites with `@@<RequiresPredicate>` / `@@<EmitFn>` (anonymous `@@(...)` lambdas produce `_localfunction_*` symbols that the LLVM JIT pass can't resolve — named functions take a stable address).

### Walker contract

```das
def match_pattern(p : SplicePattern;
                  var calls : array<tuple<ExprCall?; LinqCall?>>;
                  var top : Expression?) : MatchResult
```

Walks `calls` left-to-right. For each slot:

- `one` — current call must match (name + arity if specified); both cursors advance.
- `optional` — if current call matches, both cursors advance; otherwise the slot is skipped without consuming.
- `chain` (PR B1) — greedy match-while-in-set. Captured as `array<ExprCall?>` into `captures.many[capture_name]`. Always succeeds (0+); empty match still creates the `many` entry so emit fns can rely on `c.many |> key_exists("…")`. Pairs with `m_one_of` via the `slot_chain_of(names, cap)` convenience constructor.

After all slots, no unconsumed calls remain. If any of the above fails → `MatchResult(no_match = null)`.

Then each `RequiresPredicate` in `p.requires` is evaluated against the populated `Captures` and the peeled `top`. All must return true. If any fails → `MatchResult(no_match = null)`.

Returns `MatchResult(matched <- captures)` on full success (move semantics — `Captures` is a table). Caller binds `var r <- match_pattern(...)` and reads via `if (r is matched) { let c & = r as matched; … }`.

### Alias table (named op-name groups)

The snippet below is the projected end-state at PR D. The authoritative live list is the `alias_table` literal in [daslib/linq_fold.das](linq_fold.das). Status reflects what's populated through PR B1.

```das
// projected end-state at PR D
var alias_table : table<string; array<string>> <- {
    "order_family"               => ["order", "order_descending", "order_by", "order_by_descending"],  // PR B1 ✓
    "distinct_family"            => ["distinct", "distinct_by"],                                       // PR A ✓
    "first_family"               => ["first", "first_or_default"],                                     // PR A ✓
    "count_family"               => ["count", "long_count"],                                           // PR A ✓
    "accum_family"               => ["sum", "min", "max", "average", "aggregate",                      // PR B1 ✓
                                     "min_by", "max_by", "min_max", "min_max_by",
                                     "min_max_average", "min_max_average_by", "long_count"],
    "early_exit_family"          => ["any", "all", "contains", "first", "first_or_default"],           // PR B1 ✓
    "range_op_family"            => ["skip", "skip_while", "take_while", "take"],                      // PR B1 ✓
    "loop_terminator_family"     => union of count + accum + early_exit + last/single/element_at,      // PR B1 ✓ (loop_or_count terminator slot)
    "distinct_terminator_family" => ["count", "long_count", "sum"]                                     // PR A ✓ — narrow to terminators emit_hashtable_dedup actually handles
}
```

### Predicate library

Module-level named `RequiresPredicate` constants for reuse across patterns. As with `alias_table`, this table shows the projected end-state — see [daslib/linq_fold.das](linq_fold.das) for what's actually defined today (PR A: `array_source`, `take_arg_is_int`, `no_terminator`).

| Name | Status | Meaning |
|---|---|---|
| `array_source` | PR A ✓ | `top._type.isGoodArrayType \|\| top._type.isArray` (after `peel_each`) |
| `array_random_access` | planned | `array_source && top._type.isGoodArrayType` |
| `decs_source` | planned | `extract_decs_bridge(top) != null` |
| `inline_cmp_available` | PR B1 ✓ | `try_make_inline_cmp` succeeds on `c.single["order"]` (only `order_by[_descending]` with inline-splice-able key). Hard-wired to `"order"` capture key; promote to factory on second use |
| `has_where_or_distinct` | PR B1 ✓ | `c.single \|> key_exists("where") \|\| c.single \|> key_exists("distinct")`. For `order_fused_prefilter` row to distinguish from bare `buffer_helper_dispatch` |
| `take_arg_is_int` | PR A ✓ | captured `take`'s 2nd arg `_type.baseType == Type.tInt`; vacuous if no `take` slot |
| `arity_eq(cap, n)` / `arity_ge(cap, n)` | planned (factory) | structural checks on captured calls |
| `no_terminator` | PR A ✓ | no terminator captured (final optional slot empty); return shape decided by `ctx.expr_is_iterator` in the emit fn |
| `is_primitive_key(cap, argIdx)` | planned (factory) | `is_primitive_join_key_type` on captured arg |

PR A's `take_arg_is_int` is hard-wired to the `"take"` capture key (not a factory) because every PR A consumer uses that name. Promote to a `make_arity_eq(cap, n)` / `make_arg_type_is(cap, idx, type)` factory shape on second use, per the masterplan rule of thumb.

Inline closures (`@@(c, top) => …`) acceptable for one-off pattern-specific checks. **Rule of thumb:** promote to named predicate (or factory) on second use.

## Migration phases

| PR | Phase | Scope | Status |
|---|---|---|---|
| **A** | 0 — Foundation | Kernel types + walker + alias_table + predicate library + per-archetype unit tests. `splice_patterns` empty initially (safe state — all cascades unchanged). | complete |
| **A** | 1 — First migrations | `plan_reverse` (5 rows: Ra/Rb/R6/R-2a/R1-R4), `plan_distinct` (2 rows + return-shape switch in emit). Archetypes: `emit_counter_array`, `emit_walk_overwrite_scalar`, `emit_backward_walk`, `emit_buffer_reverse_inplace`, `emit_hashtable_dedup`. **Hard-delete imperative bodies.** | complete |
| **B1** | 2a — Array core (`plan_loop_or_count`) | `c_chain` cardinality + `Captures` wrapper struct (`single` / `many`) + `slot_chain_of(names, cap)` constructor. `collapse_chained_wheres` pre-pass (KR-1 fix). `plan_loop_or_count` migration (1 row + lane dispatch — preserves existing factoring; head c_chain matches `["where_", "select"]` greedy). | complete |
| **B2** | 2b — Array core (`plan_order_family`) | `plan_order_family` (5 rows: streaming-min / bounded-heap / order_then_plain_distinct / fused-prefilter / buffer-helper-dispatch). 4 archetypes: `emit_streaming_min`, `emit_bounded_heap`, `emit_fused_prefilter` (reused by row 5), `emit_buffer_helper_dispatch`. Captures gains `single_name` parallel-table to preserve `normalize_order_reverse` LinqCall.name swap. **Hard-delete imperative body.** | complete |
| **C** | 3 — SourceAdapter + decs mirrors | Widen `SourceAdapter` to `Array \| Decs`. Add adapter helpers `adapter_bind_name` (it/decs_tup), `adapter_element_type`, `adapter_wrap_source_loop` (Array for-loop vs Decs `for_each_archetype + build_decs_inner_for_pruned`), `adapter_wrap_invoke` (Array source-arg-invoke vs Decs zero-arg-invoke + optional outer `.to_sequence_move()` wrap). Refactor 7 emit fns to consume adapter; `emit_buffer_helper_dispatch` stays Array-only via `array_source` predicate on its row. `emit_loop_or_count_lane` gains a Decs-arm (`emit_loop_or_count_lane_decs`) that reconstructs a calls array from captures + dispatches to existing `emit_decs_*` lane fns (1st-order adapter per D1). Migrate 4 decs planners to thin pattern-table stubs reusing array-side rows; hard-delete ~970 LOC of imperative decs bodies. Preserve PR #2834's `reverse \|> take(N)` skip-into-tail fast path via dedicated `emit_decs_reverse_skip_into_tail` helper. | complete |
| **D1** | 4a — Group-by | `plan_group_by` + `plan_decs_group_by` → 2 pattern rows (`group_by_array`, `group_by_decs`) + 1 shared emit fn `emit_group_by` delegating to existing `plan_group_by_core` (unchanged sub-codegen). New `order_by_family` alias + `decs_join_invariants` predicate. Partial GroupBy↔SourceAdapter reconciliation via `to_source_adapter()` projection; `adapter_emit_source_loop` + `adapter_finalize_emission` collapsed to delegate array + plain-decs branches to PR C's `adapter_wrap_source_loop` / `adapter_wrap_invoke`. `emit_reducer_branches` 13-arm if/elif → `reducer_emitters` table of named `mk_reducer_*` fns + shared `mk_strictly_preferred` workhorse helper. **Hard-delete imperative bodies of both planners.** | complete |
| **D2** | 4b — Special cases | `plan_zip` (1 row, `SourceAdapter::Zip`). `plan_decs_join` (1 row, `SourceAdapter::DecsJoin`). `plan_decs_join` thin stub gains `collapse_chained_wheres` pre-pass. SourceAdapter widens to 4 variants. **Hard-delete imperative bodies of both planners + `finalize_zip_invoke` helper.** | complete (partial — GroupBy melt deferred to PR D3) |
| **D3** | 4c — GroupBy cleanup | Full GroupBySourceAdapter unification: deleted struct + 3 helpers (`to_source_adapter`, `adapter_emit_source_loop`, `adapter_finalize_emission`); `plan_group_by_core` takes `SourceAdapter` directly with inline prefix ternary; `emit_group_by` builds `SourceAdapter::DecsJoin` directly. ~−105 LOC. PR D2's C3 regression root cause was unrelated to typer / variant semantics: `resBindName := qn(...)` in a named-arg struct constructor silently dropped the field. Operator `:=` is invalid in named-arg position (no compile error fires — the field stays default-initialized at runtime). Changing to `=` fixed it. | complete |

**Net at end: ~-1750 LOC.**

## Migration mechanics

During migration, today's planner cascade at lines 6302-6337 stays unchanged. Each migrated `plan_X` function body becomes a 3-line stub:

```das
def private plan_reverse(var expr : Expression?) : Expression? {
    for (p in plan_reverse_patterns) {  // subset filtered by owner_plan_id
        let captures = match_pattern(p, calls, top)
        if (captures != null) return p.emit(captures, top, source_adapter, at)
    }
    return null
}
```

After PR D, collapse all stubs + cascade into one flat walk over `splice_patterns` (no `owner_plan_id` filtering needed once all are rows). **Shipped as PR E** via `try_splice_patterns` in `daslib/linq_fold.das`; the per-row `array_source` / `decs_source` predicates carry the adapter discrimination today.

## What we KEEP from today's code

All shared helpers stay as building blocks for emit archetypes:

- `flatten_linq`, `peel_each`, `extract_decs_bridge`
- `normalize_order_reverse`, `collapse_chained_selects`
- `peel_lambda_rename_var`, `peel_lambda_replace_var`, `peel_lambda_rename_2vars`
- `merge_where_cond`, `wrap_with_condition`, `wrap_with_ranges`
- `try_make_inline_cmp`, `make_inline_less_call`
- `buffer_return`, `finalize_emission_stmts`, `finalize_decs_emission`
- `qn`, `clone_expression`, `clone_type`, `strip_const_ref`
- `has_sideeffects`, `type_has_length`, `classify_terminator`
- `plan_group_by_core` (stays as sub-codegen; only its wrapper plans migrate)
- `GroupBySourceAdapter` (PR D folds it into the new `SourceAdapter`)

## Tests / exports philosophy

Default private; promote ONLY what a synthetic-input test must name. PR A's actual public surface is narrow:

- Slot construction: `Slot`, `SlotMatcher`, `SlotCardinality`, `m_literal`, `m_alias`, `c_one`, `c_opt`, `c_chain`, `slot_chain_of`
- Pattern row: `SplicePattern`
- Struct/typedefs used in test fn signatures: `Captures` (struct), `EmitCtx`, `EmitFn`
- Lint helpers tests assert on: `chain_prefix_of`, `check_pattern_table_reachable`
- `alias_table` (so tests can read which aliases populated)

Everything else stays private: the walker (`match_pattern`), the per-plan tables, the predicate library, all `emit_*` archetypes, all populate fns, `SourceAdapter` / `MatchResult` / `RequiresPredicate` / `LinqCall`. They're only called inside this module — bare names compose without cross-module visibility.

Per-archetype unit testing via direct calls is impractical anyway: emit fns are `[macro_function]` whose bodies contain `quote()` nodes the runtime can't lower (LLVM JIT bail). End-to-end behavioral tests carry the per-archetype coverage in `tests/linq/test_linq_fold_*` (each user chain exercises one or more archetypes through the splice).

## Naming (decided)

| Name | Role |
|---|---|
| `splice_patterns` | The master table — `array<SplicePattern>` |
| `SplicePattern` | Per-row struct |
| `Slot` | Chain slot |
| `SlotMatcher`, `SlotCardinality` | Variant types |
| `Captures` | Struct `{ single : table<string;ExprCall?>; single_name : table<string;string>; many : table<string;array<ExprCall?>> }`. `single` for c_one/c_opt slots, `many` for c_chain slots. `single_name` (PR B2) parallels `single` and records the post-normalize LinqCall.name — load-bearing for plan_order_family where `normalize_order_reverse` swaps the LinqCall name without rewriting ExprCall.func. The `LinqCall` record itself is accessible separately via `linqCalls[c.single_name[…]]` |
| `MatchResult` | Variant `no_match : void? \| matched : Captures` — walker return type |
| `c_chain` / `slot_chain_of(names, cap)` | Greedy run cardinality + (matcher = m_one_of(names), cardinality = c_chain()) convenience constructor — PR B1 |
| `RequiresPredicate`, `EmitFn` | Function-typedef types — see kernel snippet for current signatures |
| `EmitCtx` | Struct `{ top; src; expr_is_iterator }` passed to every emit archetype |
| `SourceAdapter` | Source-loop abstraction variant |
| `alias_table` | Named op-name groups |
| `match_pattern(...)` | Walker function |
| `plan_<X>_patterns` | Per-plan filtered subset (only during migration; deleted in PR E) |
| `try_splice_patterns(...)` | Unified dispatcher (added in PR E) — walks `splice_patterns` twice (Decs adapter if bridge present, then Array) |
| `register_<family>_rows()` | `[_macro]` helper that emplaces a family's rows into `splice_patterns` at compile time (PR E renamed from `populate_plan_<X>_patterns`) |

## PR B1 (shipped) + PR B2 (planned) sketch

### PR B1 — shipped

**Branch:** `bbatkin/linq-fold-pattern-table-prb`

**Scope (delivered):** KR-1 closure (`collapse_chained_wheres`) + `c_chain` cardinality + `Captures` wrapper struct (`single` / `many`) + `slot_chain_of(names, cap)` constructor + `plan_loop_or_count` migration (1 row, replaces 210 LOC imperative). PR A's 6 emit fns + 5 predicates mechanically migrated to `c.single[…]` (~47 sites).

### PR B2 — shipped

**Scope (delivered):** `plan_order_family` migration (5 rows, 4 emit archetypes) — imperative ~543 LOC body deleted. `Captures.single_name` parallel-table extension preserves the post-`normalize_order_reverse` LinqCall.name so emit fns see the swap (the imperative read `cll._1.name` directly; the pattern walker captures `cll._0` whose `.func.name` reflects source, not the swap). `extract_order_captures` helper centralizes state extraction across all 4 archetypes. Row 5 (`order_then_plain_distinct`) reuses `emit_fused_prefilter` with the `distinct_after` capture key.

### PR C — shipped

**Branch:** `bbatkin/linq-fold-pattern-table-prc`

**Scope (delivered):** SourceAdapter widening + 4 decs planner migrations.

**Kernel changes:**
- `SourceAdapter` gained `Decs : tuple<DecsBridgeShape?; string>` variant alongside `Array`.
- 4 new adapter helpers: `adapter_bind_name` (returns `qn("it", at)` for Array, `qn("decs_tup", at)` for Decs — drives `build_decs_inner_for_pruned` pruner pattern matching); `adapter_element_type` (returns cloned source element type from `top._type.firstType` or `bridge.elementType`); `adapter_wrap_source_loop` (Array `for(bindName in srcName) { body }` vs Decs `for_each_archetype(...) { build_decs_inner_for_pruned(bridge, tupName, body) }`); `adapter_wrap_invoke` (Array `invoke($(src):type){body}, $e(top)` vs Decs zero-arg `invoke($():retType{body})` + optional outer `.to_sequence_move()` for iter wrap).
- New predicate `decs_source` (extract_decs_bridge != null) for rows that should only match decs adapter.

**7 emit fns refactored to consume adapter (zero behavior change on Array side):**
- PR A: `emit_reverse_counter`, `emit_reverse_walk_overwrite_scalar`, `emit_reverse_buffer_inplace`, `emit_hashtable_dedup`
- PR B2: `emit_streaming_min`, `emit_bounded_heap`, `emit_fused_prefilter`
- Untouched (stay Array-only via row predicates): `emit_reverse_backward_index_walk`, `emit_reverse_backward_walk_dset_gate` (existing `array_source`), `emit_buffer_helper_dispatch` (PR C added `array_source` to its row per D4)

**`emit_hashtable_dedup` take(N) early-exit per-adapter inline** — Array uses `break` in for-loop; Decs uses `for_each_archetype_find` with bool-return to stop archetype walk (matches imperative plan_decs_distinct).

**`emit_loop_or_count_lane_decs` (~80 LOC dispatch fn):** When `ctx.src is Decs`, reconstructs a flatten_linq-shaped `calls` array from captures (head + range ops in canonical order + term), runs `extract_decs_ranges` + `compute_decs_chain_info` (the existing helpers plan_decs_unroll imperative used), classifies the terminator (separate isAccum / isEarlyExit / isMinMaxBy / isWalk / isElementAt — decs has dedicated lane fns for the last three), then dispatches to existing `emit_decs_*` lane fns unchanged. Per D1, state-hoist-above-for_each_archetype shape stays per-adapter; 4 array-side lane fns untouched.

**Decs-only fast path preserved:** `emit_decs_count_archsize` invoked as a pre-check inside `emit_loop_or_count_lane_decs` when chain is bare `count()`. PR #2834's `reverse |> take(N) |> to_array` skip-into-tail (2-pass walk: arch.size sum + for_each_archetype_find with whole-arch-skip + partial-arch skip-counter + early-exit) lifted into dedicated `emit_decs_reverse_skip_into_tail` helper that `emit_reverse_buffer_inplace` pre-checks. Preserves the 5.2× perf gain.

**4 decs planners are now thin pattern-table stubs:**
- `plan_decs_unroll` → `plan_loop_or_count_patterns`
- `plan_decs_order_family` → `plan_order_family_patterns` (Row 4 `buffer_helper_dispatch` array-only via `array_source`; decs cascades to Row 3 `fused_prefilter`)
- `plan_decs_reverse` → `plan_reverse_patterns` (backward-index rows already array-only via `array_source`)
- `plan_decs_distinct` → `plan_distinct_patterns`

Each stub runs the standard pre-passes (`normalize_order_reverse` for reverse + order_family, `collapse_chained_selects`, `collapse_chained_wheres`), then walks its plan table with a Decs adapter. **Hard-deleted ~970 LOC of imperative decs bodies; ~210 LOC of new helpers/stubs/test fixes = -681 LOC net.**

**Parity coverage:** all 198 tests in `tests/linq/test_linq_from_decs.das` pass (after updating 6 splice-shape assertions to the unified naming: `decs_buf` → `order_buf` for order paths / `` `buf `` for distinct paths, `decs_seen` → `order_seen` / `` `seen ``, etc.). Per-archetype decs test files (Step 6 in original plan) deferred — `test_linq_from_decs.das` already provides comprehensive AST-shape + behavioral coverage across all 4 planners.

**Parity GAINS via reuse** (closed for free): `take |> where` on decs unroll (`post_take_where` slot), `order |> distinct |> first` on decs (cascades to `emit_fused_prefilter`), `distinct.count(p)` parity (already mirrored). **Parity GAP deferred (D6):** `reverse |> distinct[_by]` on decs needs a parallel emit fn (decs has no random-access indexing for the backward-walk dset gate); cascades to tier-2 today.

### PR D1 — shipped

**Branch:** `bbatkin/linq-fold-pattern-table-prd1`

**Scope (delivered):** group_by family migration + partial GroupBy↔SourceAdapter reconciliation + reducer-spec data table + bundled Copilot R1 follow-up (dead `retType` cleanup).

**Kernel additions:**
- New alias `order_by_family` = `["order_by", "order_by_descending"]` — excludes bare `order` / `order_descending` (no key arg).
- New predicate `decs_join_invariants` — vacuous when no upstream_join captured; otherwise asserts empty head AND no having AND no trailing_where (v1 join-shape limits per imperative line 6011).
- New pattern table `plan_group_by_patterns` (2 rows: `group_by_array`, `group_by_decs`).
- New helper `to_source_adapter(GroupBySourceAdapter) : SourceAdapter` — projects array/decs/decs-join onto PR C's variant (decs-join → `Decs(null, "djoin_jres")` is safe at finalize: `adapter_wrap_invoke`'s Decs branch reads only the variant tag).

**GroupBy adapter helpers collapsed:**
- `adapter_emit_source_loop`: decs-join branch stays inline (~40 LOC of hash-collect + probe + result-lam bind); array + plain-decs delegate to `adapter_wrap_source_loop(to_source_adapter(adapter), body, at)`.
- `adapter_finalize_emission`: one-liner `return adapter_wrap_invoke(to_source_adapter(adapter), stmts, retType, false, at)` — array path's invoke shifts from untyped to typed (retType is unconditionally set by `plan_group_by_core`, so the inference shift is a no-op).

**Reducer-spec data table:** `emit_reducer_branches`' 13-arm if/elif on `spec.name` migrated to `reducer_emitters : table<string; ReducerEmitterFn>` lookup with 10 named `mk_reducer_*` fns (count/long_count/sum/sum_inner/min_max/min_max_inner/first/first_inner/average/average_inner). Shared `mk_strictly_preferred(workhorse, isMin, valExpr, cmpExpr)` helper collapses the 4-arm workhorse split (workhorse × isMin) between min/max bare and min/max inner_select. `mk_*` naming (vs `emit_*`) marks these as sub-codegen building blocks consumed inside `plan_group_by_core`, not pattern-table emit fns.

**`plan_group_by` + `plan_decs_group_by` migration:** shared emit fn `emit_group_by` reconstructs head calls from `c.many["head"]` (mirrors `emit_loop_or_count_lane_decs` precedent at lines 2395-2417), pulls tail captures (gb/having/groupproj/trailing_where/trailing_order/term) from `c.single`, builds GroupBySourceAdapter per source-shape (Array / Decs / Decs-with-upstream-join), and delegates to `plan_group_by_core` unchanged. Deep validations that aren't expressible as slot/predicate constraints (bridgeB extract, key-type, _type null checks) stay in the emit fn and bail to null — established cascade convention.

Both planner bodies (~165 LOC across array + decs) hard-deleted; new code (rows + emit fn + table + 10 reducer fns) adds ~250 LOC. Net delta ~+15 LOC; **wins are uniformity + lint-ability + future extensibility, not LOC**.

**Bundled cleanup:** `emit_bounded_heap` dead `retType` assignments (lines 2082/2100/2110 before refactor) deleted — return type correctly inferred from inner `return <-` statements. Follow-up to PR #2885 Copilot R1.

### PR D2 — shipped (partial)

**Branch:** `bbatkin/linq-fold-pattern-table-prd2`

**Scope (delivered):** SourceAdapter widening to 4 variants + `plan_decs_join` migration + `plan_zip` migration. Full GroupBySourceAdapter melt **deferred to PR D3** — D2 hit a subtle type-inference regression on the Theme 3 C3 chain that wasn't isolated in-session.

**Kernel additions:**
- New structs `DecsJoinShape` (9 fields: bridgeA/B, keyaLam/keybLam/resultLam, keyType/tupBType/resultType, resBindName) and `ZipShape` (6 fields: srcAExpr/srcBExpr, srcAName/srcBName, srcAType/srcBType) — payload carriers for the new variants.
- `SourceAdapter` widens from `Array | Decs` to `Array | Decs | DecsJoin | Zip`.
- `adapter_bind_name` gains DecsJoin branch (reads `resBindName` from payload). No Zip branch — emit_zip references `itA`/`itB` directly per D2-D.
- `adapter_element_type` gains DecsJoin branch (returns cloned `resultType`); defensive null Zip branch (plan_group_by_core never reaches Zip; emit_zip computes element type inline).
- `adapter_wrap_source_loop` gains DecsJoin branch (lifts PR D1's GroupBy hash-collect + probe + per-pair result-lam bind body verbatim — see deferred PR D3 for why this branch has no consumers yet) and Zip branch (plain `for(itA, itB in srcA, srcB) { body }`).
- `adapter_wrap_invoke` Decs branch widens to `Decs || DecsJoin` (zero-arg invoke); new Zip branch (2-source invoke with `force_at` + `force_generated` + `can_shadow` flag set — folds in today's `finalize_zip_invoke`).

**`plan_decs_join` migration:** 1 pattern row `decs_join_general` (chain: join c_one arity=5 → trailing_where c_opt arity=2 → trailing_select c_opt → term=count c_opt arity=1). New predicate `decs_join_no_select_with_count` matches pre-PR-D2 pop discipline (terminator=count suppresses select pop, so `join+select+count` would silently drop select side effects — bail to tier-2). `emit_decs_join` shared emit fn keeps inline collect+probe per D2-A (does NOT route through `adapter_wrap_source_loop(DecsJoin, ...)`) — the count-no-where bucket-length fast path collapses to per-pair iteration in the helper, and PR #2741's perf baseline depends on it. Thin stub gains `collapse_chained_wheres` pre-pass (today missing — unblocks multi-where parity). Deep checks (bridgeB extract, primitive key, _type nulls, iterator-typed implicit-to-array via `ctx.expr_is_iterator`) live in emit fn and bail to null per established convention. Buffer/return element type derived from `selectCall._type.firstType` when select is trailing, else `resultLam._type.firstType`; retType built explicitly as `array<resultType>` via `TypeDecl(baseType=tArray, firstType=...)` since emit fns don't have access to the full `expr._type` the imperative used. Hard-delete ~190 LOC imperative body; +27 LOC net.

**`plan_zip` migration:** 1 pattern row `zip_general` (chain: zip c_one → head c_chain[where_,select] → skip c_opt arity=2 → skip_while c_opt arity=2 → take_while c_opt arity=2 → take c_opt arity=2 → trailing_reverse c_opt → term c_opt loop_terminator_family). 3 new predicates: `zip_arity_2_or_3`, `zip_no_while_after_select` (closes today's chain-walk bail when select precedes skip_while/take_while in head — peel-against-tuple would dangle), `zip_reverse_compatible_with_term` (reverse + first/first_or_default picks a different element — bail). Positional slot ordering structurally enforces canonical chain order; emit_zip's head walk bails on where-after-select (chained-bind dedup gap mirrored from imperative). Counter + array lanes built inline; accumulator + early-exit lanes continue delegating to `emit_accumulator_lane` / `emit_early_exit_lane` (parallel-array form — already reused). Length-shortcut + trailing reverse_inplace path go through `adapter_wrap_invoke(Zip, ...)`. `finalize_zip_invoke` helper deleted (folded into adapter_wrap_invoke Zip branch). Stub also gains `collapse_chained_wheres` pre-pass (unblocks multi-where parity). Hard-delete ~350 LOC imperative body; +10 LOC net.

**Net delta:** ~+140 LOC (Commit 1 widening adds variants + helper branches; Commit 2 + 3 migrations are roughly LOC-neutral after deletes). Wins are uniformity + future extensibility, not LOC — same story as PR D1.

### PR E — shipped

**Branch:** `bbatkin/linq-fold-pattern-table-pre`

**Scope (delivered):** Final collapse promised at [linq_fold.md:212](daslib/linq_fold.md#L212). 7 per-plan tables → 1 `splice_patterns`; 12 stub fns + 12-line `LinqFold.visit` cascade → 1 `try_splice_patterns` dispatcher.

**Commit 1 — table consolidation:**
- Delete the 7 `var private plan_<X>_patterns : array<SplicePattern>` declarations; keep `splice_patterns` (previously declared empty since PR A).
- Rename `populate_plan_<X>_patterns` → `register_<X>_rows` (7 fns). Each is still `[_macro]` with the `is_compiling_macros_in_module("linq_fold")` guard, but the redundant `!empty(<table>)` per-table guard is dropped (would have conflicted on the shared table). The `is_compiling_macros_in_module` check is sufficient because [_macro] fns fire once per module-compile cycle.
- Per-identifier `replace_all`: each `plan_<X>_patterns` → `splice_patterns` (var decls + emplace sites + stub for-loops + comments). 12 stubs still walk the merged table — same loop body, different identifier.
- Order in `splice_patterns` = file order of populate fns: order_family (5 rows) → loop_or_count (1) → reverse (5) → distinct (2) → group_by (2) → decs_join (1) → zip (1) = 17 rows total.

Cross-family shadow analysis: each family's chain shape contains an op the broader `loop_or_count_general` pattern cannot consume (reverse / distinct / group_by_lazy / zip / join), so its `c_chain` head captures 0 ops and the remaining unconsumed family-specific call triggers `no_match`. Order within `splice_patterns` doesn't change which row wins for any in-tree chain.

**Commit 2 — dispatcher collapse:**
- New `[macro_function] def private try_splice_patterns(var expr : Expression?) : Expression?`: flatten + pre-pass once, walk `splice_patterns` with Decs adapter (only if `extract_decs_bridge(top) != null`) then with Array adapter (after `peel_each(top)`).
- Per-row predicates (`array_source` / `decs_source` / `decs_join_invariants`) cull rows that don't apply to the current adapter — no `target_adapter` field needed on `SplicePattern`. Both passes walk the same 17-row table top to bottom; first emit non-null wins.
- Delete 12 plan_* stubs (~280 LOC); rewrite `LinqFold.visit` from 12-line cascade into one `try_splice_patterns` call (+ unchanged Theme 6 perf-warn / tier-2 / tier-3 fall-through).

**Architectural decisions baked in:**
- **Pre-passes always run.** Today only 4 stubs called `normalize_order_reverse`; 2 group_by stubs called nothing. The pre-passes are no-ops on chains without their target shapes, so unifying is safe. Group_by chains now pass through `collapse_chained_*` / `normalize_order_reverse` for the first time — confirmed semantically equivalent by full test pass.
- **EmitCtx.top normalized.** Decs ctx always `top = null` (matched 5 of 6 decs stubs; the lone holdout `plan_decs_join` set `top = top` but `emit_decs_join` doesn't read it). Array ctx always `top = clone(top)` per match (matched 4 of 6 array stubs; `plan_group_by` + `plan_zip` previously passed raw `top` — the extra clone is observationally invisible).
- **Behavior delta vs per-family-cascade today.** Today the cascade is `decs_<family> ALL ROWS → array_<family> ALL ROWS → next family`. PR E walks `Decs ALL FAMILIES → Array ALL FAMILIES`. The two diverge only if a chain matches both family A's row AND family B's row in different adapters — impossible in practice because chain shapes are family-disjoint post-`normalize_order_reverse`. Full test pass confirms zero observable difference.

**Net delta:** −281 / +26 LOC across the two refactor commits (−255 net). Wins are uniformity, single-place-to-add-a-row, and inspectability — same headline as PRs D1/D2/D3.

**Verification:** 1689/1689 in `tests/linq` pass; full daslang suite at 9809/9815 (same skip baseline as master, no new failures).

### Pre-pass (PR B1 ✓)

- `collapse_chained_wheres(calls)` — mirrors `collapse_chained_selects` modulo composition: clone inner where lambda, rename param to fresh name, build composed body via `merge_where_cond(innerBodyFresh, outerBodyFresh)` (which is `inner && outer`), rewire chain backlink, erase inner from `calls`. **No `has_sideeffects` bail** — composition uses cloned ASTs and AND-merge preserves left-to-right evaluation order with short-circuit semantics identical to the imperative cascade. Called from `plan_reverse`, `plan_distinct`, `plan_loop_or_count` stubs. **KR-1 fix; load-bearing for plan_loop_or_count row.**

### Pattern row shipped (PR B1)

**`plan_loop_or_count`** — 1 row using the new `c_chain` cardinality for the head:

```das
SplicePattern(
    name = "loop_or_count_general",
    chain = [
        slot_chain_of(["where_", "select"], "head"),                   // c_chain — 0+ contiguous where/select
        Slot(m_literal("skip"),       c_opt(), "skip"),
        Slot(m_literal("skip_while"), c_opt(), "skip_while"),
        Slot(m_literal("take_while"), c_opt(), "take_while"),
        Slot(m_literal("take"),       c_opt(), "take"),
        Slot(m_literal("where_"),     c_opt(), "post_take_where"),     // Theme 2 5c
        Slot(m_alias("loop_terminator_family"), c_opt(), "term")
    ],
    requires = [],   // intrinsic — chain shape carries the constraints
    emit = @@<EmitFn> emit_loop_or_count_lane)
```

`emit_loop_or_count_lane` walks `c.many["head"]` left-to-right applying the same where_/select arms (AND-merge, chained-select rebinding, where-after-select projection-replace) the imperative loop did. Range ops + post-take-where + terminator come from `c.single[…]`. Pre-dispatch fast paths: `emit_length_shortcut`, `emit_any_empty_shortcut`. Lane dispatch: `classify_terminator(call_norm_name(c.single["term"]))` → `emit_counter_lane` / `emit_array_lane` / `emit_accumulator_lane` / `emit_early_exit_lane`. `emit_array_lane` refactored to take `isIter : bool` directly (was `expr : Expression?` just to read `.isIterator`) so the new emit fn can pass `ctx.expr_is_iterator` cleanly.

### Predicates added (PR B1 ✓)

- `inline_cmp_available(c, top)` — `try_make_inline_cmp(c.single["order"].arguments[1], …)`. For PR B2's `order_streaming_min` + `order_bounded_heap` rows.
- `has_where_or_distinct(c, top)` — `c.single |> key_exists("where") || c.single |> key_exists("distinct")`. For PR B2's `order_fused_prefilter` row.

### PR B2 — shipped rows

**`plan_order_family`** — 5 rows, priority order 1 → 5 (priority differs slightly from initial spec — Row 5 promoted ahead of fused_prefilter and buffer_helper since its `m_literal("distinct")` slot is the most specific discriminator):

```das
// Row 1 — streaming_min: inline-cmp + first[_or_default]
SplicePattern(
    name = "order_streaming_min",
    chain = [
        Slot(m_literal("where_"),       c_opt(), "where"),
        Slot(m_alias("distinct_family"), c_opt(), "distinct"),
        Slot(m_alias("order_family"),    c_one(), "order"),
        Slot(m_alias("first_family"),    c_one(), "term"),
        Slot(m_literal("select"),        c_opt(), "termsel"),
    ],
    requires = [@@<RequiresPredicate> inline_cmp_available],
    emit = @@<EmitFn> emit_streaming_min)

// Row 2 — bounded_heap: inline-cmp + take(N)
SplicePattern(
    name = "order_bounded_heap",
    chain = [
        Slot(m_literal("where_"),       c_opt(), "where"),
        Slot(m_alias("distinct_family"), c_opt(), "distinct"),
        Slot(m_alias("order_family"),    c_one(), "order"),
        Slot(m_literal("take"),          c_one(), "take"),
        Slot(m_literal("select"),        c_opt(), "termsel"),
    ],
    requires = [@@<RequiresPredicate> inline_cmp_available, @@<RequiresPredicate> take_arg_is_int],
    emit = @@<EmitFn> emit_bounded_heap)

// Row 3 — fused_prefilter: where or distinct present, no inline-cmp shortcut
SplicePattern(
    name = "order_fused_prefilter",
    chain = [
        Slot(m_literal("where_"),       c_opt(), "where"),
        Slot(m_alias("distinct_family"), c_opt(), "distinct"),
        Slot(m_alias("order_family"),    c_one(), "order"),
        Slot(m_literal("take"),          c_opt(), "take"),
        Slot(m_alias("first_family"),    c_opt(), "term"),
        Slot(m_literal("select"),        c_opt(), "termsel"),
    ],
    requires = [@@<RequiresPredicate> has_where_or_distinct, @@<RequiresPredicate> take_arg_is_int],
    emit = @@<EmitFn> emit_fused_prefilter)

// Row 4 — buffer_helper_dispatch: bare order, direct call to daslib helpers
SplicePattern(
    name = "order_buffer_helper_dispatch",
    chain = [
        Slot(m_alias("order_family"), c_one(), "order"),
        Slot(m_literal("take"),       c_opt(), "take"),
        Slot(m_alias("first_family"), c_opt(), "term"),
    ],
    requires = [@@<RequiresPredicate> take_arg_is_int],
    emit = @@<EmitFn> emit_buffer_helper_dispatch)

// Row 5 — order_then_plain_distinct: `order + distinct (plain)` accepted by master imperative
// (whole-tuple equality is position-invariant). distinct_by AFTER order_by would NOT be safe
// (distinct_by picks an arbitrary K1 representative regardless of sort order). distinct is
// literal "distinct" only (no alias) to forbid distinct_by here.
SplicePattern(
    name = "order_then_plain_distinct",
    chain = [
        Slot(m_alias("order_family"), c_one(), "order"),
        Slot(m_literal("distinct"),   c_one(), "distinct_after"),
        Slot(m_literal("take"),       c_opt(), "take"),
        Slot(m_alias("first_family"), c_opt(), "term"),
        Slot(m_literal("select"),     c_opt(), "termsel"),
    ],
    requires = [@@<RequiresPredicate> take_arg_is_int],
    emit = @@<EmitFn> emit_fused_prefilter)   // reuses emit_fused_prefilter with distinct_after capture
```

### Emit archetypes

| Name | Source lines | LOC | Notes |
|---|---|---|---|
| `emit_streaming_min` | 1662-1727 | ~65 | Single-best state, per-element less-test |
| `emit_bounded_heap` | 1729-1855 | ~125 | Size-N heap during walk; distinct gate variant (Theme 3 Phase 3); terminal `_select` variant |
| `emit_fused_prefilter` | 1928-2107 | ~180 | Walk into buffer with where/distinct gate; sort/min/top_n on buffer; terminal `_select` variant; internal dispatch on take/first/bare |
| `emit_buffer_helper_dispatch` | 1857-1927 | ~70 | Direct call to `order` / `top_n*` / `min_max` helpers; 4 sub-paths |
| `emit_loop_or_count_lane` | 2243-2317 + recognition state | ~150 | Single row with internal `classify_terminator` dispatch into 4 existing lane emit fns |
| `emit_terminal_select_project` | NEW shared helper | ~30 | Used by `emit_bounded_heap` + `emit_fused_prefilter` for `outBuf` projection-from-`buf` |

### Co-occurrence audit (to verify during implementation)

The imperative code has a few subtle co-occurrence rules that may not map cleanly onto the pattern table:

- **`order + distinct (plain)`**: imperative `plan_order_family` accepts `distinct` (not `distinct_by`) AFTER `order_by` because whole-tuple equality is position-invariant. **Decision (2026-05-26)**: add row 5 `order_then_plain_distinct` so PR B has byte-equivalent splice coverage to master. The row's `distinct_after` slot is `m_literal("distinct")` (not the alias), structurally forbidding `distinct_by`. Emit reuses `emit_fused_prefilter` with the new capture name.
- **`select` mid-chain in plan_loop_or_count**: chained selects need `intermediateBinds` for side-effect ordering. Already handled by emit-fn-internal recognition; pattern row captures via single `select` slot post-collapse.
- **`where` after `select` in plan_loop_or_count**: imperative does `peel_lambda_replace_var(predicate, projection)` to rebind the where pred to the projection result. Critical correctness — emit fn must replicate (the recognition state in `emit_loop_or_count_lane` covers this).

### LOC budget

| Component | Delta |
|---|---|
| New: `collapse_chained_wheres` | +30 |
| New: 5 emit archetypes (lifted from imperative) | +610 |
| New: shared `emit_terminal_select_project` | +30 |
| New: 5 pattern rows | +60 |
| New: 2 populate `[_macro]` fns | +30 |
| New: 2 predicates + 5 aliases | +25 |
| Delete: imperative `plan_loop_or_count` body | -208 |
| Delete: imperative `plan_order_family` body | -543 |
| New: stubs + KR-1 wiring | +30 |
| New: tests (`collapse_chained_wheres` + per-archetype + regression) | +200 |
| **Net** | **~+264 LOC** (refactor, code redistributed; tests dominate) |

### Test plan (additions to existing per-archetype + walker integrity)

- `test_linq_fold_collapse_chained_wheres.das` — N=2, N=3 chains; side-effect bail; on plan_reverse + plan_distinct + plan_loop_or_count + plan_order_family
- `test_linq_fold_order_streaming_min.das` — inline-cmp + first / first_or_default; with/without where; with/without distinct; with terminal `_select`
- `test_linq_fold_order_bounded_heap.das` — inline-cmp + take(N); distinct gate; terminal `_select`
- `test_linq_fold_order_fused_prefilter.das` — where + order + take/first; distinct + order + take/first; terminal `_select`
- `test_linq_fold_order_buffer_helper.das` — bare order; order + take; order + first
- `test_linq_fold_loop_or_count_terminators.das` — all 4 lanes (counter / accumulator / early_exit / array); fast paths; range ops; chained wheres post-collapse

## Known regressions to address in follow-ups

| # | Surface | Symptom | Severity | Owner PR |
|---|---|---|---|---|
| KR-1 | `plan_reverse` + `plan_distinct` pattern rows allow a single optional `where_` slot; pre-PR-A imperative `plan_*` accepted N consecutive `where_` calls and `&&`-merged via `merge_where_cond`. | `..._where(p1)._where(p2).reverse()...` and `..._where(p1)._where(p2)._distinct()...` no longer spliced; fell back to cascade. | medium | **CLOSED in PR B1** — `collapse_chained_wheres` pre-pass mirroring `collapse_chained_selects` (~50 LOC + 18 sub-runs). Called from `plan_reverse` / `plan_distinct` / `plan_loop_or_count` stubs; will be called from `plan_order_family` in PR B2. |

## Risks

1. **Pattern ordering hazard.** Pattern A's chain being a strict prefix of B's means A wins. Discipline: more-specific patterns declared first; add a lint pass that walks the table at module-init time and flags prefix conflicts.
2. **Hidden cross-plan helpers.** Some emit branches consume helpers used by other plans. These stay as-is — emit archetypes call them like the imperative code did.
3. **Mid-flight `SourceAdapter` redesign.** PR C is the highest-uncertainty phase. If `wrap_per_element` doesn't generalize cleanly to `DecsFind`, we either widen the interface or special-case. **Mid-flight redesign approved.**
4. **Bench refresh per [feedback-living-results-md]:** each PR re-runs INTERP+JIT for any bench shape it touches and refreshes `results.md`. Goal is **byte-identical or strictly faster** at each phase (refactor, not perf change).
5. **RST refresh per [feedback-living-linq-fold-patterns-rst]:** each promoted arm's row in `doc/source/reference/linq_fold_patterns.rst` gets touched — phrasing changes from "plan_X handles …" to "pattern `<name>` (archetype `<emit_fn>`) handles …".

## Decision log

- **2026-05-25** — Hybrid (declared patterns table + reusable archetypes) over pure EDSL or pure data-table.
- **2026-05-25** — Inline kernel in `linq_fold.das` (not a separate module file).
- **2026-05-25** — Bundle PR A foundation with first migrations (`plan_reverse`, `plan_distinct`) — foundation-only PR is grounded in nothing.
- **2026-05-25** — Stop and align between phases; mid-flight redesign approved.
- **2026-05-25** — Hard-cutover (no legacy imperative alongside the table).
- **2026-05-25** — `Captures` as `table<string; ExprCall?>`; emit fns reach the `LinqCall` registry record on demand via `linqCalls[call_norm_name(c)]` (avoids carrying the per-call pair in every capture entry). Initial sketch carried `tuple<ExprCall?; LinqCall?>` but it bought nothing — emit fns mostly read terminator/call arg shape from the `ExprCall`, and `call_norm_name` is the canonical way to derive the registry key anyway.
- **2026-05-25** — `SourceAdapter` stub (Array-only) in PR A; widened in PR C.
- **2026-05-25** — Per-plan stubs during migration; flat walk after PR D.
- **2026-05-25** — `arity` lives on `Slot` (structural check), not requires-predicate.
- **2026-05-25** — Named predicates over inline closures by default; inline acceptable for one-off, promote on second use.
- **2026-05-25 (PR A impl)** — `let` is const, `var` is non-const. Walker stub binds `var result = invoke(p.emit, …)` to receive non-const Expression?. Lint LINT005 misreports the required reinterpret as redundant — known asymmetry; tracked via this entry instead of suppressing in code.
- **2026-05-25 (PR A impl)** — `ExprCall.name` is the mangled generic-instance name (e.g. `__::linq\`distinct_by\`<hash>`); the user-facing name lives at the root of the `func.fromGeneric` chain. Helper `call_norm_name(ExprCall?)` walks the chain and normalizes through `linqCalls`.
- **2026-05-25 (PR A impl)** — Variant construction in gen2 uses the named-field constructor form (`SlotMatcher(literal = "x")`, `MatchResult(matched <- captures)`), same syntax as struct init. Helpers `m_literal`/`m_alias`/`c_one`/`c_opt` keep pattern rows compact.
- **2026-05-25 (PR A impl)** — Empty typed array literal: `array<T>()`. The bare `[]` lacks the element-type inference base.
- **2026-05-25 (PR A impl)** — `array_source` predicate gates only patterns that need indexed access (R-2a backward-walk, R6 backward-index). Patterns using `for (it in src)` body (Ra / Rb / R1-R4 / distinct_*) work on iterator sources too and have no source-shape gate.
- **2026-05-25 (PR A impl)** — `take_arg_is_int` predicate is vacuously true when no `take` capture is present (so it's safe on patterns with optional take). Same pattern applies for any future capture-conditional predicate.
- **2026-05-25 (PR A impl)** — PR A is a pure refactor (no arm add/extend/tighten). Per `[[feedback-living-linq-fold-patterns-rst]]` and `[[feedback-living-results-md]]`, RST and bench refresh are skipped — both are arm-shape-tracking docs, not implementation-tracking docs.
- **2026-05-26 (PR A R3)** — Intentional extension over master in `plan_reverse`: chains with BOTH a pre-reverse `_select(f)` AND a post-reverse `_select(g)` now splice (R1-R4 + Rb patterns) where master's imperative code had a `!seenSelect` guard that bailed to cascade. The two selects compose cleanly — pre-projection feeds `pushExpr`, post-projection projects the reversed survivors at return. Strictly faster, semantics preserved. Covered by `test_reverse_pre_and_post_select_array` / `_first` in `test_linq_fold_terminal_select.das`.
- **2026-05-26 (PR B1)** — Split PR B into B1 (KR-1 + `c_chain` + `plan_loop_or_count`) and B2 (`plan_order_family`). The c_chain cardinality is a kernel extension that's load-bearing for plan_loop_or_count's variable-shape head (`[where_*][select*]` interleaved); without it the row would explode into N positional optional slots and still not cover everything. Bundling kernel + first user of kernel in one PR keeps the kernel grounded.
- **2026-05-26 (PR B1)** — `Captures` migrated from `typedef Captures = table<string; ExprCall?>` to `struct Captures { single : table<string;ExprCall?>; many : table<string;array<ExprCall?>> }`. Alternatives considered: (a) overload one table with sentinel encoding — ugly and type-unsafe; (b) store all captures as `array<ExprCall?>` and index `[0]` for c_one/c_opt — fixed-shape callsites pay an awkward bracket tax. The split struct is mechanical for emit fns (`c["x"]` → `c.single["x"]`, ~47 sites swept) and leaves room for future cardinality types (`c_repeat_n`, etc.) to land in their own table.
- **2026-05-26 (PR B1)** — `c_chain` walker rule: empty match still creates an entry in `captures.many[name]` (empty array). Emit fns can rely on `c.many |> key_exists("…")` instead of branching on the array's length being > 0. Mirrors how `c_opt` slots that miss still leave `c.single |> key_exists` returning false — predictable existence semantics for emit-fn reads.
- **2026-05-26 (PR B1)** — `slot_chain_of(names, cap)` convenience constructor takes `var names : array<string>` and moves it into the SlotMatcher via `<-`. `array<string>` is non-copyable; pass-by-value-and-copy would require an explicit clone. Move-consume is the more honest signature.
- **2026-05-26 (PR B1)** — `collapse_chained_wheres` does NOT gate on `has_sideeffects` (whereas `collapse_chained_selects` does for one specific case). Reason: AND-composing two `where_` predicates preserves left-to-right short-circuit semantics — `inner(x) && outer(x)` evaluates `inner` first and short-circuits, identical to the imperative `if(inner) { if(outer) { … } }` cascade. Side effects in `inner` always fire (per element); side effects in `outer` fire only when `inner` returns true. Cascade and composition match exactly.
- **2026-05-26 (PR B1)** — `loop_terminator_family` alias must include ALL terminators `classify_terminator` returns non-UNKNOWN for. First B1 cut missed `last`/`single`/`element_at` × `_or_default` (6 EARLY_EXIT terminators); matrix run caught it via `test_linq_fold_ast` "expected 1 for-loop, got 0" failures (terminator wasn't matching the alias → planner cascaded to tier-2 imperative which emits 2 loops). Single-line fix: extend the alias. Lesson: any new alias for a c_opt terminator slot needs an audit against `classify_terminator`'s domain.
- **2026-05-26 (PR B1)** — `emit_array_lane` signature refactored: `var expr : Expression?` → `isIter : bool`. The only thing the original `expr` parameter was used for was reading `expr._type.isIterator`. The new `EmitCtx.expr_is_iterator` already carries that bool, so the refactor flows cleanly. Single callsite update (imperative caller computed `expr._type != null && expr._type.isIterator` inline before the call).
- **2026-05-26 (PR B2)** — `Captures.single_name` parallel-table added. Surfaced by a test failure during the plan_order_family migration: `_order_by(_).reverse().take(3).to_array()` returned ascending top-3 instead of descending top-3. Root cause: `normalize_order_reverse` swaps `calls[i]._1.name` (the LinqCall.name) but leaves `calls[i]._0.func` (the ExprCall function pointer) unchanged. The imperative loop read `cll._1.name` and saw the swap; the new pattern walker captures `cll._0` (ExprCall), and `call_norm_name(captured)` walks `func.fromGeneric` chain back to the user-facing source name — silently undoing the swap. Fix: walker writes both `single` (ExprCall) and `single_name` (LinqCall.name, captured at match time). Emit fns that care about post-normalize names (`extract_order_captures`, `inline_cmp_available`) read `c.single_name[key]`. Other planners (PR A reverse / distinct, PR B1 loop_or_count) can continue using `call_norm_name` since they don't run a name-swap pre-pass.
- **2026-05-26 (PR B2)** — `extract_order_captures` helper centralizes captures→state extraction across all 4 emit archetypes. Each archetype starts with `var oc <- extract_order_captures(c, at, itName)` then bails on `!oc.ok` plus path-specific gates. Trade-off vs inlining: helper introduces a struct allocation per call, but emit fns are at compile-time (cost is irrelevant) and the shared extraction kills ~80 LOC of repeated capture-reading boilerplate.
- **2026-05-26 (PR B2)** — Row 5 (`order_then_plain_distinct`) reuses `emit_fused_prefilter` rather than a 5th dedicated archetype. The runtime behavior is identical: distinct gate per-element push by set-insert on the key, then sort/min/top_n on the prefilter buffer. `extract_order_captures` reads from EITHER `c.single["distinct"]` (Row 4: distinct before order) OR `c.single["distinct_after"]` (Row 5: plain distinct after order) and normalizes both into `oc.distinctName = "distinct"`. distinct_by AFTER order is structurally excluded by the m_literal("distinct") matcher in Row 5 (the position-invariant whole-tuple equality argument only holds for plain distinct).
- **2026-05-26 (PR B2)** — Pattern row priority order matters: Row 5 (order_then_plain_distinct, c_one on `m_literal("distinct")` after order) must come BEFORE Row 4 (fused_prefilter, c_opt distinct BEFORE order). Otherwise a chain like `[order, distinct, take]` would match Row 4 with no distinct captured (the c_opt distinct slot skips since "distinct" isn't a valid order_family member, then order matches, then take), routing to the no-distinct fused_prefilter path instead of the distinct-gated one. Lint helper `chain_prefix_of` doesn't catch this since neither row is a strict prefix of the other; ordering by specificity is a hand-applied discipline.
- **2026-05-26 (PR C, D1)** — 1st-order adapter (source-loop swap only), NOT 2nd-order (state-location swap). `emit_loop_or_count_lane` gains a Decs-arm at the top (`emit_loop_or_count_lane_decs`) that dispatches the terminator class to existing `emit_decs_*` lane fns. The 4 array-side lane fns (`emit_counter_lane` / `emit_array_lane` / `emit_accumulator_lane` / `emit_early_exit_lane`) stay UNTOUCHED. Why: array binds source as invoke arg, decs hoists state above for_each_archetype (zero-arg invoke) — these invoke shapes diverge fundamentally. Unifying state location would require deep surgery on PR B1 code with high risk. Pattern recognition unifies (one row table, one terminator dispatch); state-hoisting stays per-adapter. Clean separation, minimal disruption.
- **2026-05-26 (PR C, D2)** — `array_source` predicate stays semantically unchanged: reads `top._type.isGoodArrayType || top._type.isArray`. For decs sources, `top` is `from_decs_template(...)` (iterator-typed), so `array_source` correctly returns false without semantic shift. Parallel `decs_source` predicate added for rows that should only match Decs adapter.
- **2026-05-26 (PR C, D3)** — Bind name is a SourceAdapter responsibility. Every emit fn hardcoded `let itName = qn("it", at)` for lambda peeling. For decs, peeling must target `qn("decs_tup", at)` so `build_decs_inner_for_pruned`'s pruner can fire on `tupName.<field>` access patterns. New helper `adapter_bind_name(adapter, at)` returns the right name per adapter; threaded into every `peel_lambda_rename_var` / `peel_lambda_replace_var` / `merge_where_cond` callsite across the 7 refactored emit fns.
- **2026-05-26 (PR C, D4)** — Row 4 (`buffer_helper_dispatch`) on `plan_order_family_patterns` gated with `array_source` predicate. `emit_buffer_helper_dispatch` calls daslib helpers like `order(top, key)` / `top_n*(top, N)` that need a `top` expression; Decs has no `top`, so decs adapter cascades to Row 3 (`fused_prefilter`) which materializes the buffer. Matches what the imperative decs body did for these shapes anyway.
- **2026-05-26 (PR C, D5)** — Decs-only fast paths added as pre-checks. `emit_decs_count_archsize` fires as a pre-check inside `emit_loop_or_count_lane_decs` when chain is bare `count()` (no chain head, no range ops). PR #2834's `reverse |> take(N) |> to_array` skip-into-tail (2-pass walk: arch.size sum + for_each_archetype_find with whole-arch-skip + partial-arch skip-counter + early-exit) lifted into a dedicated `emit_decs_reverse_skip_into_tail` helper that `emit_reverse_buffer_inplace` pre-checks before the general buffer path. Preserves the 5.2× perf gain.
- **2026-05-26 (PR C, D6)** — `reverse |> distinct[_by]` on decs sources deferred to a future PR. The array-side R-2a row (`emit_reverse_backward_walk_dset_gate`) uses backward index walk — requires random access; decs has none. Building a parallel decs emit fn is genuine new work (not adapter swap). For now decs cascades to tier-2 for this shape (matches today's behavior — no regression). **CLOSED 2026-05-31** by `emit_reverse_distinct_forward_keeplast` (R-2b) — not a parallel decs fn but one source-generic forward emit (table-overwrite keep-last) gated `non_array_source`, covering decs / XML / iterators at once. See the changelog entry.
- **2026-05-26 (PR C impl)** — `emit_hashtable_dedup`'s take(N) early-exit case stays per-adapter inline (Array uses `break` in for-loop; Decs uses `for_each_archetype_find` returning bool to stop archetype walk). Genuine shape divergence — the array's `break` exits the inner-for; the decs `for_each_archetype_find` semantically returns bool from the archetype lambda. Inlining a per-adapter branch is cleaner than adding a `wrap_source_loop_with_stop` variant of the adapter helper. Non-take case uses `adapter_wrap_source_loop` uniformly.
- **2026-05-26 (PR C impl)** — `emit_loop_or_count_lane_decs` reconstructs a `calls`-shaped array from captures (head + range ops in canonical order + term) so existing `extract_decs_ranges` + `compute_decs_chain_info` helpers run unchanged. LinqCall records pulled from `linqCalls` registry via `addr(linqCalls?[name] ?? default<LinqCall>)` — matches `flatten_linq`'s pattern for ref→pointer conversion.
- **2026-05-26 (PR C impl)** — Step 6 (4 new per-archetype `tests/linq/test_linq_fold_decs_*.das` files) was deferred. Existing `tests/linq/test_linq_from_decs.das` carries 198 tests across all 4 decs planners (reverse / distinct / order_family / unroll) with both AST-shape splice assertions AND end-to-end behavioral coverage. The unified emit fns are themselves tested through 6 existing themes (1–8) + `test_linq_fold_order_family.das` (12 tests) on the Array side. Adding 4 new files would mostly duplicate existing coverage. Verification of decs migration parity: all 198 `test_linq_from_decs.das` tests pass after migration (6 splice-shape assertions updated to unified naming).
- **2026-05-26 (PR D1, D1-A)** — Partial GroupBy↔SourceAdapter reconciliation: keep `GroupBySourceAdapter` for the 3-mode dispatch (array / decs / decs-join); add `to_source_adapter()` projection method onto SourceAdapter for the 2 sub-helpers that mirror SourceAdapter shape. Full unification deferred to PR D2 (when `plan_zip` / `plan_decs_join` land and a `Zip` / `DecsJoin` variant joins). Rationale: `plan_group_by_core` has zero callers that would benefit from per-mode discriminator unification (initialBindName / initialElemType / namePrefix diverge per mode and are read directly), so the GroupBy struct is load-bearing. The projection unlocks reuse of `adapter_wrap_source_loop` / `adapter_wrap_invoke` without restructuring.
- **2026-05-26 (PR D1, D1-B)** — `adapter_emit_source_loop` keeps the decs-join inline shape (~40 LOC of hash-collect + probe + result-lam bind — GroupBy-specific, no SourceAdapter variant exists for it). Array + plain-decs delegate to `adapter_wrap_source_loop(to_source_adapter(adapter), body, at)`. `adapter_finalize_emission` collapses to a one-liner `adapter_wrap_invoke(to_source_adapter(adapter), stmts, retType, false, at)`. Array path's invoke shifts from untyped to typed; safe because `plan_group_by_core` sets retType unconditionally (lines 4355 / 4398 / 4401), so inference shift is a no-op. decs-join → `Decs(null, "djoin_jres")` is safe at finalize: `adapter_wrap_invoke`'s Decs branch reads only the variant tag, never the tuple fields.
- **2026-05-26 (PR D1, D1-C)** — `plan_group_by` + `plan_decs_group_by` migrated to 2 pattern rows + 1 shared emit fn. Pattern row uses `Slot.arity` for args-length guards (count=1, join=5, group_by_lazy=2, order_by[_descending]=2, where_=2) rather than predicates — moves the 4 imperative `length(args) == N` checks from emit-fn-land to slot-land. New alias `order_by_family` excludes bare `order`/`order_descending` (no key arg, per imperative line 4480). New predicate `decs_join_invariants` enforces v1 join-shape limits (head empty, no having, no trailing_where when upstream_join captured). Decs-join fits as a SINGLE row with optional `upstream_join` slot, not 2 separate rows.
- **2026-05-26 (PR D1, D1-D)** — `emit_reducer_branches` 13-arm if/elif → `reducer_emitters` lookup table + 10 named `mk_reducer_*` fns. Naming: `mk_*` (not `emit_*`) signals "sub-codegen building block consumed by plan_group_by_core, not pattern-table emit fn" — matches existing `mk_slot_ref` / `mk_avg_acc_type` / `mk_avg_subfield` neighborhood. Shared `mk_strictly_preferred(workhorse, isMin, valExpr, cmpExpr)` helper collapses the 4-arm workhorse split (workhorse × isMin) between `mk_reducer_min_max` and `mk_reducer_min_max_inner`. LOC delta is ~+25 net (helper savings less than agent's ~30 LOC estimate); real win is uniformity + lint-ability + future 3rd-party extensibility.
- **2026-05-26 (PR D1, D1-E)** — Bundled PR #2885 Copilot R1 follow-up: dead `retType` assignments in `emit_bounded_heap` (3 LOC) dropped. Return type correctly inferred from inner `return <-` statements; `null` was already being passed to `adapter_wrap_invoke`. Lands as Commit 1 of PR D1 for clean separation from the main refactor work.
- **2026-05-26 (PR D1 impl)** — qmacro_expr / qmacro_block bodies cannot be one-liners — `qmacro_expr() { stmt }` errors on the closing `}` because the inner statement lacks a `;` or newline before brace close. Required form is multi-line `qmacro_expr() { \n    stmt \n }`. Surfaces during `mk_reducer_*` extraction; affected ~10 callsites.
- **2026-05-26 (PR D1 impl)** — Variant-as access (`(ctx.src as Decs)._0`) and table-bracket reads (`c.single["upstream_join"]`) return const values even when the outer var (`ctx`, `c`) is mutable. `let` binding preserves const; `var` binding copies into a fresh mutable local (pointer types: copies the pointer; struct types: copies the struct). For struct field initialization with non-const field types (`decsBridge : DecsBridgeShape?`), const sources fail with `can't copy constant to non-constant pointer`. Fix: use `var` in `emit_group_by` for all variant-arm / table-bracket extractions. Matches imperative precedent (e.g. line 6012 `var bridgeB = extract_decs_bridge(joinCall.arguments[1])` where joinCall was declared `var joinCall = calls.back()._0`).
- **2026-05-26 (PR D2, D2-A)** — DecsJoin variant carries payload but `emit_decs_join` keeps collect+probe inline (does NOT route through `adapter_wrap_source_loop(DecsJoin, ...)`). Reason: the count-no-where bucket-length-sum fast path emits at bucket granularity (`cnt += length(arr)` inside the `get(hash, keya, $(var arr){...})` callback), never entering the per-pair `for(bElem in arr)` loop. The adapter helper unconditionally emits the per-pair shape (GroupBy's only consumer always wants per-pair iteration), so routing through it would collapse the fast path — PR #2741 perf regression hazard. Resolution: the variant exists for typed payload carriage shared between GroupBy decs-join + emit_decs_join, but `adapter_wrap_source_loop(DecsJoin, ...)` is invoked ONLY by GroupBy.
- **2026-05-26 (PR D2, D2-B)** — Two new requires predicates plug row-admission gaps. (1) `zip_no_while_after_select`: closes today's chain-walk bail (plan_zip lines 6494/6500) when select precedes skip_while/take_while — the while-lambda peels against the zip tuple `it`, but an upstream select rebinds the element type, leaving the predicate dangling. (2) `zip_reverse_compatible_with_term`: closes today's post-walk bail (line 6644) — trailing reverse is identity for count/sum/min/max/average/any/all/contains but picks a different element for first/first_or_default. The iterator-typed implicit-to-array check in plan_decs_join (`expr._type.isGoodArrayType`) does NOT become a third predicate; pushed inside emit_decs_join (reads `ctx.expr_is_iterator`) since RequiresPredicate signature has no ctx access.
- **2026-05-26 (PR D2, D2-D)** — Zip variant's `adapter_wrap_source_loop` emits plain `for (itA, itB in srcA, srcB) { body }`; the optional `let it = (itA, itB)` bind is prepended by emit_zip inside `body` based on `needsItBind`. Only emit_zip knows whether the body references `it` (via `whereCond != null || projUsedInBody || skipWhileCond != null || takeWhileCond != null || counterPred != null`). `adapter_bind_name(Zip)` is NOT added — zero callers (only array/decs emit fns call `adapter_bind_name`; emit_zip builds `it` references directly). `adapter_wrap_invoke(Zip)` folds in today's `finalize_zip_invoke` flag set (force_at + force_generated + can_shadow on both block-arg flags) — that helper deletes.
- **2026-05-26 (PR D2, deferred D3)** — Full GroupBySourceAdapter melt (originally D2-C) deferred to PR D3. In-session melt attempt hit a type-inference regression on the Theme 3 C3 chain (`_join + _group_by + _select((R, S = _._1 |> select(...) |> sum)) + count`): the user's outer `_select`'s output type `tuple<R:string;S:int>` was being passed where the inner `select`'s lambda expects `tuple<Region:string;CarId:int>`. Bisected to Commit 4 itself (Commits 1-3 clean); ~45min spent narrowing without isolation. Defer + ship PR D2 with the 3 variant + 2 planner migration commits — partial scope keeps the headline goals (plan_zip + plan_decs_join migrations + DecsJoin/Zip variant widening), GroupBy melt is pure cleanup with no perf or behavior implications.
- **2026-05-26 (PR D2 impl)** — `let dj & = unsafe(adapter as DecsJoin)` in `adapter_wrap_source_loop`'s DecsJoin branch — initial Commit 1 form. Speculated to be a use-after-free (the `& =` reference to a variant-as temp); rewrite to direct `(adapter as DecsJoin).field` reads (after making `adapter` `var` to bypass variant const propagation) did NOT fix the C3 regression — so the reference binding was NOT the bug. Reverted to the direct-read form anyway (cleaner, no unsafe needed). Tracked for follow-up under PR D3.
- **2026-05-26 (PR D2 impl)** — `recognize_reducer_specs` typer integration is the cleanest theory left for the C3 regression: emit_group_by's switch from GroupBySourceAdapter → SourceAdapter::DecsJoin variant changes how plan_group_by_core reads `initialBindName` / `initialElemType` (struct field vs `(adapter as DecsJoin).field`). Both paths return the same string/TypeDecl pointers, but daslang variant-as may propagate const through the payload in a way the typer treats differently. PR D3 should start by adding `to_compiler_log` probes to compare what's threaded into `recognize_reducer_specs` between the two paths.
- **2026-05-26 (PR D3, root cause of D2-deferred C3 regression)** — The theory above was wrong. After `to_compiler_log` probes at construction site (emit_group_by) and read site (plan_group_by_core), `resBindName` was empty (length 0) IMMEDIATELY after the variant was constructed — not a typer issue, not a variant-as issue. Root cause: `resBindName := qn("djoin_jres", at)` inside the named-arg struct constructor silently dropped the field. The `:=` operator is invalid in named-arg position in struct constructors (only `=` is honored), and the parser does NOT error — the field stays default-initialized at runtime. Changing `:=` to `=` fixed the regression cleanly. Worth a lint rule eventually: flag `:=` in struct named-arg constructor calls.
- **2026-05-30 (capability refactor + XML cascade fusion)** — `AdapterKind`/`kind()` deleted: it forced a new enum value + a `kind()==k_X` switch arm at every consumer for each source. Replaced by capability methods the adapter answers about itself (`can_group_by` / `can_reserve_by_length` / `can_ride_array_lane` / `has_own_loop_or_count_lane` / `name_prefix`); the two emit-hook guards (`emit_loop_or_count`, `emit_reverse_skip_into_tail`) drop the kind check and rely on the null-return hook. `RequiresPredicate` gains a `src : SourceAdapter?` param (threaded through `match_pattern` from `run_splice_adapter`) so source gating asks the adapter — the first user is `can_group_by_source`. The two `group_by` patterns (`group_by_array` + `group_by_decs`) collapse into one `group_by` gated by `can_group_by_source`; `build_group_by_adapter` is the sole source/join-shape gate (ArrayAdapter already validated srcb shape, so `array_join_invariants` was removed as dead). On top of the refactor: the XML splice gate (`onlyRow = "loop_or_count_general"`) is lifted, so the cascade families fuse over the XmlAdapter — `XmlAdapter` overrides `build_group_by_adapter` (self; null on upstream join) + `emit_distinct_take_loop`, sharing a `build_xml_dom_walk` + `hoist_prelude` helper with `wrap_source_loop`. join over XML stays tier-2 (next arc).
- **2026-05-30 (leading `_select` source-projection absorption)** — a chain whose first op projects the source (`source |> _select(f) |> order/distinct/take`) bailed to tier-2: the order-family matcher had no leading-select slot, so the whole chain materialized + sorted instead of fusing (the bench m3f/m4 lanes hid this by pre-projecting their source array; XML/decs can't). Fix is source-agnostic: an optional leading `srcsel` slot on the four `wrap_source_loop`-based order rows (`streaming_min` / `bounded_heap` / `order_then_plain_distinct` / `fused_prefilter` — NOT the source-direct `buffer_helper_dispatch`), plus a `ProjectedSourceAdapter` decorator (in `linq_fold_common`) that `run_splice_adapter` wraps the adapter in when `srcsel` is captured. The decorator binds `projName = f(innerBind)` atop the per-element body and delegates loop/invoke to the inner adapter, so every emit sees the projected element with zero emit-fn changes. XML field-pruning is preserved — f's `it.<field>` reads flow into the inner materializer — so `order_distinct_take` m5f drops 2091.8 → 73.6 ns/op (~28×, 0 string clones; reads only the projected field). The slot is optional, so chains with no leading select stay byte-identical.
- **2026-05-30 (join-family unification + XML join + group_by-after-join)** — closes the join arc the cascade refactor pointed at. The two parallel join patterns (`array_join_general` inline-emit + `decs_join_general` hook) collapse into one `join_general` gated by `can_join_source` (new `can_join` capability, mirrors `can_group_by`); `emit_decs_join_hook` is renamed to the general `emit_join_hook`, and the thin `emit_join` dispatcher routes the single pattern to whichever source overrode it. `ArrayAdapter.emit_join_hook` delegates to the unchanged `emit_array_join` (the `array_join_srcb_is_array` requires-predicate folds into `emit_array_join` as a `srcb_is_array_shaped` null-guard; `decs_source` becomes dead and is removed; `decs_join_no_select_with_count` → `join_no_select_with_count`). Zero perf delta: each adapter's emitted AST is byte-identical (the gate moves `requires`→hook but `run_splice_adapter` `continue`s on both a failed `requires` and a null emit). On top of unification, XML join fuses: `XmlAdapter.emit_join_hook` collects srcB into a hash and probes from the field-pruned DOM walk (`emit_xml_join` + the 2-param `build_xml_join_invoke`); `XmlAdapter.build_group_by_adapter` now returns an `XmlJoinAdapter` on the upstream-join arm so `join |> group_by` fuses single-pass (the materialize+walk+hoist is factored into a shared `build_pruned_dom_walk`). Also: the count fast-path macro emits `cnt++` instead of `cnt += 1`.
- **2026-05-30 (XML `join |> select` iterator-typed fusion)** — a bare `join |> select` with no terminator (`_fold(from_xml |> _join |> _select)`, iterator-typed result) hit the `!countOnly && expr_is_iterator` bail in `XmlAdapter.emit_join_hook` and fell to tier-2, which materialized full join tuples (`join_to_array`) then lazily projected. The bail existed because the fused emit returns an array while the `_fold` contract wants a sequence. Fix: thread `wrapIter = !countOnly && ctx.expr_is_iterator` into `build_xml_join_invoke` (which already wraps in `.to_sequence_move()`), so the fused single-pass field-pruned build runs and its result array is exposed as a sequence — no separate `join_to_array`. **The output shape was chosen by profiling hand-coded "fake code" first** (`benchmarks/micro/join_select_shapes.das`): fused-materialize beats a streaming generator in every config (INTERP/JIT, with/without the `from_xml` string clone) — the generator's per-element resume overhead dominates, and the string clone is a flat orthogonal cost no output shape avoids. `join_select` m5f 510.7 → 192.4 INTERP (2.65×), 196.9 → 76.2 JIT. XML-only: array is eager-typed (never hits the bail) and decs's m4 lane is `to_array`-terminated; decs's own iterator-typed join bail (`emit_decs_join_impl`) is left for a separate profile.
- **2026-05-30 (XML materialize-under-guard)** — when the whole row escapes the fold but ONLY inside a where-gate (`where(pred) |> single/last/first`, `where(pred) |> to_array`, `where(pred) |> order_by`, …), `build_xml_materializer`'s `allUsed` branch used to build the full `Car` — including the `name` string clone — for *every* source element, then run the gate. So a unique-id `single()` paid 100 000 clones to return one row. Fix: `try_xml_under_guard_materialize` recognizes the bare `if (fieldCond) { … }` body shape, reads just the predicate's fields cheaply per element (`peek_xml_field` — a non-cloning sibling of `read_xml_field`), and prepends the full `build_xml_row` *inside* the guarded branch — so non-matching elements skip the clone entirely, and a string-typed predicate is guard-clone-free too, not just scalar ones. `peek_xml_field` returns a `string` attribute as a borrowed temp **`string#`** (`reinterpret<string#>(as_string(...))`), so the borrow is **type-enforced**: reading / comparing the view compiles, but *storing* it (`push`, global assign, returning it out) is a compile error that forces an explicit `clone_string` — a retained view can't silently dangle past the document's RAII scope. (`strings_boost`'s `contains` was widened to `str, sub : string | string#` so read-only predicates calling it compile; `==` / `ends_with` / `to_lower` / `find` already accept temps via `string const implicit`.) Field-only condition required (a whole-`it` use in the cond → fall back to the unconditional full build); bare-field conds (`where(_.flag)`) are handled by `flatten_xml_row_to_locals` returning the (possibly replaced) root. **Design validated by hand-coded micro-bench first** (`benchmarks/micro/single_last_shapes.das`). Wins: `single_match` m5f 330 → 57.5 INTERP (5.7×, 100 000 → 1 clone), `last_match` 335 → 224 (clones only matching rows); falls out for free on the same shape: `select_where` 339 → 227, `bare_order_where` 447 → 330. XML-only and pure optimization — count/sum/where|>count don't trigger `allUsed`, so the path is untouched; no source-agnostic emit contract change.
- **2026-05-31 (deferred materialization — handle-buffering for buffered reducers)** — the buffered reducers (`order_by`/`sort`/`reverse` + `take`/`first`, `distinct_by |> order_by`) materialized the full `Car` (its `name` clone) for *every* source element before the reducer kept only K — `from_xml_node` builds all N. Fix: the reducer buffers a cheap **surrogate** — `(orderKey, xml_node)` for the order emits, a bare `xml_node` for reverse (no key) — and `build_xml_row` runs only for the K survivors. The comparator is the fixed `_::less(a._0, b._0)` on the precomputed key; where/distinct are consumed during the walk (cheap field reads gate which elements get a surrogate), so they never enter the surrogate. **The abstraction is source-generic** (an "element handle"): the surrogate machinery + materialize-survivors tail live in `linq_fold_common` (`build_surrogate_type` / `build_surrogate_cmp` / `build_surrogate_materialize_loop`); each source supplies `defers_materialization()` + `handle_type()` + `current_handle_expr()` + `materialize_handle()`. Only `XmlAdapter` overrides them this PR (a future `linq_json` is just those 4 hooks); `array`/`decs` inherit the no-defer default and stay **byte-identical** (their backing store is pre-materialized, so their reducers already clone only ~K heap-entrants — confirmed by `benchmarks/micro/sort_distinct_take_shapes.das`, where the `array<Car?>` pointer form is slower or tied). Wired into `emit_bounded_heap` (take), `emit_fused_prefilter` (distinct-only no-take arm — the pure-where case is already materialize-under-guard'd), `emit_streaming_min` (first), and `emit_reverse_buffer_inplace` (reverse + take). **Design validated by hand-coded micro-bench first** (`benchmarks/micro/sort_distinct_take_shapes.das`). Wins (m5f INTERP / JIT): `sort_take` 338 → 69 / 17, `order_take_desc` 343 → 69, `distinct_by_order_take` 354 → 126 / 46, `select_where_order_take` 228 → 71, `distinct_by_order_to_array` 356 → 131 / 46, `sort_first` 336 → 64 / 17, `reverse_take` 360 → 90 / 70 — string clones 100 000 → K everywhere. Not deferred (inherent floor / out of scope): `bare_order_where` (already at the under-guard survivor floor), `order_reverse_normalized` (order+reverse → all rows out), `reverse_distinct_by` (tier-2, no fused emit), `groupby_first` (group_by path).
- **2026-05-31 (deferred materialization — `last` + group-by `first`)** — extends the element-handle deferral to the two remaining survivors-≪-N reducers: the full-walk `last`/`last_or_default` terminator (in `emit_early_exit_lane`) and `first`-per-group inside `plan_group_by_core`. `last` cloned the whole `Car` (`lst := it`) on *every* match and kept only the final one; over a deferring source it now stores the node **handle** per match and runs `materialize_handle` once, for the single survivor. `group_by(brand) |> select((key, first per group))` pinned the whole row (`slot := it`) in `mk_reducer_first`, forcing `wrap_source_loop` to build every element; a new `mk_reducer_first_deferred` materializes from the handle *inside the table miss-branch*, so the walk field-prunes to just the group key and `build_xml_row` runs only once per distinct group. Both ride the same four `SourceAdapter` hooks — only `XmlAdapter` defers; `array`/`decs` pass `null`/no-defer and stay byte-identical (the `emit_reducer_branches` adapter param defaults to `null`; the group-by gate also requires the bind be the raw element — `itName == bind_name`, i.e. no upstream `_select` rebinds it — since the handle yields the raw row). **Design validated by hand-coded micro-bench first** (the `last_match` / `groupby_first` lanes in `benchmarks/micro/sort_distinct_take_shapes.das`). Wins (m5f INTERP / JIT, string clones 100 000 → K): `last_match` 219 → 65 / 21 (K=1), `groupby_first` 339 → 72 / 22 (K=#brands). Closes `groupby_first` (the last item on the prior entry's floor list). Still not deferred: `bare_order_where` / `order_reverse_normalized` (all rows out), `reverse_distinct_by` (tier-2, no fused emit).
- **2026-05-31 (forward keep-last — `reverse |> distinct[_by]` over forward sources)** — the only buffered shape still falling to tier-2 over a forward source. `reverse() |> distinct_by(K) |> to_array()` means "keep the LAST forward row per key, output in reverse-discovery order." The sole fused emit was `emit_reverse_backward_walk_dset_gate` — a backward **index** walk (`src[len-1-k]`) gated `array_source`, so XML / decs / plain iterators (forward-only, no random access) cascaded: `reverse()` materialized all N, then `distinct_by` walked. New `emit_reverse_distinct_forward_keeplast` (R-2b, gated by the exact complement `non_array_source`) does a single forward pass instead — `table<key; (seq, val)>`, **OVERWRITE** the slot per element (so it ends at the last forward occurrence + its seq), then sort survivors by **descending seq** (`build_surrogate_cmp(true)`) and emit. Output-identical to the backward walk (descending forward-index of each last occurrence), proven by parity vs both `m3f` (array backward walk) and the tier-2 cascade. It rides `emit_terminator_lane` → `wrap_source_loop`, so it's source-generic: **XML defers** (the table holds `(seq, xml_node)` and `build_xml_row` runs only for the K survivors — field-pruned to the key); **decs / iterator** store the full element (no handle), winning single-pass over the cascade's reverse-buffer + second walk. `ctx.top` is `null` for decs (bridge-driven), so `elemType` falls back to `ctx.src->element_type()`; arrays still match the backward-walk row first (registered earlier), so they're byte-identical. **Design validated by hand-coded micro-bench first** (the `reverse_distinct_by` lane in `benchmarks/micro/sort_distinct_take_shapes.das`: INTERP 405.8 → 88.6, JIT 162.6 → 37.0, string clones 100 000 → #keys). Wins: `reverse_distinct_by` m5f **429 → 74 INTERP / 166.6 → 22 JIT** (clones 100 000 → 5), and the previously-`—` decs **m4 lights up at 27.7 / 5.0** (near the array fast path). Closes `reverse_distinct_by` — the last forward-source buffered floor.

## Open questions

- **Prefix-conflict lint pass** — in PR A scope or deferred? Lean PR A so it grows with the table.
- **`plan_zip` / `plan_decs_join` SourceAdapter shape** — answered in PR D2: `SourceAdapter` widens to 4 variants (`Array | Decs | DecsJoin | Zip`). DecsJoin carries 9-field `DecsJoinShape` (bridgeA/B, keyaLam/keybLam/resultLam, keyType/tupBType/resultType, resBindName). Zip carries 6-field `ZipShape` (srcAExpr/srcBExpr, srcAName/srcBName, srcAType/srcBType). DecsJoin's `adapter_wrap_source_loop` branch is consumed only by GroupBy decs-join (emit_decs_join keeps inline collect+probe per D2-A to preserve count-no-where fast path).
- **Full GroupBySourceAdapter unification** — deferred to PR D3. Attempted in PR D2 but hit a C3 type-inference regression that wasn't isolated in-session. Pure cleanup; can land separately when bandwidth allows.
- **3-source `zip(a, b, c)` splice** — TODO, revisit. Today both the predicate `zip_arity_2_or_3` and emit_zip's arity-3 branch treat n==3 as the selector-block form (`zip(a, b, $(l,r) => F)`). True 3-source zip cascades to tier-2 `_::zip`. Flagged by PR D2 Copilot review (item 4 — predicate over-admits; item 5 — RST row lies). Inherited from pre-PR-D2 imperative code. Splice extension is straightforward (extend ZipShape with srcC fields + Zip3 variant or alt-payload, generalize emit_zip's lockstep loop to 3 iters); deferred until a real bench/usage motivates it.
- **Lint: catch empty variable names in emitted AST + `:=` in named-arg struct ctor** — TODO. PR D3 root cause was `Foo(field := value)` syntax: the `:=` operator is invalid in named-arg constructor position; daslang silently drops the field (no parse error, runtime default-init). This propagated empty string `resBindName` through `(adapter as DecsJoin).resBindName`, emitting `var :tuple<...> = ...` (nameless var) in the splice. Two complementary lint angles: (a) AST-level: flag `ExprLet`/`ExprVar` declarations with empty/whitespace-only names anywhere in the compiled program (would catch BOTH macro-emission bugs of this shape AND any future hand-written `var : T = ...`); (b) source-level: flag `:=` in named-argument position of an `ExprMakeStruct` / `ExprCall`-to-ctor (the root cause). (a) is broader, (b) is precise. Belongs in `daslib/perf_lint.das` or `daslib/style_lint.das`; consider both rules.
- **Reducer-spec data table** — answered in PR D1: `table<string; ReducerEmitterFn>` lookup with named module-level `mk_reducer_*` fns. Lookup site: `reducer_emitters?[spec.name] ?? default<ReducerEmitterFn>` followed by `invoke()`. Internal workhorse splits stay inside the per-reducer fns via `mk_strictly_preferred` helper.
- **`SourceAdapter` method surface** — answered in PR C. The minimal contract turned out to be 4 helpers (`adapter_bind_name`, `adapter_element_type`, `adapter_wrap_source_loop`, `adapter_wrap_invoke`), not a method-on-variant pattern. `finalize_emission_stmts` / `finalize_decs_emission` semantics merged into `adapter_wrap_invoke`. The take(N) early-exit case stayed per-adapter inline rather than via a `wrap_source_loop_with_stop` extension — only emit_hashtable_dedup needed it in PR C, so factoring a helper was premature.

## See also

- `doc/source/reference/linq_fold_patterns.rst` — user-facing splice-pattern reference (refreshed per arm-touching PR).
- `benchmarks/sql/linq_fold_chain_audit.md` — closed-out audit that drove Themes 1-8 (PRs #2851 / #2852 / #2857 / #2861 / #2862 / #2865 / #2866 / #2874 / #2875).
- `benchmarks/sql/results.md` — INTERP+JIT matrix refreshed per splice-touching PR.
