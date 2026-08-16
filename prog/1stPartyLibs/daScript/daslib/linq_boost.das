options gen2
options indenting = 4

module linq_boost shared public

//! LINQ operator-pipe macro support.
//!
//! Provides ``|>`` pipe-syntax rewrites for LINQ operations so that
//! ``arr |> where(...) |> select(...)`` chains compile into efficient
//! iterator pipelines.
//! Requires the ``linq`` module.

require daslib/linq public
require daslib/linq_fold public

require daslib/ast_boost
require daslib/templates_boost
require daslib/macro_boost

def private clone_iterator_type(it : TypeDeclPtr) : TypeDeclPtr {
    var res = clone_type(it.firstType)
    res.flags.ref = false
    res.flags.constant = true
    return <- res
}

// Bucket-surface lambda stamping. When the chain element is the group_by_lazy shape `tuple<K, array<E>>`, an untyped lambda passed to a direct `<bind>._1 |> <op>(<lambda>)` call can't infer against the fully generic tier-2 params (error 30303) — but the chain type knows E, so the macro stamps it, same as the outer-param injection in visit().
class private BucketLambdaStamper : AstVisitor {
    bindName : string
    elemT : TypeDeclPtr
    def BucketLambdaStamper(n : string; var et : TypeDeclPtr) {
        bindName = n
        elemT = et
    }
    def override visitExprCall(var expr : ExprCall?) : ExpressionPtr {
        let nm = string(expr.name)
        if ((nm != "select" && nm != "select_to_array" && nm != "sum" && nm != "min"
                && nm != "max" && nm != "average" && nm != "min_by" && nm != "max_by")
                || length(expr.arguments) != 2) {
            return expr
        }
        var recv = expr.arguments[0]
        if (recv != null && recv is ExprRef2Value) {
            recv = (recv as ExprRef2Value).subexpr
        }
        if (recv == null || !(recv is ExprField)) return expr
        var fld = recv as ExprField
        if (fld.name != "_1") return expr
        var base = fld.value
        if (base != null && base is ExprRef2Value) {
            base = (base as ExprRef2Value).subexpr
        }
        if (base == null || !(base is ExprVar) || (base as ExprVar).name != bindName) return expr
        var lam = expr.arguments[1]
        if (lam == null || !(lam is ExprMakeBlock)) return expr
        var blk = (lam as ExprMakeBlock)._block
        if (blk == null || !(blk is ExprBlock)) return expr
        var eb = blk as ExprBlock
        if (length(eb.arguments) != 1) return expr
        if (eb.arguments[0]._type == null || eb.arguments[0]._type.isAutoOrAlias) {
            var et = clone_type(elemT)
            et.flags.ref = false
            et.flags.constant = true
            eb.arguments[0]._type = et
        }
        return expr
    }
}

[macro_function]
def private stamp_bucket_reducer_lambdas(var projExpr : ExpressionPtr; bindName : string; iterType : TypeDeclPtr) {
    if (projExpr == null || iterType == null || iterType.baseType != Type.tTuple
            || length(iterType.argTypes) != 2) {
        return
    }
    var bucketT = iterType.argTypes[1]
    if (bucketT == null || bucketT.baseType != Type.tArray || bucketT.firstType == null) return
    var sc = new BucketLambdaStamper(bindName, clone_type(bucketT.firstType))
    make_visitor(*sc) $(astVisitorAdapter) {
        visit_expression(projExpr, astVisitorAdapter)
    }
    unsafe {
        delete sc
    }
}

class private AstCallMacro_LinqPred2 : AstCallMacro {
    //! Base call macro for LINQ-style two-argument predicate operators.
    predName = "where_"
    //! This macro converts ``_where(iterator, expression)`` into equivalent inline code.
    //!     where_(iterator, $(_) => expression)
    //!
    def override visit(prog : ProgramPtr; mod : Module?; var call : ExprCallMacro?) : Expression? {   // nolint:STYLE037 - one arm per accepted predicate/iterable shape
        //! Visits the LINQ predicate macro call and rewrites it with a lambda argument.
        macro_verify(call.arguments |> length == 2, prog, call.at, "expecting {predName}(iterator, predicate)")
        assume arg0 = call.arguments[0]
        assume arg0type = call.arguments[0]._type
        macro_verify(arg0type != null, prog, call.at, "input argument is not inferred yet")
        macro_verify(arg0type.isIterator || arg0type.isGoodArrayType, prog, call.at, "expecting iterator or array")
        macro_verify(!arg0type.firstType.isAutoOrAlias, prog, call.at, "iterable type cannot be auto or alias")
        // replacing function
        var iterType = clone_iterator_type(arg0type)
        // bucket-surface inner lambdas (`<bind>._1 |> select/max/…(<untyped lambda>)`) get the bucket element type stamped before the rewrite — see BucketLambdaStamper above
        var bucketBind = "_"
        if (call.arguments[1] is ExprMakeBlock) {
            var bblk = (call.arguments[1] as ExprMakeBlock)._block
            if (bblk != null && bblk is ExprBlock && length((bblk as ExprBlock).arguments) == 1) {
                bucketBind = string((bblk as ExprBlock).arguments[0].name)
            }
        }
        stamp_bucket_reducer_lambdas(call.arguments[1], bucketBind, iterType)
        var res : ExpressionPtr
        if (call.arguments[1] is ExprMakeBlock) {
            // named-variable form: the argument is already a `$(x) => …` block (e.g. emitted by linq_das).
            var blk = call.arguments[1] as ExprMakeBlock
            var inner = blk._block as ExprBlock
            macro_verify(length(inner.arguments) == 1, prog, call.at, "{predName}: block argument must take exactly one parameter")
            if (inner.arguments[0]._type == null || inner.arguments[0]._type.isAutoOrAlias) {
                inner.arguments[0]._type = clone_type(iterType)
            }
            res = qmacro($c(predName)($e(arg0), $e(call.arguments[1])))
        } else {
            res = qmacro($c(predName)($e(arg0), $(_ : $t(iterType)) => $e(call.arguments[1])))
        }
        // allow inner `_` to shadow an outer `_` when chains nest (e.g. `_where(_.x._in(arr |> _select(_.y)))`)
        (((res as ExprCall).arguments[1] as ExprMakeBlock)._block as ExprBlock).arguments[0].flags.can_shadow = true
        res.force_at(call.at)
        return res
    }
}

[call_macro(name="_where")]
class private LinqWhere : AstCallMacro_LinqPred2 {
    //! implements _where(iterator, expression) shorthand notation that expands into where_(iterator, $(_) => expression).
    override predName = "where_"
}

[call_macro(name="_where_to_array")]
class private LinqWhereToArray : AstCallMacro_LinqPred2 {
    //! implements _where_to_array(iterator, expression) shorthand notation that expands into where_to_array(iterator, $(_) => expression).
    override predName = "where_to_array"
}

[call_macro(name="_select")]
class private LinqSelect : AstCallMacro_LinqPred2 {
    //! implements _select(iterator, expression) shorthand notation that expands into select(iterator, $(_) => expression).
    override predName = "select"
}

[call_macro(name="_select_to_array")]
class private LinqSelectToArray : AstCallMacro_LinqPred2 {
    //! implements _select_to_array(iterator, expression) shorthand notation that expands into select_to_array(iterator, $(_) => expression).
    override predName = "select_to_array"
}

[call_macro(name="_min_by")]
class private LinqMinBy : AstCallMacro_LinqPred2 {
    //! implements _min_by(iterator, expression) shorthand notation that expands into min_by(iterator, $(_) => expression).
    override predName = "min_by"
}

[call_macro(name="_max_by")]
class private LinqMaxBy : AstCallMacro_LinqPred2 {
    //! implements _max_by(iterator, expression) shorthand notation that expands into max_by(iterator, $(_) => expression).
    override predName = "max_by"
}

[call_macro(name="_min_max_by")]
class private LinqMinMaxBy : AstCallMacro_LinqPred2 {
    //! implements _min_max_by(iterator, expression) shorthand notation that expands into min_max_by(iterator, $(_) => expression).
    override predName = "min_max_by"
}

[call_macro(name="_min_max_average_by")]
class private LinqMinMaxAverageBy : AstCallMacro_LinqPred2 {
    //! implements _min_max_average_by(iterator, expression) shorthand notation that expands into min_max_average_by(iterator, $(_) => expression).
    override predName = "min_max_average_by"
}

[call_macro(name="_skip_while")]
class private LinqSkipWhile : AstCallMacro_LinqPred2 {
    //! implements _skip_while(iterator, expression) shorthand notation that expands into skip_while(iterator, $(_) => expression).
    override predName = "skip_while"
}

[call_macro(name="_take_while")]
class private LinqTakeWhile : AstCallMacro_LinqPred2 {
    //! implements _take_while(iterator, expression) shorthand notation that expands into take_while(iterator, $(_) => expression).
    override predName = "take_while"
}

[call_macro(name="_all")]
class private LinqAll : AstCallMacro_LinqPred2 {
    //! implements _all(iterator, expression) shorthand notation that expands into all(iterator, $(_) => expression).
    override predName = "all"
}

[call_macro(name="_any")]
class private LinqAny : AstCallMacro_LinqPred2 {
    //! implements _any(iterator, expression) shorthand notation that expands into any(iterator, $(_) => expression).
    override predName = "any"
}

[call_macro(name="_none")]
class private LinqNone : AstCallMacro_LinqPred2 {
    //! implements _none(iterator, expression) shorthand notation that expands into none(iterator, $(_) => expression).
    override predName = "none"
}

[call_macro(name="_count")]
class private LinqCount : AstCallMacro_LinqPred2 {
    //! implements _count(iterator, expression) shorthand notation that expands into count(iterator, $(_) => expression).
    override predName = "count"
}

[call_macro(name="_long_count")]
class private LinqLongCount : AstCallMacro_LinqPred2 {
    //! implements _long_count(iterator, expression) shorthand notation that expands into long_count(iterator, $(_) => expression).
    override predName = "long_count"
}

[call_macro(name="_unique_by")]
class private LinqUnique : AstCallMacro_LinqPred2 {
    //! implements _unique_by(iterator, expression) shorthand notation that expands into unique_by(iterator, $(_) => expression).
    override predName = "unique_by"
}

[call_macro(name="_unique_by_to_array")]
class private LinqUniqueToArray : AstCallMacro_LinqPred2 {
    //! implements _unique_by_to_array(iterator, expression) shorthand notation that expands into unique_by_to_array(iterator, $(_) => expression).
    override predName = "unique_by_to_array"
}

[call_macro(name="_distinct_by")]
class private LinqDistinctBy : AstCallMacro_LinqPred2 {
    //! implements _distinct_by(iterator, expression) shorthand notation that expands into distinct_by(iterator, $(_) => expression).
    override predName = "distinct_by"
}

[call_macro(name="_distinct_by_to_array")]
class private LinqDistinctByToArray : AstCallMacro_LinqPred2 {
    //! implements _distinct_by_to_array(iterator, expression) shorthand notation that expands into distinct_by_to_array(iterator, $(_) => expression).
    override predName = "distinct_by_to_array"
}

[call_macro(name="_order_by")]
class private LinqOrderBy : AstCallMacro_LinqPred2 {
    //! implements _order_by(iterator, expression) shorthand notation that expands into order_by(iterator, $(_) => expression).
    override predName = "order_by"
}

[call_macro(name="_order_by_to_array")]
class private LinqOrderByToArray : AstCallMacro_LinqPred2 {
    //! implements _order_by_to_array(iterator, expression) shorthand notation that expands into order_by_to_array(iterator, $(_) => expression).
    override predName = "order_by_to_array"
}

[call_macro(name="_order_by_descending")]
class private LinqOrderByDescending : AstCallMacro_LinqPred2 {
    //! implements _order_by_descending(iterator, expression) shorthand notation that expands into order_by_descending(iterator, $(_) => expression).
    override predName = "order_by_descending"
}

[call_macro(name="_order_by_descending_to_array")]
class private LinqOrderByDescendingToArray : AstCallMacro_LinqPred2 {
    //! implements _order_by_descending_to_array(iterator, expression) shorthand notation that expands into order_by_descending_to_array(iterator, $(_) => expression).
    override predName = "order_by_descending_to_array"
}

class private AstCallMacro_LinqOrderByKeys : AstCallMacro {
    //! Base call macro for the multi-key order operator. The key argument is already a `$(x) => (k1, k2, …)` block (tuple of keys); the third argument is a compile-time `uint` descMask (bit i → key i descending). Injects the element type onto the key block parameter, then passes all three through to `order_by_keys` / `order_by_keys_to_array`.
    predName = "order_by_keys"

    def override visit(prog : ProgramPtr; mod : Module?; var call : ExprCallMacro?) : Expression? {
        macro_verify(call.arguments |> length == 3, prog, call.at, "expecting {predName}(iterator, key-lambda, descMask)")
        assume arg0 = call.arguments[0]
        assume arg0type = call.arguments[0]._type
        macro_verify(arg0type != null, prog, call.at, "input argument is not inferred yet")
        macro_verify(arg0type.isIterator || arg0type.isGoodArrayType, prog, call.at, "expecting iterator or array")
        macro_verify(!arg0type.firstType.isAutoOrAlias, prog, call.at, "iterable type cannot be auto or alias")
        var iterType = clone_iterator_type(arg0type)
        var res : ExpressionPtr
        if (call.arguments[1] is ExprMakeBlock) {
            // named-variable form (e.g. emitted by linq_das): inject the element type onto the user's parameter and pass the `$(x) => (k1, k2, …)` block through verbatim.
            var blk = call.arguments[1] as ExprMakeBlock
            var inner = blk._block as ExprBlock
            macro_verify(length(inner.arguments) == 1, prog, call.at, "{predName}: key block must take exactly one parameter")
            if (inner.arguments[0]._type == null || inner.arguments[0]._type.isAutoOrAlias) {
                inner.arguments[0]._type = clone_type(iterType)
            }
            res = qmacro($c(predName)($e(arg0), $e(call.arguments[1]), $e(call.arguments[2])))
        } else {
            // bare-expression form: wrap `(_.a, _.b)` into `$(_ : iterType) => (_.a, _.b)`.
            res = qmacro($c(predName)($e(arg0), $(_ : $t(iterType)) => $e(call.arguments[1]), $e(call.arguments[2])))
        }
        (((res as ExprCall).arguments[1] as ExprMakeBlock)._block as ExprBlock).arguments[0].flags.can_shadow = true
        res.force_at(call.at)
        return res
    }
}

[call_macro(name="_order_by_keys")]
class private LinqOrderByKeys : AstCallMacro_LinqOrderByKeys {
    //! implements _order_by_keys(iterator, $(x) => (k1, k2, …), descMask) — multi-key order with
    //! per-key direction. Expands into order_by_keys(iterator, key-lambda, descMask).
    override predName = "order_by_keys"
}

[call_macro(name="_order_by_keys_to_array")]
class private LinqOrderByKeysToArray : AstCallMacro_LinqOrderByKeys {
    //! implements _order_by_keys_to_array(iterator, $(x) => (k1, k2, …), descMask) shorthand notation
    //! that expands into order_by_keys_to_array(iterator, key-lambda, descMask).
    override predName = "order_by_keys_to_array"
}

class private AstCallMacro_LinqPredII2 : AstCallMacro {
    //! Base call macro for LINQ-style two-iterator two-argument predicate operators.
    predName = "sequence_equal_by"
    //! implements sequence_equal_by(i1, i2, expresion) shorthand notation that expands into sequence_equal_by(i1, i2, $(_) => expression).
    def override visit(prog : ProgramPtr; mod : Module?; var call : ExprCallMacro?) : Expression? {
        //! Visits the LINQ two-iterator predicate macro call and rewrites it with a lambda argument.
        macro_verify(call.arguments |> length == 3, prog, call.at, "expecting {predName}(iterator or array, iterator or array, predicate)")
        macro_verify(call.arguments[0]._type != null, prog, call.at, "expecting iterator or array")
        macro_verify(call.arguments[0]._type.isIterator || call.arguments[0]._type.isGoodArrayType, prog, call.at, "expecting iterator or array")
        macro_verify(call.arguments[1]._type != null, prog, call.at, "expecting iterator or array")
        macro_verify(call.arguments[1]._type.isIterator || call.arguments[1]._type.isGoodArrayType, prog, call.at, "expecting iterator or array")
        macro_verify(!call.arguments[0]._type.firstType.isAutoOrAlias, prog, call.at, "iterator or array type cannot be auto or alias")
        // replacing function
        var iterType = clone_iterator_type(call.arguments[0]._type)
        var res : ExpressionPtr
        if (call.arguments[2] is ExprMakeBlock) {
            // named-variable form: the argument is already a `$(x) => …` block — inject the element type onto the user's parameter and pass it through (see AstCallMacro_LinqPred2 for rationale).
            var blk = call.arguments[2] as ExprMakeBlock
            var inner = blk._block as ExprBlock
            macro_verify(length(inner.arguments) == 1, prog, call.at, "{predName}: block argument must take exactly one parameter")
            if (inner.arguments[0]._type == null || inner.arguments[0]._type.isAutoOrAlias) {
                inner.arguments[0]._type = clone_type(iterType)
            }
            res = qmacro($c(predName)($e(call.arguments[0]), $e(call.arguments[1]), $e(call.arguments[2])))
        } else {
            res = qmacro($c(predName)($e(call.arguments[0]), $e(call.arguments[1]), $(_ : $t(iterType)) => $e(call.arguments[2])))
        }
        // allow inner `_` to shadow an outer `_` when chains nest
        (((res as ExprCall).arguments[2] as ExprMakeBlock)._block as ExprBlock).arguments[0].flags.can_shadow = true
        res.force_at(call.at)
        return res
    }
}

[call_macro(name="_sequence_equal_by")]
class private LinqPred3 : AstCallMacro_LinqPredII2 {
    //! implements _sequence_equal_by(iterator1, iterator2, expression) shorthand notation that expands into sequence_equal_by(iterator1, iterator2, $(_) => expression).
    override predName = "sequence_equal_by"
}

[call_macro(name="_except_by")]
class private LinqExceptBy : AstCallMacro_LinqPredII2 {
    //! implements _except_by(iterator1, iterator2, expression) shorthand notation that expands into except_by(iterator1, iterator2, $(_) => expression).
    override predName = "except_by"
}

[call_macro(name="_except_by_to_array")]
class private LinqExceptByToArray : AstCallMacro_LinqPredII2 {
    //! implements _except_by_to_array(iterator1, iterator2, expression) shorthand notation that expands into except_by_to_array(iterator1, iterator2, $(_) => expression).
    override predName = "except_by_to_array"
}

[call_macro(name="_intersect_by")]
class private LinqIntersectBy : AstCallMacro_LinqPredII2 {
    //! implements _intersect_by(iterator1, iterator2, expression) shorthand notation that expands into intersect_by(iterator1, iterator2, $(_) => expression).
    override predName = "intersect_by"
}

[call_macro(name="_intersect_by_to_array")]
class private LinqIntersectByToArray : AstCallMacro_LinqPredII2 {
    //! implements _intersect_by_to_array(iterator1, iterator2, expression) shorthand notation that expands into intersect_by_to_array(iterator1, iterator2, $(_) => expression).
    override predName = "intersect_by_to_array"
}

[call_macro(name="_union_by")]
class private LinqUnionBy : AstCallMacro_LinqPredII2 {
    //! implements _union_by(iterator1, iterator2, expression) shorthand notation that expands into union_by(iterator1, iterator2, $(_) => expression).
    override predName = "union_by"
}

class private AstCallMacro_LinqContainsFlip : AstCallMacro {
    //! Base call macro for ``_in`` / ``_not_in``.
    opName = "_in"
    negated = false
    def override visit(prog : ProgramPtr; mod : Module?; var call : ExprCallMacro?) : Expression? {
        macro_verify(call.arguments |> length == 2, prog, call.at, "expecting {opName}(element, subquery)")
        assume subquery = call.arguments[1]
        assume subqueryType = subquery._type
        macro_verify(subqueryType != null, prog, call.at, "subquery argument is not inferred yet")
        macro_verify(subqueryType.isIterator || subqueryType.isGoodArrayType, prog, call.at, "expecting iterator or array as the subquery argument")
        macro_verify(!subqueryType.firstType.isAutoOrAlias, prog, call.at, "subquery element type cannot be auto or alias")
        var res : Expression?
        if (negated) {
            res = qmacro(!contains($e(subquery), $e(call.arguments[0])))
        } else {
            res = qmacro(contains($e(subquery), $e(call.arguments[0])))
        }
        res.force_at(call.at)
        return res
    }
}

[call_macro(name="_in")]
class private LinqIn : AstCallMacro_LinqContainsFlip {
    //! implements _in(element, subquery) shorthand notation that expands into contains(subquery, element).
    override opName = "_in"
    override negated = false
}

[call_macro(name="_not_in")]
class private LinqNotIn : AstCallMacro_LinqContainsFlip {
    //! implements _not_in(element, subquery) shorthand notation that expands into !contains(subquery, element).
    override opName = "_not_in"
    override negated = true
}

struct private EquiJoinParts {
    //! Result of parsing an equi-join ``on`` lambda body.
    ok : bool
    keya_body : Expression?
    keyb_body : Expression?
    larg_name : string
    rarg_name : string
}

def private parse_equi_join_on(var on_pred : Expression?; prog : ProgramPtr; at : LineInfo) : EquiJoinParts {
    //! Validate the ``on`` predicate of ``_join`` / ``_left_join`` and extract the two key-selector bodies.
    var parts : EquiJoinParts
    if (on_pred == null || !(on_pred is ExprMakeBlock)) {
        macro_error(prog, at, "equi-join `on` predicate must be a 2-arg lambda `(l, r) => l.X == r.Y`")
        return parts
    }
    var mkblk = on_pred as ExprMakeBlock
    if (mkblk._block == null || !(mkblk._block is ExprBlock)) {
        macro_error(prog, on_pred.at, "equi-join `on` predicate must be a 2-arg lambda `(l, r) => l.X == r.Y`")
        return parts
    }
    var blk = mkblk._block as ExprBlock
    if (blk.arguments |> length != 2 || blk.list |> length != 1 || !(blk.list[0] is ExprReturn)) {
        macro_error(prog, on_pred.at, "equi-join `on` predicate must be a 2-arg lambda with a single `return l.X == r.Y`")
        return parts
    }
    var ret = blk.list[0] as ExprReturn
    if (ret.subexpr == null || !(ret.subexpr is ExprOp2)) {
        macro_error(prog, on_pred.at, "equi-join `on` predicate body must be a single `==` comparison `l.X == r.Y`")
        return parts
    }
    var op2 = ret.subexpr as ExprOp2
    if (op2.op != "==") {
        macro_error(prog, on_pred.at, "equi-join `on` predicate must use `==` - theta joins and `&&`-chained multi-key equality are not supported; use raw SQL instead")
        return parts
    }
    parts.ok = true
    parts.keya_body = op2.left
    parts.keyb_body = op2.right
    parts.larg_name = string(blk.arguments[0].name)
    parts.rarg_name = string(blk.arguments[1].name)
    return parts
}

def private rename_var_in_expr(var src : Expression?; from_name, to_name : string) : Expression? {
    // Clone ``src`` and rename every ``ExprVar`` named ``from_name`` to ``to_name``.
    return clone_and_rename_var(src, from_name, to_name)
}

class private AstCallMacro_LinqJoinBase : AstCallMacro {
    //! Base call macro for ``_join`` and ``_left_join`` equi-join sugar.
    fnName = "join"
    def override visit(prog : ProgramPtr; mod : Module?; var call : ExprCallMacro?) : Expression? {
        macro_verify(call.arguments |> length == 4, prog, call.at, "expecting {fnName}(srca, srcb, on, into) — 4 arguments")
        assume srcaType = call.arguments[0]._type
        assume srcbType = call.arguments[1]._type
        macro_verify(srcaType != null, prog, call.at, "first argument is not inferred yet")
        macro_verify(srcbType != null, prog, call.at, "second argument is not inferred yet")
        macro_verify(srcaType.isIterator || srcaType.isGoodArrayType, prog, call.at, "first argument must be an iterator or array")
        macro_verify(srcbType.isIterator || srcbType.isGoodArrayType, prog, call.at, "second argument must be an iterator or array")
        macro_verify(!srcaType.firstType.isAutoOrAlias, prog, call.at, "first argument element type cannot be auto or alias")
        macro_verify(!srcbType.firstType.isAutoOrAlias, prog, call.at, "second argument element type cannot be auto or alias")
        var parts = parse_equi_join_on(call.arguments[2], prog, call.at)
        if (!parts.ok) return null
        var iterTypeA = clone_iterator_type(srcaType)
        var iterTypeB = clone_iterator_type(srcbType)
        let la_fresh = "__la__"
        let lb_fresh = "__lb__"
        var keya_body = rename_var_in_expr(parts.keya_body, parts.larg_name, la_fresh)
        var keyb_body = rename_var_in_expr(parts.keyb_body, parts.rarg_name, lb_fresh)
        // Auto-type the result lambda's args when untyped — symmetric with `on`, which accepts `(l,r)=>...` without types. Lets users write `_join(..., $(l,r)=>l.id==r.id, $(l,r)=>(l.x,r.y))` against any source — including from_decs_template(type<T>) where the iterator element is a tuple<...> painful to spell by hand. param0 is always TA; param1 comes from `result_param1_type`: plain `_join` → TB; `_group_join` → array<TB>. `_left_join` / `_right_join` / `_full_outer_join` need Option<T> on one or both sides, so they return null and their untyped result lambdas stay user-spelled.
        var p1type = self.result_param1_type(iterTypeB, call.at)
        if (p1type != null) {
            var resultArg = call.arguments[3]
            if (resultArg != null && resultArg is ExprMakeBlock) {
                var rblk = (resultArg as ExprMakeBlock)._block
                if (rblk != null && rblk is ExprBlock) {
                    var rb = rblk as ExprBlock
                    if (rb.arguments |> length == 2
                            && (rb.arguments[0]._type == null || rb.arguments[0]._type.isAutoOrAlias)
                            && (rb.arguments[1]._type == null || rb.arguments[1]._type.isAutoOrAlias)) {
                        rb.arguments[0]._type = clone_type(iterTypeA)
                        rb.arguments[1]._type = p1type
                    }
                }
            }
        }
        var res = qmacro($c(fnName)(
            $e(call.arguments[0]),
            $e(call.arguments[1]),
            $($i(la_fresh) : $t(iterTypeA)) => $e(keya_body),
            $($i(lb_fresh) : $t(iterTypeB)) => $e(keyb_body),
            $e(call.arguments[3])
        ))
        res.force_at(call.at)
        return res
    }
    // param1 element type for an untyped 2-arg result lambda, or null to leave it user-spelled (the Option-side
    // outer joins). param0 is always TA. Plain join → TB; group join → array<TB>.
    def result_param1_type(iterTypeB : TypeDeclPtr; at : LineInfo) : TypeDeclPtr {
        return null
    }
}

[call_macro(name="_join")]
class private LinqJoin : AstCallMacro_LinqJoinBase {
    //! implements _join(srca, srcb, on, result) shorthand notation that expands into join(srca, srcb, keya, keyb, result).
    override fnName = "join"
    def override result_param1_type(iterTypeB : TypeDeclPtr; at : LineInfo) : TypeDeclPtr {
        return clone_type(iterTypeB)
    }
}

[call_macro(name="_group_join")]
class private LinqGroupJoin : AstCallMacro_LinqJoinBase {
    //! implements _group_join(srca, srcb, on, result) shorthand — C# GroupJoin (outer).
    override fnName = "group_join"
    def override result_param1_type(iterTypeB : TypeDeclPtr; at : LineInfo) : TypeDeclPtr {
        // array<TB> — the match collection. Clear const/ref so it's a bare element array, matching the bucket type.
        var elemB = clone_type(iterTypeB)
        elemB.flags.constant = false
        elemB.flags.ref = false
        return new TypeDecl(baseType = Type.tArray, at = at, firstType = elemB)
    }
}

[call_macro(name="_left_join")]
class private LinqLeftJoin : AstCallMacro_LinqJoinBase {
    //! implements _left_join(srca, srcb, on, result) shorthand notation that expands into left_join(srca, srcb, keya, keyb, result).
    override fnName = "left_join"
}

[call_macro(name="_right_join")]
class private LinqRightJoin : AstCallMacro_LinqJoinBase {
    //! implements _right_join(srca, srcb, on, result) shorthand notation that expands into right_join(srca, srcb, keya, keyb, result).
    override fnName = "right_join"
}

[call_macro(name="_full_outer_join")]
class private LinqFullOuterJoin : AstCallMacro_LinqJoinBase {
    //! implements _full_outer_join(srca, srcb, on, result) shorthand notation that expands into full_outer_join(srca, srcb, keya, keyb, result).
    override fnName = "full_outer_join"
}

[call_macro(name="_cross_join")]
class private LinqCrossJoin : AstCallMacro {
    //! implements _cross_join(srca, srcb, result) shorthand notation that expands into cross_join(srca, srcb, result). 3-arg form with no `on` lambda — every (TA, TB) pair surfaces. Use `_where` after the cross to filter; that's the explicit-shape inner join.
    def override visit(prog : ProgramPtr; mod : Module?; var call : ExprCallMacro?) : Expression? {
        macro_verify(call.arguments |> length == 3, prog, call.at, "expecting _cross_join(srca, srcb, into) — 3 arguments")
        assume srcaType = call.arguments[0]._type
        assume srcbType = call.arguments[1]._type
        macro_verify(srcaType != null, prog, call.at, "first argument is not inferred yet")
        macro_verify(srcbType != null, prog, call.at, "second argument is not inferred yet")
        macro_verify(srcaType.isIterator || srcaType.isGoodArrayType, prog, call.at, "first argument must be an iterator or array")
        macro_verify(srcbType.isIterator || srcbType.isGoodArrayType, prog, call.at, "second argument must be an iterator or array")
        macro_verify(!srcaType.firstType.isAutoOrAlias, prog, call.at, "first argument element type cannot be auto or alias")
        macro_verify(!srcbType.firstType.isAutoOrAlias, prog, call.at, "second argument element type cannot be auto or alias")
        // Auto-type the result lambda's args when untyped — symmetric with _join's result lambda. Lets the linq_das reader (and users) write `_cross_join(a, b, $(x, y) => ...)` without spelling element types.
        var iterTypeA = clone_iterator_type(srcaType)
        var iterTypeB = clone_iterator_type(srcbType)
        var resultArg = call.arguments[2]
        if (resultArg != null && resultArg is ExprMakeBlock) {
            var rblk = (resultArg as ExprMakeBlock)._block
            if (rblk != null && rblk is ExprBlock) {
                var rb = rblk as ExprBlock
                if (rb.arguments |> length == 2
                        && (rb.arguments[0]._type == null || rb.arguments[0]._type.isAutoOrAlias)
                        && (rb.arguments[1]._type == null || rb.arguments[1]._type.isAutoOrAlias)) {
                    rb.arguments[0]._type = clone_type(iterTypeA)
                    rb.arguments[1]._type = clone_type(iterTypeB)
                }
            }
        }
        var res = qmacro(cross_join(
            $e(call.arguments[0]),
            $e(call.arguments[1]),
            $e(call.arguments[2])
        ))
        res.force_at(call.at)
        return res
    }
}

def private inject_block_param_type(var arg : ExpressionPtr; idx : int; ty : TypeDeclPtr) {
    //! Set the `idx`-th parameter type of a `$(…) => …` block argument to `ty`, when that parameter is still untyped (null / auto). Shared by the `_select_many` two-stage expansion (see below).
    if (arg == null || !(arg is ExprMakeBlock)) return
    var blk = (arg as ExprMakeBlock)._block
    if (blk == null || !(blk is ExprBlock)) return
    var b = blk as ExprBlock
    if (idx < (b.arguments |> length) && (b.arguments[idx]._type == null || b.arguments[idx]._type.isAutoOrAlias)) {
        b.arguments[idx]._type = clone_type(ty)
    }
}

[call_macro(name="_select_many")]
class private LinqSelectMany : AstCallMacro {
    //! Stage 1 of the correlated SelectMany sugar — `_select_many(src, $(o) => coll, $(o, x) => result)`.
    def override visit(prog : ProgramPtr; mod : Module?; var call : ExprCallMacro?) : Expression? {
        macro_verify(call.arguments |> length == 3, prog, call.at, "expecting _select_many(src, collection_selector, result_selector) — 3 arguments")
        assume srcaType = call.arguments[0]._type
        macro_verify(srcaType != null, prog, call.at, "first argument is not inferred yet")
        macro_verify(srcaType.isIterator || srcaType.isGoodArrayType, prog, call.at, "first argument must be an iterator or array")
        macro_verify(!srcaType.firstType.isAutoOrAlias, prog, call.at, "first argument element type cannot be auto or alias")
        var iterTypeA = clone_iterator_type(srcaType)
        inject_block_param_type(call.arguments[1], 0, iterTypeA)
        inject_block_param_type(call.arguments[2], 0, iterTypeA)
        // make_call (not qmacro) so the deferred stage dispatches as a call-macro, not a plain function.
        var res = make_call(call.at, "_select_many_2")
        var rc = res as ExprCallMacro
        rc.arguments |> emplace <| call.arguments[0]
        rc.arguments |> emplace <| call.arguments[1]
        rc.arguments |> emplace <| call.arguments[2]
        res.force_at(call.at)
        return res
    }
}

[call_macro(name="_select_many_2")]
class private LinqSelectMany2 : AstCallMacro {
    //! Stage 2 of `_select_many` (see above). The collection selector's parameter is now typed, so its
    //! body — and hence its element type — is inferred. Read that element type, inject it onto the result
    //! selector's SECOND parameter, and expand into `select_many_pair`.
    def override visit(prog : ProgramPtr; mod : Module?; var call : ExprCallMacro?) : Expression? {
        macro_verify(call.arguments |> length == 3, prog, call.at, "expecting _select_many_2(src, collection_selector, result_selector) — 3 arguments")
        assume collType = call.arguments[1]._type
        macro_verify(collType != null && collType.firstType != null, prog, call.at, "collection selector is not inferred yet")
        var collReturn = collType.firstType
        macro_verify(collReturn.isIterator || collReturn.isGoodArrayType, prog, call.at, "collection selector must return an iterator or array")
        macro_verify(!collReturn.firstType.isAutoOrAlias, prog, call.at, "collection element type cannot be auto or alias")
        var iterTypeB = clone_iterator_type(collReturn)
        inject_block_param_type(call.arguments[2], 1, iterTypeB)
        var res = qmacro(select_many_pair(
            $e(call.arguments[0]),
            $e(call.arguments[1]),
            $e(call.arguments[2])
        ))
        res.force_at(call.at)
        return res
    }
}

[call_macro(name="_group_by_lazy")]
class private LinqGroupByLazy : AstCallMacro_LinqPred2 {
    //! implements _group_by_lazy(iterator, key) shorthand notation that expands into group_by_lazy(iterator, $(_) => key). Yields one ``tuple<key; array<elem>>`` per bucket — IGrouping shape, suitable for in-memory per-group processing (top-N per group, group-scoped filtering, grouped JSON emission). See spec ``history/dasSQLITE/API_REWORK.md "### 19-group_by"``.
    override predName = "group_by_lazy"
}

[call_macro(name="_group_by")]
class private LinqGroupBy : AstCallMacro_LinqPred2 {
    //! implements _group_by(iterator, key) shorthand notation that expands into group_by_lazy(iterator, $(_) => key) — same backing function as ``_group_by_lazy``. In-memory both names are aliases:
    override predName = "group_by_lazy"
}

[call_macro(name="_group_by_to_array")]
class private LinqGroupByToArray : AstCallMacro_LinqPred2 {
    //! implements _group_by_to_array(iterator, key) shorthand notation that expands into group_by_lazy_to_array(iterator, $(_) => key).
    override predName = "group_by_lazy_to_array"
}

[call_macro(name="_group_by_lazy_to_array")]
class private LinqGroupByLazyToArray : AstCallMacro_LinqPred2 {
    //! implements _group_by_lazy_to_array(iterator, key) shorthand notation
    //! that expands into group_by_lazy_to_array(iterator, $(_) => key).
    //! Returns an ``array<tuple<key; array<elem>>>`` instead of an iterator.
    override predName = "group_by_lazy_to_array"
}

[call_macro(name="_having")]
class private LinqHaving : AstCallMacro_LinqPred2 {
    //! implements _having(iterator, expression) shorthand notation that expands into having_(iterator, $(_) => expression).
    override predName = "having_"
}

[call_macro(name="_having_to_array")]
class private LinqHavingToArray : AstCallMacro_LinqPred2 {
    //! implements _having_to_array(iterator, expression) shorthand notation
    //! that expands into having_to_array(iterator, $(_) => expression).
    override predName = "having_to_array"
}

[call_macro(name="_union_by_to_array")]
class private LinqUnionByToArray : AstCallMacro_LinqPredII2 {
    //! implements _union_by_to_array(iterator1, iterator2, expression) shorthand notation that expands into union_by_to_array(iterator1, iterator2, $(_) => expression).
    override predName = "union_by_to_array"
}
