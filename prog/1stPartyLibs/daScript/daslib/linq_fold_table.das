options gen2
options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options _comment_hygiene = true

module linq_fold_table shared public

//! linq_fold table source adapter: ``TableAdapter`` + ``extract_table_source``. Lets ``_fold`` over an
//! ``each_kv(tab)`` / ``keys(tab)`` / ``values(tab)`` chain emit a fused slot-walk loop over the table
//! instead of riding the generic iterator tier. The kv lane scans which sides of the pair the chain
//! touches and prunes the walk to a keys-only / values-only single iterator (the table analog of XML
//! field-pruning); bare ``count()`` folds to O(1) ``length(tab)``; a plain ``distinct`` over raw keys/kv
//! elements is dropped (keys are unique by construction). In-tree companion to ``daslib/linq_fold``
//! (required unconditionally; the matcher returns null for non-table chains). Emits ``keys`` / ``values``
//! / the kv zip BY NAME at the user's splice site. See history/linq_fold/LINQ_TO_TABLE.md.

require daslib/ast_boost
require daslib/ast_match
require daslib/match
require daslib/templates_boost
require daslib/macro_boost
require daslib/linq_fold_common public

enum TableLane {
    KV
    KEYS
    VALUES
}

// Per-element table walk; the KV lane prunes to the cheapest iterator set from the body's
// `it.key` / `it.value` usage (the table analog of XML field-pruning, see LINQ_TO_TABLE.md).
[macro_function]
def private build_table_walk(lane : TableLane; srcName, bindName : string; var body : Expression?;
                             var breakGuard : Expression?; at : LineInfo) : Expression? {
    var inner : array<Expression?>
    if (breakGuard != null) {
        inner |> push <| qmacro_expr() {
            break if ($e(breakGuard))
        }
    }
    // keys() deliberately yields NON-const elements (writable temp copies), so the engine-visible bind is a `let` rebind of the loop var — keys are workhorse types, the copy is free, and downstream ==const composition (push_clone of a bare projected key) needs the const.
    let kIter = qn("tkvk", at)
    let vIter = qn("tkvv", at)
    if (lane == TableLane.KEYS) {
        inner |> push <| qmacro_expr() {
            let $i(bindName) = $i(kIter)
        }
        inner |> push(body)
        return <- qmacro_block() {
            for ($i(kIter) in keys($i(srcName))) {
                $b(inner)
            }
        }
    }
    if (lane == TableLane.VALUES) {
        // values over the const table param yield `V& const` — bind directly, no rebind copy
        inner |> push(body)
        return <- qmacro_block() {
            for ($i(bindName) in values($i(srcName))) {
                $b(inner)
            }
        }
    }
    // KV lane
    var (allUsed, usedFields) = collect_row_usage(body, bindName)
    if (allUsed) {
        inner |> push <| qmacro_expr() {
            let $i(bindName) = (key = $i(kIter), value = $i(vIter))
        }
        inner |> push(body)
        return <- qmacro_block() {
            for ($i(kIter), $i(vIter) in keys($i(srcName)), values($i(srcName))) { // nolint:LINT002 parallel-for must bind both vars
                $b(inner)
            }
        }
    }
    let useKey = usedFields |> has_value("key")
    let useValue = usedFields |> has_value("value")
    let kLocal = qn("tkey", at)
    var fieldToLocal <- { "key" => kLocal, "value" => vIter }
    body = flatten_row_to_locals(body, bindName, fieldToLocal)
    if (useKey) {
        inner |> push <| qmacro_expr() {
            let $i(kLocal) = $i(kIter)
        }
    }
    inner |> push(body)
    if (useValue && !useKey) {
        return <- qmacro_block() {
            for ($i(vIter) in values($i(srcName))) {
                $b(inner)
            }
        }
    }
    if (useKey && useValue) {
        return <- qmacro_block() {
            for ($i(kIter), $i(vIter) in keys($i(srcName)), values($i(srcName))) { // nolint:LINT002 parallel-for must bind both vars
                $b(inner)
            }
        }
    }
    // key-only AND no-field-touched (e.g. bare count walk) both ride the cheaper keys iterator
    return <- qmacro_block() {
        for ($i(kIter) in keys($i(srcName))) { // nolint:LINT002 body may not read the key (bare-count walk)
            $b(inner)
        }
    }
}

// Single flat for-loop over the table inside a 1-param invoke binding the table, like ArrayAdapter over
// an array — except the loop iterator(s) derive from the table param at the splice site.
class TableAdapter : SourceAdapter {
    tabExpr  : Expression?      // the table expression (argument of each_kv/keys/values)
    srcName  : string           // invoke param name binding the table
    elemType : TypeDeclPtr      // KV: tuple<key:K;value:V>; KEYS: K; VALUES: V
    lane     : TableLane
    def override name_prefix() : string {
        return "tab_"
    }
    def override supports_direct_return() : bool {
        return true   // single flat for-loop inside the invoke; a mid-loop `return` exits the invoke
    }
    def override can_reserve_by_length() : bool {
        return true   // length(tab) is O(1); the shared reserve hint reads loop_source_expr/loop_source_name
    }
    def override const can_join() : bool {
        return true   // rides emit_array_join: direct-return lead loop via wrap_source_loop
    }
    def override const can_group_by() : bool {
        return true   // plan_group_by_core drives the bucket-fill through wrap_source_loop (kv pruning free)
    }
    def override emit_join_hook(var c : Captures; var ctx : EmitCtx; at : LineInfo) : Expression? {
        return emit_array_join(c, ctx, at)
    }
    def override build_group_by_adapter(var c : Captures; var ctx : EmitCtx; at : LineInfo) : SourceAdapter? {
        // join |> group_by over a table lead stays a deferred edge (LINQ_TO_TABLE.md) — null bails to tier-2
        if (c.single |> key_exists("upstream_join")) return null
        return new TableAdapter(tabExpr = clone_expression(tabExpr), srcName = qn("tsrc", at),
                                elemType = clone_type(elemType), lane = lane)
    }
    def override loop_source_expr() : Expression? {
        // Feeds the reserve hint (type_has_length covers tables). The backward-index reverse lanes that
        // also read loop_source_expr gate on array_source, which is false here — matchTop stays iterator-typed.
        return tabExpr
    }
    def override loop_source_name() : string {
        return srcName
    }
    def override bind_name(at : LineInfo) : string {
        return qn("it", at)
    }
    def override element_type() : TypeDeclPtr {
        return clone_type(elemType)
    }
    def override count_shortcut(opName : string; at : LineInfo) : Expression? {
        return emit_length_shortcut(opName, tabExpr, srcName, at)
    }
    def override wrap_source_loop(loopShape : LoopDispatch; var body : Expression?; at : LineInfo) : Expression? {
        return build_table_walk(lane, srcName, bind_name(at), body, null, at)
    }
    def override emit_distinct_take_loop(bindName : string; takenName : string; takeLimName : string; var perElement : Expression?; at : LineInfo) : Expression? {
        var breakGuard = qmacro($i(takenName) >= $i(takeLimName))
        return build_table_walk(lane, srcName, bindName, perElement, breakGuard, at)
    }
    def override invoke_param_type() : TypeDeclPtr {
        // Const-accepting param: the source table is often a `let`, and a non-const source adds-const cleanly.
        var tabType = strip_const_ref(clone_type(tabExpr._type))
        tabType.flags.constant = true
        return tabType
    }
    def override wrap_invoke(var stmts : array<Expression?>; retType : TypeDeclPtr; wrapIter : bool; at : LineInfo) : Expression? {
        var tabType = invoke_param_type()
        var tabClone = clone_expression(tabExpr)
        tabClone.genFlags.alwaysSafe = true
        let sn = srcName
        var emission : Expression?
        if (retType != null) {
            emission = qmacro(invoke($($i(sn) : $t(tabType)) : $t(retType) {
                $b(stmts)
            }, $e(tabClone)))
        } else {
            emission = qmacro(invoke($($i(sn) : $t(tabType)) {
                $b(stmts)
            }, $e(tabClone)))
        }
        emission = finalize_invoke(emission, at)
        if (wrapIter) {
            emission = qmacro($e(emission).to_sequence_move())
            emission.force_generated(true)
        }
        return emission
    }
}

// ===== Point-lookup folds — `where(kv.key == X [&& residual])` + terminator → O(1) key probe =====
// any/contains → key_exists, count → key_exists?1:0, first[_or_default] (± select) → tab?[X] probe, with the
// scan's exact semantics; residual conjuncts run on the probed element only. Table: linq_fold_patterns.rst.

[macro_function]
def private match_key_probe_side(var keySide, otherSide : Expression?; lane : TableLane; bindName : string) : Expression? {
    // X must be loop-invariant AND side-effect free — the scan evaluates X per element, a probe once
    if (!is_bare_key_ref(keySide, bindName, lane == TableLane.KV)
            || expr_uses_var(otherSide, bindName) || has_sideeffects(otherSide)) {
        return null
    }
    return clone_expression(otherSide)
}

// Decompose a peeled predicate body (binder renamed to bindName) as `<key-ref> == X [&& conjunct …]`;
// returns cloned X, AND-rebuilds the rest into residual. Key-equality must be LEFTMOST: keys are unique, so
// right of it runs at most once on both paths (no purity gate); a LEFT conjunct runs per scan element — decline.
[macro_function]
def private extract_key_probe(var pred : Expression?; lane : TableLane; bindName : string; var residual : Expression?&) : Expression? {
    // peel the left-assoc `&&` spine; right arms collect outermost-first (= reversed source order)
    var leaf = pred
    var conjuncts : array<Expression?>
    while (leaf != null && leaf is ExprOp2 && (leaf as ExprOp2).op == "&&") {
        var andOp = leaf as ExprOp2
        conjuncts |> push(andOp.right)
        leaf = andOp.left
    }
    var lhs, rhs : ExpressionPtr
    if (!qmatch(leaf, $e(lhs) == $e(rhs)).matched) return null
    var probe = match_key_probe_side(lhs, rhs, lane, bindName)
    if (probe == null) {
        probe = match_key_probe_side(rhs, lhs, lane, bindName)
    }
    if (probe == null) return null
    for (c in conjuncts) {
        var cc = clone_expression(c)
        residual = residual == null ? cc : merge_where_cond(cc, residual)
    }
    return probe
}

// Miss-path emission per terminator: any → false, count → 0, first_or_default → bound default, first → panic.
[macro_function]
def private make_probe_miss(termName : string; dName : string) : Expression? {
    if (termName == "any") {
        return qmacro_expr() {
            return false
        }
    }
    if (termName == "count") {
        return qmacro_expr() {
            return 0
        }
    }
    if (termName == "first_or_default") {
        return qmacro_expr() {
            return $i(dName)
        }
    }
    return qmacro_expr() {
        panic("sequence contains no elements")
    }
}

[macro_function]
def try_table_point_lookup(var calls : array<tuple<ExprCall?; LinqCall?>>; var adapter : TableAdapter?; at : LineInfo) : Expression? {
    if (adapter.lane == TableLane.VALUES) return null
    let n = length(calls)
    if (n < 1 || n > 3) return null
    var termCall = calls[n - 1]._0
    let termName = calls[n - 1]._1.name
    let termArgs = length(termCall.arguments)
    var selCall : ExprCall?
    var keyX : Expression?
    var residual : Expression?
    let kName = qn("plk_k", at)
    let dName = qn("plk_d", at)
    let pName = qn("plk_p", at)
    // keys-lane elements ARE the key, so the probe local doubles as the residual/projection binder
    let bindName = adapter.lane == TableLane.KEYS ? kName : qn("plk_kv", at)
    var predArg : Expression?
    if (n == 1) {
        // predicate-form terminators, and the keys-lane contains
        if ((termName == "any" || termName == "count") && termArgs == 2) {
            predArg = termCall.arguments[1]
        } elif (termName == "contains" && termArgs == 2 && adapter.lane == TableLane.KEYS) {
            // element evaluated exactly once on both paths — no invariance/purity gate needed
            keyX = clone_expression(termCall.arguments[1])
        } else {
            return null
        }
    } else {
        // collapse_chained_wheres ran at flatten time, so a leading where run is already ONE call here
        if (calls[0]._1.name != "where_" || length(calls[0]._0.arguments) != 2) return null
        predArg = calls[0]._0.arguments[1]
        if (n == 3) {
            if (calls[1]._1.name != "select" || length(calls[1]._0.arguments) != 2
                    || (termName != "first" && termName != "first_or_default")) {
                return null
            }
            selCall = calls[1]._0
        } elif (!((termName == "any" || termName == "count" || termName == "first") && termArgs == 1)
                && !(termName == "first_or_default" && termArgs == 2)) {
            return null
        }
    }
    if (predArg != null) {
        var predBody = peel_lambda_rename_var(predArg, bindName)
        keyX = extract_key_probe(predBody, adapter.lane, bindName, residual)
    }
    if (keyX == null) return null
    let sn = adapter.srcName
    // residual-free boolean / counting probes — bare key_exists, no element touch
    if (residual == null && (termName == "any" || termName == "contains")) {
        var anyStmts <- qmacro_block_to_array() {
            return key_exists($i(sn), $e(keyX))
        }
        return adapter->wrap_invoke(anyStmts, null, false, at)
    }
    if (residual == null && termName == "count") {
        var cntStmts <- qmacro_block_to_array() {
            return key_exists($i(sn), $e(keyX)) ? 1 : 0
        }
        return adapter->wrap_invoke(cntStmts, null, false, at)
    }
    // element probes: first / first_or_default ± select, and any/count carrying a residual conjunct
    var retT : TypeDeclPtr
    if (termName == "first" || termName == "first_or_default") {
        retT = strip_const_ref(clone_type(termCall._type))
        retT.flags.removeConstant = false
    }
    var stmts : array<Expression?>
    if (termName == "first_or_default") {
        // eager default bind, matching linq.das argument evaluation order
        stmts |> push <| qmacro_expr() {
            let $i(dName) = $e(termCall.arguments[1])
        }
    }
    stmts |> push <| qmacro_expr() {
        let $i(kName) = $e(keyX)
    }
    if (adapter.lane == TableLane.KEYS) {
        stmts |> push <| qmacro_expr() {
            if (!key_exists($i(sn), $i(kName))) {
                $e(make_probe_miss(termName, dName))
            }
        }
    } else {
        // KV lane: probe the value pointer, materialize the (key, value) pair on hit. Table safe-index is
        // unsafe (the pointer dangles on rehash) — fine here, the generated invoke never mutates the table.
        stmts |> push_from <| qmacro_block_to_array() {
            let $i(pName) = unsafe($i(sn)?[$i(kName)])
            if ($i(pName) == null) {
                $e(make_probe_miss(termName, dName))
            }
        }
        if (residual != null || selCall != null) {
            stmts |> push <| qmacro_expr() {
                let $i(bindName) = (key = $i(kName), value = *$i(pName))
            }
        }
    }
    // residual conjuncts evaluate on the probed element only — false routes to the same miss path
    if (residual != null) {
        stmts |> push <| qmacro_expr() {
            if (!$e(residual)) {
                $e(make_probe_miss(termName, dName))
            }
        }
    }
    if (termName == "any") {
        stmts |> push <| qmacro_expr() {
            return true
        }
        return adapter->wrap_invoke(stmts, null, false, at)
    }
    if (termName == "count") {
        stmts |> push <| qmacro_expr() {
            return 1
        }
        return adapter->wrap_invoke(stmts, null, false, at)
    }
    if (selCall != null) {
        var proj = peel_lambda_rename_var(selCall.arguments[1], bindName)
        stmts |> push <| qmacro_expr() {
            return $e(proj)
        }
    } elif (adapter.lane == TableLane.KEYS) {
        stmts |> push <| qmacro_expr() {
            return $i(kName)
        }
    } elif (residual != null) {
        stmts |> push <| qmacro_expr() {
            return $i(bindName)
        }
    } else {
        stmts |> push <| qmacro_expr() {
            return (key = $i(kName), value = *$i(pName))
        }
    }
    return adapter->wrap_invoke(stmts, retT, false, at)
}

// Recognize an `each_kv(tab)` / `keys(tab)` / `values(tab)` chain top. Returns the call (caller reads
// arguments[0] = the table, `_type.firstType` = element); null otherwise. Name + table-typed-arg match,
// like extract_json_source — the strong arg-type gate keeps an unrelated user `keys` from firing this.
[macro_function]
def extract_table_source(var top : Expression?) : ExprCall? {
    if (top == null || !(top is ExprCall)) return null
    var c = top as ExprCall
    let name = get_call_short_name(c)
    if ((name != "each_kv" && name != "keys" && name != "values")
            || c._type == null || !c._type.isIterator || c._type.firstType == null
            || (c.arguments |> length) != 1) {
        return null
    }
    let srcT = c.arguments[0]._type
    if (srcT == null || !srcT.isGoodTableType) return null
    return c
}

// Drop plain `distinct` over raw keys/kv elements — table keys are unique by construction, so the whole
// dedup-set machinery is a no-op. Only when every call BEFORE the distinct preserves element uniqueness
// (filters/ranges/reorders — never a `select`, which reshapes elements). `distinct_by` keeps its own key.
[macro_function]
def drop_redundant_distinct(var calls : array<tuple<ExprCall?; LinqCall?>>) {
    var dropIdx = -1
    for (i in range(length(calls))) {
        let name = calls[i]._1.name
        if (name == "distinct") {
            dropIdx = i
            break
        }
        // uniqueness-preserving prefix ops only
        if (name != "where_" && name != "skip" && name != "take"
                && name != "skip_while" && name != "take_while" && name != "reverse") {
            return
        }
    }
    if (dropIdx < 0 || length(calls) <= 1) return   // keep a bare `distinct`-only chain for the generic lanes
    calls |> erase(dropIdx)
}
