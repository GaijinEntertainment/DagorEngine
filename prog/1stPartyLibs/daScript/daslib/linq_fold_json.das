options gen2
options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options _comment_hygiene = true

module linq_fold_json shared public

//! linq_fold JSON source adapter: ``JsonAdapter`` + ``JsonJoinAdapter`` + ``extract_json_source``. Lets ``_fold`` over a
//! ``from_json(jv, type<Row>)`` chain emit an inlined ``for (e in jv.value as _array)`` walk that reads
//! only the fields the chain touches (via ``read_json_field``), instead of materializing every element
//! whole. A struct row is built **by name** — each top-level field via ``read_json_field`` (Option B,
//! "like XML"), so a custom whole-row ``from_JV(Row)`` override is never honored; only a non-struct
//! element type falls back to ``from_JV``. In-tree companion to ``daslib/linq_fold`` (required
//! unconditionally — the adapter is always compiled in, unlike XML's pugixml gate; the matcher just
//! returns null in a JSON-less program). Emits ``from_json`` / ``read_json_field`` (and ``from_JV`` for
//! non-struct elements) BY NAME (resolved at the user's splice site, like linq_fold_decs emits
//! ``for_each_archetype``), so this module pulls in no json dependency. See daslib/linq_fold.md.

require daslib/ast_boost
require daslib/ast_match
require daslib/templates_boost
require daslib/linq_fold_common public

// Field-pruning row-usage scanning (RowUsageScanner / collect_row_usage / RowFieldFlattener /
// flatten_row_to_locals / hoist_prelude) is shared with the XML adapter — see daslib/linq_fold_common.das.

// Whole-row by-name materialize: assign every top-level field via read_json_field (by name), so the JSON
// linq source reads flat objects field-by-field like from_xml_node and NEVER calls the overridable struct
// from_JV. Field VALUES still convert via from_JV (nested/scalar overrides honored); only a whole-row
// from_JV(T) override is bypassed — by design (B / "like XML"): for that, materialize to an array first.
// Caller declares `var <bindName>` (or it is pre-declared) and the `<defName>` default row.
[macro_function]
def private emit_full_row_by_name(bindName : string; declT : TypeDeclPtr; defName, curName : string;
                                  var dest : array<Expression?>) {
    for (fld in declT.structType.fields) {
        let fname = string(fld.name)
        dest |> push <| qmacro_expr() {
            $i(bindName).$f(fname) = read_json_field($i(curName), $v(fname), $i(defName).$f(fname))
        }
    }
}

// Materialize-under-guard: when the whole row escapes only inside a bare `if (fieldCond) { ... }`, read
// the predicate's fields cheaply (read_json_field — no peek/borrow split, JSON is heap-owned) and build the
// full by-name row only for matches. False (caller does the full build) unless the body is that exact shape.
[macro_function]
def private try_json_under_guard_materialize(var body : Expression?; bindName : string; declT : TypeDeclPtr;
                                             defName, curName : string; at : LineInfo;
                                             var prelude : array<Expression?>; var inner : array<Expression?>) : bool {
    if (!(body is ExprIfThenElse)) return false
    var ite = body as ExprIfThenElse
    if (ite.if_false != null || ite.cond == null || ite.if_true == null) return false
    var (condAllUsed, condFields) = collect_row_usage(ite.cond, bindName)
    if (condAllUsed || empty(condFields)) return false   // whole-`it` in the guard, or no field to prune
    sort(condFields)
    var fieldToLocal <- {for (f in condFields); f => qn("jf_{f}", at)}
    prelude |> push <| qmacro_expr() {
        let $i(defName) = default<$t(declT)>
    }
    inner |> reserve(length(inner) + length(condFields) + 1)
    for (f in condFields) {
        let local = fieldToLocal[f]
        inner |> push <| qmacro_expr() {
            let $i(local) = read_json_field($i(curName), $v(f), $i(defName).$f(f))
        }
    }
    ite.cond = flatten_row_to_locals(ite.cond, bindName, fieldToLocal)
    // Prepend the by-name full materialize to the guarded branch so only matching elements build the row.
    var guarded : array<Expression?>   // nolint:STYLE012 — array-literal-of-qmacro_expr is parse-fragile
    guarded |> push <| qmacro_expr() {
        var $i(bindName) <- default<$t(declT)>
    }
    emit_full_row_by_name(bindName, declT, defName, curName, guarded)
    guarded |> push(ite.if_true)
    ite.if_true = stmts_to_expr(guarded)
    inner |> push(body)
    return true
}

// Fills `prelude` (the hoisted default-row decl, prune path only) + `inner` (per-element, ends with
// `body`): allUsed -> full from_JV(it); no field refs -> body only; pruned -> one `let jf_<f>` read per
// referenced field with body's `it.<f>` rewritten to it (struct dropped). See linq_fold.md.
[macro_function]
def private build_json_materializer(var body : Expression?; bindName : string; declT : TypeDeclPtr;
                                    defName, curName : string; at : LineInfo;
                                    var prelude : array<Expression?>; var inner : array<Expression?>) {
    var (allUsed, usedFields) = collect_row_usage(body, bindName)
    if (allUsed) {
        if (try_json_under_guard_materialize(body, bindName, declT, defName, curName, at, prelude, inner)) {
            return
        }
        // whole row escapes: read every field by name (no struct from_JV — see emit_full_row_by_name).
        if (declT.baseType == Type.tStructure) {
            prelude |> push <| qmacro_expr() {
                let $i(defName) = default<$t(declT)>
            }
            inner |> push <| qmacro_expr() {
                var $i(bindName) <- default<$t(declT)>
            }
            emit_full_row_by_name(bindName, declT, defName, curName, inner)
        } else {
            inner |> push <| qmacro_expr() {
                var $i(bindName) <- from_JV($i(curName), type<$t(declT)>)
            }
        }
        inner |> push(body)
        return
    }
    if (empty(usedFields)) {
        inner |> push(body)
        return
    }
    sort(usedFields)   // deterministic emission (AOT hash / test stability)
    var fieldToLocal <- {for (f in usedFields); f => qn("jf_{f}", at)}
    prelude |> push <| qmacro_expr() {
        let $i(defName) = default<$t(declT)>
    }
    inner |> reserve(length(inner) + length(usedFields) + 1)
    for (f in usedFields) {
        let local = fieldToLocal[f]
        inner |> push <| qmacro_expr() {
            let $i(local) = read_json_field($i(curName), $v(f), $i(defName).$f(f))
        }
    }
    body = flatten_row_to_locals(body, bindName, fieldToLocal)
    inner |> push(body)
}

// Build the JSON array walk shared by wrap_source_loop and emit_distinct_take_loop. `for` over the array
// makes a `continue` (skip/skip_while) land on the next element; non-null breakGuard emits `break if (guard)`
// first (distinct+take early-exit). The `is _array` guard yields nothing for a null/non-array source.
[macro_function]
def private build_json_array_walk(srcName : string; curName : string; var inner : array<Expression?>;
                                  var breakGuard : Expression?) : Expression? {
    var elemBody : array<Expression?>
    if (breakGuard != null) {
        elemBody |> push <| qmacro_expr() {
            break if ($e(breakGuard))
        }
    }
    elemBody |> push_from(inner)
    return qmacro_block() {
        if ($i(srcName) != null && $i(srcName).value is _array) {
            for ($i(curName) in $i(srcName).value as _array) {
                $b(elemBody)
            }
        }
    }
}

// Materialize the per-element row (field-pruned) + emit the array walk + hoist the prelude. Shared by
// wrap_source_loop and emit_distinct_take_loop. Non-null breakGuard emits `break if (guard)` first.
[macro_function]
def private build_pruned_array_walk(srcName : string; elemType : TypeDeclPtr; bindName : string;
                                    var body : Expression?; var breakGuard : Expression?; at : LineInfo) : Expression? {
    let curName = qn("jcur", at)
    let defName = qn("jdef", at)
    var declT = strip_const_ref(clone_type(elemType))
    var prelude : array<Expression?>
    var inner : array<Expression?>
    build_json_materializer(body, bindName, declT, defName, curName, at, prelude, inner)
    var loop = build_json_array_walk(srcName, curName, inner, breakGuard)
    return hoist_prelude(prelude, loop, at)
}

// 2-source invoke binding the JSON value (param 1, named `srcName` so the array walk in bodyStmts reads it)
// + the in-memory srcB array (param 2). Shared by JsonAdapter.emit_join_hook (standalone) and
// JsonJoinAdapter.wrap_invoke (group_by-after-join); const-forced source so a `let` JsonValue? binds.
[macro_function]
def private build_json_join_invoke(var jsonExpr : Expression?; srcName : string; var bodyStmts : array<Expression?>;
                                   srcBName : string; srcBType : TypeDeclPtr; var srcBExpr : Expression?;
                                   retType : TypeDeclPtr; wrapIter : bool; at : LineInfo) : Expression? {
    var jsonType = strip_const_ref(clone_type(jsonExpr._type))
    jsonType.flags.constant = true
    var jsonClone = clone_expression(jsonExpr)
    jsonClone.genFlags.alwaysSafe = true
    var srcbClone = clone_expression(srcBExpr)
    srcbClone.genFlags.alwaysSafe = true
    let sn = srcName
    var emission : Expression?
    if (retType != null) {
        emission = qmacro(invoke($($i(sn) : $t(jsonType), $i(srcBName) : $t(srcBType)) : $t(retType) {
            $b(bodyStmts)
        }, $e(jsonClone), $e(srcbClone)))
    } else {
        emission = qmacro(invoke($($i(sn) : $t(jsonType), $i(srcBName) : $t(srcBType)) {
            $b(bodyStmts)
        }, $e(jsonClone), $e(srcbClone)))
    }
    emission = finalize_invoke(emission, at)
    if (wrapIter) {
        emission = qmacro($e(emission).to_sequence_move())
        emission.force_generated(true)
    }
    return emission
}

// Single flat for-loop over the JSON array, like ArrayAdapter (unlike decs's 2-level archetype walk or
// XML's DOM child-element while-walk). No name variants — the source IS the array.
class JsonAdapter : SourceAdapter {
    jsonExpr : Expression?
    srcName  : string
    elemType : TypeDeclPtr
    def override name_prefix() : string {
        return "json_"
    }
    def override supports_direct_return() : bool {
        return true   // single flat for-loop inside the invoke; a mid-loop `return` exits the invoke
    }
    def override const can_group_by() : bool {
        return true
    }
    def override const can_join() : bool {
        return true
    }
    def override emit_join_hook(var c : Captures; var ctx : EmitCtx; at : LineInfo) : Expression? {
        // JSON standalone join: hashed equi-join collecting srcB into a table, then probing from the
        // field-pruned array walk (mirrors emit_array_join, but srcA is the JSON value, not an array param).
        // group join over a JSON source isn't spliced yet — defer to tier-2 (the array adapter handles it).
        if (c.single_name |> key_exists("join") && c.single_name["join"] == "group_join") return null
        var joinCall = c.single["join"]
        if (!srcb_is_array_shaped(c, "join")) return null
        var whereLam : Expression?
        if (c.single |> key_exists("trailing_where")) {
            whereLam = c.single["trailing_where"] != null ? c.single["trailing_where"].arguments[1] : null
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
        // Iterator-typed result (no count terminator): the fused single-pass build is the measured winner over
        // a streaming generator — materialize the field-pruned result array, then wrap as a sequence to honor
        // the iterator contract.
        let wrapIter = !countOnly && ctx.expr_is_iterator
        var srcbSrc = joinCall.arguments[1]
        if (srcbSrc == null || srcbSrc._type == null || srcbSrc._type.firstType == null) return null
        var keyaLam = joinCall.arguments[2]
        var keybLam = joinCall.arguments[3]
        if (keyaLam == null || keybLam == null || keyaLam._type == null) return null
        let keyType = strip_const_ref(clone_type(keyaLam._type.firstType))
        if (!is_primitive_join_key_type(keyType)) return null
        let tupAName = bind_name(at)   // = "it"; keya/result peel to read it.<f> so the materializer prunes
        let srcBName = qn("jjoin_srcB", at)
        let tupBName = qn("jjoin_tup_b", at)
        let hashName = qn("jjoin_hash", at)
        var keyaBody = peel_lambda_rename_var(keyaLam, tupAName)
        var keybBody = peel_lambda_rename_var(keybLam, tupBName)
        if (keyaBody == null || keybBody == null) return null
        let tupBType = strip_const_ref(clone_type(srcbSrc._type.firstType))
        var pieces = build_join_standalone_pieces(joinCall, whereLam, selectLam, leadWhereLam, countOnly, keyaBody, tupAName, hashName, tupBType, false, "jjoin", at)
        if (pieces == null) return null
        var allStmts := pieces.preludeStmts
        allStmts |> push <| qmacro_expr() {
            var $i(hashName) : table<$t(keyType); array<$t(tupBType)>>
        }
        allStmts |> push <| qmacro_expr() {
            for ($i(tupBName) in $i(srcBName)) {
                // nolint:PERF006 per-key bucket size unknown ahead of time
                $i(hashName)[$e(keybBody)] |> push_clone($i(tupBName))
            }
        }
        allStmts |> push(wrap_source_loop(LoopDispatch(Each = null), pieces.probeOuter, at))
        allStmts |> push(pieces.returnStmt)
        return build_json_join_invoke(jsonExpr, srcName, allStmts, srcBName, invoke_src_param_type(srcbSrc), srcbSrc, pieces.invokeRetType, wrapIter, at)
    }
    def override build_group_by_adapter(var c : Captures; var ctx : EmitCtx; at : LineInfo) : SourceAdapter? {
        if (c.single |> key_exists("upstream_join")) {
            // join |> group_by over JSON: srcA = the JSON value (array-walked), srcB = join's second arg.
            // plan_group_by_core drives the bucket-fill through JsonJoinAdapter.wrap_source_loop (pruning free).
            var joinCall = c.single["upstream_join"]
            var core = extract_upstream_join_core(joinCall)
            var srcb = extract_upstream_join_array_srcb(joinCall)
            if (core == null || srcb == null) {
                return null
            }
            return new JsonJoinAdapter(shape = JsonJoinShape(
                jsonExpr = clone_expression(jsonExpr),
                srcName = qn("jsrc", at),
                elemType = clone_type(elemType),
                srcBExpr = srcb.srcbClone,
                srcBName = qn("jjoin_srcB", at),
                srcBType = srcb.srcbType,
                keyaLam = core.keyaLam,
                keybLam = core.keybLam,
                resultLam = core.resultLam,
                keyType = core.keyType,
                tupBType = srcb.tupBType,
                resultType = core.resultType,
                resBindName = qn("jjoin_jres", at)))
        }
        // join |> group_by deferred to tier-2 unless it hit the upstream-join arm; plain group_by clones self.
        return new JsonAdapter(jsonExpr = clone_expression(jsonExpr), srcName = qn("jsrc", at),
                               elemType = clone_type(elemType))
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
    def override wrap_source_loop(loopShape : LoopDispatch; var body : Expression?; at : LineInfo) : Expression? {
        return build_pruned_array_walk(srcName, elemType, bind_name(at), body, null, at)
    }
    def override emit_distinct_take_loop(bindName : string; takenName : string; takeLimName : string; var perElement : Expression?; at : LineInfo) : Expression? {
        // distinct(...) |> take(N): same field-pruned walk, breaking once N distinct elements are taken.
        var breakGuard = qmacro($i(takenName) >= $i(takeLimName))
        return build_pruned_array_walk(srcName, elemType, bindName, perElement, breakGuard, at)
    }
    def override wrap_invoke(var stmts : array<Expression?>; retType : TypeDeclPtr; wrapIter : bool; at : LineInfo) : Expression? {
        // Const-accepting param: the source `jv` is often a `let` (const JsonValue?), and a non-const source
        // adds-const cleanly — so the single fused invoke binds either. Strip ref, force const.
        var jsonType = strip_const_ref(clone_type(jsonExpr._type))
        jsonType.flags.constant = true
        var jsonClone = clone_expression(jsonExpr)
        jsonClone.genFlags.alwaysSafe = true
        let sn = srcName
        var emission : Expression?
        if (retType != null) {
            emission = qmacro(invoke($($i(sn) : $t(jsonType)) : $t(retType) {
                $b(stmts)
            }, $e(jsonClone)))
        } else {
            emission = qmacro(invoke($($i(sn) : $t(jsonType)) {
                $b(stmts)
            }, $e(jsonClone)))
        }
        emission = finalize_invoke(emission, at)
        if (wrapIter) {
            emission = qmacro($e(emission).to_sequence_move())
            emission.force_generated(true)
        }
        return emission
    }
    // Deferred materialization: a buffered reducer (order/take, distinct) holds `(orderKey, JsonValue?)`
    // surrogates and builds the row only for the K survivors. The handle is the per-element `jcur`; the
    // survivor materialize reads each field by name (like XML), never the overridable struct from_JV.
    def override const defers_materialization() : bool {
        return true
    }
    def override handle_type() : TypeDeclPtr {
        var ht = strip_const_ref(clone_type(jsonExpr._type))
        if (ht.firstType != null) {
            ht.firstType.flags.constant = true   // `JsonValue const?` — matches `jsrc as _array`'s const elements
        }
        return ht
    }
    def override current_handle_expr(at : LineInfo) : Expression? {
        let cn = qn("jcur", at)
        return qmacro($i(cn))
    }
    def override materialize_handle(var handleExpr : Expression?; outBind : string; at : LineInfo) : Expression? {
        var declT = strip_const_ref(clone_type(elemType))
        if (declT.baseType != Type.tStructure) {
            return qmacro_expr() {
                $i(outBind) <- from_JV($e(handleExpr), type<$t(declT)>)
            }
        }
        // distinct prefix from the array-walk's "jdef" (line ~159): a group_by + first/order/distinct/take
        // chain fires both materialize sites at the same `at`, and qn keys only on (prefix, at) — same
        // prefix there would shadow (see groupby_first m6f).
        let defName = qn("jhdef", at)
        let curName = qn("jhandle", at)
        var stmts : array<Expression?>   // nolint:STYLE012 -- list grown further by emit_full_row_by_name below
        stmts |> push <| qmacro_expr() {
            let $i(curName) = $e(handleExpr)
        }
        stmts |> push <| qmacro_expr() {
            let $i(defName) = default<$t(declT)>
        }
        emit_full_row_by_name(outBind, declT, defName, curName, stmts)
        return stmts_to_expr(stmts)
    }
}

// JSON analog of XmlJoinShape — srcA = JSON value (array-walked), srcB = in-memory array.
struct JsonJoinShape {
    jsonExpr : Expression?
    srcName  : string
    elemType : TypeDeclPtr
    srcBExpr : Expression?
    srcBName : string
    srcBType : TypeDeclPtr
    keyaLam, keybLam, resultLam : Expression?
    keyType, tupBType, resultType : TypeDeclPtr
    resBindName : string
}

// group_by-after-join over JSON — returned from JsonAdapter.build_group_by_adapter on the upstream-join arm.
// plan_group_by_core drives the bucket-fill through wrap_source_loop (collect srcB into a hash, probe from
// the field-pruned array walk). Mirrors XmlJoinAdapter.
class JsonJoinAdapter : SourceAdapter {
    shape : JsonJoinShape
    def override name_prefix() : string {
        return "jjoin_"
    }
    def override supports_direct_return() : bool {
        return true   // collect for{} + probe array for-walk, both single-level inside the invoke
    }
    def override bind_name(at : LineInfo) : string {
        return shape.resBindName
    }
    def override element_type() : TypeDeclPtr {
        return clone_type(shape.resultType)
    }
    def override wrap_source_loop(loopShape : LoopDispatch; var body : Expression?; at : LineInfo) : Expression? {
        let jhashName = qn("jjoin_jhash", at)
        let arrName   = qn("jjoin_jarr", at)
        let tupBName  = qn("jjoin_jtb", at)
        let tupAName  = qn("it", at)   // srcA row bind; keya/result peel to read it.<f> so the materializer prunes
        var pieces = build_join_adapter_pieces(shape.keyaLam, shape.keybLam, shape.resultLam, tupAName, tupBName, jhashName, arrName, shape.resBindName, shape.tupBType, shape.resultType, body, at)
        if (pieces == null) return null
        var probeWalk = build_pruned_array_walk(shape.srcName, shape.elemType, tupAName, pieces.probeOuter, null, at)
        let keyType = shape.keyType
        let tupBType = shape.tupBType
        let srcBName = shape.srcBName
        return <- qmacro_block() {
            var $i(jhashName) : table<$t(keyType); array<$t(tupBType)>>
            for ($i(tupBName) in $i(srcBName)) {
                $e(pieces.collectInner)
            }
            $e(probeWalk)
        }
    }
    def override wrap_invoke(var stmts : array<Expression?>; retType : TypeDeclPtr; wrapIter : bool; at : LineInfo) : Expression? {
        return build_json_join_invoke(shape.jsonExpr, shape.srcName, stmts, shape.srcBName, shape.srcBType, shape.srcBExpr, retType, wrapIter, at)
    }
}

// Recognize a `from_json(jv, type<Row>)` chain top. Returns the call (caller reads arguments[0] = jv,
// `_type.firstType` = Row); null otherwise. Name-only match — no json types referenced here, so a
// JSON-less program never resolves a `from_json` and this returns null (the chain falls to the array tier).
[macro_function]
def extract_json_source(var top : Expression?) : ExprCall? {
    if (top == null || !(top is ExprCall)) return null
    var c = top as ExprCall
    if (get_call_short_name(c) != "from_json" || c._type == null || !c._type.isIterator
            || c._type.firstType == null || length(c.arguments) != 2) return null
    // Confirm arg0 is a json::JsonValue? pointer. Name-only match would spuriously fire on an unrelated user
    // `from_json` (this adapter is required unconditionally) and emit unresolved from_JV. No json dep — string-matched.
    let srcT = c.arguments[0]._type
    if (srcT == null || !srcT.isPointer || srcT.firstType == null || srcT.firstType.structType == null
            || srcT.firstType.structType.name != "JsonValue"
            || srcT.firstType.structType._module == null || srcT.firstType.structType._module.name != "json") {
        return null
    }
    return c
}
