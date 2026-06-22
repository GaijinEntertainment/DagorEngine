options gen2
options indenting = 4
options _comment_hygiene = true

module linq_fold_decs shared public

//! linq_fold decs (ECS) source adapters: DecsAdapter / DecsJoinAdapter, the decs-bridge tuple
//! scanners/rewriters, and the decs-specific emit hooks (count_shortcut, reverse-skip-into-tail,
//! join, min_by/max_by). Terminators ride the generic lanes in linq_fold_common via DecsAdapter.
//! Requires linq_fold_common. See daslib/linq_fold.md.

require daslib/linq public
require daslib/ast_boost
require daslib/ast_match
require daslib/templates_boost
require daslib/macro_boost
require daslib/linq_fold_common public

// Stamp the assembled decs walk with the call-site LineInfo + the generated flag, optionally
// wrapping it back into an iterator. Decs-only finalizer for the adapter emit paths below.
[macro_function]
def finalize_decs_emission(var emission : Expression?; at : LineInfo; wrapToIter : bool) : Expression? {
    emission.force_at(at)
    emission.force_generated(true)
    if (wrapToIter) {
        emission = qmacro($e(emission).to_sequence_move())
    }
    return emission
}

// Hashed equi-join payload — feeds the per-pair `let resBindName = result(tupA,tupB)` bind used by both GroupBy's decs-join arm and `emit_decs_join_impl`. The 2 consumers differ on whether the per-pair loop is the right granularity (GroupBy: yes; emit_decs_join_impl keeps a bucket-length fast path for count-no-where).
struct DecsJoinShape {
    bridgeA, bridgeB : DecsBridgeShape?
    keyaLam, keybLam, resultLam : Expression?
    keyType, tupBType, resultType : TypeDeclPtr
    resBindName : string
}

class DecsAdapter : SourceAdapter {
    bridge : DecsBridgeShape?
    tupName : string
    // Terminator-path context (null/empty on the group_by/distinct path, where the chain + ranges are
    // already baked into the body the group_by core hands to wrap_source_loop). When set, wrap_source_loop
    // owns the decs in-loop ranges + chain-binding + range prelude + for_each{,_find} dispatch.
    chainInfo : DecsChainInfo?
    calls : array<tuple<ExprCall?; LinqCall?>>
    intermediateEnd : int
    rangeInfo : DecsRangeInfo?
    def override name_prefix() : string {
        return "decs_"
    }
    def override has_own_loop_or_count_lane() : bool {
        return true
    }
    def override effective_dispatch(base : LoopDispatch) : LoopDispatch {
        // take / take_while emit an early-exit `return true` from the inner-for, which only works inside a
        // for_each_archetype_find bool-lambda — so an otherwise-Each terminator must walk via Find.
        if (rangeInfo != null && base is Each && (rangeInfo.takeExpr != null || rangeInfo.takeWhileCond != null)) {
            return LoopDispatch(Find = null)
        }
        return base
    }
    def override const can_group_by() : bool {
        return true
    }
    def override const can_join() : bool {
        return true
    }
    def override emit_loop_or_count(var c : Captures; var ctx : EmitCtx; at : LineInfo) : Expression? {
        return emit_loop_or_count_lane_decs(bridge, tupName, c, ctx, at)
    }
    def override emit_reverse_skip_into_tail(var takeArg : Expression?; isIter : bool; var termLam : Expression?; termElemType : TypeDeclPtr; at : LineInfo) : Expression? {
        return emit_decs_reverse_skip_into_tail(takeArg, bridge, tupName, isIter, termLam, termElemType, at)
    }
    def override emit_distinct_take_loop(bindName : string; takenName : string; takeLimName : string; var perElement : Expression?; at : LineInfo) : Expression? {
        // take(N) early-exit: for_each_archetype_find returning bool stops the archetype walk (mirrors plan_decs_distinct).
        if (bridge == null) return null   // bridge is non-null by construction; guard mirrors element_type() for the nullable field type
        let archName = bridge.archName
        var innerBody : Expression? = qmacro_block() {
            break if ($i(takenName) >= $i(takeLimName))
            $e(perElement)
        }
        var prunedFor = build_decs_inner_for_pruned(bridge, tupName, innerBody, at)
        let br = bridge
        return <- qmacro_expr() {
            for_each_archetype_find($e(br.reqHashExpr), $e(br.erqExpr), $($i(archName) : Archetype) : bool {
                $e(prunedFor)
                return $i(takenName) >= $i(takeLimName)
            })
        }
    }
    def override build_group_by_adapter(var c : Captures; var ctx : EmitCtx; at : LineInfo) : SourceAdapter? {
        if (c.single |> key_exists("upstream_join")) {
            var joinCall = c.single["upstream_join"]
            var bridgeB = extract_decs_bridge(joinCall.arguments[1])
            if (bridgeB == null) {
                return null
            }
            var keyaLam = joinCall.arguments[2]
            var keybLam = joinCall.arguments[3]
            var resultLam = joinCall.arguments[4]
            if (keyaLam == null || keybLam == null || resultLam == null
                    || keyaLam._type == null || resultLam._type == null
                    || resultLam._type.firstType == null) {
                return null
            }
            var keyType = strip_const_ref(clone_type(keyaLam._type.firstType))
            if (!is_primitive_join_key_type(keyType)) {
                return null
            }
            var resultType = strip_const_ref(clone_type(resultLam._type.firstType))
            var tupBType = strip_const_ref(clone_type(bridgeB.elementType))
            return new DecsJoinAdapter(shape = DecsJoinShape(
                bridgeA = bridge,
                bridgeB = bridgeB,
                keyaLam = keyaLam,
                keybLam = keybLam,
                resultLam = resultLam,
                keyType = keyType,
                tupBType = tupBType,
                resultType = resultType,
                resBindName = qn("djoin_jres", at)))
        }
        return new DecsAdapter(bridge = bridge, tupName = qn("decs_tup", at))
    }
    def override emit_join_hook(var c : Captures; var ctx : EmitCtx; at : LineInfo) : Expression? {
        return emit_decs_join_impl(bridge, c, ctx, at)
    }
    def override count_shortcut(opName : string; at : LineInfo) : Expression? {
        // Only plain count() — arch.size sum truncates to int, so long_count must walk for an int64-safe total.
        if (opName != "count") return null
        return emit_decs_count_archsize(bridge, at)
    }
    def override bind_name(at : LineInfo) : string {
        // Terminator path: the post-chain element bind the generic lane's per-element work references
        // (wrap_source_loop's chain-walk introduces it). Otherwise the raw source-tuple bind. Returned
        // directly (not a fresh qn) so peeling can't desync from the loop if `at` ever differs.
        if (chainInfo != null) return chainInfo.finalBind
        return tupName
    }
    def override element_type() : TypeDeclPtr {
        if (bridge == null) {
            return null
        }
        if (chainInfo != null) return clone_type(chainInfo.finalType)
        return clone_type(bridge.elementType)
    }
    def override wrap_source_loop(loopShape : LoopDispatch; var body : Expression?; at : LineInfo) : Expression? {
        // Terminator path (chainInfo set): wrap decs in-loop ranges + chain-binding around the per-element
        // body, prune unused columns, hoist the range prelude, and frame the walk per loopShape. group_by /
        // distinct path (chainInfo null): the body is already complete — just prune + a plain for_each.
        let archName = bridge.archName
        let br = bridge
        var inner = body
        var prelude : array<Expression?>
        if (chainInfo != null) {
            let names <- make_decs_range_names(at)
            inner = wrap_inner_for_with_decs_ranges(inner, rangeInfo, names)
            inner = wrap_decs_chain(inner, chainInfo, calls, intermediateEnd, at)
            if (inner == null) return null
            prelude <- decs_range_prelude(rangeInfo, names)
        }
        var forExprNode = build_decs_inner_for_pruned(br, tupName, inner, at)
        var loopExpr : Expression?
        if (loopShape is FindReturn) {
            loopExpr = qmacro_expr() {
                return for_each_archetype_find($e(br.reqHashExpr), $e(br.erqExpr), $($i(archName) : Archetype) : bool {
                    $e(forExprNode)
                    return false
                })
            }
        } elif (loopShape is FindReturnNegated) {
            loopExpr = qmacro_expr() {
                return !for_each_archetype_find($e(br.reqHashExpr), $e(br.erqExpr), $($i(archName) : Archetype) : bool {
                    $e(forExprNode)
                    return false
                })
            }
        } elif (loopShape is Find) {
            loopExpr = qmacro_expr() {
                for_each_archetype_find($e(br.reqHashExpr), $e(br.erqExpr), $($i(archName) : Archetype) : bool {
                    $e(forExprNode)
                    return false
                })
            }
        } else {
            loopExpr = qmacro_expr() {
                for_each_archetype($e(br.reqHashExpr), $e(br.erqExpr), $($i(archName) : Archetype) {
                    $e(forExprNode)
                })
            }
        }
        if (empty(prelude)) return loopExpr
        prelude |> push(loopExpr)
        return stmts_to_expr(prelude)
    }
    def override wrap_invoke(var stmts : array<Expression?>; retType : TypeDeclPtr; wrapIter : bool; at : LineInfo) : Expression? {
        var emission : Expression?
        if (retType != null) {
            emission = qmacro(invoke($() : $t(retType) {
                $b(stmts)
            }))
        } else {
            emission = qmacro(invoke($() {
                $b(stmts)
            }))
        }
        return finalize_decs_emission(emission, at, wrapIter)
    }
}

class DecsJoinAdapter : SourceAdapter {
    shape : DecsJoinShape
    def override name_prefix() : string {
        return "djoin_"
    }
    def override bind_name(at : LineInfo) : string {
        return shape.resBindName
    }
    def override element_type() : TypeDeclPtr {
        return clone_type(shape.resultType)
    }
    def override wrap_source_loop(loopShape : LoopDispatch; var body : Expression?; at : LineInfo) : Expression? {
        let jhashName = qn("djoin_jhash", at)
        let arrName   = qn("djoin_jarr", at)
        let tupAName  = qn("djoin_jta", at)
        let tupBName  = qn("djoin_jtb", at)
        let bridgeA = shape.bridgeA
        let bridgeB = shape.bridgeB
        var tupBindA = build_decs_tup_bind(bridgeA, tupAName, at)
        var tupBindB = build_decs_tup_bind(bridgeB, tupBName, at)
        if (tupBindA == null || tupBindB == null) {
            return null
        }
        var pieces = build_join_adapter_pieces(shape.keyaLam, shape.keybLam, shape.resultLam, tupAName, tupBName, jhashName, arrName, shape.resBindName, shape.tupBType, shape.resultType, body, at)
        if (pieces == null) {
            return null
        }
        var collectInner = build_decs_inner_for(bridgeB, tupBindB, pieces.collectInner, at)
        var probeInner = build_decs_inner_for(bridgeA, tupBindA, pieces.probeOuter, at)
        let keyType = shape.keyType
        let tupBType = shape.tupBType
        return <- qmacro_block() {
            var $i(jhashName) : table<$t(keyType); array<$t(tupBType)>>
            for_each_archetype($e(bridgeB.reqHashExpr), $e(bridgeB.erqExpr), $($i(bridgeB.archName) : Archetype) {
                $e(collectInner)
            })
            for_each_archetype($e(bridgeA.reqHashExpr), $e(bridgeA.erqExpr), $($i(bridgeA.archName) : Archetype) {
                $e(probeInner)
            })
        }
    }
    def override wrap_invoke(var stmts : array<Expression?>; retType : TypeDeclPtr; wrapIter : bool; at : LineInfo) : Expression? {
        var emission : Expression?
        if (retType != null) {
            emission = qmacro(invoke($() : $t(retType) {
                $b(stmts)
            }))
        } else {
            emission = qmacro(invoke($() {
                $b(stmts)
            }))
        }
        return finalize_decs_emission(emission, at, wrapIter)
    }
}

[macro_function]
def private make_decs_term_adapter(var bridge : DecsBridgeShape?; var chainInfo : DecsChainInfo?;
                                   var rangeInfo : DecsRangeInfo?; var calls : array<tuple<ExprCall?; LinqCall?>>;
                                   intermediateEnd : int) : SourceAdapter? {
    // var params so the const-bound context pointers can land in the adapter's non-const fields.
    return new DecsAdapter(bridge = bridge, tupName = chainInfo.tupName, chainInfo = chainInfo,
                           calls <- calls, intermediateEnd = intermediateEnd, rangeInfo = rangeInfo)
}

[macro_function]
def emit_loop_or_count_lane_decs(var bridge : DecsBridgeShape?; tupName : string; var c : Captures; var ctx : EmitCtx; at : LineInfo) : Expression? {
    let hasTerm = c.single |> key_exists("term")
    var termCall : ExprCall?
    var termName : string
    if (hasTerm) {
        termCall = c.single["term"]
        termName = c.single_name["term"]
    }
    // D5 fast path: bare `count()` (1-arg, no chain) → arch.size sum, no entity walk.
    let noRangeOps = (!(c.single |> key_exists("skip")) && !(c.single |> key_exists("skip_while"))
        && !(c.single |> key_exists("take_while")) && !(c.single |> key_exists("take")))
    if (hasTerm && termName == "count" && length(termCall.arguments) == 1
            && (c.many["head"] |> empty) && noRangeOps) return ctx.src->count_shortcut("count", at)
    // Reconstruct calls = head + range ops (canonical order) + term, so existing helpers (compute_decs_chain_info, extract_decs_ranges) work unchanged. LinqCall records come from the linqCalls registry by call_norm_name; addr() to convert ref→pointer matches flatten_linq's pattern.
    var calls : array<tuple<ExprCall?; LinqCall?>>
    calls |> reserve(length(c.many["head"]) + 5)        // head + up to 4 range slots + term
    for (call in c.many["head"]) {
        let nm = call_norm_name(call)
        if (!(linqCalls |> key_exists(nm))) return null
        var lc & = unsafe(linqCalls?[nm] ?? default<LinqCall>)
        calls |> push((call, unsafe(addr(lc))))
    }
    let rangeSlots = ["skip", "skip_while", "take_while", "take"]
    for (sn in rangeSlots) {
        if (c.single |> key_exists(sn)) {
            let scall = c.single[sn]
            let snm = call_norm_name(scall)
            if (!(linqCalls |> key_exists(snm))) return null
            var lc & = unsafe(linqCalls?[snm] ?? default<LinqCall>)
            calls |> push((scall, unsafe(addr(lc))))
        }
    }
    if (hasTerm) {
        if (!(linqCalls |> key_exists(termName))) return null
        var lc & = unsafe(linqCalls?[termName] ?? default<LinqCall>)
        calls |> push((termCall, unsafe(addr(lc))))
    }
    let intermediateEnd = hasTerm ? length(calls) - 1 : length(calls)
    var rangeInfo = extract_decs_ranges(calls, intermediateEnd, tupName, at)
    if (rangeInfo == null) return null
    var chainInfo = compute_decs_chain_info(calls, rangeInfo.chainEnd, tupName, bridge, at)
    if (chainInfo == null) return null
    if (c.single |> key_exists("post_take_where")) {
        var ptwArg = c.single["post_take_where"].arguments[1]
        if (ptwArg == null) return null
        rangeInfo.postTakeWhereCond = peel_lambda_rename_var(ptwArg, chainInfo.finalBind)
        if (rangeInfo.postTakeWhereCond == null) return null
    }
    // Terminator classification mirrors plan_decs_unroll imperative (linq_fold.das:5781-5791); differs from classify_terminator's 4-lane split because decs has dedicated min_max_by / walk / element_at emit fns with hoisted state.
    let isAccum = (termName == "count" || termName == "long_count" || termName == "sum"
        || termName == "min" || termName == "max" || termName == "average")
    let isEarlyExit = (termName == "first" || termName == "first_or_default"
        || termName == "any" || termName == "all" || termName == "contains")
    let isMinMaxBy = termName == "min_by" || termName == "max_by"
    let isWalk = (termName == "last" || termName == "last_or_default"
        || termName == "single" || termName == "single_or_default"
        || termName == "aggregate")
    let isElementAt = termName == "element_at" || termName == "element_at_or_default"
    if (isAccum) {
        let finalBind = chainInfo.finalBind
        var elementType = clone_type(chainInfo.finalType)
        let accName = qn("decs_acc", at)
        let names <- make_range_names(at)
        var adapter = make_decs_term_adapter(bridge, chainInfo, rangeInfo, calls, rangeInfo.chainEnd)
        if (termName == "count" || termName == "long_count") {
            // count / long_count → counter lane. The chain (incl. side-effecting selects) runs in
            // wrap_source_loop; the body just increments. _count(pred) and post-take-where gate the bump.
            var loopBody : Expression? = qmacro_expr() {
                $i(accName) ++
            }
            if (length(termCall.arguments) > 1) {
                var predExpr = peel_lambda_rename_var(termCall.arguments[1], finalBind)
                if (predExpr == null) return null
                loopBody = wrap_with_condition(loopBody, predExpr)
            }
            loopBody = wrap_with_condition(loopBody, clone_expression(rangeInfo.postTakeWhereCond))
            return emit_counter_lane(adapter, accName, finalBind, names, null, null, null, loopBody, at, termName == "long_count")
        }
        // sum / min / max / average require a projecting select (no sense summing the raw tuple).
        if (chainInfo.selectCount == 0 || elementType == null) return null
        var noBinds : array<Expression?>
        var noPre : array<Expression?>
        return emit_accumulator_lane(termName, adapter, null, null, clone_expression(rangeInfo.postTakeWhereCond),
            noBinds, noPre, elementType, accName, finalBind, names, null, null, null, null, at)
    }
    if (isMinMaxBy) return emit_decs_min_max_by(bridge, termName, chainInfo, rangeInfo, calls, rangeInfo.chainEnd, termCall, at)
    if (isEarlyExit || isWalk || isElementAt) {
        // No-predicate `last()` / `last_or_default()` over indexable decs sources: read the last
        // archetype's [size-1] directly (O(num_archetypes)), behavior-identical to the walk. Predicated/
        // chained/ranged last can't random-index → keeps the walk. Gate: empty chain, no ranges, indexable.
        if ((termName == "last" || termName == "last_or_default")
                && rangeInfo.chainEnd == 0
                && rangeInfo.skipExpr == null && rangeInfo.takeExpr == null
                && rangeInfo.skipWhileCond == null && rangeInfo.takeWhileCond == null
                && rangeInfo.postTakeWhereCond == null
                && chainInfo.selectCount == 0
                && decs_can_random_index(bridge)) {
            var fast = emit_decs_last_random_index(bridge, termName, termCall, chainInfo.finalType, at)
            if (fast != null) return fast
        }
        // first/first_or_default/any/all/contains/last/single/single_or_default/aggregate/element_at — the
        // shared early-exit lane. The DecsAdapter walks (chain+ranges+for_each{,_find}); the lane supplies the
        // per-op !supports_direct_return state path + the adapter's effective dispatch (Each→Find under take).
        var elementType = clone_type(chainInfo.finalType)
        let names <- make_range_names(at)
        var adapter = make_decs_term_adapter(bridge, chainInfo, rangeInfo, calls, rangeInfo.chainEnd)
        var noBinds : array<Expression?>
        var noPre : array<Expression?>
        return emit_early_exit_lane(termName, adapter, null, null, clone_expression(rangeInfo.postTakeWhereCond),
            noBinds, noPre, elementType, termCall, chainInfo.finalBind, names, null, null, null, null, at)
    }
    // Implicit to_array: only when outer expr was array-typed (caller wrote `.to_array()`, peeled by linqCalls skip=true). Iterator-typed chains cascade to tier-2 here, matches imperative plan_decs_unroll line 5813.
    if (ctx.expr_is_iterator) return null
    // to_array → the shared fold-array lane: buf decl + push_clone(finalBind), tail moves it out.
    let finalBind = chainInfo.finalBind
    var elemType = clone_type(chainInfo.finalType)
    let bufName = qn("decs_buf", at)
    var adapter = make_decs_term_adapter(bridge, chainInfo, rangeInfo, calls, rangeInfo.chainEnd)
    var pushExpr : Expression? = qmacro_expr() {
        $i(bufName) |> push_clone($i(finalBind))
    }
    pushExpr = wrap_with_condition(pushExpr, clone_expression(rangeInfo.postTakeWhereCond))
    var prologueStmts : array<Expression?>
    var reserveStmts : array<Expression?>
    var preCondStmts : array<Expression?>
    var tailStmts : array<Expression?>
    tailStmts |> push <| qmacro_expr() {
        return <- $i(bufName)
    }
    var retType = new TypeDecl(baseType = Type.tArray)
    retType.firstType = clone_type(elemType)
    return emit_fold_array_lane(FoldArraySpec(
        prologueStmts <- prologueStmts,
        bufElemType = elemType,
        bufName = bufName,
        reserveStmts <- reserveStmts,
        preCondStmts <- preCondStmts,
        perElementPush = pushExpr,
        tailStmts <- tailStmts,
        outElemType = retType,
        wrapIter = false,
        dispatch = adapter->effective_dispatch(LoopDispatch(Each = null))), adapter, at)
}

// Decs `reverse |> take(N) [|> _select(F)] |> to_array` skip-into-tail: 2-pass walk (arch.size sum, then for_each_archetype_find with whole-arch-skip + partial-arch skip-counter + early-exit). Preserves PR #2834's 5.2× gain. When terminalSelectLam != null, projects the K reversed elements at the end (K push_clones of projection vs N push_clones of full tuple in the catch-all path).
// `terminalSelectElemType` must be the typer-resolved element type of the termsel call result (caller reads it from `c.single["termsel"]._type.firstType`) — peel_lambda_rename_var's invoke-fallback shape for non-peelable lambdas has a null `_type` at macro stage, so deriving projType from the peeled body would mis-classify those as untyped and bail. Caller is responsible for ensuring termselElemType is non-null before this fast-path is selected; otherwise the gate falls through to the catch-all which has its own termsel handling.
[macro_function]
def emit_decs_reverse_skip_into_tail(var takeExpr : Expression?; bridge : DecsBridgeShape?; tupName : string; needIterWrap : bool; var terminalSelectLam : Expression?; terminalSelectElemType : TypeDeclPtr; at : LineInfo) : Expression? {
    let bufName = qn("buf", at)
    let takeNName = qn("take_n", at)
    let totalName = qn("decs_total", at)
    let actualName = qn("decs_actual", at)
    let skipName = qn("decs_skip", at)
    let seenName = qn("decs_seen", at)
    let skipsLeftName = qn("decs_skips", at)
    let projElemName = qn("decs_rts_e", at)
    let projBufName = qn("decs_rts_buf", at)
    let archName = bridge.archName
    var bufElemType = strip_const_ref(clone_type(bridge.elementType))
    var projBody : Expression?
    var projType : TypeDeclPtr
    if (terminalSelectLam != null) {
        projBody = peel_lambda_rename_var(terminalSelectLam, projElemName)
        if (projBody == null) return null
        projType = strip_const_ref(clone_type(terminalSelectElemType))
        if (projType == null) return null
    }
    // Per-archetype tail collection. Indexable sources (get_ro): random-index the boundary archetype's
    // tail [skipsLeft .. arch.size) instead of continue-walking its head (O(K), not O(N)). Iterator
    // sources (default-init field → get_default_ro): keep the walk-and-skip. Both fill buf forward.
    var collectExpr : Expression?
    if (decs_can_random_index(bridge)) {
        collectExpr = build_decs_index_collect(bridge, bufName, skipsLeftName, actualName, at)
    } else {
        let tupBind = build_decs_tup_bind(bridge, tupName, at)
        var innerBody : Expression? = qmacro_block() {
            if ($i(skipsLeftName) > 0_l) {
                $i(skipsLeftName) --
                continue
            }
            $e(tupBind)
            $i(bufName) |> push_clone($i(tupName))
            if (long_length($i(bufName)) >= $i(actualName)) {
                break
            }
        }
        var clonedForExpr = clone_expression(bridge.forExpr)
        var clonedFor = clonedForExpr as ExprFor
        var newForBody = new ExprBlock(at = at)
        newForBody.list |> push(innerBody)
        clonedFor.body = newForBody
        collectExpr = clonedForExpr
    }
    var emission : Expression?
    if (terminalSelectLam == null) {
        emission = qmacro(invoke($() : array<$t(bufElemType)> {
            var $i(totalName) = 0_l
            for_each_archetype($e(bridge.reqHashExpr), $e(bridge.erqExpr), $($i(archName) : Archetype) {
                $i(totalName) += $i(archName).size
            })
            let $i(takeNName) = $e(takeExpr)
            let $i(actualName) = ($i(takeNName) <= 0) ? 0_l : ((int64($i(takeNName)) < $i(totalName)) ? int64($i(takeNName)) : $i(totalName))
            let $i(skipName) = $i(totalName) - $i(actualName)
            var $i(bufName) : array<$t(bufElemType)>
            if ($i(actualName) == 0_l) {
                return <- $i(bufName)
            }
            $i(bufName) |> reserve(int($i(actualName)))
            var $i(seenName) = 0_l
            for_each_archetype_find($e(bridge.reqHashExpr), $e(bridge.erqExpr), $($i(archName) : Archetype) : bool {
                if ($i(seenName) + $i(archName).size <= $i(skipName)) {
                    $i(seenName) += $i(archName).size
                    return false
                }
                var $i(skipsLeftName) = ($i(skipName) > $i(seenName)) ? ($i(skipName) - $i(seenName)) : 0_l
                $e(collectExpr)
                $i(seenName) += $i(archName).size
                return long_length($i(bufName)) >= $i(actualName)
            })
            _::reverse_inplace($i(bufName))
            return <- $i(bufName)
        }))
    } else {
        emission = qmacro(invoke($() : array<$t(projType)> {
            var $i(totalName) = 0_l
            for_each_archetype($e(bridge.reqHashExpr), $e(bridge.erqExpr), $($i(archName) : Archetype) {
                $i(totalName) += $i(archName).size
            })
            let $i(takeNName) = $e(takeExpr)
            let $i(actualName) = ($i(takeNName) <= 0) ? 0_l : ((int64($i(takeNName)) < $i(totalName)) ? int64($i(takeNName)) : $i(totalName))
            let $i(skipName) = $i(totalName) - $i(actualName)
            var $i(bufName) : array<$t(bufElemType)>
            var $i(projBufName) : array<$t(projType)>
            if ($i(actualName) == 0_l) {
                return <- $i(projBufName)
            }
            $i(bufName) |> reserve(int($i(actualName)))
            var $i(seenName) = 0_l
            for_each_archetype_find($e(bridge.reqHashExpr), $e(bridge.erqExpr), $($i(archName) : Archetype) : bool {
                if ($i(seenName) + $i(archName).size <= $i(skipName)) {
                    $i(seenName) += $i(archName).size
                    return false
                }
                var $i(skipsLeftName) = ($i(skipName) > $i(seenName)) ? ($i(skipName) - $i(seenName)) : 0_l
                $e(collectExpr)
                $i(seenName) += $i(archName).size
                return long_length($i(bufName)) >= $i(actualName)
            })
            _::reverse_inplace($i(bufName))
            $i(projBufName) |> reserve(length($i(bufName)))
            for ($i(projElemName) in $i(bufName)) {
                $i(projBufName) |> push_clone($e(projBody))
            }
            return <- $i(projBufName)
        }))
    }
    return finalize_decs_emission(emission, at, needIterWrap)
}

[macro_function]
def build_decs_tup_bind(bridge : DecsBridgeShape?; tupName : string; at : LineInfo) : Expression? {
    // Named-tuple bind: `var tup = (n1=iter1, n2=iter2, ...)` — peel_lambda_rename_var peels user lambdas substituting `_.userName` → `tup.userName`.
    var mkTup = new ExprMakeTuple(at = at)
    mkTup.recordNames |> resize(length(bridge.userNames))
    for (i in 0 .. length(bridge.userNames)) {
        mkTup.recordNames[i] := bridge.userNames[i]
        mkTup.values |> emplace_new(new ExprVar(at = at, name := bridge.iterNames[i]))
    }
    return qmacro_expr() {
        var $i(tupName) = $e(mkTup)
    }
}

[macro_function]
def decs_can_random_index(bridge : DecsBridgeShape?) : bool {
    // Random-index needs every component source to be an indexable array (get_ro). A [decs_template]
    // field carrying a default-init compiles to get_default_ro → an iterator (not indexable) → bail.
    if (bridge == null || !(bridge.forExpr is ExprFor)) return false
    var fe = bridge.forExpr as ExprFor
    if (empty(fe.sources)) return false
    for (s in fe.sources) {
        if (s == null || s._type == null || s._type.baseType != Type.tArray) return false
    }
    return true
}

[macro_function]
def build_decs_index_tup(bridge : DecsBridgeShape?; idxName : string; at : LineInfo) : Expression? {
    // Named tuple `(userName_i = get_ro(arch, comp_i, def)[idx], ...)` — the bridge's component sources,
    // indexed directly. get_ro is [unsafe_outside_of_for]; mark each source + the index read alwaysSafe
    // (mirrors decs_boost append_index_lookup) so no `unsafe` wrap is needed.
    var fe = bridge.forExpr as ExprFor
    var mkTup = new ExprMakeTuple(at = at)
    mkTup.recordNames |> resize(length(bridge.userNames))
    for (i in 0 .. length(bridge.userNames)) {
        mkTup.recordNames[i] := bridge.userNames[i]
        var src = clone_expression(fe.sources[i])
        src.genFlags.alwaysSafe = true
        force_generated(src, true)
        var idxExpr : Expression? = qmacro_expr() {
            $e(src)[$i(idxName)]
        }
        idxExpr.genFlags.alwaysSafe = true
        force_generated(idxExpr, true)
        mkTup.values |> emplace_new(idxExpr)
    }
    return mkTup
}

[macro_function]
def build_decs_index_collect(bridge : DecsBridgeShape?; bufName : string; skipsLeftName : string;
                             actualName : string; at : LineInfo) : Expression? {
    // reverse|>take tail collect: random-index [skipsLeft .. arch.size) into buf. Boundary archetype skips
    // its head by indexing past it; later archetypes have skipsLeft==0. get_ro lives inside the idx for-loop.
    let archName = bridge.archName
    let idxName = qn("decs_rix", at)
    var tup = build_decs_index_tup(bridge, idxName, at)
    return qmacro_block() {
        for ($i(idxName) in range64($i(skipsLeftName), $i(archName).size)) {
            $i(bufName) |> push_clone($e(tup))
            break if (long_length($i(bufName)) >= $i(actualName))
        }
    }
}

[macro_function]
def emit_decs_last_random_index(bridge : DecsBridgeShape?; termName : string; var termCall : ExprCall?;
                                elemType : TypeDeclPtr; at : LineInfo) : Expression? {
    // No-predicate decs `last()` / `last_or_default()`: read the last non-empty archetype's [size-1]
    // directly instead of walking all N. for_each_archetype visits in order + skips empties, so the last
    // overwrite of res is the global-last (O(num_archetypes)). Caller gates trivial-chain + indexable.
    var bt = strip_const_ref(clone_type(elemType))
    if (bt == null) return null
    let archName = bridge.archName
    let liName = qn("decs_rli", at)
    let foundName = qn("decs_rfound", at)
    let resName = qn("decs_rres", at)
    var tup = build_decs_index_tup(bridge, liName, at)
    var archBody = qmacro_block() {
        let $i(liName) = $i(archName).size - 1_l   // arch.size is int64 — no int truncation for >INT_MAX archetypes
        $i(resName) := $e(tup)
        $i(foundName) = true
    }
    var emission : Expression?
    if (termName == "last") {
        emission = qmacro(invoke($() : $t(bt) {
            var $i(foundName) = false
            var $i(resName) = default<$t(bt)>
            for_each_archetype($e(bridge.reqHashExpr), $e(bridge.erqExpr), $($i(archName) : Archetype) {
                $e(archBody)
            })
            if (!$i(foundName)) {
                panic("sequence contains no elements")
            }
            return <- $i(resName)
        }))
    } else {
        // last_or_default: bind the user default EAGERLY before iteration (matches the walk lane's
        // emit_default_bind, linq_fold_common) so its side effects fire even on a non-empty query;
        // early-return it when empty. Lazy tail-only eval would diverge from the walk / iterator fallback.
        let dvalName = qn("decs_rdval", at)
        let dvCanCopy = (termCall.arguments[1]._type != null && termCall.arguments[1]._type.canCopy)
        var bindStmt = emit_default_bind(dvalName, termCall.arguments[1], dvCanCopy)
        var retStmt = emit_default_return(dvalName, dvCanCopy)
        emission = qmacro(invoke($() : $t(bt) {
            var $i(foundName) = false
            var $i(resName) = default<$t(bt)>
            $e(bindStmt)
            for_each_archetype($e(bridge.reqHashExpr), $e(bridge.erqExpr), $($i(archName) : Archetype) {
                $e(archBody)
            })
            if (!$i(foundName)) {
                $e(retStmt)
            }
            return <- $i(resName)
        }))
    }
    return finalize_decs_emission(emission, at, false)
}

[macro_function]
def build_decs_inner_for(bridge : DecsBridgeShape?; var tupBind : Expression?; var body : Expression?; at : LineInfo) : Expression? {
    // Clone bridge's inner multi-iter for-loop; substitute body with [tupBind, body]. Reuses bridge.archName so cloned get_ro sources stay valid.
    var forBodyStmts <- [tupBind, body]
    var forBody = stmts_to_expr(forBodyStmts)
    var clonedForExpr = clone_expression(bridge.forExpr)
    var clonedFor = clonedForExpr as ExprFor
    var newForBody = new ExprBlock(at = at)
    newForBody.list |> push(forBody)
    clonedFor.body = newForBody
    return clonedForExpr
}

// Walks a chain body to collect which fields of the decs_tup named-tuple bind are actually referenced. allFieldsUsed flips when the bind is referenced as a whole-var (not via field access) — e.g. push_clone(decs_tup) in a bare to_array, or pass-to-user-fn — so the helper falls back to unpruned emission and preserves the user-visible output shape. Also picks up bare iter-var refs (when `iterToUser` is non-empty) so the trivial-let-elision path — which rewrites `decs_sel_N`-bound projections to the iter var directly, bypassing `decs_tup` entirely — still hits the pruned arm instead of falling through to unpruned-default.
class DecsTupUsageScanner : AstVisitor {
    tupName       : string
    iterToUser    : table<string; string>
    usedFields    : table<string>
    allFieldsUsed : bool = false
    inTargetField : bool = false
    def DecsTupUsageScanner(n : string; var i2u : table<string; string>) {
        tupName = n
        iterToUser <- i2u
    }
    def override preVisitExprField(expr : ExprField?) : void {
        var v = expr.value
        if (v is ExprRef2Value) {
            v = (v as ExprRef2Value).subexpr
        }
        if (v is ExprVar && (v as ExprVar).name == tupName) {
            usedFields |> insert(string(expr.name))
            inTargetField = true
        }
    }
    def override visitExprField(var expr : ExprField?) : ExpressionPtr {
        inTargetField = false
        return expr
    }
    def override preVisitExprVar(expr : ExprVar?) : void {
        if (expr.name == tupName && !inTargetField) {
            allFieldsUsed = true
            return
        }
        let nm = string(expr.name)
        if (key_exists(iterToUser, nm)) {
            usedFields |> insert(iterToUser[nm])
        }
    }
}

[macro_function]
def collect_decs_tup_usage(var e : Expression?; tupName : string; bridge : DecsBridgeShape?) : tuple<bool; array<string>> {
    // Returns (allFieldsUsed, usedFieldNames). usedFieldNames is unordered — caller filters bridge.userNames in original order to preserve get_ro emission order. The bridge feeds an iter-var → user-name reverse map so bare iter-var refs (from trivial-let elision) count as field usage too.
    var allUsed = false
    var used : array<string>
    if (e == null) return (allUsed, used)
    var iterToUser <- {for (i in 0 .. length(bridge.iterNames)); bridge.iterNames[i] => bridge.userNames[i]}
    var sc = new DecsTupUsageScanner(tupName, iterToUser)
    make_visitor(*sc) $(astVisitorAdapter) {
        visit_expression(e, astVisitorAdapter)
    }
    allUsed = sc.allFieldsUsed
    for (k in keys(sc.usedFields)) {
        used |> push(k)
    }
    unsafe {
        delete sc
    }
    return (allUsed, used)
}

// Rewrites every whole-var ref to `decs_tup` into a freshly synthesized `(userName1 = iterVar1, ...)` named-tuple literal — same shape build_decs_tup_bind produces, but inlined at each use site. Lets emitters that store the whole element (last/single/aggregate seed of element type, min_by/max_by, to_array push_clone) avoid forcing the unpruned bind path: after rewrite the body contains only `decs_tup.<field>` refs (flattenable) plus iter-var refs in the synthesized literals (picked up by the usage scanner). build_decs_inner_for_pruned then flattens + drops the bind entirely. Inside ExprField the inner ExprVar is left alone so the field path still matches.
class DecsTupWholeRewriter : AstVisitor {
    tupName       : string
    userNames     : array<string>
    iterNames     : array<string>
    inTargetField : bool = false
    rewroteAny    : bool = false
    def DecsTupWholeRewriter(t : string; var un, in_ : array<string>) {
        tupName = t
        userNames <- un
        iterNames <- in_
    }
    def override preVisitExprField(expr : ExprField?) : void {
        var v = expr.value
        if (v is ExprRef2Value) {
            v = (v as ExprRef2Value).subexpr
        }
        if (v is ExprVar && (v as ExprVar).name == tupName) {
            inTargetField = true
        }
    }
    def override visitExprField(var expr : ExprField?) : ExpressionPtr {
        inTargetField = false
        return expr
    }
    def override visitExprVar(var expr : ExprVar?) : ExpressionPtr {
        if (expr.name != tupName || inTargetField) return expr
        var mkTup = new ExprMakeTuple(at = expr.at)
        mkTup.recordNames |> resize(length(userNames))
        for (i in 0 .. length(userNames)) {
            mkTup.recordNames[i] := userNames[i]
            mkTup.values |> emplace_new(new ExprVar(at = expr.at, name := iterNames[i]))
        }
        rewroteAny = true
        return mkTup
    }
}

[macro_function]
def rewrite_decs_tup_wholes(var body : Expression?; tupName : string; bridge : DecsBridgeShape?) : bool {
    if (body == null) return false
    var unCopy <- [for (n in bridge.userNames); n]
    var inCopy <- [for (n in bridge.iterNames); n]
    var rw = new DecsTupWholeRewriter(tupName, unCopy, inCopy)
    make_visitor(*rw) $(astVisitorAdapter) {
        visit_expression(body, astVisitorAdapter)
    }
    let any = rw.rewroteAny
    unsafe {
        delete rw
    }
    return any
}

// When the body only references `decs_tup` via field access (no whole-var refs), the named-tuple wrap plus every per-element `decs_tup.<field>` read are pure overhead — rewrite each field access to the matching iter var directly and skip the bind. Drops one tuple-make + one field-read per kept field per iteration.
class DecsTupFieldFlattener : AstVisitor {
    tupName     : string
    fieldToIter : table<string; string>
    def DecsTupFieldFlattener(t : string; var f : table<string; string>) {
        tupName = t
        fieldToIter <- f
    }
    def override visitExprField(var expr : ExprField?) : ExpressionPtr {
        var v = expr.value
        if (v is ExprRef2Value) {
            v = (v as ExprRef2Value).subexpr
        }
        if (v is ExprVar && (v as ExprVar).name == tupName) {
            let fname = string(expr.name)
            if (key_exists(fieldToIter, fname)) return new ExprVar(at = expr.at, name := fieldToIter[fname])
        }
        return expr
    }
}

[macro_function]
def flatten_decs_tup_to_iter(var body : Expression?; tupName : string; var fieldToIter : table<string; string>) : void {
    if (body == null) return
    var fl = new DecsTupFieldFlattener(tupName, fieldToIter)
    make_visitor(*fl) $(astVisitorAdapter) {
        visit_expression(body, astVisitorAdapter)
    }
    unsafe {
        delete fl
    }
}

[macro_function]
def build_decs_inner_for_pruned(bridge : DecsBridgeShape?;
                                tupName : string;
                                var body : Expression?;
                                at : LineInfo) : Expression? {
    // Walks body for `tupName.<field>` references; when pruning is safe + beneficial, emits the inner multi-iter for with unused get_ro slots dropped and a matching shrunk named-tuple bind. Otherwise falls through to the unpruned path so user-visible shape stays intact (bare to_array, push_clone(decs_tup), pass-to-user-fn).
    var (allUsed, usedNames) = collect_decs_tup_usage(body, tupName, bridge)
    // Whole-var refs (`lst := decs_tup`, `push_clone(buf, decs_tup)`, etc) would force the unpruned bind path — but they're all semantically equivalent to a synthesized `(userName1=iter1, ...)` literal. Rewrite them inline so the flatten + bind-elision path unlocks; saves the per-iteration `var decs_tup = (...)` wrap (~12 ns/op on last/single/select_where 100K-row benches).
    if (allUsed && rewrite_decs_tup_wholes(body, tupName, bridge)) {
        var (allUsed2, usedNames2) = collect_decs_tup_usage(body, tupName, bridge)
        allUsed = allUsed2
        usedNames <- usedNames2
    }
    // Fallback to the unpruned bind ONLY for whole-var refs that survived rewrite (pass-to-user-fn) or the edge case where the body never touches decs_tup at all. The "all fields used via field access" case still benefits from flatten + bind elision — no slots dropped, but the per-iter tuple-make and field reads disappear.
    if (allUsed || empty(usedNames)) {
        var tupBind = build_decs_tup_bind(bridge, tupName, at)
        return build_decs_inner_for(bridge, tupBind, body, at)
    }
    var usedSet : table<string>
    for (n in usedNames) {
        usedSet |> insert(n)
    }
    var fieldToIter <- {for (i in 0 .. length(bridge.userNames));
        bridge.userNames[i] => bridge.iterNames[i];
        where key_exists(usedSet, bridge.userNames[i])}
    // Diagnostic-friendly fallback when some `usedNames` entry has no bridge match. Today the typer pipeline checks user `_.<field>` lambdas before plan_decs_unroll runs, so this is unreachable in practice — but if a future macro ever synthesizes `decs_tup.<unknownField>`, the unpruned bind keeps the typer's "unknown field of <named-tuple>" diagnostic intact instead of degrading it to "unknown identifier decs_tup".
    if (length(fieldToIter) < length(usedSet)) {
        var tupBind = build_decs_tup_bind(bridge, tupName, at)
        return build_decs_inner_for(bridge, tupBind, body, at)
    }
    // Pruned path: no whole-var ref → no need for the named tuple. Rewrite every `decs_tup.<userName>` in the body to the matching iter var, then drop the bind entirely. Eliminates one tuple-make + N per-element field reads per iter.
    flatten_decs_tup_to_iter(body, tupName, fieldToIter)
    // Clone forExpr; drop the iter slots not in usedSet from all 5 parallel dasvectors (sources/iterators/iteratorsAt/iteratorsAka/iteratorsTags). iteratorVariables is cleared so the typer rebuilds it during re-type — same pattern as daslib/soa.das:381.
    var clonedForExpr = clone_expression(bridge.forExpr)
    var clonedFor = clonedForExpr as ExprFor
    let nSlots = length(bridge.userNames)
    for (i in 0 .. nSlots) {
        let revIdx = nSlots - 1 - i
        if (!key_exists(usedSet, bridge.userNames[revIdx])) {
            clonedFor.sources       |> erase(revIdx)
            clonedFor.iterators     |> erase(revIdx)
            clonedFor.iteratorsAt   |> erase(revIdx)
            clonedFor.iteratorsAka  |> erase(revIdx)
            clonedFor.iteratorsTags |> erase(revIdx)
        }
    }
    clonedFor.iteratorVariables |> clear()
    // Install body — body now references iter vars directly; no tupBind needed.
    var newForBody = new ExprBlock(at = at)
    newForBody.list |> push(body)
    clonedFor.body = newForBody
    return clonedForExpr
}

struct DecsChainInfo {
    tupName     : string            // source bind the chain was peeled against — the inner-for bind name; make_decs_term_adapter passes this instead of regenerating qn("decs_tup", at)
    bindAt      : array<string>    // bind name visible at each chain position
    elidedAt    : array<bool>      // true when the select at position i was elided (peeled to a single decs_tup field access → reused iter var as next bind, no `let decs_sel_N = ...` emit needed)
    finalBind   : string            // bind name AFTER full chain — what terminator references
    finalType   : TypeDeclPtr       // element type AFTER full chain (constant + ref stripped)
    selectCount : int               // number of `select` ops in chain; 0 means finalBind == tupName
}

[macro_function]
def compute_decs_chain_info(var calls : array<tuple<ExprCall?; LinqCall?>>;
                            intermediateEnd : int;
                            tupName : string;
                            bridge : DecsBridgeShape?;
                            at : LineInfo) : DecsChainInfo? {
    // Each select introduces a fresh `decs_sel{N}` bind; subsequent ops peel against it. Returns null on any non-where_/select op.
    var info = new DecsChainInfo(
        tupName := tupName,
        finalBind := tupName,
        finalType = clone_type(bridge.elementType),
        selectCount = 0
    )
    info.bindAt |> reserve(intermediateEnd)
    info.elidedAt |> reserve(intermediateEnd)
    var curBind = tupName
    var curType : TypeDeclPtr = clone_type(bridge.elementType)
    for (i in 0 .. intermediateEnd) {
        info.bindAt |> push(curBind)
        info.elidedAt |> push(false)
        var cll & = unsafe(calls[i])
        let opName = cll._1.name
        if (opName == "select") {
            info.selectCount ++
            var peeled = peel_lambda_rename_var(cll._0.arguments[1], info.bindAt[i])
            if (peeled == null || peeled._type == null) return null
            // Trivial-let elision: `_select(_.userName)` against decs_tup → iter var directly. The pruned-for keeps the iter var (its name = bridge.iterNames[idx]) and the flattener would otherwise rewrite the synthetic `let decs_sel_N = decs_tup.userName` to `let decs_sel_N = <iterVar>` — a pure copy. Renaming finalBind to the iter var name skips the binding entirely and the action references it natively. Typer wraps both the projection root (`peeled` may be ExprRef2Value) AND the ExprVar inside the ExprField in ExprRef2Value — peel both.
            var elided = false
            if (curBind == tupName) {
                var top = peeled
                if (top is ExprRef2Value) {
                    top = (top as ExprRef2Value).subexpr
                }
                if (top is ExprField) {
                    var pf = top as ExprField
                    var pv = pf.value
                    if (pv is ExprRef2Value) {
                        pv = (pv as ExprRef2Value).subexpr
                    }
                    if (pv is ExprVar && (pv as ExprVar).name == tupName) {
                        let fname = string(pf.name)
                        for (ui in 0 .. length(bridge.userNames)) {
                            if (bridge.userNames[ui] == fname) {
                                curBind = bridge.iterNames[ui]
                                elided = true
                                info.elidedAt[i] = true
                                break
                            }
                        }
                    }
                }
            }
            if (!elided) {
                curBind = "`decs_sel`{at.line}`{at.column}`{info.selectCount}"
            }
            curType = clone_type(peeled._type)
        } elif (opName != "where_") return null
    }
    info.finalBind := curBind
    info.finalType = curType
    if (info.finalType != null) {
        info.finalType.flags.constant = false
        info.finalType.flags.ref = false
    }
    return info
}

[macro_function, unused_argument(at)]
def wrap_decs_chain(var action : Expression?;
                    chainInfo : DecsChainInfo?;
                    var calls : array<tuple<ExprCall?; LinqCall?>>;
                    intermediateEnd : int;
                    at : LineInfo) : Expression? {
    // Reverse-walk chain, wrapping action with `if (pred) { ... }` for where_ and `let bindN+1 = proj; ...` for select.
    var current = action
    for (rev in 0 .. intermediateEnd) {
        let i = intermediateEnd - 1 - rev
        var cll & = unsafe(calls[i])
        let opName = cll._1.name
        let bindHere = chainInfo.bindAt[i]
        if (opName == "where_") {
            var pred = peel_lambda_rename_var(cll._0.arguments[1], bindHere)
            if (pred == null) return null
            current = qmacro_expr() {
                if ($e(pred)) {
                    $e(current)
                }
            }
        } elif (opName == "select") {
            // Skip emission entirely when this select was elided in compute_decs_chain_info — the next bind is already the iter var the flattener would have produced for `let nextBind = decs_tup.userName`. No copy needed; downstream `current` already references the iter var by the elided name.
            continue if (chainInfo.elidedAt[i])
            var proj = peel_lambda_rename_var(cll._0.arguments[1], bindHere)
            if (proj == null) return null
            let nextBind = (i + 1 < intermediateEnd) ? chainInfo.bindAt[i + 1] : chainInfo.finalBind
            var letStmt = qmacro_expr() {
                let $i(nextBind) = $e(proj)
            }
            var stmts : array<Expression?>
            stmts |> reserve(2)
            stmts |> push(letStmt)
            stmts |> push(current)
            current = stmts_to_expr(stmts)
        } else {
            return null
        }
    }
    return current
}

struct DecsRangeInfo {
    skipExpr          : Expression?
    takeExpr          : Expression?
    skipWhileCond     : Expression?    // Slice 5b: predicate peeled against tupName (source tuple)
    takeWhileCond     : Expression?    // Slice 5b: predicate peeled against tupName (source tuple)
    postTakeWhereCond : Expression?    // gates per-element work AFTER take cap ticks; peeled against chainInfo.finalBind (mirrors array-side postTakeWhereCond, Theme 2 5c)
    chainEnd          : int
}

struct DecsRangeStateNames {
    skipName      : string
    takeCountName : string
    takeLimitName : string    // decs adds an explicit take-limit local so the take(N) expression evaluates exactly once at invoke entry.
    skippingName  : string
}

[macro_function]
def make_decs_range_names(at : LineInfo) : DecsRangeStateNames {
    return <- DecsRangeStateNames(
        skipName      = qn("decs_skip", at),
        takeCountName = qn("decs_takec", at),
        takeLimitName = qn("decs_takel", at),
        skippingName  = qn("decs_skipping", at))
}

[macro_function, unused_argument(at)]
def extract_decs_ranges(var calls : array<tuple<ExprCall?; LinqCall?>>;
                        intermediateEnd : int;
                        tupName : string;
                        at : LineInfo) : DecsRangeInfo? {
    // Slice 5a/5b: split intermediate chain into (where_/select prefix) + (skip? skip_while? take_while? take? suffix). Canonical order matches array-side (linq_fold.das:1607-1634): skip → skip_while → take_while → take, all after any where_. skip_while/take_while predicates use the SOURCE tuple (tupName); bail when a select appears in the prefix (matches array-side seenSelect bail).
    var info = new DecsRangeInfo(chainEnd = intermediateEnd)
    var firstRangeIdx = intermediateEnd
    for (i in 0 .. intermediateEnd) {
        let nm = calls[i]._1.name
        if (nm == "take" || nm == "skip" || nm == "take_while" || nm == "skip_while") {
            firstRangeIdx = i
            break
        }
    }
    if (firstRangeIdx == intermediateEnd) return info
    // Detect whether the suffix contains any predicate-driven range (skip_while/take_while) — if so, prefix must be select-free (mirrors array-side seenSelect bail at linq_fold.das:1615/1623).
    var hasWhilePred = false
    for (i in firstRangeIdx .. intermediateEnd) {
        let nm = calls[i]._1.name
        if (nm == "skip_while" || nm == "take_while") {
            hasWhilePred = true
            break
        }
    }
    if (hasWhilePred) {
        for (i in 0 .. firstRangeIdx) {
            if (calls[i]._1.name == "select") return null
        }
    }
    for (i in firstRangeIdx .. intermediateEnd) {
        var cll & = unsafe(calls[i])
        let nm = cll._1.name
        if (nm == "skip") {
            if (info.skipExpr != null || info.skipWhileCond != null || info.takeWhileCond != null || info.takeExpr != null || length(cll._0.arguments) < 2) return null
            var arg = cll._0.arguments[1]
            if (arg == null || arg._type == null || arg._type.baseType != Type.tInt) return null
            info.skipExpr = clone_expression(arg)
        } elif (nm == "skip_while") {
            if (info.skipWhileCond != null || info.takeWhileCond != null || info.takeExpr != null || length(cll._0.arguments) < 2) return null
            var pred = peel_lambda_rename_var(cll._0.arguments[1], tupName)
            if (pred == null) return null
            info.skipWhileCond = pred
        } elif (nm == "take_while") {
            if (info.takeWhileCond != null || info.takeExpr != null || length(cll._0.arguments) < 2) return null
            var pred = peel_lambda_rename_var(cll._0.arguments[1], tupName)
            if (pred == null) return null
            info.takeWhileCond = pred
        } elif (nm == "take") {
            if (info.takeExpr != null || length(cll._0.arguments) < 2) return null
            var arg = cll._0.arguments[1]
            if (arg == null || arg._type == null || arg._type.baseType != Type.tInt) return null
            info.takeExpr = clone_expression(arg)
        } else {
            return null
        }
    }
    info.chainEnd = firstRangeIdx
    return info
}

[macro_function]
def wrap_inner_for_with_decs_ranges(var body : Expression?;
                                    rangeInfo : DecsRangeInfo?;
                                    names : DecsRangeStateNames) : Expression? {
    // Slice 5a/5b: prefix per-element body with range guards in canonical order: take-cap → skip-counter → skip_while flag → take_while break → takeC++. Mirrors array-side wrap_with_ranges (linq_fold.das:556). All early exits (`return true`) work both for plain inner-for break AND for the bool-lambda archetype-find — caller switches outer iterator to for_each_archetype_find when takeExpr != null OR takeWhileCond != null. Counters / flags live at invoke scope (decs_range_prelude). Take limit is read from a hoisted local (takeLimitName) so the user's take(N) expression evaluates exactly once at invoke entry.
    if (rangeInfo.skipExpr == null && rangeInfo.takeExpr == null && rangeInfo.skipWhileCond == null && rangeInfo.takeWhileCond == null) return body
    let skipName      = names.skipName
    let takeCountName = names.takeCountName
    let takeLimitName = names.takeLimitName
    let skippingName  = names.skippingName
    var stmts : array<Expression?>
    stmts |> reserve(6)
    if (rangeInfo.takeExpr != null) {
        // `>=` so non-positive N short-circuits on first iteration. `return true` exits both inner for AND the archetype bool lambda.
        stmts |> push <| qmacro_expr() {
            if ($i(takeCountName) >= $i(takeLimitName)) {
                return true
            }
        }
    }
    if (rangeInfo.skipExpr != null) {
        stmts |> push <| qmacro_expr() {
            if ($i(skipName) > 0) {
                $i(skipName) --
                continue
            }
        }
    }
    if (rangeInfo.skipWhileCond != null) {
        stmts |> push <| qmacro_expr() {
            if ($i(skippingName)) {
                if ($e(rangeInfo.skipWhileCond)) {
                    continue
                }
                $i(skippingName) = false
            }
        }
    }
    if (rangeInfo.takeWhileCond != null) {
        // First false ends iteration globally (across all archetypes). `return true` exits inner-for AND archetype-find lambda.
        stmts |> push <| qmacro_expr() {
            if (!($e(rangeInfo.takeWhileCond))) {
                return true
            }
        }
    }
    if (rangeInfo.takeExpr != null) {
        // takeC++ AFTER skip_while / take_while so gated-out elements don't eat the take(N) budget (matches array-side comment at linq_fold.das:558).
        stmts |> push <| qmacro_expr() {
            $i(takeCountName) ++
        }
    }
    stmts |> push(body)
    return stmts_to_expr(stmts)
}

[macro_function]
def decs_range_prelude(rangeInfo : DecsRangeInfo?; names : DecsRangeStateNames) : array<Expression?> {
    // Counter / flag init statements hoisted ABOVE the for_each_archetype call so they survive across archetypes (take cap, skip count, skip_while flag are all globally-stateful). takeLimit hoisted to a local so the user's take(N) expression evaluates exactly once (matches take(src, total:int) semantics).
    let skipName      = names.skipName
    let takeCountName = names.takeCountName
    let takeLimitName = names.takeLimitName
    let skippingName  = names.skippingName
    var stmts : array<Expression?>
    if (rangeInfo.skipExpr != null) {
        stmts |> push <| qmacro_expr() {
            var $i(skipName) = $e(rangeInfo.skipExpr)
        }
    }
    if (rangeInfo.takeExpr != null) {
        stmts |> push_from <| qmacro_block_to_array() {
            let $i(takeLimitName) = $e(rangeInfo.takeExpr)
            var $i(takeCountName) = 0
        }
    }
    if (rangeInfo.skipWhileCond != null) {
        // Slice 5b: skip_while flag is one-way (flips false on first non-matching elem, stays). Hoisted so the flag persists across archetypes.
        stmts |> push <| qmacro_expr() {
            var $i(skippingName) = true
        }
    }
    return <- stmts
}

[macro_function]
def emit_decs_count_archsize(bridge : DecsBridgeShape?; at : LineInfo) : Expression? {
    // Bare count(): no chain ops, sum arch.size per archetype — skips the per-entity walk entirely.
    let accName = qn("decs_acc", at)
    let archName = qn("decs_arch", at)
    // arch.size is int64; count() returns int and the += site truncates past INT_MAX — chain `long_count()` when an int64-safe total is required.
    var emission : Expression? = qmacro(invoke($() : int {
        var $i(accName) = 0
        for_each_archetype($e(bridge.reqHashExpr), $e(bridge.erqExpr), $($i(archName) : Archetype) {
            $i(accName) += int($i(archName).size)
        })
        return $i(accName)
    }))
    emission.force_at(at)
    emission.force_generated(true)
    return emission
}

[macro_function]
def emit_decs_min_max_by(var bridge : DecsBridgeShape?;
                         opName : string;
                         var chainInfo : DecsChainInfo?;
                         var rangeInfo : DecsRangeInfo?;
                         var calls : array<tuple<ExprCall?; LinqCall?>>;
                         intermediateEnd : int;
                         terminatorCall : ExprCall?;
                         at : LineInfo) : Expression? {
    // Slice 4: min_by / max_by — track best (key, element) pair across archetypes. Element retained via `:=`, matches min_by_impl's empty→default semantics. Slice 5a adds take/skip ranges via rangeInfo.
    let firstName = qn("decs_first", at)
    let bestKeyName = qn("decs_bkey", at)
    let bestElemName = qn("decs_belem", at)
    let keyBindName = qn("decs_key", at)
    let finalBind = chainInfo.finalBind
    var elemType = clone_type(chainInfo.finalType)
    var keyExpr = peel_lambda_rename_var(terminatorCall.arguments[1], finalBind)
    if (keyExpr == null || keyExpr._type == null) return null
    var keyType = clone_type(keyExpr._type)
    keyType.flags.constant = false
    keyType.flags.ref = false
    let workhorse = keyType.isWorkhorseType
    let opCmp = (opName == "min_by") ? "min" : "max"
    var compareExpr = min_max_compare(workhorse, opCmp, keyBindName, bestKeyName)
    var perElement : Expression? = qmacro_block() {
        let $i(keyBindName) = $e(keyExpr)
        if ($i(firstName)) {
            $i(bestKeyName) := $i(keyBindName)
            $i(bestElemName) := $i(finalBind)
            $i(firstName) = false
        } elif ($e(compareExpr)) {
            $i(bestKeyName) := $i(keyBindName)
            $i(bestElemName) := $i(finalBind)
        }
    }
    var preludeStmts <- qmacro_block_to_array() {
        var $i(firstName) = true
        var $i(bestKeyName) : $t(keyType)
        var $i(bestElemName) : $t(elemType)
    }
    var tailStmts : array<Expression?>
    tailStmts |> push <| qmacro_expr() {
        return $i(bestElemName)
    }
    let dispatch : LoopDispatch = ((rangeInfo.takeExpr != null || rangeInfo.takeWhileCond != null)
        ? LoopDispatch(Find = null)
        : LoopDispatch(Each = null))
    // Each+tail walk over the generic terminator lane: the field-carrying DecsAdapter owns the chain +
    // ranges + for_each{,_find} dispatch; postTakeWhereCond (if any) gates the per-element work innermost.
    var adapter = make_decs_term_adapter(bridge, chainInfo, rangeInfo, calls, intermediateEnd)
    var ptw : Expression?
    if (rangeInfo.postTakeWhereCond != null) {
        ptw = clone_expression(rangeInfo.postTakeWhereCond)
    }
    var perStmts : array<Expression?>
    perStmts |> push(perElement)
    return emit_terminator_lane(TerminatorSpec(
        prologue <- preludeStmts,
        perElementStmts <- perStmts,
        postTakeWhereCond = ptw,
        tailStmts <- tailStmts,
        outElemType = elemType,
        dispatch = dispatch), adapter, at)
}

// ── decs group_by splice (Slice 5e — state-table over for_each_archetype) ───────

[macro_function]
def emit_decs_join_impl(bridgeA : DecsBridgeShape?; var c : Captures; var ctx : EmitCtx; at : LineInfo) : Expression? {
    // PR D2-A: keep collect+probe inline (not via adapter_wrap_source_loop) to preserve the count-no-where bucket-length fast path.
    // Bail to tier-2 unless both sides are from_decs_template eager bridges and it's a plain join — group join
    // over a decs source isn't spliced yet (the array adapter handles group joins).
    if (bridgeA == null || (c.single_name |> key_exists("join") && c.single_name["join"] == "group_join")) return null
    var joinCall = c.single["join"]
    var whereLam : Expression?
    if (c.single |> key_exists("trailing_where")) {
        whereLam = joinCall != null && c.single["trailing_where"] != null ? c.single["trailing_where"].arguments[1] : null
        if (whereLam == null) return null
    }
    var selectLam : Expression?
    if (c.single |> key_exists("trailing_select")) {
        var selCall = c.single["trailing_select"]
        if (selCall == null || selCall._type == null || selCall._type.firstType == null) return null
        selectLam = selCall.arguments[1]
        if (selectLam == null) return null
    }
    var leadWhereLam = extract_join_lead_where(c)
    if (c.single |> key_exists("lead_where") && leadWhereLam == null) return null
    let countOnly = c.single |> key_exists("term")
    // Iterator-typed implicit-to-array: emission returns array<elemType> uniformly; iterator sink would need a wrap that's not applied here. Cascade.
    if (!countOnly && ctx.expr_is_iterator) return null
    var bridgeB = extract_decs_bridge(joinCall.arguments[1])
    if (bridgeB == null) return null
    // keya/keyb are LinqJoin-macro-synthesized: $($i(la_fresh) : iterTypeA) => keya_body — 1-arg single-return. Block return type IS the key type.
    var keyaLam = joinCall.arguments[2]
    var keybLam = joinCall.arguments[3]
    if (keyaLam == null || keybLam == null || keyaLam._type == null) return null
    let keyType = strip_const_ref(clone_type(keyaLam._type.firstType))
    if (!is_primitive_join_key_type(keyType)) return null
    let tupAName = qn("decs_tup_a", at)
    let tupBName = qn("decs_tup_b", at)
    let hashName = qn("decs_hash", at)
    var keyaBody = peel_lambda_rename_var(keyaLam, tupAName)
    var keybBody = peel_lambda_rename_var(keybLam, tupBName)
    if (keyaBody == null || keybBody == null) return null
    var tupBindA = build_decs_tup_bind(bridgeA, tupAName, at)
    var tupBindB = build_decs_tup_bind(bridgeB, tupBName, at)
    if (tupBindA == null || tupBindB == null) return null
    let tupBType = strip_const_ref(clone_type(bridgeB.elementType))
    var pieces = build_join_standalone_pieces(joinCall, whereLam, selectLam, leadWhereLam, countOnly, keyaBody, tupAName, hashName, tupBType, false, "decs", at)
    if (pieces == null) return null
    var collectBody <- qmacro_block_to_array() {
        // nolint:PERF006 per-key bucket size unknown ahead of time
        $i(hashName)[$e(keybBody)] |> push_clone($i(tupBName))
    }
    var collectInner = build_decs_inner_for(bridgeB, tupBindB, stmts_to_expr(collectBody), at)
    var probeInner = build_decs_inner_for(bridgeA, tupBindA, pieces.probeOuter, at)
    var allStmts : array<Expression?>
    allStmts |> push_from(pieces.preludeStmts)
    allStmts |> push <| qmacro_expr() {
        var $i(hashName) : table<$t(keyType); array<$t(tupBType)>>
    }
    allStmts |> push <| qmacro_expr() {
        for_each_archetype($e(bridgeB.reqHashExpr), $e(bridgeB.erqExpr), $($i(bridgeB.archName) : Archetype) {
            $e(collectInner)
        })
    }
    allStmts |> push <| qmacro_expr() {
        for_each_archetype($e(bridgeA.reqHashExpr), $e(bridgeA.erqExpr), $($i(bridgeA.archName) : Archetype) {
            $e(probeInner)
        })
    }
    allStmts |> push(pieces.returnStmt)
    var invokeRetType = pieces.invokeRetType
    var emission = qmacro(invoke($() : $t(invokeRetType) {
        $b(allStmts)
    }))
    return finalize_decs_emission(emission, at, false)
}

// ── array join splice (chunk N+2 — array mirror of register_decs_join_rows) ───────

