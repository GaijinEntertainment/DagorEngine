options gen2
options indenting = 4
// Intentionally opt in for user-facing docs: keep comments concise and actionable.
options _comment_hygiene = true

module linq_fold shared public

//! LINQ ``_fold`` macro — fuse-time-aware pipeline rewriting.
//!
//! ``_fold(expr)`` is a three-tier cascade:
//!
//! 1. **Splice** — when the chain matches a recognized pattern, emit a single
//!    fused for-loop with predicates and projections inlined (best perf,
//!    no intermediate iterators or buffers).
//! 2. **``fold_linq_default``** — when splice can't fire, emit an
//!    array-shape pipeline (``call → array → call → array``) with
//!    ``_inplace`` variants reusing the same buffer and explicit ``delete``
//!    on the previous stage. One buffer at a time, no iterator overhead.
//! 3. **``clone_expression``** — raw passthrough when the chain has no
//!    recognized linq operators (``flatten_linq`` returns empty).
//!
//! All three tiers preserve semantics — ``_fold(chain)`` is observationally
//! equivalent to ``chain``, just faster when patterns match. The macro is
//! always safe to apply.
//!
//! Requires the ``linq`` module for the backing operator functions.

require daslib/linq public

require daslib/ast_boost
require daslib/ast_match
require daslib/templates_boost
require daslib/macro_boost
require strings
require daslib/linq_fold_common public
require daslib/linq_fold_array public
require daslib/linq_fold_decs public
require daslib/linq_fold_json public
require daslib/linq_fold_table public
require daslib/linq_fold_sql
require daslib/module_group
require [linq_fold_source]

[macro_function]
def private fold_linq_default(var expr : Expression?) : Expression? {   // nolint:STYLE037 - one arm per linq call shape in the folded chain
    var blk = new ExprBlock(at = expr.at)
    var (top, calls) = flatten_linq(expr)
    if (empty(calls)) return null
    var topExpr = clone_expression(top)
    var topExprType = clone_type(top._type)
    let srcName = qn("fsrc", expr.at)
    var topValue : Expression? = qmacro($i(srcName))
    var argIndex = 0
    let argMax = length(calls)
    while (argIndex < argMax) {
        {
            var cll & = unsafe(calls[argIndex])
            let newArgName = qn("fpass_{argIndex}", expr.at)
            var callName = cll._1.name
            var inplace = false
            if (cll._0._type.isIterator || cll._0._type.isGoodArrayType) {
                let sep : string = ends_with(callName, "_") ? "" : "_"
                if (argIndex == 0 && top._type.isIterator && !cll._1.noToArrayVariant) {
                    callName = "{callName}{sep}to_array"
                } elif (argIndex != 0 && cll._1.inplace) {
                    callName = "{callName}{sep}inplace"
                    inplace = true
                }
            }
            for (i in cll._1.recursive) {
                if (i >= 1 && i < cll._0.arguments |> length && is_linq_call(cll._0.arguments[i])) {
                    var argExpr = make_call(cll._0.at, "_fold")
                    (argExpr as ExprCallMacro).arguments |> emplace <| cll._0.arguments[i]
                    cll._0.arguments[i] = argExpr
                }
            }
            var newCall = qmacro($c(callName)($e(topValue)))
            let numArgs = cll._0.arguments |> length
            for (i in 1..numArgs) {
                var argClone = clone_expression(cll._0.arguments[i])
                if (match_call_in_module(argClone, "each", "builtin") != null) {
                    argClone.genFlags.alwaysSafe = true
                }
                (newCall as ExprCall).arguments |> emplace_new <| argClone
            }
            if (inplace) {
                blk.list |> emplace(newCall)
            } else {
                blk.list |> emplace_new <| qmacro_expr() {
                    var $i(newArgName) = $e(newCall)  // nolint:PERF009,LINT019 - qmacro template line; PERF009 fires only in the downstream expansion
                }
                (blk.list.back() as ExprLet).variables[0].flags.can_shadow = true
                if (argIndex != 0) {
                    blk.list |> emplace_new <| qmacro_expr() {
                        delete $e(topValue)
                    }
                }
                topValue = qmacro($i(newArgName))
            }
            argIndex ++
        }
    }
    if (expr._type.isIterator) {
        blk.list |> emplace_new <| qmacro_expr() {
            return <- $e(topValue).to_sequence_move()
        }
    } else {
        blk.list |> emplace_new <| qmacro_expr() {
            return <- $e(topValue)
        }
    }
    topExpr.genFlags.alwaysSafe = true
    var res = qmacro(invoke($($i(srcName) : $t(topExprType)) { $e(blk); }, $e(topExpr)))
    (((res as ExprInvoke).arguments[0] as ExprMakeBlock)._block as ExprBlock).arguments[0].flags.can_shadow = true
    res.force_generated(true)
    return res
}

[_macro]
def private register_all_linq_fold_rows {
    if (!is_compiling_macros_in_module("linq_fold")) return
    build_order_family_rows()
    build_loop_or_count_rows()
    build_reverse_rows()
    build_distinct_rows()
    build_group_by_rows()
    build_join_rows()
    build_zip_rows()
    call_module_group("linq_fold_source", "register_linq_fold_source")
}

[macro_function]
def private run_splice_adapter(var calls : array<tuple<ExprCall?; LinqCall?>>; var matchTop, ctxTop : Expression?;
                               var adapter : SourceAdapter?; exprIsIter : bool; at : LineInfo;
                               onlyRow : string = "") : Expression? {
    for (p in splice_patterns) {
        if (onlyRow != "" && p.name != onlyRow) continue
        var r <- match_pattern(p, calls, matchTop, adapter)
        if (r is matched) {
            var caps <- r as matched
            var effSrc = adapter
            var usable = true
            if (caps.single |> key_exists("srcsel")) {
                var selCall = caps.single["srcsel"]
                if (selCall != null && length(selCall.arguments) >= 2 && selCall.arguments[1] != null
                        && selCall._type != null && selCall._type.firstType != null) {
                    effSrc = new ProjectedSourceAdapter(inner = adapter,
                        selectLam = clone_expression(selCall.arguments[1]),
                        projType = clone_type(selCall._type.firstType),
                        projName = qn("psel", at))
                } else {
                    usable = false
                }
            }
            if (usable) {
                var ctx = EmitCtx(top = ctxTop, src = effSrc, expr_is_iterator = exprIsIter)
                var result = invoke(p.emit, caps, ctx, at)
                if (result != null) return result
            }
        }
    }
    return null
}

[macro_function]
def private try_splice_patterns(prog : ProgramPtr; var expr : Expression?) : Expression? {
    var (top, calls) = flatten_linq(expr)
    if (expr._type == null) return null
    if (extract_sql_source(expr) != null && sql_linq_loaded(prog)) {
        var sqlCall = make_call(expr.at, "_sql")
        (sqlCall as ExprCallMacro).arguments |> emplace_new <| clone_expression(expr)
        return sqlCall
    }
    if (empty(calls)) return null
    normalize_order_reverse(calls)
    collapse_chained_selects(calls)
    collapse_chained_wheres(calls)
    let at = calls[0]._0.at
    let exprIsIter = expr._type.isIterator
    var bridge = extract_decs_bridge(top)
    if (bridge != null) {
        return run_splice_adapter(calls, top, null,
            new DecsAdapter(bridge = bridge, tupName = qn("decs_tup", at)), exprIsIter, at)
    }
    for (recognize in linq_fold_sources) {
        var m = invoke(recognize, top, at)
        if (m.adapter != null) {
            return run_splice_adapter(calls, m.matchTop, m.ctxTop, m.adapter, exprIsIter, at)
        }
    }
    var jsonCall = extract_json_source(top)
    if (jsonCall != null) {
        var jtopClone = clone_expression(top)
        return run_splice_adapter(calls, jtopClone, jtopClone,
            new JsonAdapter(jsonExpr = clone_expression(jsonCall.arguments[0]), srcName = qn("jsrc", at),
                            elemType = clone_type(jsonCall._type.firstType)), exprIsIter, at)
    }
    var tabCall = extract_table_source(top)
    if (tabCall != null) {
        let tabName = get_call_short_name(tabCall)
        let valT = tabCall.arguments[0]._type.secondType
        if (tabName != "each_kv" || (valT != null && valT.canCopy)) {
            let lane = tabName == "each_kv" ? TableLane.KV : (tabName == "keys" ? TableLane.KEYS : TableLane.VALUES)
            var ttopClone = clone_expression(top)
            if (ttopClone._type != null && ttopClone._type.firstType != null) {
                ttopClone._type.firstType.flags.removeConstant = false
            }
            var elemT = clone_type(tabCall._type.firstType)
            elemT.flags.removeConstant = false
            var tadapter = new TableAdapter(tabExpr = clone_expression(tabCall.arguments[0]), srcName = qn("tsrc", at),
                                            elemType = elemT, lane = lane)
            if (!exprIsIter) {
                var probe = try_table_point_lookup(calls, tadapter, at)
                if (probe != null) return probe
            }
            if (lane != TableLane.VALUES) {
                drop_redundant_distinct(calls)
            }
            return run_splice_adapter(calls, ttopClone, ttopClone, tadapter, exprIsIter, at)
        }
    }
    top = peel_each(top)
    var topClone = clone_expression(top)
    return run_splice_adapter(calls, top, topClone,
        new ArrayAdapter(top = topClone, srcName = qn("source", at)), exprIsIter, at)
}

[call_macro(name="_fold")]
class private LinqFold : AstCallMacro {
    def override visit(prog : ProgramPtr; mod : Module?; var call : ExprCallMacro?) : Expression? {
        macro_verify(call.arguments |> length == 1, prog, call.at, "expecting _fold(expression)")
        macro_verify(call.arguments[0]._type != null, prog, call.at, "expecting linq expression")
        macro_verify(!call.arguments[0]._type.isAutoOrAlias, prog, call.at, "_fold: chain type is not fully inferred yet")
        var res = try_splice_patterns(prog, call.arguments[0])
        if (res != null) return res
        if (!(prog._options |> find_arg("_no_linq_perf_warn") ?as tBool ?? false)) {
            var (top, calls) = flatten_linq(call.arguments[0])
            if (!empty(calls) && extract_decs_bridge(top) != null) {
                to_compiler_log("{describe(call.at)}: *warning* `_fold`: from_decs_template source survived dispatch - no splice arm claimed this chain, so the bridge materializes a temp `res` array and the tier-2 cascade runs on the materialized buffer. Rewrite the chain to a recognized decs shape (see doc/source/reference/linq_fold_patterns.rst), or suppress with `options _no_linq_perf_warn = true`.\n")
            }
        }
        res = fold_linq_default(call.arguments[0])
        if (res != null) return res
        return clone_expression(call.arguments[0])
    }
}
