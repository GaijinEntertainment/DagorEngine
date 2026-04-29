options gen2
options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false

module linq_boost shared public

//! LINQ operator-pipe macro support.
//!
//! Provides ``|>`` pipe-syntax rewrites for LINQ operations so that
//! ``arr |> where(...) |> select(...)`` chains compile into efficient
//! iterator pipelines.
//! Requires the ``linq`` module.

require daslib/linq public

require daslib/ast_boost
require daslib/templates_boost
require daslib/macro_boost

def private clone_iterator_type(it : TypeDeclPtr) : TypeDeclPtr {
    var inscope res <- clone_type(it.firstType)
    res.flags &= ~TypeDeclFlags.ref
    res.flags |= TypeDeclFlags.constant
    return <- res
}

class private AstCallMacro_LinqPred2 : AstCallMacro {
    //! Base call macro for LINQ-style two-argument predicate operators.
    predName = "where_"
    //! This macro converts _where(iterator, expresion) into::
    //!     where_(iterator, $(_) => expression)
    //!
    def override visit(prog : ProgramPtr; mod : Module?; var call : smart_ptr<ExprCallMacro>) : ExpressionPtr {
        //! Visits the LINQ predicate macro call and rewrites it with a lambda argument.
        // verifying arguments
        macro_verify(call.arguments |> length == 2, prog, call.at, "expecting {predName}(iterator, predicate)")
        assume arg0 = call.arguments[0]
        assume arg0type = call.arguments[0]._type
        macro_verify(arg0type != null, prog, call.at, "input argument is not inferred yet")
        macro_verify(arg0type.isIterator || arg0type.isGoodArrayType, prog, call.at, "expecting iterator or array")
        macro_verify(!arg0type.firstType.isAutoOrAlias, prog, call.at, "iterable type cannot be auto or alias")
        // replacing function
        var inscope iterType <- clone_iterator_type(arg0type)
        var inscope res <- qmacro($c(predName)($e(arg0), $(_ : $t(iterType)) => $e(call.arguments[1])))
        res.force_at(call.at)
        return <- res
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

[call_macro(name="_count")]
class private LinqCount : AstCallMacro_LinqPred2 {
    //! implements _count(iterator, expression) shorthand notation
    //! that expands into count(iterator, $(_) => expression)
    //! for example::
    //!
    //!   each(foo)._count(_ > 3)
    override predName = "count"
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

class private AstCallMacro_LinqPredII2 : AstCallMacro {
    //! Base call macro for LINQ-style two-iterator two-argument predicate operators.
    predName = "sequence_equal_by"
    //! implements sequence_equal_by(i1, i2, expresion) shorthand notation
    //! that expands into sequence_equal_by(i1, i2, $(_) => expression)
    //! for example::
    //!
    //!   each(foo1)._sequence_equal_by(each(foo2), _.id)
    def override visit(prog : ProgramPtr; mod : Module?; var call : smart_ptr<ExprCallMacro>) : ExpressionPtr {
        //! Visits the LINQ two-iterator predicate macro call and rewrites it with a lambda argument.
        // verifying arguments
        macro_verify(call.arguments |> length == 3, prog, call.at, "expecting {predName}(iterator or array, iterator or array, predicate)")
        macro_verify(call.arguments[0]._type != null, prog, call.at, "expecting iterator or array")
        macro_verify(call.arguments[0]._type.isIterator || call.arguments[0]._type.isGoodArrayType, prog, call.at, "expecting iterator or array")
        macro_verify(call.arguments[1]._type != null, prog, call.at, "expecting iterator or array")
        macro_verify(call.arguments[1]._type.isIterator || call.arguments[1]._type.isGoodArrayType, prog, call.at, "expecting iterator or array")
        macro_verify(!call.arguments[0]._type.firstType.isAutoOrAlias, prog, call.at, "iterator or array type cannot be auto or alias")
        // replacing function
        var inscope iterType <- clone_iterator_type(call.arguments[0]._type)
        var inscope res <- qmacro($c(predName)($e(call.arguments[0]), $e(call.arguments[1]), $(_ : $t(iterType)) => $e(call.arguments[2])))
        res.force_at(call.at)
        return <- res
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

[call_macro(name="_union_by_to_array")]
class private LinqUnionByToArray : AstCallMacro_LinqPredII2 {
    //! implements _union_by_to_array(iterator1, iterator2, expression) shorthand notation
    //! that expands into union_by_to_array(iterator1, iterator2, $(_) => expression)
    //! for example::
    //!
    //!   each(foo1)._union_by_to_array(each(foo2), _.id)
    override predName = "union_by_to_array"
}

def private is_call_or_generic(var expr : Expression?, name, moduleName : string) : ExprCall? {
    if (expr is ExprCall) {
        var call = expr as ExprCall
        if (call.func.name == name && call.func._module.name == moduleName) {
            return call
        } elif (call.func.fromGeneric != null && call.func.fromGeneric.name == name && call.func.fromGeneric._module.name == moduleName) {
            return call
        }
    }
    return null
}

def private is_call_or_generic_arg0(var expr : Expression?, name, moduleName : string) : Expression? {
    //! returns null if not to_array, otherwise returns the argument of to_array
    var call = expr.is_call_or_generic(name, moduleName)
    return call.arguments[0].get_ptr() if (call != null)
    return null
}

def private fold_linq_cond(var expr : ExpressionPtr; argName : string) : ExpressionPtr {
    // if its a block with a single argument and a single return
    // we rename argument to specified value, and give back the return value
    // otherwise its an invoke
    if (expr is ExprMakeBlock) {
        var mblk = expr as ExprMakeBlock
        var blk = mblk._block as ExprBlock
        if (blk.arguments |> length == 1 && blk.list |> length == 1 && blk.list[0] is ExprReturn) {
            var ret = blk.list[0] as ExprReturn
            if (ret.subexpr != null) {
                var inscope res <- clone_expression(ret.subexpr)
                var inscope rules : Template
                rules |> renameVariable(string(blk.arguments[0].name), argName)
                apply_template(rules, res.at, res)
                return <- res
            }
        }
    }
    return <- qmacro(invoke($e(expr), $i(argName)))
}

struct private LinqCall {
    //! Internal struct representing a chained LINQ method call.
    name : string
    moduleName : string = "linq"
    skip : bool = false             // if specified, expression is skipped in folding
    inplace : bool = false          // if specified, expression is inplace and does not create new variable
    recursive : array<int>          // indices of arguments to apply fold_linq_default on
}

var private linqCalls = {
// filtering data
    "where_" => LinqCall(name = "where_"),
    "where_to_array" => LinqCall(name = "where_"),
// transform operations
    "select" => LinqCall(name = "select"),
    "select_to_array" => LinqCall(name = "select"),
    "select_many" => LinqCall(name = "select_many"),
    "select_many_to_array" => LinqCall(name = "select_many"),
    "zip" => LinqCall(name = "zip", recursive = [1, 2]),
    "zip_to_array" => LinqCall(name = "zip", recursive = [1, 2]),
// sorting data
    "reverse" => LinqCall(name = "reverse", inplace = true),
    "reverse_to_array" => LinqCall(name = "reverse", inplace = true),
    "order" => LinqCall(name = "order", inplace = true),
    "order_to_array" => LinqCall(name = "order", inplace = true),
    "order_descending" => LinqCall(name = "order_descending", inplace = true),
    "order_descending_to_array" => LinqCall(name = "order_descending", inplace = true),
    "order_by" => LinqCall(name = "order_by", inplace = true),
    "order_by_to_array" => LinqCall(name = "order_by", inplace = true),
    "order_by_descending" => LinqCall(name = "order_by_descending", inplace = true),
    "order_by_descending_to_array" => LinqCall(name = "order_by_descending", inplace = true),
// aggregate
    "count" => LinqCall(name = "count"),
    "long_count" => LinqCall(name = "long_count"),
    "average" => LinqCall(name = "average"),
    "min" => LinqCall(name = "min"),
    "min_by" => LinqCall(name = "min_by"),
    "max" => LinqCall(name = "max"),
    "max_by" => LinqCall(name = "max_by"),
    "min_max" => LinqCall(name = "min_max"),
    "min_max_by" => LinqCall(name = "min_max_by"),
    "min_max_average" => LinqCall(name = "min_max_average"),
    "min_max_average_by" => LinqCall(name = "min_max_average_by"),
    "aggregate" => LinqCall(name = "aggregate"),
    "sum" => LinqCall(name = "sum"),
// ranges
    "skip" => LinqCall(name = "skip", inplace = true),
    "skip_to_array" => LinqCall(name = "skip", inplace = true),
    "skip_while" => LinqCall(name = "skip_while"),
    "skip_while_to_array" => LinqCall(name = "skip_while"),
    "take" => LinqCall(name = "take", inplace = true),
    "take_to_array" => LinqCall(name = "take", inplace = true),
    "take_while" => LinqCall(name = "take_while"),
    "take_while_to_array" => LinqCall(name = "take_while"),
    "skip_last" => LinqCall(name = "skip_last", inplace = true),
    "skip_last_to_array" => LinqCall(name = "skip_last", inplace = true),
    "take_last" => LinqCall(name = "take_last", inplace = true),
    "take_last_to_array" => LinqCall(name = "take_last", inplace = true),
    "chunk" => LinqCall(name = "chunk"),
    "chunk_to_array" => LinqCall(name = "chunk"),
// concatenation operations
    "concat" => LinqCall(name = "concat", recursive = [1], inplace = true),
    "concat_to_array" => LinqCall(name = "concat", recursive = [1], inplace = true),
    "prepend" => LinqCall(name = "prepend", inplace = true),
    "prepend_to_array" => LinqCall(name = "prepend", inplace = true),
    "append" => LinqCall(name = "append", inplace = true),
    "append_to_array" => LinqCall(name = "append", inplace = true),
// set operations
    "distinct" => LinqCall(name = "distinct", inplace = true),
    "distinct_to_array" => LinqCall(name = "distinct", inplace = true),
    "distinct_by" => LinqCall(name = "distinct_by", inplace = true),
    "distinct_by_to_array" => LinqCall(name = "distinct_by", inplace = true),
    "union" => LinqCall(name = "union", recursive = [1]),
    "union_to_array" => LinqCall(name = "union", recursive = [1]),
    "union_by" => LinqCall(name = "union_by", recursive = [1]),
    "union_by_to_array" => LinqCall(name = "union_by", recursive = [1]),
    "except" => LinqCall(name = "except", recursive = [1]),
    "except_to_array" => LinqCall(name = "except", recursive = [1]),
    "except_by" => LinqCall(name = "except_by", recursive = [1]),
    "except_by_to_array" => LinqCall(name = "except_by", recursive = [1]),
    "intersect" => LinqCall(name = "intersect", recursive = [1]),
    "intersect_to_array" => LinqCall(name = "intersect", recursive = [1]),
    "intersect_by" => LinqCall(name = "intersect_by", recursive = [1]),
    "intersect_by_to_array" => LinqCall(name = "intersect_by", recursive = [1]),
    "unique" => LinqCall(name = "unique", inplace = true),
    "unique_to_array" => LinqCall(name = "unique", inplace = true),
    "unique_by" => LinqCall(name = "unique_by", inplace = true),
    "unique_by_to_array" => LinqCall(name = "unique_by", inplace = true),
// element operations
    "element_at" => LinqCall(name = "element_at"),
    "element_at_or_default" => LinqCall(name = "element_at_or_default"),
    "first" => LinqCall(name = "first"),
    "first_or_default" => LinqCall(name = "first_or_default"),
    "last" => LinqCall(name = "last"),
    "last_or_default" => LinqCall(name = "last_or_default"),
    "single" => LinqCall(name = "single"),
    "single_or_default" => LinqCall(name = "single_or_default"),
// join operations
    "join" => LinqCall(name = "join", recursive = [1]),
    "join_to_array" => LinqCall(name = "join", recursive = [1]),
    "group_join" => LinqCall(name = "group_join", recursive = [1]),
    "group_join_to_array" => LinqCall(name = "group_join", recursive = [1]),
    "group_by" => LinqCall(name = "group_by"),
    "group_by_to_array" => LinqCall(name = "group_by"),
// querying data
    "any" => LinqCall(name = "any"),
    "all" => LinqCall(name = "all"),
    "contains" => LinqCall(name = "contains"),
// skippable
    "to_table" => LinqCall(name = "to_table"),
    "to_array" => LinqCall(name = "to_array", moduleName = "builtin", skip = true),
    "to_sequence" => LinqCall(name = "to_sequence", skip = true),
}

[macro_function]
def private is_linq_call(var expr : Expression?) : bool {
    if (expr is ExprCall) {
        var call = expr as ExprCall
        var funcName = string(call.func.name)
        while (call.func.fromGeneric != null) {
            call.func = call.func.fromGeneric.get_ptr()
            funcName = string(call.func.name)
        }
        if (linqCalls.key_exists(funcName)) {
            let linqCall & = unsafe(linqCalls?[funcName] ?? default<LinqCall>)
            if (linqCall.moduleName == call.func._module.name) {
                return true
            }
        }
    }
    return false
}

[macro_function]
def private flatten_linq(var expr : ExpressionPtr)  {
    var top = get_ptr(expr)
    var calls : array<tuple<ExprCall?; LinqCall?>>
    while (top != null) {
        if (top is ExprCall) {
            var topCall = top as ExprCall
            var topFunc = topCall.func
            while (topFunc.fromGeneric != null) {
                topFunc = topFunc.fromGeneric.get_ptr()
            }
            let funcName = string(topFunc.name)
            if (!linqCalls.key_exists(funcName)) {
                break
            }
            var linqCall & = unsafe(linqCalls?[funcName] ?? default<LinqCall>)
            if (linqCall.moduleName != topFunc._module.name) {
                break
            }
            if (!linqCall.skip) {
                calls.push((topCall, unsafe(addr(linqCall))))
            }
            top = topCall.arguments[0].get_ptr()
        } else {
            break
        }
    }
    calls.reverse_inplace()
    // ok, at this point top is source
    return <- (top, calls)
}

struct private FoldSequence {
    //! Internal struct representing a fold operation sequence.
    calls : array<string>
    folder : function<(
        argIndex : int;
        var topValue : ExpressionPtr;
        var blk : ExprBlock?;
        var calls : array<tuple<ExprCall?; LinqCall?>>
    ) : ExpressionPtr>
}

var private g_foldSeq = [               // those are applied in order
// order and distinct (for both orders)
    FoldSequence(
        calls = ["order", "distinct" ],
        folder = @@fold_order_distinct
    ),
    FoldSequence(
        calls = ["distinct", "order" ],
        folder = @@fold_order_distinct
    ),
// select and where
    FoldSequence(
        calls = ["where_", "select" ],
        folder = @@fold_where_select
    ),
    FoldSequence(
        calls = ["select", "where_"],
        folder = @@fold_select_where
    ),
    FoldSequence(
        calls = ["where_"],
        folder = @@fold_where
    ),
    FoldSequence(
        calls = ["select"],
        folder = @@fold_select
    ),
]

[macro_function]
def private append_comprehension(argIndex : int; var topValue : ExpressionPtr; var comprehension : ExpressionPtr; var blk : ExprBlock?; at : LineInfo) : ExpressionPtr {
    comprehension.force_at(at)
    comprehension.force_generated(true)
    let newArgName = "pass_{argIndex}"
    blk.list |> emplace_new <| qmacro_expr() {
        var $i(newArgName) <- $e(comprehension)
    }
    (blk.list.back() as ExprLet).variables[0].flags |= VariableFlags.can_shadow
    if (argIndex != 0) {
        blk.list |> emplace_new <| qmacro_expr() {
            delete $e(topValue)
        }
    }
    return <- qmacro($i(newArgName))
}

[macro_function]
def private fold_order_distinct(argIndex : int; var topValue : ExpressionPtr; var blk : ExprBlock?; var calls : array<tuple<ExprCall?; LinqCall?>>) : ExpressionPtr {
    //! replaces order + distinct into a single order + unique
    var eOrder = calls[0]._0
    var eDistinct = calls[1]._0
    if (argIndex == 0) {
        var inscope comprehension <- qmacro(order_unique_folded($e(topValue)))
        return <- append_comprehension(argIndex, topValue, comprehension, blk, calls[0]._0.at)
    } else {
        blk.list |> emplace_new <| qmacro(order_unique_folded_inplace($e(topValue)))
        return <- clone_expression(topValue)
    }
}

[macro_function]
def private fold_where_select(argIndex : int; var topValue : ExpressionPtr; var blk : ExprBlock?; var calls : array<tuple<ExprCall?; LinqCall?>>) : ExpressionPtr {
    //! folds where + select into a single comprehension
    var eWhere = calls[0]._0
    var eSelect = calls[1]._0
    var inscope iterType <- clone_type(eWhere.arguments[0]._type.firstType)
    var inscope whereCond : ExpressionPtr
    whereCond |> move_new <| fold_linq_cond(eWhere.arguments[1], "it")
    var inscope selectExpr : ExpressionPtr
    if (eSelect != null) {
        selectExpr |> move_new <| fold_linq_cond(eSelect.arguments[1], "it")
    } else {
        selectExpr |> move_new <| new ExprVar(name := "it", at = topValue.at, _type := iterType)
    }
    var inscope comprehension <- qmacro([for (it in $e(topValue)); $e(selectExpr); where $e(whereCond)])
    return <- append_comprehension(argIndex, topValue, comprehension, blk, calls[0]._0.at)
}

[macro_function]
def private fold_where(argIndex : int; var topValue : ExpressionPtr; var blk : ExprBlock?; var calls : array<tuple<ExprCall?; LinqCall?>>) : ExpressionPtr {
    //! folds where + select into a single comprehension
    var eWhere = calls[0]._0
    var inscope whereCond : ExpressionPtr
    whereCond |> move_new <| fold_linq_cond(eWhere.arguments[1], "it")
    var inscope comprehension <- qmacro([for (it in $e(topValue)); it; where $e(whereCond)])
    return <- append_comprehension(argIndex, topValue, comprehension, blk, calls[0]._0.at)
}

[macro_function]
def private fold_select(argIndex : int; var topValue : ExpressionPtr; var blk : ExprBlock?; var calls : array<tuple<ExprCall?; LinqCall?>>) : ExpressionPtr {
    //! folds select into a single comprehension
    var eSelect = calls[0]._0
    var inscope selectExpr : ExpressionPtr
    selectExpr |> move_new <| fold_linq_cond(eSelect.arguments[1], "it")
    var inscope comprehension <- qmacro([for (it in $e(topValue)); $e(selectExpr)])
    return <- append_comprehension(argIndex, topValue, comprehension, blk, calls[0]._0.at)
}

[macro_function]
def private fold_select_where(argIndex : int; var topValue : ExpressionPtr; var blk : ExprBlock?; var calls : array<tuple<ExprCall?; LinqCall?>>) : ExpressionPtr {
    //! folds where + select into a single comprehension-like expression
    //! note, order of select and where is reversed
    var eWhere = calls[1]._0
    var eSelect = calls[0]._0
    var inscope iterType <- clone_type(eWhere.arguments[0]._type.firstType)
    var inscope whereCond : ExpressionPtr
    let srcName = "`source`{argIndex}`{eSelect.at.line}`{eSelect.at.column}"
    let valName = "`value`{argIndex}`{eSelect.at.line}`{eSelect.at.column}"
    let itName = "`it`{argIndex}`{eSelect.at.line}`{eSelect.at.column}"
    let arrName = "`arr`{argIndex}`{eSelect.at.line}`{eSelect.at.column}"
    whereCond |> move_new <| fold_linq_cond(eWhere.arguments[1], valName)
    var inscope selectExpr : ExpressionPtr
    if (eSelect != null) {
        selectExpr |> move_new <| fold_linq_cond(eSelect.arguments[1], itName)
    } else {
        selectExpr |> move_new <| new ExprVar(name := itName, at = topValue.at, _type := iterType)
    }
    var inscope comprehension <- qmacro(invoke($($i(srcName) : typedecl($e(topValue)) - const) {
            var $i(arrName) : array<$t(iterType)>
            for ($i(itName) in $i(srcName)) {
                static_if (typeinfo is_workhorse($e(selectExpr))) {
                    var $i(valName) = $e(selectExpr)
                    if ($e(whereCond)) {
                        $i(arrName).emplace($i(valName))
                    }
                } else {
                    var $i(valName) <- $e(selectExpr)
                    if ($e(whereCond)) {
                        $i(arrName).emplace($i(valName))
                    }
                }
            }
            return <- $i(arrName)
        }, $e(topValue)))
    return <- append_comprehension(argIndex, topValue, comprehension, blk, calls[0]._0.at)
}

[macro_function]
def private fold_linq_default(var expr : ExpressionPtr) : ExpressionPtr {
    //! fold sequence into
    //!   invoke ( top, $ ( it ) : auto {
    //!      var pass_0 = call0(it,...) // or call0_to_array(it,...)
    //!      var pass_1 = call1(pass_0,...)
    //!      ...
    //!      return <- pass_N // or pass_N.to_sequence() if expr is iterator
    //!   } )
    //!   skip var if call is inplace
    //!   skip delete if call is not first
    //!   if call is first and source is iterator, then call is call_to_array
    //!   if call is last and expr is iterator, then return is pass_N.to_sequence()
    //!   if expr is not linq call, return null
    var inscope blk <- new ExprBlock(at = expr.at)
    var (top, calls) = flatten_linq(expr)
    if (calls |> length == 0) {
        return <- default<ExpressionPtr>
    }
    var inscope topExpr <- clone_expression(unsafe(reinterpret<ExpressionPtr>(top)))
    var inscope topExprType <- clone_type(top._type)
    var inscope topValue = qmacro(source)
    let lastCallIndex = calls |> length - 1
    var argIndex = 0
    let argMax = length(calls)
    while (argIndex < argMax) {
        {
            var cll & = unsafe(calls[argIndex])
            let newArgName = "pass_{argIndex}"
            var callName = cll._1.name
            var inplace = false
            // lets find folding sequences
            var found : FoldSequence?
            for (fs in g_foldSeq) {
                if (fs.calls |> length + argIndex - 1 < argMax) {
                    var match = true
                    for (i in 0 .. fs.calls |> length) {
                        if (fs.calls[i] != unsafe(calls[argIndex + i]._1.name)) {
                            match = false
                            break
                        }
                    }
                    if (match) {
                        found = unsafe(addr(fs))
                        break
                    }
                }
            }
            if (found != null) {
                var sub = subarray(calls, argIndex .. argIndex + found.calls |> length)
                topValue |> move_new <| found.folder(argIndex, topValue, blk.get_ptr(), sub)
                argIndex += found.calls |> length
            } else {
                if (cll._0._type.isIterator || cll._0._type.isGoodArrayType) {
                    if (argIndex == 0 && top._type.isIterator) {
                        callName = "{callName}_to_array"
                    } elif (argIndex != 0 && cll._1.inplace) {
                        callName = "{callName}_inplace"
                        inplace = true
                    }
                }
                for (i in cll._1.recursive) {   // this is where we make recursion
                    if (i >= 1 && i < cll._0.arguments |> length && is_linq_call(cll._0.arguments[i].get_ptr())) {
                        var inscope argExpr <- make_call(cll._0.at, "_fold")
                        (argExpr as ExprCallMacro).arguments |> emplace <| cll._0.arguments[i]
                        cll._0.arguments[i] |> move <| argExpr
                    }
                }
                var inscope newCall = qmacro($c(callName)($e(topValue)))
                let numArgs = cll._0.arguments |> length
                for (i in 1..numArgs) {
                    (newCall as ExprCall).arguments |> emplace_new <| clone_expression(cll._0.arguments[i])
                }
                if (inplace) {
                    blk.list |> emplace(newCall)
                } else {
                    blk.list |> emplace_new <| qmacro_expr() {
                        var $i(newArgName) = $e(newCall)
                    }
                    (blk.list.back() as ExprLet).variables[0].flags |= VariableFlags.can_shadow
                    if (argIndex != 0) {
                        blk.list |> emplace_new <| qmacro_expr() {
                            delete $e(topValue)
                        }
                    }
                    topValue |> move_new <| qmacro($i(newArgName))
                }
                argIndex ++
            }
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
    topExpr.genFlags |= ExprGenFlags.alwaysSafe
    var inscope res <- qmacro(invoke($(source : $t(topExprType)) { $e(blk); }, $e(topExpr)))
    (((res as ExprInvoke).arguments[0] as ExprMakeBlock)._block as ExprBlock).arguments[0].flags |= VariableFlags.can_shadow
    res.force_generated(true)
    return <- res
}

[call_macro(name="_fold")]
class private LinqFold : AstCallMacro {
    //! implements _fold(expression) that folds LINQ expressions into optimized sequnences
    //! for example::
    //!
    //!     _fold(each(foo)._where(_ > 5)._select(_ * 2))
    //!
    //! expands into a single comprehension that does all operations in one pass
    def override visit(prog : ProgramPtr; mod : Module?; var call : smart_ptr<ExprCallMacro>) : ExpressionPtr {
        //! Visits the _fold macro call and folds LINQ expressions into optimized sequences.
        macro_verify(call.arguments |> length == 1, prog, call.at, "expecting _fold(expression)")
        macro_verify(call.arguments[0]._type != null, prog, call.at, "expecting linq expression")
        var inscope res : ExpressionPtr <- fold_linq_default(call.arguments[0])
        if (res == null) {
            prog |> macro_error(call.at, "cannot fold LINQ expression\n{describe(call.arguments[0])}")
            return <- res
        }
        return <- res
    }
}

