options gen2
options indenting = 4
options _comment_hygiene = true

module linq_fold_array shared public

//! linq_fold array source adapters: ArrayAdapter / ZipAdapter / ArrayJoinAdapter plus the
//! array-shaped emit lanes (counter / accumulator / early-exit / loop-or-count / zip) and
//! their row-builders. Requires linq_fold_common. See daslib/linq_fold.md.

require daslib/linq public
require daslib/ast_boost
require daslib/ast_match
require daslib/templates_boost
require daslib/macro_boost
require strings
require daslib/linq_fold_common public

// 2-source lockstep zip payload — feeds `for (itA, itB in srcA, srcB) { body }` and the 2-source outer invoke.
struct ZipShape {
    srcAExpr, srcBExpr : Expression?
    srcAName, srcBName : string
    srcAType, srcBType : TypeDeclPtr
}

// Array mirror of DecsJoinShape — same role (per-pair result bind for the cross-arm group_by case), 2 array sources iterated via plain `for` loops + a 2-source outer invoke. srcAType / srcBType are the FULL source types (array<T> / iterator<T>) for invoke-param spelling; tupAType / tupBType are the element types post strip_const_ref.
struct ArrayJoinShape {
    srcAExpr, srcBExpr : Expression?
    srcAName, srcBName : string
    srcAType, srcBType : TypeDeclPtr
    tupAType, tupBType : TypeDeclPtr
    keyaLam, keybLam, resultLam : Expression?
    keyType, resultType : TypeDeclPtr
    resBindName : string
}

class ArrayAdapter : SourceAdapter {
    top : Expression?
    srcName : string
    def override can_reserve_by_length() : bool {
        return true
    }
    def override const can_group_by() : bool {
        return true
    }
    def override const can_join() : bool {
        return true
    }
    def override supports_direct_return() : bool {
        return true
    }
    def override emit_join_hook(var c : Captures; var ctx : EmitCtx; at : LineInfo) : Expression? {
        return emit_array_join(c, ctx, at)
    }
    def override arrayTop() : Expression? {
        return top
    }
    def override arraySrcName() : string {
        return srcName
    }
    def override bind_name(at : LineInfo) : string {
        return qn("it", at)
    }
    def override element_type() : TypeDeclPtr {
        if (top == null || top._type == null) {
            return null
        }
        return clone_type(top._type.firstType)
    }
    def override wrap_source_loop(loopShape : LoopDispatch; var body : Expression?; at : LineInfo) : Expression? {
        let bindName = qn("it", at)
        let sn = srcName
        return <- qmacro_block() {
            for ($i(bindName) in $i(sn)) {
                $e(body)
            }
        }
    }
    def override wrap_invoke(var stmts : array<Expression?>; retType : TypeDeclPtr; wrapIter : bool; at : LineInfo) : Expression? {
        var srcParamType = invoke_src_param_type(top)
        var topExpr = clone_expression(top)
        topExpr.genFlags.alwaysSafe = true
        let sn = srcName
        var emission : Expression?
        if (retType != null) {
            emission = qmacro(invoke($($i(sn) : $t(srcParamType)) : $t(retType) {
                $b(stmts)
            }, $e(topExpr)))
        } else {
            emission = qmacro(invoke($($i(sn) : $t(srcParamType)) {
                $b(stmts)
            }, $e(topExpr)))
        }
        emission = finalize_invoke(emission, at)
        if (wrapIter) {
            emission = qmacro($e(emission).to_sequence_move())
            emission.force_generated(true)
        }
        return emission
    }
    def override emit_distinct_take_loop(bindName : string; takenName : string; takeLimName : string; var perElement : Expression?; at : LineInfo) : Expression? {
        let sn = srcName
        return <- qmacro_expr() {
            for ($i(bindName) in $i(sn)) {
                if ($i(takenName) >= $i(takeLimName)) {
                    break
                }
                $e(perElement)
            }
        }
    }
    def override count_shortcut(opName : string; at : LineInfo) : Expression? {
        if (!type_has_length(top._type)) return null      // no O(1) length (iterator source) → walk
        return emit_length_shortcut(opName, top, srcName, at)
    }
    def override build_group_by_adapter(var c : Captures; var ctx : EmitCtx; at : LineInfo) : SourceAdapter? {
        if (top._type == null) {
            return null
        }
        if (c.single |> key_exists("upstream_join")) {
            // srcA = chain top; srcB = join's second arg. plan_group_by_core consumes the ArrayJoin adapter via wrap_source_loop.
            var joinCall = c.single["upstream_join"]
            var srcbSrc = joinCall.arguments[1]
            if (srcbSrc == null || srcbSrc._type == null || srcbSrc._type.firstType == null
                    || !(srcbSrc._type.isGoodArrayType || srcbSrc._type.isArray || srcbSrc._type.isIterator)) {
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
            var tupAType = strip_const_ref(clone_type(top._type.firstType))
            var tupBType = strip_const_ref(clone_type(srcbSrc._type.firstType))
            var topClone = clone_expression(top)
            topClone.genFlags.alwaysSafe = true
            var srcbClone = clone_expression(srcbSrc)
            srcbClone.genFlags.alwaysSafe = true
            return new ArrayJoinAdapter(shape = ArrayJoinShape(
                srcAExpr = topClone,
                srcBExpr = srcbClone,
                srcAName = qn("ajoin_srcA", at),
                srcBName = qn("ajoin_srcB", at),
                srcAType = invoke_src_param_type(top),
                srcBType = invoke_src_param_type(srcbSrc),
                tupAType = tupAType,
                tupBType = tupBType,
                keyaLam = keyaLam,
                keybLam = keybLam,
                resultLam = resultLam,
                keyType = keyType,
                resultType = resultType,
                resBindName = qn("ajoin_jres", at)))
        }
        return new ArrayAdapter(top = top, srcName = qn("source", at))
    }
}

class ZipAdapter : SourceAdapter {
    shape : ZipShape
    def override name_prefix() : string {
        return "zip_"
    }
    def override supports_direct_return() : bool {
        return true   // wrap_source_loop emits a real `for (itA, itB in ...)` inside the invoke
    }
    def override bind_name(at : LineInfo) : string {
        return qn("it", at)
    }
    def override element_type() : TypeDeclPtr {
        return null
    }
    def override count_shortcut(opName : string; at : LineInfo) : Expression? {
        // min(len a, len b) when BOTH sources have O(1) length; else null → walk the counter lane.
        if (!type_has_length(shape.srcAExpr._type) || !type_has_length(shape.srcBExpr._type)) return null
        let aN = shape.srcAName
        let bN = shape.srcBName
        var bodyStmts : array<Expression?>
        if (opName == "count") {
            bodyStmts |> push <| qmacro_expr() {
                return length($i(aN)) < length($i(bN)) ? length($i(aN)) : length($i(bN))
            }
        } elif (type_has_long_length(shape.srcAExpr._type) && type_has_long_length(shape.srcBExpr._type)) {
            bodyStmts |> push <| qmacro_expr() {
                return long_length($i(aN)) < long_length($i(bN)) ? long_length($i(aN)) : long_length($i(bN))
            }
        } else {
            bodyStmts |> push <| qmacro_expr() {
                return int64(length($i(aN)) < length($i(bN)) ? length($i(aN)) : length($i(bN)))
            }
        }
        return wrap_invoke(bodyStmts, null, false, at)
    }
    def override wrap_source_loop(loopShape : LoopDispatch; var body : Expression?; at : LineInfo) : Expression? {
        let srcAName = shape.srcAName
        let srcBName = shape.srcBName
        return <- qmacro_block() {
            for (itA, itB in $i(srcAName), $i(srcBName)) { // nolint:LINT002 parallel-for must bind both vars; emit_zip body reads them only when needsItBind
                $e(body)
            }
        }
    }
    def override wrap_invoke(var stmts : array<Expression?>; retType : TypeDeclPtr; wrapIter : bool; at : LineInfo) : Expression? {
        let srcAName = shape.srcAName
        let srcBName = shape.srcBName
        let srcAType = shape.srcAType
        let srcBType = shape.srcBType
        let srcAExpr = shape.srcAExpr
        let srcBExpr = shape.srcBExpr
        var emission : Expression?
        if (retType != null) {
            emission = qmacro(invoke($($i(srcAName) : $t(srcAType), $i(srcBName) : $t(srcBType)) : $t(retType) {
                $b(stmts)
            }, $e(srcAExpr), $e(srcBExpr)))
        } else {
            emission = qmacro(invoke($($i(srcAName) : $t(srcAType), $i(srcBName) : $t(srcBType)) {
                $b(stmts)
            }, $e(srcAExpr), $e(srcBExpr)))
        }
        emission.force_at(at)
        emission.force_generated(true)
        let blk = (emission as ExprInvoke).arguments[0] as ExprMakeBlock
        (blk._block as ExprBlock).arguments[0].flags.can_shadow = true
        (blk._block as ExprBlock).arguments[1].flags.can_shadow = true
        if (wrapIter) {
            emission = qmacro($e(emission).to_sequence_move())
            emission.force_generated(true)
        }
        return emission
    }
}

class ArrayJoinAdapter : SourceAdapter {
    shape : ArrayJoinShape
    def override name_prefix() : string {
        return "ajoin_"
    }
    def override supports_direct_return() : bool {
        return true   // probe `for (tupA in srcA)` is a real loop inside the invoke
    }
    def override bind_name(at : LineInfo) : string {
        return shape.resBindName
    }
    def override element_type() : TypeDeclPtr {
        return clone_type(shape.resultType)
    }
    def override wrap_source_loop(loopShape : LoopDispatch; var body : Expression?; at : LineInfo) : Expression? {
        let jhashName = qn("ajoin_jhash", at)
        let arrName   = qn("ajoin_jarr", at)
        let tupAName  = qn("ajoin_jta", at)
        let tupBName  = qn("ajoin_jtb", at)
        var pieces = build_join_adapter_pieces(shape.keyaLam, shape.keybLam, shape.resultLam, tupAName, tupBName, jhashName, arrName, shape.resBindName, shape.tupBType, shape.resultType, body, at)
        if (pieces == null) {
            return null
        }
        let keyType = shape.keyType
        let tupBType = shape.tupBType
        let srcAName = shape.srcAName
        let srcBName = shape.srcBName
        return <- qmacro_block() {
            var $i(jhashName) : table<$t(keyType); array<$t(tupBType)>>
            for ($i(tupBName) in $i(srcBName)) {
                $e(pieces.collectInner)
            }
            for ($i(tupAName) in $i(srcAName)) {
                $e(pieces.probeOuter)
            }
        }
    }
    def override wrap_invoke(var stmts : array<Expression?>; retType : TypeDeclPtr; wrapIter : bool; at : LineInfo) : Expression? {
        let srcAName = shape.srcAName
        let srcBName = shape.srcBName
        let srcAType = shape.srcAType
        let srcBType = shape.srcBType
        let srcAExpr = shape.srcAExpr
        let srcBExpr = shape.srcBExpr
        var emission : Expression?
        if (retType != null) {
            emission = qmacro(invoke($($i(srcAName) : $t(srcAType), $i(srcBName) : $t(srcBType)) : $t(retType) {
                $b(stmts)
            }, $e(srcAExpr), $e(srcBExpr)))
        } else {
            emission = qmacro(invoke($($i(srcAName) : $t(srcAType), $i(srcBName) : $t(srcBType)) {
                $b(stmts)
            }, $e(srcAExpr), $e(srcBExpr)))
        }
        emission.force_at(at)
        emission.force_generated(true)
        let blk = (emission as ExprInvoke).arguments[0] as ExprMakeBlock
        (blk._block as ExprBlock).arguments[0].flags.can_shadow = true
        (blk._block as ExprBlock).arguments[1].flags.can_shadow = true
        if (wrapIter) {
            emission = qmacro($e(emission).to_sequence_move())
            emission.force_generated(true)
        }
        return emission
    }
}

// ===== plan_reverse =====

[macro_function]
def build_zip_rows {
    splice_patterns |> emplace <| SplicePattern(
        name = "zip_general",
        chain <- [
            Slot(matcher = m_literal("zip"), cardinality = c_one(), capture_name = "zip"),
            slot_chain_of(["where_", "select"], "head"),
            Slot(matcher = m_literal("skip"),       cardinality = c_opt(), capture_name = "skip", arity = 2),
            Slot(matcher = m_literal("skip_while"), cardinality = c_opt(), capture_name = "skip_while", arity = 2),
            Slot(matcher = m_literal("take_while"), cardinality = c_opt(), capture_name = "take_while", arity = 2),
            Slot(matcher = m_literal("take"),       cardinality = c_opt(), capture_name = "take", arity = 2),
            Slot(matcher = m_literal("reverse"),    cardinality = c_opt(), capture_name = "trailing_reverse"),
            Slot(matcher = m_alias("loop_terminator_family"), cardinality = c_opt(), capture_name = "term")
        ],
        requires <- [
            @@ < RequiresPredicate > zip_arity_2_or_3,
            @@ < RequiresPredicate > zip_no_while_after_select,
            @@ < RequiresPredicate > zip_reverse_compatible_with_term,
            @@ < RequiresPredicate > take_arg_is_int
        ],
        emit = @@ < EmitFn > emit_zip
    )
}

[macro_function]
def emit_zip(var c : Captures; var ctx : EmitCtx; at : LineInfo) : Expression? {
    // 2-source lockstep zip splice. Bare zip (array/iterator) + fused where_/select/take/skip/take_while/skip_while + count/long_count/sum/min/max/average/first/first_or_default/any/all/contains terminators. 3-arg zip(a,b,sel) pre-lowers the selector. Trailing reverse on array lane emits reverse_inplace (Theme 8 C4). Counter (count/long_count) and array lanes are built inline here; accumulator + early-exit lanes delegate to emit_accumulator_lane / emit_early_exit_lane (parallel-array form).
    var zipCall = c.single["zip"]
    let zipArgCount = zipCall.arguments |> length
    // Terminator name + counter predicate (2-arg count/long_count).
    var lastName = ""
    var counterPred : Expression?
    if (c.single |> key_exists("term")) {
        var termCall = c.single["term"]
        let candidateName = c.single_name["term"]
        let candidateArgs = termCall.arguments |> length
        if (candidateName == "count" || candidateName == "long_count") {
            if (candidateArgs != 1 && candidateArgs != 2) return null
            if (candidateArgs == 2) {
                if (termCall.arguments[1] == null) return null
                counterPred = clone_expression(termCall.arguments[1])
            }
            lastName = candidateName
        } elif (candidateName == "sum" || candidateName == "min" || candidateName == "max" || candidateName == "average" || candidateName == "first") {
            if (candidateArgs != 1) return null
            lastName = candidateName
        } elif (candidateName == "first_or_default" || candidateName == "contains") {
            if (candidateArgs != 2) return null
            lastName = candidateName
        } elif (candidateName == "any") {
            if (candidateArgs != 1 && candidateArgs != 2) return null
            lastName = candidateName
        } elif (candidateName == "all") {
            if (candidateArgs != 2) return null
            lastName = candidateName
        } else {
            // Recognized by loop_terminator_family alias but not by zip splice (e.g. last/single/element_at). Cascade.
            return null
        }
    }
    let srcAName = qn("srcA", at)
    let srcBName = qn("srcB", at)
    let bufName  = qn("buf", at)
    let accName  = qn("acc", at)
    let names <- make_range_names(at)
    let itName = qn("it", at)
    var srcAExpr = peel_each(clone_expression(zipCall.arguments[0]))
    var srcBExpr = peel_each(clone_expression(zipCall.arguments[1]))
    if (srcAExpr == null || srcAExpr._type == null || srcBExpr == null || srcBExpr._type == null) return null
    srcAExpr.genFlags.alwaysSafe = true
    srcBExpr.genFlags.alwaysSafe = true
    var srcAType = invoke_src_param_type(srcAExpr)
    var srcBType = invoke_src_param_type(srcBExpr)
    var elementType = clone_type(zipCall._type.firstType)
    var whereCond : Expression?
    var projection : Expression?
    var skipExpr : Expression?
    var takeExpr : Expression?
    var skipWhileCond : Expression?
    var takeWhileCond : Expression?
    var seenSelect = false
    let seenReverse = c.single |> key_exists("trailing_reverse")
    var allProjectionsPure = true
    if (zipArgCount == 3) {
        var resultLam = zipCall.arguments[2]
        if (resultLam == null || !(resultLam is ExprMakeBlock)) return null
        var mblk = resultLam as ExprMakeBlock
        var blk = mblk._block as ExprBlock
        if (blk == null || blk.arguments |> length != 2 || blk.list |> length != 1
                || !(blk.list[0] is ExprReturn)) return null
        var ret = blk.list[0] as ExprReturn
        if (ret.subexpr == null || ret.subexpr._type == null) return null
        var projBody = clone_expression(ret.subexpr)
        var projElemType = strip_const_ref(clone_type(ret.subexpr._type))
        var zipRules : Template
        zipRules |> replaceVariable(string(blk.arguments[0].name), qmacro($i(itName)._0))
        zipRules |> replaceVariable(string(blk.arguments[1].name), qmacro($i(itName)._1))
        projBody = apply_template(zipRules, projBody.at, projBody)
        projection = projBody
        elementType = projElemType
        seenSelect = true
        if (has_sideeffects(projection)) {
            allProjectionsPure = false
        }
    }
    // Head walk: where_/select left-to-right. After-select where is the same "chained-bind dedup" gap the imperative bailed on (line 6583).
    for (call in c.many["head"]) {
        let opName = call_norm_name(call)
        if (opName == "where_") {
            if (seenSelect) return null
            var predicate : Expression? = peel_lambda_rename_var(call.arguments[1], itName)
            if (predicate == null) return null
            if (whereCond == null) {
                whereCond = predicate
            } else {
                whereCond = qmacro($e(whereCond) && $e(predicate))
            }
        } elif (opName == "select") {
            if (seenSelect) return null
            // selectCall._type.firstType is a typedecl(result_selector(...)) formula that doesn't resolve in _fold context; pull from the projection block's return expression instead.
            var projRet = peel_lambda_single_return(call.arguments[1])
            if (projRet == null || projRet._type == null) return null
            var projElemType = strip_const_ref(clone_type(projRet._type))
            projection = peel_lambda_rename_var(call.arguments[1], itName)
            if (projection == null) return null
            elementType = projElemType
            seenSelect = true
            if (has_sideeffects(projection)) {
                allProjectionsPure = false
            }
        }
    }
    // Range / while ops from c.single (positional slots already enforce canonical order: skip → skip_while → take_while → take).
    if (c.single |> key_exists("skip")) {
        var skipArg = c.single["skip"].arguments[1]
        if (skipArg == null || skipArg._type == null || skipArg._type.baseType != Type.tInt) return null
        skipExpr = clone_expression(skipArg)
    }
    if (c.single |> key_exists("skip_while")) {
        var swArg = c.single["skip_while"].arguments[1]
        if (swArg == null) return null
        skipWhileCond = peel_lambda_rename_var(swArg, itName)
        if (skipWhileCond == null) return null
    }
    if (c.single |> key_exists("take_while")) {
        var twArg = c.single["take_while"].arguments[1]
        if (twArg == null) return null
        takeWhileCond = peel_lambda_rename_var(twArg, itName)
        if (takeWhileCond == null) return null
    }
    if (c.single |> key_exists("take")) {
        var takeArg = c.single["take"].arguments[1]
        if (takeArg == null || takeArg._type == null || takeArg._type.baseType != Type.tInt) return null
        takeExpr = clone_expression(takeArg)
    }
    let vprojName = qn("vproj", at)
    let bindProjForCounter = counterPred != null && projection != null
    if (counterPred != null) {
        if (projection != null) {
            counterPred = peel_lambda_replace_var(counterPred, qmacro($i(vprojName)))
        } else {
            counterPred = peel_lambda_rename_var(counterPred, itName)
        }
        if (counterPred == null) return null
    }
    let noLimits = skipExpr == null && takeExpr == null && skipWhileCond == null && takeWhileCond == null
    let noChain = whereCond == null && projection == null && noLimits && counterPred == null
    let bothHaveLength = type_has_length(srcAExpr._type) && type_has_length(srcBExpr._type)
    let isCounter = lastName == "count" || lastName == "long_count"
    if (seenSelect && !allProjectionsPure && !noLimits) return null
    let lane = classify_terminator(lastName)
    // ZipShape feeds wrap_invoke; the for(itA,itB in srcA,srcB) is built locally because emit_zip prepends the optional `let it = (itA,itB)` bind based on needsItBind (D2-D — only emit_zip knows). Built up-front so the accumulator / early-exit lanes ride this adapter directly.
    var zipShape = ZipShape(
        srcAExpr = srcAExpr,
        srcBExpr = srcBExpr,
        srcAName := srcAName,
        srcBName := srcBName,
        srcAType = srcAType,
        srcBType = srcBType
    )
    var adapter = new ZipAdapter(shape = zipShape)
    if (lane == LinqLane.ACCUMULATOR && !isCounter) {
        if (projection == null) return null
        var preCondStmts : array<Expression?>
        preCondStmts |> push <| qmacro_expr() {     // nolint:STYLE012
            let $i(itName) = (itA, itB)
        }
        var intermediateBinds : array<Expression?>
        return emit_accumulator_lane(lastName, adapter, projection, whereCond, null,
            intermediateBinds, preCondStmts, elementType, accName, itName, names,
            skipExpr, takeExpr, skipWhileCond, takeWhileCond, at)
    }
    if (lane == LinqLane.EARLY_EXIT) {
        var preCondStmts : array<Expression?>
        preCondStmts |> push <| qmacro_expr() {     // nolint:STYLE012
            let $i(itName) = (itA, itB)
        }
        var intermediateBinds : array<Expression?>
        let terminatorCall = c.single["term"]
        return emit_early_exit_lane(lastName, adapter, projection, whereCond, null,
            intermediateBinds, preCondStmts, elementType, terminatorCall, itName, names,
            skipExpr, takeExpr, skipWhileCond, takeWhileCond, at)
    }
    if (isCounter) {
        // count / long_count. O(1) min-length shortcut when both sources have length
        // (ZipAdapter.count_shortcut); otherwise walk via the shared counter lane (int64 acc for long_count).
        if (noChain) {
            var sc = adapter->count_shortcut(lastName, at)
            if (sc != null) return sc
        }
        var stmts : array<Expression?>
        if (bindProjForCounter) {
            stmts |> push <| qmacro_expr() {
                var $i(vprojName) = $e(projection)
            }
        } elif (seenSelect && !allProjectionsPure) {
            let finalBindName = qn("vfinal", at)
            stmts |> push <| qmacro_expr() {
                var $i(finalBindName) = $e(projection)
            }
        }
        if (counterPred != null) {
            stmts |> push <| qmacro_expr() {
                if ($e(counterPred)) {
                    $i(accName) ++
                }
            }
        } else {
            stmts |> push <| qmacro_expr() {
                $i(accName) ++
            }
        }
        wrap_with_ranges(stmts, skipExpr, takeExpr, skipWhileCond, takeWhileCond, names)
        var loopBody = wrap_with_condition(stmts_to_expr(stmts), whereCond)
        let projUsedInBody = projection != null && (!allProjectionsPure || bindProjForCounter)
        let needsItBind = whereCond != null || projUsedInBody || skipWhileCond != null || takeWhileCond != null || counterPred != null
        if (needsItBind) {
            var preStmts : array<Expression?>
            preStmts |> reserve(2)
            preStmts |> push <| qmacro_expr() {     // nolint:STYLE012
                let $i(itName) = (itA, itB)
            }
            preStmts |> push(loopBody)
            loopBody = stmts_to_expr(preStmts)
        }
        return emit_counter_lane(adapter, accName, itName, names,
            skipExpr, takeExpr, skipWhileCond, loopBody, at, lastName == "long_count")
    }
    // Buffer / to_array lane — ride the shared fold-array lane over the ZipAdapter.
    let needsItBindLocal = whereCond != null || projection != null || skipWhileCond != null || takeWhileCond != null
    var pushStmts : array<Expression?>
    if (projection != null) {
        pushStmts |> push <| qmacro_expr() {
            $i(bufName) |> push_clone($e(projection))
        }
    } elif (needsItBindLocal) {
        pushStmts |> push <| qmacro_expr() {
            $i(bufName) |> push_clone($i(itName))
        }
    } else {
        pushStmts |> push <| qmacro_expr() {
            $i(bufName) |> push_clone((itA, itB))
        }
    }
    wrap_with_ranges(pushStmts, skipExpr, takeExpr, skipWhileCond, takeWhileCond, names)
    var perElementPush = stmts_to_expr(pushStmts)
    var preCondStmts : array<Expression?>
    if (needsItBindLocal) {
        preCondStmts |> push <| qmacro_expr() {     // nolint:STYLE012
            let $i(itName) = (itA, itB)
        }
    }
    var prologueStmts : array<Expression?>
    append_ranges_prelude(prologueStmts, skipExpr, takeExpr, skipWhileCond, names)
    var reserveStmts : array<Expression?>
    if (noChain && bothHaveLength) {
        reserveStmts |> push <| qmacro_expr() {
            $i(bufName) |> reserve(length($i(srcAName)) < length($i(srcBName)) ? length($i(srcAName)) : length($i(srcBName)))
        }
    }
    var tailStmts : array<Expression?>
    if (seenReverse) {
        tailStmts |> push <| qmacro_expr() {
            _::reverse_inplace($i(bufName))
        }
    }
    tailStmts |> push(buffer_return(bufName, ctx.expr_is_iterator))
    return emit_fold_array_lane(FoldArraySpec(
        prologueStmts <- prologueStmts,
        bufElemType = elementType,
        bufName = bufName,
        reserveStmts <- reserveStmts,
        preCondStmts <- preCondStmts,
        whereCond = whereCond,
        perElementPush = perElementPush,
        tailStmts <- tailStmts,
        wrapIter = false
    ), adapter, at)
}

