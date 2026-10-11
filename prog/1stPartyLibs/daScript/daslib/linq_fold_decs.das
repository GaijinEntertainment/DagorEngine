options gen2
options indenting = 4
options _comment_hygiene = true
options _function_length = 145

module linq_fold_decs shared public

//! linq_fold decs (ECS) source adapters: DecsAdapter / DecsJoinAdapter, the decs-bridge tuple
//! scanners/rewriters, and the decs-specific emit hooks (count_shortcut, reverse-skip-into-tail,
//! join, min_by/max_by). Terminators ride the generic lanes in linq_fold_common via DecsAdapter.
//! Requires linq_fold_common. See daslib/ARCHITECTURE_LINQ.md sec. 37.

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

struct DecsJoinShape {
    bridgeA, bridgeB : DecsBridgeShape?
    keyaLam, keybLam, resultLam : Expression?
    keyType, tupBType, resultType : TypeDeclPtr
    resBindName : string
}

class DecsAdapter : SourceAdapter {
    bridge : DecsBridgeShape?
    tupName : string
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
        if (bridge == null) return null
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
            var core = extract_upstream_join_core(joinCall)
            if (core == null) {
                return null
            }
            return new DecsJoinAdapter(shape = DecsJoinShape(
                bridgeA = bridge,
                bridgeB = bridgeB,
                keyaLam = core.keyaLam,
                keybLam = core.keybLam,
                resultLam = core.resultLam,
                keyType = core.keyType,
                tupBType = strip_const_ref(clone_type(bridgeB.elementType)),
                resultType = core.resultType,
                resBindName = qn("djoin_jres", at)))
        }
        return new DecsAdapter(bridge = bridge, tupName = qn("decs_tup", at))
    }
    def override emit_join_hook(var c : Captures; var ctx : EmitCtx; at : LineInfo) : Expression? {
        return emit_decs_join_impl(bridge, c, ctx, at)
    }
    def override count_shortcut(opName : string; at : LineInfo) : Expression? {
        if (opName != "count") return null
        return emit_decs_count_archsize(bridge, at)
    }
    def override bind_name(at : LineInfo) : string {
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
        let archName = bridge.archName
        let br = bridge
        var inner = body
        var prelude : array<Expression?>
        if (chainInfo != null) {
            let names <- make_decs_range_names(at)
            inner = wrap_inner_for_with_decs_ranges(inner, rangeInfo, names)
            inner = wrap_decs_chain(inner, chainInfo, calls, intermediateEnd, at)
            if (inner == null) return null
            prelude <- decs_range_prelude(rangeInfo, names)  // nolint:PERF030 - fresh local, nothing to release
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
            $e(join_hash_decl(jhashName, keyType, tupBType))
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
    return new DecsAdapter(bridge = bridge, tupName = chainInfo.tupName, chainInfo = chainInfo,
                           calls <- calls, intermediateEnd = intermediateEnd, rangeInfo = rangeInfo)
}

[macro_function]
def emit_loop_or_count_lane_decs(var bridge : DecsBridgeShape?; tupName : string; var c : Captures; var ctx : EmitCtx; at : LineInfo) : Expression? {  // nolint:STYLE037 - one arm per loop-lane shape
    let hasTerm = c.single |> key_exists("term")
    var termCall : ExprCall?
    var termName : string
    if (hasTerm) {
        termCall = c.single["term"]
        termName = c.single_name["term"]
    }
    let noRangeOps = (!(c.single |> key_exists("skip")) && !(c.single |> key_exists("skip_while"))
        && !(c.single |> key_exists("take_while")) && !(c.single |> key_exists("take")))
    if (hasTerm && termName == "count" && length(termCall.arguments) == 1
            && (c.many["head"] |> empty) && noRangeOps) return ctx.src->count_shortcut("count", at)
    var calls : array<tuple<ExprCall?; LinqCall?>>
    calls |> reserve(length(c.many["head"]) + 5)
    for (call in c.many["head"]) {
        let nm = call_norm_name(call)
        if (!(linqCalls |> key_exists(nm))) return null
        let lc = unsafe(linqCalls?[nm])
        calls |> push((call, lc))
    }
    let rangeSlots = ["skip", "skip_while", "take_while", "take"]
    for (sn in rangeSlots) {
        if (c.single |> key_exists(sn)) {
            let scall = c.single[sn]
            let snm = call_norm_name(scall)
            if (!(linqCalls |> key_exists(snm))) return null
            let lc = unsafe(linqCalls?[snm])
            calls |> push((scall, lc))
        }
    }
    if (hasTerm) {
        if (!(linqCalls |> key_exists(termName))) return null
        let lc = unsafe(linqCalls?[termName])
        calls |> push((termCall, lc))
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
    if (termName == "to_table") return null
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
        if (chainInfo.selectCount == 0 || elementType == null) return null
        var noBinds : array<Expression?>
        var noPre : array<Expression?>
        return emit_accumulator_lane(termName, adapter, null, null, clone_expression(rangeInfo.postTakeWhereCond),
            noBinds, noPre, elementType, accName, finalBind, names, null, null, null, null, at)
    }
    if (isMinMaxBy) return emit_decs_min_max_by(bridge, termName, chainInfo, rangeInfo, calls, rangeInfo.chainEnd, termCall, at)
    if (isEarlyExit || isWalk || isElementAt) {
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
        var elementType = clone_type(chainInfo.finalType)
        let names <- make_range_names(at)
        var adapter = make_decs_term_adapter(bridge, chainInfo, rangeInfo, calls, rangeInfo.chainEnd)
        var noBinds : array<Expression?>
        var noPre : array<Expression?>
        return emit_early_exit_lane(termName, adapter, null, null, clone_expression(rangeInfo.postTakeWhereCond),
            noBinds, noPre, elementType, termCall, chainInfo.finalBind, names, null, null, null, null, at)
    }
    let unknownTerminator = hasTerm && termName != "to_array"
    if (unknownTerminator || ctx.expr_is_iterator) return null
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
            $i(bufName) |> reserve($i(actualName))
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
            $i(bufName) |> reserve($i(actualName))
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
    var bt = strip_const_ref(clone_type(elemType))
    if (bt == null) return null
    let archName = bridge.archName
    let liName = qn("decs_rli", at)
    let foundName = qn("decs_rfound", at)
    let resName = qn("decs_rres", at)
    var tup = build_decs_index_tup(bridge, liName, at)
    var archBody = qmacro_block() {
        let $i(liName) = $i(archName).size - 1_l
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
    var forBodyStmts <- [tupBind, body]
    var forBody = stmts_to_expr(forBodyStmts)
    var clonedForExpr = clone_expression(bridge.forExpr)
    var clonedFor = clonedForExpr as ExprFor
    var newForBody = new ExprBlock(at = at)
    newForBody.list |> push(forBody)
    clonedFor.body = newForBody
    return clonedForExpr
}

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
    var (allUsed, usedNames) = collect_decs_tup_usage(body, tupName, bridge)
    if (allUsed && rewrite_decs_tup_wholes(body, tupName, bridge)) {
        var (allUsed2, usedNames2) = collect_decs_tup_usage(body, tupName, bridge)
        allUsed = allUsed2
        usedNames <- usedNames2
    }
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
    if (length(fieldToIter) < length(usedSet)) {
        var tupBind = build_decs_tup_bind(bridge, tupName, at)
        return build_decs_inner_for(bridge, tupBind, body, at)
    }
    flatten_decs_tup_to_iter(body, tupName, fieldToIter)
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
    var newForBody = new ExprBlock(at = at)
    newForBody.list |> push(body)
    clonedFor.body = newForBody
    return clonedForExpr
}

struct DecsChainInfo {
    tupName     : string
    bindAt      : array<string>
    elidedAt    : array<bool>
    finalBind   : string
    finalType   : TypeDeclPtr
    selectCount : int
}

[macro_function]
def compute_decs_chain_info(var calls : array<tuple<ExprCall?; LinqCall?>>;
                            intermediateEnd : int;
                            tupName : string;
                            bridge : DecsBridgeShape?;
                            at : LineInfo) : DecsChainInfo? {
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
    info.finalBind = curBind
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
    skipWhileCond     : Expression?
    takeWhileCond     : Expression?
    postTakeWhereCond : Expression?
    chainEnd          : int
}

struct DecsRangeStateNames {
    skipName      : string
    takeCountName : string
    takeLimitName : string
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
def extract_decs_ranges(var calls : array<tuple<ExprCall?; LinqCall?>>;  // nolint:STYLE037 - one arm per range-combinator kind
                        intermediateEnd : int;
                        tupName : string;
                        at : LineInfo) : DecsRangeInfo? {
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
    if (rangeInfo.skipExpr == null && rangeInfo.takeExpr == null && rangeInfo.skipWhileCond == null && rangeInfo.takeWhileCond == null) return body
    let skipName      = names.skipName
    let takeCountName = names.takeCountName
    let takeLimitName = names.takeLimitName
    let skippingName  = names.skippingName
    var stmts : array<Expression?>
    stmts |> reserve(6)
    if (rangeInfo.takeExpr != null) {
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
        stmts |> push <| qmacro_expr() {
            if (!($e(rangeInfo.takeWhileCond))) {
                return true
            }
        }
    }
    if (rangeInfo.takeExpr != null) {
        stmts |> push <| qmacro_expr() {
            $i(takeCountName) ++
        }
    }
    stmts |> push(body)
    return stmts_to_expr(stmts)
}

[macro_function]
def decs_range_prelude(rangeInfo : DecsRangeInfo?; names : DecsRangeStateNames) : array<Expression?> {
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
        stmts |> push <| qmacro_expr() {
            var $i(skippingName) = true
        }
    }
    return <- stmts
}

[macro_function]
def emit_decs_count_archsize(bridge : DecsBridgeShape?; at : LineInfo) : Expression? {
    let accName = qn("decs_acc", at)
    let archName = qn("decs_arch", at)
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

[macro_function]
def emit_decs_join_impl(bridgeA : DecsBridgeShape?; var c : Captures; ctx : EmitCtx; at : LineInfo) : Expression? {
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
    if (!countOnly && ctx.expr_is_iterator) return null
    var bridgeB = extract_decs_bridge(joinCall.arguments[1])
    if (bridgeB == null) return null
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
        $i(hashName)[$e(keybBody)] |> push_clone($i(tupBName))
    }
    var collectInner = build_decs_inner_for(bridgeB, tupBindB, stmts_to_expr(collectBody), at)
    var probeInner = build_decs_inner_for(bridgeA, tupBindA, pieces.probeOuter, at)
    var allStmts := pieces.preludeStmts
    allStmts |> push(join_hash_decl(hashName, keyType, tupBType))
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

