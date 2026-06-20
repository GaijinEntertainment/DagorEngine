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

class private AstCallMacro_LinqPred2 : AstCallMacro {
    //! Base call macro for LINQ-style two-argument predicate operators.
    predName = "where_"
    //! This macro converts _where(iterator, expresion) into::
    //!     where_(iterator, $(_) => expression)
    //!
    def override visit(prog : ProgramPtr; mod : Module?; var call : ExprCallMacro?) : Expression? {
        //! Visits the LINQ predicate macro call and rewrites it with a lambda argument.
        // verifying arguments
        macro_verify(call.arguments |> length == 2, prog, call.at, "expecting {predName}(iterator, predicate)")
        assume arg0 = call.arguments[0]
        assume arg0type = call.arguments[0]._type
        macro_verify(arg0type != null, prog, call.at, "input argument is not inferred yet")
        macro_verify(arg0type.isIterator || arg0type.isGoodArrayType, prog, call.at, "expecting iterator or array")
        macro_verify(!arg0type.firstType.isAutoOrAlias, prog, call.at, "iterable type cannot be auto or alias")
        // replacing function
        var iterType = clone_iterator_type(arg0type)
        var res : ExpressionPtr
        if (call.arguments[1] is ExprMakeBlock) {
            // named-variable form: the argument is already a `$(x) => …` block (e.g. emitted by linq_das).
            // Inject the element type onto the user's parameter and pass the block through, instead of
            // wrapping a bare expression in `$(_ : iterType) => …`. The query's range variable thus survives verbatim.
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
        // allow inner `_` to shadow an outer `_` when chains nest
        // (e.g. `_where(_.x._in(arr |> _select(_.y)))`)
        (((res as ExprCall).arguments[1] as ExprMakeBlock)._block as ExprBlock).arguments[0].flags.can_shadow = true
        res.force_at(call.at)
        return res
    }
}

[call_macro(name="_where")]
class private LinqWhere : AstCallMacro_LinqPred2 {
    //! implements _where(iterator, expression) shorthand notation
    //! that expands into where_(iterator, $(_) => expression)
    //! for example::
    //!
    //!   each(foo)._where(_ < 5)
    override predName = "where_"
}

[call_macro(name="_where_to_array")]
class private LinqWhereToArray : AstCallMacro_LinqPred2 {
    //! implements _where_to_array(iterator, expression) shorthand notation
    //! that expands into where_to_array(iterator, $(_) => expression)
    //! for example::
    //!
    //!   each(foo)._where_to_array(_ < 5)
    override predName = "where_to_array"
}

[call_macro(name="_select")]
class private LinqSelect : AstCallMacro_LinqPred2 {
    //! implements _select(iterator, expression) shorthand notation
    //! that expands into select(iterator, $(_) => expression)
    //! for example::
    //!
    //!   each(foo)._select(_ * 2)
    override predName = "select"
}

[call_macro(name="_select_to_array")]
class private LinqSelectToArray : AstCallMacro_LinqPred2 {
    //! implements _select_to_array(iterator, expression) shorthand notation
    //! that expands into select_to_array(iterator, $(_) => expression)
    //! for example::
    //!
    //!   each(foo)._select_to_array(_ * 2)
    override predName = "select_to_array"
}

[call_macro(name="_min_by")]
class private LinqMinBy : AstCallMacro_LinqPred2 {
    //! implements _min_by(iterator, expression) shorthand notation
    //! that expands into min_by(iterator, $(_) => expression)
    //! for example::
    //!
    //!   each(foo)._min_by(_.value)
    override predName = "min_by"
}

[call_macro(name="_max_by")]
class private LinqMaxBy : AstCallMacro_LinqPred2 {
    //! implements _max_by(iterator, expression) shorthand notation
    //! that expands into max_by(iterator, $(_) => expression)
    //! for example::
    //!
    //!   each(foo)._max_by(_.value)
    override predName = "max_by"
}

[call_macro(name="_min_max_by")]
class private LinqMinMaxBy : AstCallMacro_LinqPred2 {
    //! implements _min_max_by(iterator, expression) shorthand notation
    //! that expands into min_max_by(iterator, $(_) => expression)
    //! for example::
    //!
    //!   each(foo)._min_max_by(_.value)
    override predName = "min_max_by"
}

[call_macro(name="_min_max_average_by")]
class private LinqMinMaxAverageBy : AstCallMacro_LinqPred2 {
    //! implements _min_max_average_by(iterator, expression) shorthand notation
    //! that expands into min_max_average_by(iterator, $(_) => expression)
    //! for example::
    //!
    //!   each(foo)._min_max_average_by(_.value)
    override predName = "min_max_average_by"
}

[call_macro(name="_skip_while")]
class private LinqSkipWhile : AstCallMacro_LinqPred2 {
    //! implements _skip_while(iterator, expression) shorthand notation
    //! that expands into skip_while(iterator, $(_) => expression)
    //! for example::
    //!
    //!   each(foo)._skip_while(_ < 5)
    override predName = "skip_while"
}

[call_macro(name="_take_while")]
class private LinqTakeWhile : AstCallMacro_LinqPred2 {
    //! implements _take_while(iterator, expression) shorthand notation
    //! that expands into take_while(iterator, $(_) => expression)
    //! for example::
    //!
    //!   each(foo)._take_while(_ < 5)
    override predName = "take_while"
}

[call_macro(name="_all")]
class private LinqAll : AstCallMacro_LinqPred2 {
    //! implements _all(iterator, expression) shorthand notation
    //! that expands into all(iterator, $(_) => expression)
    //! for example::
    //!
    //!   each(foo)._all(_ < 5)
    override predName = "all"
}

[call_macro(name="_any")]
class private LinqAny : AstCallMacro_LinqPred2 {
    //! implements _any(iterator, expression) shorthand notation
    //! that expands into any(iterator, $(_) => expression)
    //! for example::
    //!
    //!   each(foo)._any(_ < 5)
    override predName = "any"
}

[call_macro(name="_none")]
class private LinqNone : AstCallMacro_LinqPred2 {
    //! implements _none(iterator, expression) shorthand notation
    //! that expands into none(iterator, $(_) => expression)
    //! for example::
    //!
    //!   each(foo)._none(_ < 0)
    //!
    //! Explicit name (rather than ``!_any(...)``) because downstream
    //! AST walkers — notably the ``_sql`` translator that emits
    //! ``NOT EXISTS (…)`` — cannot reliably pattern-match ``!expr``
    //! across intermediate transforms.
    override predName = "none"
}

[call_macro(name="_count")]
class private LinqCount : AstCallMacro_LinqPred2 {
    //! implements _count(iterator, expression) shorthand notation
    //! that expands into count(iterator, $(_) => expression)
    //! for example::
    //!
    //!   each(foo)._count(_ > 3)
    override predName = "count"
}

[call_macro(name="_long_count")]
class private LinqLongCount : AstCallMacro_LinqPred2 {
    //! implements _long_count(iterator, expression) shorthand notation
    //! that expands into long_count(iterator, $(_) => expression)
    //! for example::
    //!
    //!   each(foo)._long_count(_ > 3)
    override predName = "long_count"
}

[call_macro(name="_unique_by")]
class private LinqUnique : AstCallMacro_LinqPred2 {
    //! implements _unique_by(iterator, expression) shorthand notation
    //! that expands into unique_by(iterator, $(_) => expression)
    //! for example::
    //!
    //!   each(foo)._unique_by(_.id)
    override predName = "unique_by"
}

[call_macro(name="_unique_by_to_array")]
class private LinqUniqueToArray : AstCallMacro_LinqPred2 {
    //! implements _unique_by_to_array(iterator, expression) shorthand notation
    //! that expands into unique_by_to_array(iterator, $(_) => expression)
    //! for example::
    //!
    //!   each(foo)._unique_by_to_array(_.id)
    override predName = "unique_by_to_array"
}

[call_macro(name="_distinct_by")]
class private LinqDistinctBy : AstCallMacro_LinqPred2 {
    //! implements _distinct_by(iterator, expression) shorthand notation
    //! that expands into distinct_by(iterator, $(_) => expression)
    //! for example::
    //!
    //!   each(foo)._distinct_by(_.id)
    override predName = "distinct_by"
}

[call_macro(name="_distinct_by_to_array")]
class private LinqDistinctByToArray : AstCallMacro_LinqPred2 {
    //! implements _distinct_by_to_array(iterator, expression) shorthand notation
    //! that expands into distinct_by_to_array(iterator, $(_) => expression)
    //! for example::
    //!
    //!   each(foo)._distinct_by_to_array(_.id)
    override predName = "distinct_by_to_array"
}

[call_macro(name="_order_by")]
class private LinqOrderBy : AstCallMacro_LinqPred2 {
    //! implements _order_by(iterator, expression) shorthand notation
    //! that expands into order_by(iterator, $(_) => expression)
    //! for example::
    //!
    //!   each(foo)._order_by(_.id)
    override predName = "order_by"
}

[call_macro(name="_order_by_to_array")]
class private LinqOrderByToArray : AstCallMacro_LinqPred2 {
    //! implements _order_by_to_array(iterator, expression) shorthand notation
    //! that expands into order_by_to_array(iterator, $(_) => expression)
    //! for example::
    //!
    //!   each(foo)._order_by_to_array(_.id)
    override predName = "order_by_to_array"
}

[call_macro(name="_order_by_descending")]
class private LinqOrderByDescending : AstCallMacro_LinqPred2 {
    //! implements _order_by_descending(iterator, expression) shorthand notation
    //! that expands into order_by_descending(iterator, $(_) => expression)
    //! for example::
    //!
    //!   each(foo)._order_by_descending(_.id)
    override predName = "order_by_descending"
}

[call_macro(name="_order_by_descending_to_array")]
class private LinqOrderByDescendingToArray : AstCallMacro_LinqPred2 {
    //! implements _order_by_descending_to_array(iterator, expression) shorthand notation
    //! that expands into order_by_descending_to_array(iterator, $(_) => expression)
    //! for example::
    //!
    //!   each(foo)._order_by_descending_to_array(_.id)
    override predName = "order_by_descending_to_array"
}

class private AstCallMacro_LinqOrderByKeys : AstCallMacro {
    //! Base call macro for the multi-key order operator. The key argument is already a
    //! `$(x) => (k1, k2, …)` block (tuple of keys); the third argument is a compile-time `uint`
    //! descMask (bit i → key i descending). Injects the element type onto the key block parameter,
    //! then passes all three through to `order_by_keys` / `order_by_keys_to_array`.
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
            // named-variable form (e.g. emitted by linq_das): inject the element type onto the user's
            // parameter and pass the `$(x) => (k1, k2, …)` block through verbatim.
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
    //! implements sequence_equal_by(i1, i2, expresion) shorthand notation
    //! that expands into sequence_equal_by(i1, i2, $(_) => expression)
    //! for example::
    //!
    //!   each(foo1)._sequence_equal_by(each(foo2), _.id)
    def override visit(prog : ProgramPtr; mod : Module?; var call : ExprCallMacro?) : Expression? {
        //! Visits the LINQ two-iterator predicate macro call and rewrites it with a lambda argument.
        // verifying arguments
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
            // named-variable form: the argument is already a `$(x) => …` block — inject the element type
            // onto the user's parameter and pass it through (see AstCallMacro_LinqPred2 for rationale).
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
    //! implements _sequence_equal_by(iterator1, iterator2, expression) shorthand notation
    //! that expands into sequence_equal_by(iterator1, iterator2, $(_) => expression)
    //! for example::
    //!
    //!   each(foo1)._sequence_equal_by(each(foo2), _.id)
    override predName = "sequence_equal_by"
}

[call_macro(name="_except_by")]
class private LinqExceptBy : AstCallMacro_LinqPredII2 {
    //! implements _except_by(iterator1, iterator2, expression) shorthand notation
    //! that expands into except_by(iterator1, iterator2, $(_) => expression)
    //! for example::
    //!
    //!   each(foo1)._except_by(each(foo2), _.id)
    override predName = "except_by"
}

[call_macro(name="_except_by_to_array")]
class private LinqExceptByToArray : AstCallMacro_LinqPredII2 {
    //! implements _except_by_to_array(iterator1, iterator2, expression) shorthand notation
    //! that expands into except_by_to_array(iterator1, iterator2, $(_) => expression)
    //! for example::
    //!
    //!   each(foo1)._except_by_to_array(each(foo2), _.id)
    override predName = "except_by_to_array"
}

[call_macro(name="_intersect_by")]
class private LinqIntersectBy : AstCallMacro_LinqPredII2 {
    //! implements _intersect_by(iterator1, iterator2, expression) shorthand notation
    //! that expands into intersect_by(iterator1, iterator2, $(_) => expression)
    //! for example::
    //!
    //!   each(foo1)._intersect_by(each(foo2), _.id)
    override predName = "intersect_by"
}

[call_macro(name="_intersect_by_to_array")]
class private LinqIntersectByToArray : AstCallMacro_LinqPredII2 {
    //! implements _intersect_by_to_array(iterator1, iterator2, expression) shorthand notation
    //! that expands into intersect_by_to_array(iterator1, iterator2, $(_) => expression)
    //! for example::
    //!
    //!   each(foo1)._intersect_by_to_array(each(foo2), _.id)
    override predName = "intersect_by_to_array"
}

[call_macro(name="_union_by")]
class private LinqUnionBy : AstCallMacro_LinqPredII2 {
    //! implements _union_by(iterator1, iterator2, expression) shorthand notation
    //! that expands into union_by(iterator1, iterator2, $(_) => expression)
    //! for example::
    //!
    //!   each(foo1)._union_by(each(foo2), _.id)
    override predName = "union_by"
}

class private AstCallMacro_LinqContainsFlip : AstCallMacro {
    //! Base call macro for ``_in`` / ``_not_in``.
    //!
    //! Rewrites ``x._in(subquery)`` into ``contains(subquery, x)`` —
    //! i.e. flips arg order so the surface syntax reads "x is in collection"
    //! while the backing function keeps its iterable-first signature.
    //! ``_not_in`` additionally negates the result.
    //!
    //! Both forms are in-memory-useful on any ``array<T>`` / ``iterator<T>``;
    //! under the ``_sql`` translator they emit ``x IN (…)`` and
    //! ``x NOT IN (…)``. Separate positive / negative names (not
    //! ``!x._in(...)``) because the ``_sql`` AST walker cannot reliably
    //! pattern-match ``!expr`` across intermediate transforms.
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
    //! implements _in(element, subquery) shorthand notation
    //! that expands into contains(subquery, element)
    //! for example::
    //!
    //!   x._in([iterator for(y in 0..5); y])   // true if x is one of 0..4
    override opName = "_in"
    override negated = false
}

[call_macro(name="_not_in")]
class private LinqNotIn : AstCallMacro_LinqContainsFlip {
    //! implements _not_in(element, subquery) shorthand notation
    //! that expands into !contains(subquery, element)
    //! for example::
    //!
    //!   x._not_in([iterator for(y in 0..5); y])
    //!
    //! Separate name (not ``!x._in(...)``) so the ``_sql`` translator
    //! can emit ``NOT IN`` without pattern-matching ``!expr``.
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
    //! Validate the ``on`` predicate of ``_join`` / ``_left_join`` and
    //! extract the two key-selector bodies.
    //!
    //! Accepts only the canonical equi-join shape
    //! ``$(l : TA, r : TB) => l.X == r.Y`` — a 2-arg lambda whose body
    //! is a single ``==`` comparison. Anything else (theta joins,
    //! ``AND``-chained multi-key equality, function calls on either
    //! side) emits a ``macro_error`` pointing at raw SQL as the
    //! escape hatch. The spec lives in
    //! modules/dasSQLITE/API_REWORK.md "### 23-joins".
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
        macro_error(prog, on_pred.at, "equi-join `on` predicate must use `==` — theta joins and `&&`-chained multi-key equality are not supported; use raw SQL instead")
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
    //!
    //! Rewrites ``_join(srca, srcb, (l,r) => l.X == r.Y, into)`` into
    //! ``join(srca, srcb, $(l:TA) => l.X, $(r:TB) => r.Y, into)``.
    //!
    //! Equi-join constraint (see modules/dasSQLITE/API_REWORK.md
    //! "### 23-joins"): the ``on`` predicate must be a single ``==``
    //! between one left-arg expression and one right-arg expression.
    //! Theta joins and ``&&``-chained multi-key equality are
    //! ``macro_error`` at the ``on`` argument. Side mapping is
    //! positional — left side of ``==`` becomes the left key selector,
    //! right side becomes the right. A reversed canonical form
    //! (``r.Y == l.X``) surfaces as a type error on the generated
    //! ``join`` call, which is still a compile-time diagnostic.
    //!
    //! The two key-selector bodies are cloned and their original
    //! lambda-arg variable names are renamed to fresh
    //! ``__la__`` / ``__lb__`` identifiers, so the synthesized
    //! single-arg blocks have stable parameter names regardless of
    //! what the user wrote.
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
        // Auto-type the result lambda's args when untyped — symmetric with `on`, which accepts `(l,r)=>...`
        // without types. Lets users write `_join(..., $(l,r)=>l.id==r.id, $(l,r)=>(l.x,r.y))` against any source
        // — including from_decs_template(type<T>) where the iterator element is a tuple<...> painful to spell by
        // hand. param0 is always TA; param1 comes from `result_param1_type`: plain `_join` → TB; `_group_join` →
        // array<TB>. `_left_join` / `_right_join` / `_full_outer_join` need Option<T> on one or both sides, so they
        // return null and their untyped result lambdas stay user-spelled.
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
    //! implements _join(srca, srcb, on, result) shorthand notation
    //! that expands into join(srca, srcb, keya, keyb, result).
    //! `on` must be an equi-join lambda `(l, r) => l.X == r.Y`.
    //! for example::
    //!
    //!   each(persons)._join(each(pets),
    //!       $(p : Person, pet : Pet) => p.name == pet.owner.name,
    //!       $(p : Person, pet : Pet) => (p.name, pet.name))
    override fnName = "join"
    def override result_param1_type(iterTypeB : TypeDeclPtr; at : LineInfo) : TypeDeclPtr {
        return clone_type(iterTypeB)
    }
}

[call_macro(name="_group_join")]
class private LinqGroupJoin : AstCallMacro_LinqJoinBase {
    //! implements _group_join(srca, srcb, on, result) shorthand — C# GroupJoin (outer).
    //! Same `on` equi-key shape as _join, but `result`'s right-side parameter is
    //! `array<TB>`: every TA surfaces, paired with the array of matching TB
    //! (empty when none match).
    //! for example::
    //!
    //!   each(persons)._group_join(each(pets),
    //!       $(p : Person, pet : Pet) => p.name == pet.owner.name,
    //!       $(p : Person, pets : array<Pet>) => (p.name, pets |> length))
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
    //! implements _left_join(srca, srcb, on, result) shorthand notation
    //! that expands into left_join(srca, srcb, keya, keyb, result).
    //! Same shape as _join, but `result`'s right-side parameter is
    //! `Option<TB>` — `none` when the left row has no match,
    //! `some(tb)` otherwise.
    //! for example::
    //!
    //!   each(persons)._left_join(each(pets),
    //!       $(p : Person, pet : Pet) => p.name == pet.owner.name,
    //!       $(p : Person, pet : Option<Pet>) => (p.name, pet |> is_some))
    override fnName = "left_join"
}

[call_macro(name="_right_join")]
class private LinqRightJoin : AstCallMacro_LinqJoinBase {
    //! implements _right_join(srca, srcb, on, result) shorthand notation
    //! that expands into right_join(srca, srcb, keya, keyb, result).
    //! Mirror of `_left_join` with the Option on the LEFT side — every
    //! TB row surfaces; TA is `Option<TA>`, `none` when no left match.
    //! for example::
    //!
    //!   each(persons)._right_join(each(pets),
    //!       $(p : Person, pet : Pet) => p.name == pet.owner.name,
    //!       $(p : Option<Person>, pet : Pet) => (pet.name, p |> is_some))
    override fnName = "right_join"
}

[call_macro(name="_full_outer_join")]
class private LinqFullOuterJoin : AstCallMacro_LinqJoinBase {
    //! implements _full_outer_join(srca, srcb, on, result) shorthand notation
    //! that expands into full_outer_join(srca, srcb, keya, keyb, result).
    //! Both sides are `Option<T>` — every left row and every right row
    //! surfaces, paired with `none` on the other side when unmatched.
    //! for example::
    //!
    //!   each(persons)._full_outer_join(each(pets),
    //!       $(p : Person, pet : Pet) => p.name == pet.owner.name,
    //!       $(p : Option<Person>, pet : Option<Pet>) => (p |> is_some, pet |> is_some))
    override fnName = "full_outer_join"
}

[call_macro(name="_cross_join")]
class private LinqCrossJoin : AstCallMacro {
    //! implements _cross_join(srca, srcb, result) shorthand notation
    //! that expands into cross_join(srca, srcb, result). 3-arg form
    //! with no `on` lambda — every (TA, TB) pair surfaces. Use `_where`
    //! after the cross to filter; that's the explicit-shape inner join.
    //! for example::
    //!
    //!   each(persons)._cross_join(each(pets),
    //!       $(p : Person, pet : Pet) => (p.name, pet.name))
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
        // Auto-type the result lambda's args when untyped — symmetric with _join's result lambda. Lets the
        // linq_das reader (and users) write `_cross_join(a, b, $(x, y) => ...)` without spelling element types.
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
    //! Set the `idx`-th parameter type of a `$(…) => …` block argument to `ty`, when that parameter is
    //! still untyped (null / auto). Shared by the `_select_many` two-stage expansion (see below).
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
    //! Injects the outer element type onto the collection selector's parameter and the result selector's
    //! FIRST parameter, then defers to `_select_many_2`. Two stages are required because the inner element
    //! type is only knowable after the collection selector's body is inferred — which cannot happen within
    //! a single macro pass (injecting a parameter type does not re-run inference on the block body).
    //! for example::
    //!
    //!   each(orders)._select_many($(o) => o.lines |> to_sequence(), $(o, x) => (id = o.id, sku = x.sku))
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
    //! implements _group_by_lazy(iterator, key) shorthand notation that
    //! expands into group_by_lazy(iterator, $(_) => key). Yields one
    //! ``tuple<key; array<elem>>`` per bucket — IGrouping shape, suitable
    //! for in-memory per-group processing (top-N per group, group-scoped
    //! filtering, grouped JSON emission). See spec
    //! ``modules/dasSQLITE/API_REWORK.md "### 19-group_by"``.
    //! for example::
    //!
    //!   each(people)._group_by_lazy(_.age)
    override predName = "group_by_lazy"
}

[call_macro(name="_group_by")]
class private LinqGroupBy : AstCallMacro_LinqPred2 {
    //! implements _group_by(iterator, key) shorthand notation that
    //! expands into group_by_lazy(iterator, $(_) => key) — same backing
    //! function as ``_group_by_lazy``. In-memory both names are aliases:
    //! the chain works identically with or without an outer ``_sql(...)``
    //! wrapper. The two names exist so downstream macros (notably the
    //! ``dasSQLITE`` ``_sql`` translator) can pattern-match on intent —
    //! ``_group_by`` signals "I will aggregate, please emit ``GROUP BY``
    //! in SQL", ``_group_by_lazy`` signals "I want IGrouping shape, do
    //! per-group windowing in memory" (``macro_error`` inside ``_sql``).
    //!
    //! C#-style usage in plain in-memory chains — access the key via
    //! ``_._0`` and the group elements via ``_._1`` in the next ``_select``::
    //!
    //!   arr._group_by(_.City)._select((
    //!       City   = _._0,
    //!       N      = _._1 |> length,
    //!       AvgAge = _._1 |> select($(u : Person) => u.Age) |> average()))
    //!
    //! The inner ``select`` lambda parameter is intentionally **named**
    //! (``$(u : Person)``), not ``_`` — the outer ``_select`` lambda
    //! already binds ``_`` to the group tuple, and daslang flags a
    //! same-name-different-type lambda nested inside as a shadowing
    //! error. Naming the inner parameter sidesteps it.
    //!
    //! The ``_sql`` macro recognises this same shape (``_._0`` as the
    //! group key column, ``_._1 |> length`` as ``COUNT(*)``,
    //! ``_._1 |> select($(u:T) => u.X) |> sum()`` as ``SUM("X")``, and
    //! the same pattern for ``average``/``min``/``max``) and emits
    //! ``GROUP BY`` plus the corresponding SQL aggregates.
    override predName = "group_by_lazy"
}

[call_macro(name="_group_by_to_array")]
class private LinqGroupByToArray : AstCallMacro_LinqPred2 {
    //! implements _group_by_to_array(iterator, key) shorthand notation
    //! that expands into group_by_lazy_to_array(iterator, $(_) => key).
    //! Returns an ``array<tuple<key; array<elem>>>`` — same backing
    //! function as ``_group_by_lazy_to_array``; see ``_group_by`` for
    //! the rationale behind the two names.
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
    //! implements _having(iterator, expression) shorthand notation that
    //! expands into having_(iterator, $(_) => expression).
    //!
    //! In-memory semantics are identical to ``_where`` — the dedicated
    //! name lets the SQL translator emit a ``HAVING`` clause (vs.
    //! ``WHERE``) without pattern-matching across intermediate transforms.
    //! Pre-aggregate filters still go through ``_where``; chain position
    //! (before vs. after ``_group_by``) selects the right one. See spec
    //! ``modules/dasSQLITE/API_REWORK.md "### 19-group_by"``.
    //! for example::
    //!
    //!   each(buckets)._having(_._1 |> length > 5)
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
    //! implements _union_by_to_array(iterator1, iterator2, expression) shorthand notation
    //! that expands into union_by_to_array(iterator1, iterator2, $(_) => expression)
    //! for example::
    //!
    //!   each(foo1)._union_by_to_array(each(foo2), _.id)
    override predName = "union_by_to_array"
}
