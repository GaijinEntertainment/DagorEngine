options gen2
options indenting = 4
options _comment_hygiene = true
options _cyclomatic_complexity = 58
options _function_length = 558

module linq_fold_common shared public

//! linq_fold source-independent kernel: pattern-table types, walker, predicate library,
//! chain transforms, generic emit lanes, and the abstract ``SourceAdapter`` base. Carries
//! the shared ``splice_patterns`` registry. See daslib/ARCHITECTURE_LINQ.md sec. 37.

require daslib/linq public
require daslib/ast_boost
require daslib/ast_match
require daslib/match
require daslib/templates_boost
require daslib/macro_boost
require strings

// LinqCall — registry record describing an operator name (where_/select/distinct/...) and its fold disposition.
struct LinqCall {
    name : string
    moduleName : string = "linq"
    skip : bool = false
    inplace : bool = false
    noToArrayVariant : bool = false
    recursive : array<int>
}

variant LoopDispatch {
    Each              : void?
    Find              : void?
    FindReturn        : void?
    FindReturnNegated : void?
}

[macro_interface]
class SourceAdapter {
    def abstract bind_name(at : LineInfo) : string
    def abstract element_type() : TypeDeclPtr
    def abstract wrap_source_loop(loopShape : LoopDispatch; var body : Expression?; at : LineInfo) : Expression?
    def abstract wrap_invoke(var stmts : array<Expression?>; retType : TypeDeclPtr; wrapIter : bool; at : LineInfo) : Expression?
    def can_reserve_by_length() : bool {
        return false
    }
    def has_own_loop_or_count_lane() : bool {
        return false
    }
    def const can_group_by() : bool {
        return false
    }
    def const can_join() : bool {
        return false
    }
    def supports_direct_return() : bool {
        return false
    }
    def effective_dispatch(base : LoopDispatch) : LoopDispatch {
        return base
    }
    def name_prefix() : string {
        return ""
    }
    def emit_loop_or_count(var c : Captures; var ctx : EmitCtx; at : LineInfo) : Expression? {
        return null
    }
    def emit_reverse_skip_into_tail(var takeArg : Expression?; isIter : bool; var termLam : Expression?; termElemType : TypeDeclPtr; at : LineInfo) : Expression? {
        return null
    }
    // nolint:STYLE014 — No-predicate `last` / `reverse |> first`: a backward-walking source returns the
    // last element in ONE step. Only for the no-where case — a predicated last scans backward to unknown
    // depth, and reverse DOM traversal is ~2x cache-hostile per node (profiled), so predicated last keeps
    // the forward path. valueExpr = peeled pre-reverse projection (refs bindName) / null = whole element;
    // termSelLam = raw post-reverse _select / null; defaultExpr non-null = *_or_default / null = panic.
    def emit_reverse_last_backward(bindName : string; var valueExpr : Expression?; var termSelLam : Expression?;
                                   var defaultExpr : Expression?; rowType : TypeDeclPtr;
                                   outElemType : TypeDeclPtr; at : LineInfo) : Expression? {
        return null
    }
    def emit_distinct_take_loop(bindName : string; takenName : string; takeLimName : string; var perElement : Expression?; at : LineInfo) : Expression? {
        return null
    }
    def build_group_by_adapter(var c : Captures; var ctx : EmitCtx; at : LineInfo) : SourceAdapter? {
        return null
    }
    def emit_join_hook(var c : Captures; var ctx : EmitCtx; at : LineInfo) : Expression? {
        return null
    }
    def const defers_materialization() : bool {
        return false
    }
    def handle_type() : TypeDeclPtr {
        return null
    }
    def current_handle_expr(at : LineInfo) : Expression? {
        return null
    }
    def materialize_handle(var handleExpr : Expression?; outBind : string; at : LineInfo) : Expression? {
        return null
    }
    def count_shortcut(opName : string; at : LineInfo) : Expression? {
        return null
    }
    def loop_source_expr() : Expression? {
        return null
    }
    def loop_source_name() : string {
        return ""
    }
    def invoke_param_type() : TypeDeclPtr {
        var top = loop_source_expr()
        return top != null ? invoke_src_param_type(top) : null
    }
}

class ProjectedSourceAdapter : SourceAdapter {
    inner     : SourceAdapter?
    selectLam : Expression?
    projType  : TypeDeclPtr
    projName  : string
    def override name_prefix() : string {
        return inner->name_prefix()
    }
    def override bind_name(at : LineInfo) : string {
        return projName
    }
    def override supports_direct_return() : bool {
        return inner->supports_direct_return()
    }
    def override element_type() : TypeDeclPtr {
        return clone_type(projType)
    }
    def override wrap_source_loop(loopShape : LoopDispatch; var body : Expression?; at : LineInfo) : Expression? {
        let innerBind = inner->bind_name(at)
        var projExpr = peel_lambda_rename_var(selectLam, innerBind)
        let pn = projName
        var newBody = qmacro_block() {
            let $i(pn) = $e(projExpr)
            $e(body)
        }
        return inner->wrap_source_loop(loopShape, newBody, at)
    }
    def override wrap_invoke(var stmts : array<Expression?>; retType : TypeDeclPtr; wrapIter : bool; at : LineInfo) : Expression? {
        return inner->wrap_invoke(stmts, retType, wrapIter, at)
    }
}

class RowUsageScanner : AstVisitor {
    bindName      : string
    usedFields    : table<string>
    allFieldsUsed : bool = false
    inTargetField : bool = false
    def RowUsageScanner(n : string) {
        bindName = n
    }
    def override preVisitExprField(expr : ExprField?) : void {
        var v = expr.value
        if (v is ExprRef2Value) {
            v = (v as ExprRef2Value).subexpr
        }
        if (v is ExprVar && (v as ExprVar).name == bindName) {
            usedFields |> insert(string(expr.name))
            inTargetField = true
        }
    }
    def override visitExprField(var expr : ExprField?) : ExpressionPtr {
        inTargetField = false
        return expr
    }
    def override preVisitExprVar(expr : ExprVar?) : void {
        if (expr.name == bindName && !inTargetField) {
            allFieldsUsed = true
        }
    }
}

[macro_function]
def collect_row_usage(var body : Expression?; bindName : string) : tuple<bool; array<string>> {
    var allUsed = false
    var used : array<string>
    if (body == null) return (allUsed, used)
    var sc = new RowUsageScanner(bindName)
    make_visitor(*sc) $(astVisitorAdapter) {
        visit_expression(body, astVisitorAdapter)
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

[macro_function]
def needs_full_row_bind(usedFields : array<string>; allUsed : bool) : bool {
    //! true when the walk must bind the whole row rather than key/value locals
    if (allUsed) return true
    for (f in usedFields) {
        if (f != "key" && f != "value") return true
    }
    return false
}

class RowFieldFlattener : AstVisitor {
    bindName     : string
    fieldToLocal : table<string; string>
    def RowFieldFlattener(b : string; var f : table<string; string>) {
        bindName = b
        fieldToLocal <- f
    }
    def override visitExprField(var expr : ExprField?) : ExpressionPtr {
        var v = expr.value
        if (v is ExprRef2Value) {
            v = (v as ExprRef2Value).subexpr
        }
        if (v is ExprVar && (v as ExprVar).name == bindName) {
            let fname = string(expr.name)
            if (key_exists(fieldToLocal, fname)) return new ExprVar(at = expr.at, name := fieldToLocal[fname])
        }
        return expr
    }
}

[macro_function]
def flatten_row_to_locals(var body : Expression?; bindName : string; var fieldToLocal : table<string; string>) : Expression? {
    if (body == null) return body
    var fl = new RowFieldFlattener(bindName, fieldToLocal)
    make_visitor(*fl) $(astVisitorAdapter) {
        visit_expression(body, astVisitorAdapter)
    }
    unsafe {
        delete fl
    }
    return body
}

[macro_function]
def hoist_prelude(var prelude : array<Expression?>; var loop : Expression?; at : LineInfo) : Expression? {
    if (empty(prelude)) return loop
    var outBlk = new ExprBlock(at = at)
    for (s in prelude) {
        outBlk.list |> push(s)
    }
    for (s in (loop as ExprBlock).list) {
        outBlk.list |> push(s)
    }
    return outBlk
}

variant SlotMatcher {
    literal : string
    one_of  : array<string>
    alias   : string
}

variant SlotCardinality {
    one      : void?
    optional : void?
    chain    : void?
}

struct Slot {
    matcher      : SlotMatcher
    cardinality  : SlotCardinality
    capture_name : string = ""
    arity        : int = -1
}

struct Captures {
    single      : table<string; ExprCall?>
    single_name : table<string; string>
    many        : table<string; array<ExprCall?>>
}

variant MatchResult {
    no_match : void?
    matched  : Captures
}

typedef RequiresPredicate = function<(var c : Captures; var top : Expression?; src : SourceAdapter?) : bool>

struct EmitCtx {
    top              : Expression?
    src              : SourceAdapter?
    expr_is_iterator : bool
}

typedef EmitFn = function<(var c : Captures; var ctx : EmitCtx; at : LineInfo) : Expression?>

struct LinqFoldSourceMatch {
    //! A source recognizer's answer: the adapter and the chain tops it runs under.
    //! A null `adapter` is "not this source".
    matchTop : Expression?
    ctxTop   : Expression?
    adapter  : SourceAdapter?
}

typedef LinqFoldSourceRecognizer = function<(var top : Expression?; at : LineInfo) : LinqFoldSourceMatch>

var linq_fold_sources : array<LinqFoldSourceRecognizer>

def add_linq_fold_source(recognizer : LinqFoldSourceRecognizer) {
    //! Registers a source recognizer in the current macro context; `linq_fold` tries them
    //! after its sql and decs sources and before json, table and array, in registration order.
    //! A recognizer already registered is not added twice.
    for (r in linq_fold_sources) {
        if (r == recognizer) {
            return
        }
    }
    linq_fold_sources |> push(recognizer)
}

struct SplicePattern {
    name     : string
    chain    : array<Slot>
    requires : array<RequiresPredicate>
    emit     : EmitFn
}

def m_literal(s : string) : SlotMatcher {
    return SlotMatcher(literal = s)
}
def m_alias(s : string) : SlotMatcher {
    return SlotMatcher(alias = s)
}
def c_one() : SlotCardinality {
    return SlotCardinality(one = null)
}
def c_opt() : SlotCardinality {
    return SlotCardinality(optional = null)
}
def c_chain() : SlotCardinality {
    return SlotCardinality(chain = null)
}

def slot_chain_of(var names : array<string>; cap : string) : Slot {
    return Slot(matcher = SlotMatcher(one_of <- names), cardinality = c_chain(), capture_name = cap)
}

def slot_matchers_equal(a : SlotMatcher; b : SlotMatcher) : bool {
    if (a is literal && b is literal) return (a as literal) == (b as literal)
    if (a is alias && b is alias) return (a as alias) == (b as alias)
    if (a is one_of && b is one_of) {
        let la & = unsafe(a as one_of)
        let lb & = unsafe(b as one_of)
        if (length(la) != length(lb)) return false
        for (name in la) {
            var found = false
            for (other in lb) {
                if (other == name) { found = true; break; }
            }
            if (!found) return false
        }
        return true
    }
    return false
}

def slots_structurally_match(a : Slot; b : Slot) : bool {
    return (slot_matchers_equal(a.matcher, b.matcher)
        && (a.cardinality is one)      == (b.cardinality is one)
        && (a.cardinality is optional) == (b.cardinality is optional)
        && (a.cardinality is chain)    == (b.cardinality is chain)
        && a.arity == b.arity)
}

def chain_prefix_of(a : array<Slot>; b : array<Slot>) : bool {
    if (length(a) >= length(b)) return false
    for (i in 0 .. length(a)) {
        if (!slots_structurally_match(a[i], b[i])) return false
    }
    return true
}

[unused_argument(name)]
def check_pattern_table_reachable(name : string; patterns : array<SplicePattern>) : bool {
    for (i in 0 .. length(patterns)) {
        for (j in i + 1 .. length(patterns)) {
            if (chain_prefix_of(patterns[i].chain, patterns[j].chain)) return false
        }
    }
    return true
}

def call_norm_name(call : ExprCall?) : string {
    var topFunc = call.func
    while (topFunc.fromGeneric != null) {
        topFunc = topFunc.fromGeneric
    }
    let raw = string(topFunc.name)
    if (linqCalls |> key_exists(raw)) return linqCalls[raw].name
    return raw
}

var alias_table : table<string; array<string>> <- {
    "distinct_family"            => ["distinct", "distinct_by"],
    "first_family"               => ["first", "first_or_default"],
    "count_family"               => ["count", "long_count"],
    "distinct_terminator_family" => ["count", "long_count", "sum"],
    "order_family"               => ["order", "order_descending", "order_by", "order_by_descending", "order_by_keys"],
    "range_op_family"            => ["skip", "skip_while", "take_while", "take"],
    "accum_family"               => ["sum", "min", "max", "average", "aggregate",
                                     "min_by", "max_by", "min_max", "min_max_by",
                                     "min_max_average", "min_max_average_by", "long_count"],
    "early_exit_family"          => ["any", "all", "contains", "first", "first_or_default"],
    "loop_terminator_family"     => ["count", "long_count", "sum", "min", "max", "average", "aggregate",
                                     "min_by", "max_by", "min_max", "min_max_by",
                                     "min_max_average", "min_max_average_by",
                                     "any", "all", "contains", "first", "first_or_default",
                                     "last", "last_or_default", "single", "single_or_default",
                                     "element_at", "element_at_or_default", "to_table"],
    "order_by_family"            => ["order_by", "order_by_descending"]
}

def slot_matches_call(slot : Slot; call : ExprCall?; name : string; pattern_name : string) : bool {
    var in_set = false
    if (slot.matcher is literal) {
        in_set = (slot.matcher as literal) == name
    } elif (slot.matcher is one_of) {
        for (n in slot.matcher as one_of) {
            if (n == name) { in_set = true; break; }
        }
    } elif (slot.matcher is alias) {
        let key = slot.matcher as alias
        if (alias_table |> key_exists(key)) {
            for (n in alias_table[key]) {
                if (n == name) { in_set = true; break; }
            }
        } else {
            panic("match_pattern: unknown alias '{key}' in pattern '{pattern_name}'")
        }
    }
    return in_set && (slot.arity == -1 || length(call.arguments) == slot.arity)
}

def match_pattern(p : SplicePattern;
                  var calls : array<tuple<ExprCall?; LinqCall?>>;
                  var top : Expression?;
                  src : SourceAdapter?) : MatchResult {
    var captures : Captures
    var slot_i = 0
    var call_i = 0
    while (slot_i < length(p.chain)) {
        let slot & = unsafe(p.chain[slot_i])
        if (slot.cardinality is chain) {
            var captured : array<ExprCall?>
            while (call_i < length(calls) && slot_matches_call(slot, calls[call_i]._0, calls[call_i]._1.name, p.name)) {
                captured |> push(calls[call_i]._0)
                call_i ++
            }
            if (slot.capture_name != "") {
                captures.many[slot.capture_name] <- captured
            }
            slot_i ++
            continue
        }
        var matched_here = false
        if (call_i < length(calls)) {
            let cur & = unsafe(calls[call_i])
            matched_here = slot_matches_call(slot, cur._0, cur._1.name, p.name)
        }
        if (matched_here) {
            if (slot.capture_name != "") {
                captures.single |> insert(slot.capture_name, calls[call_i]._0)
                captures.single_name |> insert(slot.capture_name, calls[call_i]._1.name)
            }
            slot_i ++
            call_i ++
        } elif (slot.cardinality is one) {
            return MatchResult(no_match = null)
        } else {
            slot_i ++
        }
    }
    if (call_i < length(calls)) return MatchResult(no_match = null)
    for (pred in p.requires) {
        if (!invoke(pred, captures, top, src)) return MatchResult(no_match = null)
    }
    return MatchResult(matched <- captures)
}

[unused_argument(c, src)]
def array_source(var c : Captures; var top : Expression?; src : SourceAdapter?) : bool {
    return top != null && top._type != null && (top._type.isGoodArrayType || top._type.isArray)
}

def non_array_source(var c : Captures; var top : Expression?; src : SourceAdapter?) : bool {
    return !array_source(c, top, src)
}

[unused_argument(c, top)]
def can_join_source(var c : Captures; var top : Expression?; src : SourceAdapter?) : bool {
    return src != null && src->can_join()
}

[unused_argument(c, top)]
def can_group_by_source(var c : Captures; var top : Expression?; src : SourceAdapter?) : bool {
    return src != null && src->can_group_by()
}

[unused_argument(top, src)]
def take_arg_is_int(var c : Captures; var top : Expression?; src : SourceAdapter?) : bool {
    if (!(c.single |> key_exists("take"))) return true
    let take = c.single["take"]
    return (take != null && length(take.arguments) >= 2 && take.arguments[1] != null
            && take.arguments[1]._type != null && take.arguments[1]._type.baseType == Type.tInt)
}

[unused_argument(top, src)]
def no_terminator(var c : Captures; var top : Expression?; src : SourceAdapter?) : bool {
    return !(c.single |> key_exists("term"))
}

[unused_argument(top, src)]
def inline_cmp_available(var c : Captures; var top : Expression?; src : SourceAdapter?) : bool {
    if (!(c.single |> key_exists("order"))) return false
    let orderCall = c.single["order"]
    if (orderCall == null || orderCall._type == null || orderCall._type.firstType == null
            || (orderCall.arguments |> length) < 2) return false
    let orderName = c.single_name["order"]
    if (orderName != "order_by" && orderName != "order_by_descending") return false
    return try_make_inline_cmp(orderCall.arguments[1], orderName, orderCall._type.firstType, orderCall.at) != null
}

[unused_argument(top, src)]
def has_where_or_distinct(var c : Captures; var top : Expression?; src : SourceAdapter?) : bool {
    return (c.single |> key_exists("where")) || (c.single |> key_exists("distinct"))
}

[unused_argument(top, src)]
def decs_join_invariants(var c : Captures; var top : Expression?; src : SourceAdapter?) : bool {
    if (!(c.single |> key_exists("upstream_join"))) return true
    return ((c.many["head"] |> empty)
            && !(c.single |> key_exists("having"))
            && !(c.single |> key_exists("trailing_where")))
}

def srcb_is_array_shaped(var c : Captures; captureName : string) : bool {
    if (!(c.single |> key_exists(captureName))) return true
    let jc = c.single[captureName]
    if (jc == null || (jc.arguments |> length) < 2) return false
    let srcb = jc.arguments[1]
    if (srcb == null || srcb._type == null) return false
    return srcb._type.isGoodArrayType || srcb._type.isArray || srcb._type.isIterator
}

[unused_argument(top, src)]
def no_order_with_count(var c : Captures; var top : Expression?; src : SourceAdapter?) : bool {
    return !((c.single |> key_exists("trailing_order")) && (c.single |> key_exists("term")))
}

[unused_argument(top, src)]
def join_no_select_with_count(var c : Captures; var top : Expression?; src : SourceAdapter?) : bool {
    return !((c.single |> key_exists("trailing_select")) && (c.single |> key_exists("term")))
}

[unused_argument(top, src)]
def zip_arity_2_or_3(var c : Captures; var top : Expression?; src : SourceAdapter?) : bool {
    if (!(c.single |> key_exists("zip"))) return true
    let n = c.single["zip"].arguments |> length
    return n == 2 || n == 3
}

[unused_argument(top, src)]
def zip_no_while_after_select(var c : Captures; var top : Expression?; src : SourceAdapter?) : bool {
    if (!(c.single |> key_exists("skip_while")) && !(c.single |> key_exists("take_while"))) return true
    for (call in c.many["head"]) {
        if (call_norm_name(call) == "select") return false
    }
    return true
}

[unused_argument(top, src)]
def zip_reverse_compatible_with_term(var c : Captures; var top : Expression?; src : SourceAdapter?) : bool {
    if (!(c.single |> key_exists("trailing_reverse")) || !(c.single |> key_exists("term"))) return true
    let name = c.single_name["term"]
    return name != "first" && name != "first_or_default"
}

var splice_patterns : array<SplicePattern>

var linqCalls = {
    "where_" => LinqCall(name = "where_"),
    "where_to_array" => LinqCall(name = "where_"),
    "select" => LinqCall(name = "select"),
    "select_to_array" => LinqCall(name = "select"),
    "select_many" => LinqCall(name = "select_many"),
    "select_many_to_array" => LinqCall(name = "select_many"),
    "zip" => LinqCall(name = "zip", recursive = [1, 2]),
    "zip_to_array" => LinqCall(name = "zip", recursive = [1, 2]),
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
    "order_by_keys" => LinqCall(name = "order_by_keys", inplace = true),
    "order_by_keys_to_array" => LinqCall(name = "order_by_keys", inplace = true),
    "top_n" => LinqCall(name = "top_n", noToArrayVariant = true),
    "top_n_by" => LinqCall(name = "top_n_by", noToArrayVariant = true),
    "top_n_descending" => LinqCall(name = "top_n_descending", noToArrayVariant = true),
    "top_n_by_descending" => LinqCall(name = "top_n_by_descending", noToArrayVariant = true),
    "top_n_by_with_cmp" => LinqCall(name = "top_n_by_with_cmp", noToArrayVariant = true),
    "count" => LinqCall(name = "count"),
    "long_count" => LinqCall(name = "long_count"),
    "average" => LinqCall(name = "average"),
    "min" => LinqCall(name = "min"),
    "min_by" => LinqCall(name = "min_by", noToArrayVariant = true),
    "max" => LinqCall(name = "max"),
    "max_by" => LinqCall(name = "max_by", noToArrayVariant = true),
    "min_max" => LinqCall(name = "min_max", noToArrayVariant = true),
    "min_max_by" => LinqCall(name = "min_max_by", noToArrayVariant = true),
    "min_max_average" => LinqCall(name = "min_max_average", noToArrayVariant = true),
    "min_max_average_by" => LinqCall(name = "min_max_average_by", noToArrayVariant = true),
    "aggregate" => LinqCall(name = "aggregate", noToArrayVariant = true),
    "sum" => LinqCall(name = "sum"),
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
    "concat" => LinqCall(name = "concat", recursive = [1, 2, 3, 4, 5, 6, 7], inplace = true),
    "concat_to_array" => LinqCall(name = "concat", recursive = [1, 2, 3, 4, 5, 6, 7], inplace = true),
    "prepend" => LinqCall(name = "prepend", inplace = true),
    "prepend_to_array" => LinqCall(name = "prepend", inplace = true),
    "append" => LinqCall(name = "append", inplace = true),
    "append_to_array" => LinqCall(name = "append", inplace = true),
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
    "element_at" => LinqCall(name = "element_at", noToArrayVariant = true),
    "element_at_or_default" => LinqCall(name = "element_at_or_default", noToArrayVariant = true),
    "first" => LinqCall(name = "first", noToArrayVariant = true),
    "first_or_default" => LinqCall(name = "first_or_default", noToArrayVariant = true),
    "last" => LinqCall(name = "last", noToArrayVariant = true),
    "last_or_default" => LinqCall(name = "last_or_default", noToArrayVariant = true),
    "single" => LinqCall(name = "single", noToArrayVariant = true),
    "single_or_default" => LinqCall(name = "single_or_default", noToArrayVariant = true),
    "join" => LinqCall(name = "join", recursive = [1]),
    "join_to_array" => LinqCall(name = "join", recursive = [1]),
    "left_join" => LinqCall(name = "left_join", recursive = [1]),
    "left_join_to_array" => LinqCall(name = "left_join", recursive = [1]),
    "group_join" => LinqCall(name = "group_join", recursive = [1]),
    "group_join_to_array" => LinqCall(name = "group_join", recursive = [1]),
    "group_by" => LinqCall(name = "group_by"),
    "group_by_to_array" => LinqCall(name = "group_by"),
    "group_by_lazy" => LinqCall(name = "group_by_lazy"),
    "group_by_lazy_to_array" => LinqCall(name = "group_by_lazy"),
    "having_" => LinqCall(name = "having_"),
    "having_to_array" => LinqCall(name = "having_"),
    "any" => LinqCall(name = "any"),
    "all" => LinqCall(name = "all"),
    "contains" => LinqCall(name = "contains"),
    "to_table" => LinqCall(name = "to_table"),
    "to_array" => LinqCall(name = "to_array", moduleName = "builtin", skip = true),
    "to_sequence" => LinqCall(name = "to_sequence", skip = true),
}

[macro_function]
def is_linq_call(expr : Expression?) : bool {
    if (expr is ExprCall) {
        let call = expr as ExprCall
        var fn = call.func
        while (fn.fromGeneric != null) {
            fn = fn.fromGeneric
        }
        let funcName = string(fn.name)
        if (linqCalls.key_exists(funcName)) {
            let linqCall = unsafe(linqCalls?[funcName])
            if (linqCall.moduleName == fn._module.name) return true
        }
    }
    return false
}

[macro_function]
def normalize_order_reverse(var calls : array<tuple<ExprCall?; LinqCall?>>) {
    var i = 0
    while (i + 1 < length(calls)) {
        let curName = calls[i]._1.name
        let nextName = calls[i + 1]._1.name
        var flipped = ""
        if (nextName == "reverse") {
            if (curName == "order_by") {
                flipped = "order_by_descending"
            } elif (curName == "order_by_descending") {
                flipped = "order_by"
            } elif (curName == "order") {
                flipped = "order_descending"
            } elif (curName == "order_descending") {
                flipped = "order"
            }
        }
        if (flipped != "" && linqCalls.key_exists(flipped)) {
            let linqCall = unsafe(linqCalls?[flipped])
            calls[i] = (calls[i]._0, linqCall)
            calls |> erase(i + 1)
        } else {
            i ++
        }
    }
}

typedef ComposeFn = function<(innerLam, outerLam : Expression?; freshName : string) : Expression?>

[macro_function]
def collapse_adjacent_calls(var calls : array<tuple<ExprCall?; LinqCall?>>;
                            opName, freshNamePrefix : string;
                            composeBodies : ComposeFn) {
    var i = 0
    while (i + 1 < length(calls)) {
        if (calls[i]._1.name != opName || calls[i + 1]._1.name != opName) {
            i ++
            continue
        }
        var innerLam = calls[i]._0.arguments[1]
        var outerLam = calls[i + 1]._0.arguments[1]
        if (innerLam == null || outerLam == null
                || !(innerLam is ExprMakeBlock) || !(outerLam is ExprMakeBlock)) {
            i ++
            continue
        }
        var innerMblk = innerLam as ExprMakeBlock
        var innerBlk = innerMblk._block as ExprBlock
        var outerMblk = outerLam as ExprMakeBlock
        var outerBlk = outerMblk._block as ExprBlock
        if (innerBlk == null || outerBlk == null
                || innerBlk.arguments |> length != 1 || outerBlk.arguments |> length != 1
                || innerBlk.list |> length != 1 || outerBlk.list |> length != 1
                || !(innerBlk.list[0] is ExprReturn) || !(outerBlk.list[0] is ExprReturn)) {
            i ++
            continue
        }
        var innerRet = innerBlk.list[0] as ExprReturn
        var outerRet = outerBlk.list[0] as ExprReturn
        if (innerRet.subexpr == null || outerRet.subexpr == null) {
            i ++
            continue
        }
        let freshName = qn(freshNamePrefix, innerLam.at)
        var composed = invoke(composeBodies, innerLam, outerLam, freshName)
        if (composed == null) {
            i ++
            continue
        }
        var newLam = clone_expression(innerLam)
        var newMblk = newLam as ExprMakeBlock
        var newBlk = newMblk._block as ExprBlock
        if (newBlk == null || newBlk.list |> length != 1 || !(newBlk.list[0] is ExprReturn)) {
            i ++
            continue
        }
        var newRet = newBlk.list[0] as ExprReturn
        newBlk.arguments[0].name := freshName
        newRet.subexpr = composed
        calls[i + 1]._0.arguments[1] = newLam
        calls[i + 1]._0.arguments[0] = calls[i]._0.arguments[0]
        calls |> erase(i)
    }
}

[macro_function]
def compose_select_bodies(innerLam, outerLam : Expression?; freshName : string) : Expression? {
    var outerMblk = outerLam as ExprMakeBlock
    var outerBlk = outerMblk._block as ExprBlock
    var outerRet = outerBlk.list[0] as ExprReturn
    var renamedInnerBody = peel_lambda_rename_var(innerLam, freshName)
    if (renamedInnerBody == null || has_sideeffects(renamedInnerBody)) return null
    var rules : Template
    rules |> replaceVariable(string(outerBlk.arguments[0].name), renamedInnerBody)
    return apply_template(rules, outerRet.subexpr.at, outerRet.subexpr)
}

[macro_function]
def collapse_chained_selects(var calls : array<tuple<ExprCall?; LinqCall?>>) {
    collapse_adjacent_calls(calls, "select", "cs", @@ < ComposeFn > compose_select_bodies)
}

[macro_function]
def compose_where_bodies(innerLam, outerLam : Expression?; freshName : string) : Expression? {
    var innerBodyFresh = peel_lambda_rename_var(innerLam, freshName)
    var outerBodyFresh = peel_lambda_rename_var(outerLam, freshName)
    if (innerBodyFresh == null || outerBodyFresh == null) return null
    return merge_where_cond(innerBodyFresh, outerBodyFresh)
}

[macro_function]
def collapse_chained_wheres(var calls : array<tuple<ExprCall?; LinqCall?>>) {
    collapse_adjacent_calls(calls, "where_", "cw", @@ < ComposeFn > compose_where_bodies)
}

[macro_function]
def flatten_linq(var expr : Expression?)  {
    var top = expr
    var calls : array<tuple<ExprCall?; LinqCall?>>
    while (top != null) {
        if (top is ExprCall) {
            var topCall = top as ExprCall
            var topFunc = topCall.func
            while (topFunc.fromGeneric != null) {
                topFunc = topFunc.fromGeneric
            }
            let funcName = string(topFunc.name)
            if (!linqCalls.key_exists(funcName)) break
            let linqCall = unsafe(linqCalls?[funcName])
            if (linqCall.moduleName != topFunc._module.name) break
            if (!linqCall.skip) {
                calls.push((topCall, linqCall))
            }
            top = topCall.arguments[0]
        } else {
            break
        }
    }
    calls.reverse_inplace()
    return <- (top, calls)
}

[macro_function]
def type_has_length(t : TypeDecl?) : bool {
    if (t == null) return false
    return (t.isGoodArrayType || t.isGoodTableType || t.isString
        || t.isArray || t.isRange)
}

def type_has_long_length(t : TypeDecl?) : bool {
    if (t == null) return false
    return t.isGoodArrayType || t.isGoodTableType
}

enum LinqLane {
    UNKNOWN
    ARRAY
    COUNTER
    ACCUMULATOR
    EARLY_EXIT
}

[macro_function]
def classify_terminator(name : string) : LinqLane {
    if (name == "count") return LinqLane.COUNTER
    if (name == "where_" || name == "select" || name == "take" || name == "skip"
            || name == "take_while" || name == "skip_while" || name == "to_table") return LinqLane.ARRAY
    if (name == "sum" || name == "min" || name == "max" || name == "average" || name == "long_count"
            || is_min_max_family(name)) return LinqLane.ACCUMULATOR
    if (name == "first" || name == "first_or_default" || name == "any" || name == "all" || name == "contains"
            || name == "last" || name == "last_or_default"
            || name == "single" || name == "single_or_default"
            || name == "element_at" || name == "element_at_or_default"
            || name == "aggregate") return LinqLane.EARLY_EXIT
    return LinqLane.UNKNOWN
}

[macro_function]
def peel_each(var top : Expression?) : Expression? {
    if (!(top is ExprCall)) return top
    var topCall = top as ExprCall
    if (match_call_in_module(top, "each", "builtin") == null || topCall.arguments |> length != 1) return top
    let argExpr = topCall.arguments[0]
    if ((argExpr == null || argExpr._type == null)
            || (!argExpr._type.isGoodArrayType && !argExpr._type.isArray)) return top
    return clone_expression(argExpr)
}

[macro_function]
def finalize_invoke(var res : Expression?; at : LineInfo) : Expression? {
    res.force_at(at)
    res.force_generated(true)
    let blk = (res as ExprInvoke).arguments[0] as ExprMakeBlock
    var blkBlock = blk._block as ExprBlock
    for (i in 0 .. length(blkBlock.arguments)) {
        blkBlock.arguments[i].flags.can_shadow = true
    }
    return res
}

[clone(top), macro_function]
def emit_length_shortcut(opName : string; var top : Expression?; srcName : string; at : LineInfo) : Expression? {
    // nolint:STYLE014 — O(1) count shortcut, skips the walk. Emits `invoke((src) { return <X> }, top)`:
    //   count                          → length(src)            // no int() cast — interp won't fold it and PERF020 fires
    //   long_count (array / table)     → long_length(src)
    //   long_count (string/dim/range)  → int64(length(src))     // length() returns int here; cast widens
    var topExpr = clone_expression(top)
    topExpr.genFlags.alwaysSafe = true
    var res : Expression?
    if (opName == "long_count" && type_has_long_length(top._type)) {
        res = qmacro(invoke($($i(srcName) : typedecl($e(topExpr))) {
            return long_length($i(srcName))
        }, $e(topExpr)))
    } elif (opName == "long_count") {
        res = qmacro(invoke($($i(srcName) : typedecl($e(topExpr))) {
            return int64(length($i(srcName)))
        }, $e(topExpr)))
    } else {
        res = qmacro(invoke($($i(srcName) : typedecl($e(topExpr))) {
            return length($i(srcName))
        }, $e(topExpr)))
    }
    return finalize_invoke(res, at)
}

[macro_function]
def invoke_src_param_type(top : Expression?) : TypeDeclPtr {
    var srcParamType = clone_type(top._type)
    if (top._type.isIterator) {
        srcParamType.flags.constant = false
    }
    return srcParamType
}

[macro_function]
def prepend_binds(var stmts : array<Expression?>; var intermediateBinds : array<Expression?>) {
    if (empty(intermediateBinds)) return
    var prefixed : array<Expression?>
    prefixed |> reserve(long_length(intermediateBinds) + long_length(stmts))
    prefixed |> push_from(intermediateBinds)
    prefixed |> push_from(stmts)
    swap(stmts, prefixed)
}

[macro_function]
def stmts_to_expr(var stmts : array<Expression?>) : Expression? {
    if (length(stmts) == 1) return stmts[0]
    return <- qmacro_block() {
        $b(stmts)
    }
}

[macro_function]
def wrap_with_condition(var body : Expression?; var cond : Expression?) : Expression? {
    if (cond == null) return body
    return <- qmacro_expr() {
        if ($e(cond)) {
            $e(body)
        }
    }
}

[macro_function]
def prepend_precond(var body : Expression?; var preCondStmts : array<Expression?>) : Expression? {
    if (empty(preCondStmts)) return body
    var stmts : array<Expression?>
    stmts |> reserve(long_length(preCondStmts) + 1_l)
    stmts |> push_from(preCondStmts)
    stmts |> push(body)
    return stmts_to_expr(stmts)
}

struct RangeStateNames {
    skipName      : string
    takeCountName : string
    skippingName  : string
}

[macro_function]
def make_range_names(at : LineInfo) : RangeStateNames {
    return <- RangeStateNames(
        skipName      = qn("skip", at),
        takeCountName = qn("tc", at),
        skippingName  = qn("skipping", at))
}

[macro_function]
def append_ranges_prelude(var preludeStmts : array<Expression?>; var skipExpr, takeExpr, skipWhileCond : Expression?;
                          names : RangeStateNames) {
    // nolint:STYLE014 — emits the loop-level range state, hoisted ABOVE the source loop (one var per active op):
    //   var skipN = <skipExpr>      // skip(N) remaining-counter
    //   var tc = 0                  // take(N) taken-counter
    //   var skipping = true         // skip_while flag (one-way; flips false on the first failing predicate)
    let skipName      = names.skipName
    let takeCountName = names.takeCountName
    let skippingName  = names.skippingName
    if (skipExpr != null) {
        preludeStmts |> push <| qmacro_expr() {
            var $i(skipName) = $e(skipExpr)
        }
    }
    if (takeExpr != null) {
        preludeStmts |> push <| qmacro_expr() {
            var $i(takeCountName) = 0
        }
    }
    if (skipWhileCond != null) {
        preludeStmts |> push <| qmacro_expr() {
            var $i(skippingName) = true
        }
    }
}

[macro_function]
def wrap_with_ranges(var stmts : array<Expression?>; var skipExpr, takeExpr, skipWhileCond, takeWhileCond : Expression?;
                     names : RangeStateNames) {
    // nolint:STYLE014 — prepends the per-iteration range guards to `stmts` (takeCount++ sits AFTER the skip/while
    // gates so gated-out elements don't eat the take(N) budget). Emits, in order, then the original `stmts`:
    //   if (tc >= <takeExpr>) { break }                                  // take(N)
    //   if (skipN > 0) { skipN--; continue }                             // skip(N)
    //   if (skipping) { if (<skipWhileCond>) continue; skipping = false } // skip_while
    //   if (!(<takeWhileCond>)) { break }                                // take_while
    //   tc ++                                                            // take(N) counter bump
    //   <stmts…>
    if (skipExpr == null && takeExpr == null && skipWhileCond == null && takeWhileCond == null) return
    let skipName      = names.skipName
    let takeCountName = names.takeCountName
    let skippingName  = names.skippingName
    var prefixed : array<Expression?>
    prefixed |> reserve(length(stmts) + 5)
    if (takeExpr != null) {
        prefixed |> push <| qmacro_expr() {
            if ($i(takeCountName) >= $e(takeExpr)) {
                break
            }
        }
    }
    if (skipExpr != null) {
        prefixed |> push <| qmacro_expr() {
            if ($i(skipName) > 0) {
                $i(skipName) --
                continue
            }
        }
    }
    if (skipWhileCond != null) {
        prefixed |> push <| qmacro_expr() {
            if ($i(skippingName)) {
                if ($e(skipWhileCond)) {
                    continue
                }
                $i(skippingName) = false
            }
        }
    }
    if (takeWhileCond != null) {
        prefixed |> push <| qmacro_expr() {
            if (!($e(takeWhileCond))) {
                break
            }
        }
    }
    if (takeExpr != null) {
        prefixed |> push <| qmacro_expr() {
            $i(takeCountName) ++
        }
    }
    prefixed |> push_from(stmts)
    swap(stmts, prefixed)
}

[macro_function]
def min_max_compare(workhorse : bool; opName : string; valName, accName : string) : Expression? {
    if (workhorse && opName == "min") return <- qmacro($i(valName) < $i(accName))
    if (workhorse) return <- qmacro($i(valName) > $i(accName))
    if (opName == "min") return <- qmacro(_::less($i(valName), $i(accName)))
    return <- qmacro(_::less($i(accName), $i(valName)))
}

[macro_function]
def build_accumulator_perelement_stmts(opName : string;
                                       accName, valBindName, firstName, cntName : string;
                                       var valueExpr : Expression?;
                                       workhorse : bool;
                                       isDoubleAccType : bool) : array<Expression?> {
    // nolint:STYLE014 — per-element accumulator update for sum / min / max / average. valueExpr = cloned
    // projection or element bind. Emits:
    //   sum     → acc += valueExpr
    //   average → acc += double(valueExpr); cnt ++          // double() skipped when acc is already double
    //   min/max → let val = valueExpr
    //             if (first) { acc := val; first = false } elif (<val beats acc>) acc := val
    // Preludes + return-expr stay caller-side (build_accumulator_spec) — acc types / empty-source guards differ.
    var stmts : array<Expression?>
    if (opName == "sum") {
        stmts |> push <| qmacro_expr() {
            $i(accName) += $e(valueExpr)
        }
    } elif (opName == "average") {
        if (isDoubleAccType) {
            stmts |> push_from <| qmacro_block_to_array() {
                $i(accName) += $e(valueExpr)
                $i(cntName) ++
            }
        } else {
            stmts |> push_from <| qmacro_block_to_array() {
                $i(accName) += double($e(valueExpr))
                $i(cntName) ++
            }
        }
    } elif (opName == "min" || opName == "max") {
        var compareExpr = min_max_compare(workhorse, opName, valBindName, accName)
        stmts |> push_from <| qmacro_block_to_array() {
            let $i(valBindName) = $e(valueExpr)
            if ($i(firstName)) {
                $i(accName) := $i(valBindName)
                $i(firstName) = false
            } elif ($e(compareExpr)) {
                $i(accName) := $i(valBindName)
            }
        }
    }
    return <- stmts
}

struct AccumulatorSpec {
    prologue   : array<Expression?>
    returnExpr : Expression?
    retType    : TypeDeclPtr
}

[macro_function, unused_argument(at)]
def build_accumulator_spec(opName : string; accName, cntName, firstName : string;
                           accType : TypeDeclPtr; at : LineInfo) : AccumulatorSpec {
    // nolint:STYLE014 — per-op accumulator prologue (var decls) + return expr (paired with the per-element
    // update from build_accumulator_perelement_stmts). Emits:
    //   sum     → prologue `var acc : T = default<T>`;                          return `acc`
    //   average → prologue `var acc : double = 0lf; var cnt : uint64 = 0ul`;    return `cnt != 0 ? acc / double(cnt) : 0lf`
    //   min/max → prologue `var first = true; var acc : T`;                     return `acc`   (first-element latch)
    var sp : AccumulatorSpec
    if (opName == "sum") {
        sp.prologue <- qmacro_block_to_array() {
            var $i(accName) : $t(accType) = default<$t(accType)>
        }
        sp.returnExpr = qmacro_expr() { $i(accName); }
        sp.retType = clone_type(accType)
    } elif (opName == "average") {
        sp.prologue <- qmacro_block_to_array() {
            var $i(accName) : double = 0lf
            var $i(cntName) : uint64 = 0ul
        }
        sp.returnExpr = qmacro_expr() { $i(cntName) != 0ul ? $i(accName) / double($i(cntName)) : 0lf; }
        sp.retType = new TypeDecl(baseType = Type.tDouble)
    } elif (opName == "min" || opName == "max") {
        sp.prologue <- qmacro_block_to_array() {
            var $i(firstName) = true
            var $i(accName) : $t(accType)
        }
        sp.returnExpr = qmacro_expr() { $i(accName); }
        sp.retType = clone_type(accType)
    }
    return <- sp
}

struct TerminatorSpec {
    prologue          : array<Expression?>
    preCondStmts      : array<Expression?>
    intermediateBinds : array<Expression?>
    whereCond         : Expression?
    postTakeWhereCond : Expression?
    perElementStmts   : array<Expression?>
    skipExpr          : Expression?
    takeExpr          : Expression?
    skipWhileCond     : Expression?
    takeWhileCond     : Expression?
    rangeNames        : RangeStateNames
    tailStmts         : array<Expression?>
    outElemType       : TypeDeclPtr
    wrapIter          : bool
    dispatch          : LoopDispatch = LoopDispatch(Each = null)
}

[macro_function]
def emit_terminator_lane(var spec : TerminatorSpec; var adapter : SourceAdapter?; at : LineInfo) : Expression? {
    // nolint:STYLE014 — central single-pass assembler: stitches a TerminatorSpec into one source loop inside an
    // invoke. The counter / accumulator / early-exit lanes all build a spec and hand it here. Generated skeleton
    // (each slot drops out when its spec field is null/empty):
    //   invoke((src…) {
    //       <range prelude>                        // append_ranges_prelude: `var skipN = M` / `var tc = 0` / `var skipping = true`
    //       <spec.prologue>                        // e.g. `var acc = 0` (counter / accumulator seed)
    //       wrap_source_loop(spec.dispatch) {      // adapter: array → `for`, decs → for_each_archetype, xml → DOM walk
    //           <spec.preCondStmts>                // per-element binds, inside the loop but outside the where-gate
    //           if (spec.whereCond) {
    //               <range guards>                 // wrap_with_ranges: skip/take/skip_while/take_while → continue / break
    //               if (spec.postTakeWhereCond) {  // trailing where, applied after take
    //                   <spec.intermediateBinds>   // chained-select running-projection binds
    //                   <spec.perElementStmts>     // the op's per-element work
    //               }
    //           }
    //       }
    //       <spec.tailStmts>                       // e.g. `return acc`
    //   }, src…)
    var perStmts <- spec.perElementStmts
    prepend_binds(perStmts, spec.intermediateBinds)
    if (spec.postTakeWhereCond != null) {
        var gated = wrap_with_condition(stmts_to_expr(perStmts), spec.postTakeWhereCond)
        perStmts |> clear
        perStmts |> push(gated)
    }
    wrap_with_ranges(perStmts, spec.skipExpr, spec.takeExpr, spec.skipWhileCond, spec.takeWhileCond, spec.rangeNames)
    var loopBody = prepend_precond(wrap_with_condition(stmts_to_expr(perStmts), spec.whereCond), spec.preCondStmts)
    var bodyStmts : array<Expression?>
    bodyStmts |> reserve(length(spec.prologue) + length(spec.tailStmts) + 5)
    append_ranges_prelude(bodyStmts, spec.skipExpr, spec.takeExpr, spec.skipWhileCond, spec.rangeNames)
    bodyStmts |> push_from(spec.prologue)
    var srcLoop = adapter->wrap_source_loop(spec.dispatch, loopBody, at)
    if (srcLoop == null) return null
    bodyStmts |> push(srcLoop)
    bodyStmts |> push_from(spec.tailStmts)
    return adapter->wrap_invoke(bodyStmts, spec.outElemType, spec.wrapIter, at)
}

variant DistinctGateKind {
    NonKey : void?
    KeyLam : Expression?
}

struct DistinctGateSpec {
    kind        : DistinctGateKind
    bindName    : string
    bufElemType : TypeDeclPtr
    dsetName    : string
    dkeyName    : string
}

struct FoldArraySpec {
    prologueStmts    : array<Expression?>
    bufElemType      : TypeDeclPtr
    bufName          : string
    bufDeclStmt      : Expression?
    postBufDeclStmts : array<Expression?>
    reserveStmts     : array<Expression?>
    preCondStmts    : array<Expression?>
    whereCond       : Expression?
    distinctGate    : DistinctGateSpec?
    perElementPush  : Expression?
    tailStmts       : array<Expression?>
    outElemType     : TypeDeclPtr
    wrapIter        : bool
    customSourceLoop : Expression?
    dispatch        : LoopDispatch = LoopDispatch(Each = null)
}

// nolint:STYLE014 — composes the per-element body the fold-array lane wraps (distinct gate innermost →
// where-cond → preCondStmts). Exposed so customSourceLoop callers wrap the exact same body in their own
// outer-loop shape. Emits:
//   <spec.preCondStmts>
//   if (spec.whereCond) {
//       let dkey = unique_key(<distinct key>)                              // distinctGate only
//       if (!dset |> key_exists(dkey)) { dset |> insert(dkey); <spec.perElementPush> }
//   }
[macro_function]
def compose_fold_per_element(var spec : FoldArraySpec) : Expression? {
    var perElement = spec.perElementPush
    if (spec.distinctGate != null) {
        let dg = spec.distinctGate
        var dkeyExpr : Expression?
        if (dg.kind is KeyLam) {
            dkeyExpr = peel_lambda_rename_var(dg.kind as KeyLam, dg.bindName)
            if (dkeyExpr == null) return null
        } else {
            dkeyExpr = qmacro($i(dg.bindName))
        }
        let dkeyName = dg.dkeyName
        let dsetName = dg.dsetName
        perElement = qmacro_block() {
            let $i(dkeyName) = _::unique_key($e(dkeyExpr))
            if (!$i(dsetName) |> key_exists($i(dkeyName))) {
                $i(dsetName) |> insert($i(dkeyName))
                $e(perElement)
            }
        }
    }
    return prepend_precond(wrap_with_condition(perElement, spec.whereCond), spec.preCondStmts)
}

[macro_function]
def emit_fold_array_lane(var spec : FoldArraySpec; var adapter : SourceAdapter?; at : LineInfo) : Expression? {
    // nolint:STYLE014 — fold-into-array assembler (sibling of emit_terminator_lane, for ops that materialize a
    // buffer): declare buf, run the source loop pushing survivors, return buf. Generated skeleton (slots =
    // FoldArraySpec fields; each drops out when null/empty):
    //   invoke((src…) {
    //       <spec.prologueStmts>                       // e.g. takeN bind, BEFORE bufDecl
    //       var buf : array<spec.bufElemType>
    //       var inscope dset : table<unique_key(…)>    // only when spec.distinctGate != null
    //       <spec.postBufDeclStmts>                    // e.g. bounded-heap early-return guards
    //       <spec.reserveStmts>                        // e.g. buf |> reserve(length(src))
    //       wrap_source_loop(spec.dispatch) {          // or spec.customSourceLoop when the caller pre-composed the loop
    //           <spec.preCondStmts>
    //           if (spec.whereCond) {                  // compose_fold_per_element: distinct gate is INSIDE the where,
    //               let dkey = unique_key(<key>)       //   so filtered-out rows never poison the dedup set
    //               if (!dset |> key_exists(dkey)) {
    //                   dset |> insert(dkey)
    //                   <spec.perElementPush>          // e.g. buf |> push_clone(projection)
    //               }
    //           }
    //       }
    //       <spec.tailStmts>                           // includes the return (e.g. `return <- buf`)
    //   }, src…)
    var bodyStmts : array<Expression?>
    bodyStmts |> reserve(length(spec.prologueStmts) + length(spec.postBufDeclStmts)
                          + length(spec.reserveStmts) + length(spec.tailStmts) + 5)
    bodyStmts |> push_from(spec.prologueStmts)
    let bufName = spec.bufName
    var bufElemType = spec.bufElemType
    if (spec.bufDeclStmt != null) {
        bodyStmts |> push <| spec.bufDeclStmt
    } else {
        bodyStmts |> push <| qmacro_expr() {
            var $i(bufName) : array<$t(bufElemType)>
        }
    }
    if (spec.distinctGate != null) {
        let dg = spec.distinctGate
        let dsetName = dg.dsetName
        var dgBufType = clone_type(dg.bufElemType)
        if (dg.kind is KeyLam) {
            var keyLam = clone_expression(dg.kind as KeyLam)  // nolint:PERF023 load-bearing const-strip: $e wants Expression?, dg.kind reads as const
            bodyStmts |> push <| qmacro_expr() {
                var inscope $i(dsetName) : table<typedecl(_::unique_key(invoke($e(keyLam), default<$t(dgBufType)>)))>
            }
        } else {
            bodyStmts |> push <| qmacro_expr() {
                var inscope $i(dsetName) : table<typedecl(_::unique_key(default<$t(dgBufType)>))>
            }
        }
    }
    bodyStmts |> push_from(spec.postBufDeclStmts)
    bodyStmts |> push_from(spec.reserveStmts)
    if (spec.customSourceLoop != null) {
        bodyStmts |> push <| spec.customSourceLoop
    } else {
        var perElement = compose_fold_per_element(spec)
        if (perElement == null) return null
        var srcLoop = adapter->wrap_source_loop(spec.dispatch, perElement, at)
        if (srcLoop == null) return null
        bodyStmts |> push(srcLoop)
    }
    bodyStmts |> push_from(spec.tailStmts)
    return adapter->wrap_invoke(bodyStmts, spec.outElemType, spec.wrapIter, at)
}

// nolint:STYLE014 — terminal `_select` re-iteration tail. selectLam == null → just `return <- buf`.
// Non-null → re-project every buffered element into a fresh buffer:
//   var outBuf : array<selectElemType>
//   outBuf |> reserve(length(buf))
//   for (e in buf) { outBuf |> push_clone(<select(e)>) }
//   return <- outBuf
// Returns the raw array; the lane's wrapIter knob adds .to_sequence_move() at the outer invoke.
[macro_function]
def build_terminal_select_tail(bufName : string; bufElemType : TypeDeclPtr; selectLam : Expression?;
                               selectElemType : TypeDeclPtr; at : LineInfo) : array<Expression?> {
    var stmts : array<Expression?>
    if (selectLam == null) {
        stmts |> push <| qmacro_expr() {
            return <- $i(bufName)
        }
        return <- stmts
    }
    let outBufName = qn("proj_buf", at)
    let elemName   = qn("proj_e", at)
    var projBody = peel_lambda_replace_var(selectLam, qmacro($i(elemName)))
    stmts |> push_from <| qmacro_block_to_array() {
        var $i(outBufName) : array<$t(selectElemType)>
        $i(outBufName) |> reserve(length($i(bufName)))
        for ($i(elemName) in $i(bufName)) {
            $i(outBufName) |> push_clone($e(projBody))
        }
    }
    if (bufElemType.isSafeToDelete) {
        stmts |> push <| qmacro_expr() {
            delete $i(bufName)
        }
    }
    stmts |> push <| qmacro_expr() {
        return <- $i(outBufName)
    }
    return <- stmts
}

[macro_function]
def emit_any_empty_shortcut(var top : Expression?; srcName : string; at : LineInfo) : Expression? {
    var topExpr = clone_expression(top)
    topExpr.genFlags.alwaysSafe = true
    var res = qmacro(invoke($($i(srcName) : typedecl($e(topExpr))) {
        return !empty($i(srcName))
    }, $e(topExpr)))
    return finalize_invoke(res, at)
}

[macro_function]
def merge_where_cond(var curr : Expression?; var pred : Expression?) : Expression? {
    if (curr == null) return pred
    return <- qmacro($e(curr) && $e(pred))
}

[macro_function]
def strip_const_ref(var t : TypeDeclPtr) : TypeDeclPtr {
    if (t != null) {
        t.flags.constant = false
        t.flags.ref = false
    }
    return t
}

[macro_function]
def mut_result_type(projection : Expression?; elementType : TypeDeclPtr) : TypeDeclPtr {
    return strip_const_ref(clone_type(projection != null ? projection._type : elementType))
}

[macro_function, unused_argument(itName)]
def emit_counter_lane(var adapter : SourceAdapter?; accName, itName : string; names : RangeStateNames;
                      var skipExpr, takeExpr, skipWhileCond : Expression?;
                      var loopBody : Expression?; at : LineInfo; isLong : bool = false) : Expression? {
    // nolint:STYLE014 — count() / long_count(): seed a counter and ride emit_terminator_lane. The caller passes a
    // pre-wrapped loopBody (where / binds / post-take / in-loop ranges already applied) whose work is `acc ++`; this
    // lane only adds the seed + range prelude + `return acc` tail. isLong → int64 acc (long_count), else int. Emits:
    //   invoke((src…) {
    //       <range prelude>
    //       var acc = 0                       // `var acc : int64 = 0l` when isLong
    //       for (it in src) { … acc ++ … }    // = loopBody, caller-supplied
    //       return acc
    //   }, src…)
    var prologue : array<Expression?>
    append_ranges_prelude(prologue, skipExpr, takeExpr, skipWhileCond, names)
    if (isLong) {
        prologue |> push_from <| qmacro_block_to_array() {
            var $i(accName) : int64 = 0l
        }
    } else {
        prologue |> push_from <| qmacro_block_to_array() {
            var $i(accName) = 0
        }
    }
    var perElementStmts : array<Expression?>
    perElementStmts |> push(loopBody)
    var tailStmts <- qmacro_block_to_array() {
        return $i(accName)
    }
    return emit_terminator_lane(TerminatorSpec(
        prologue <- prologue,
        perElementStmts <- perElementStmts,
        tailStmts <- tailStmts,
        dispatch = adapter->effective_dispatch(LoopDispatch(Each = null))
    ), adapter, at)
}

[macro_function]
def emit_accumulator_lane(
                          opName : string;
                          var adapter : SourceAdapter?;
                          var projection : Expression?;
                          var whereCond : Expression?;
                          var postTakeWhereCond : Expression?;
                          var intermediateBinds : array<Expression?>;
                          var preCondStmts : array<Expression?>;
                          var elementType : TypeDeclPtr;
                          accName, itName : string;
                          names : RangeStateNames;
                          var skipExpr, takeExpr, skipWhileCond, takeWhileCond : Expression?;
                          at : LineInfo
                          ) : Expression? {
    // nolint:STYLE014 — single-pass value reducer: sum / min / max / average (count/long_count ride emit_counter_lane).
    // Seeds an accumulator, folds each projected value in, returns the result — all via emit_terminator_lane. The
    // per-op seed / fold / return come from build_accumulator_spec + build_accumulator_perelement_stmts. Emits:
    //   invoke((src…) {
    //       var acc = <seed>              // sum: 0; min/max: first-element latch (a `first` flag); average: sum + cnt
    //       for (it in src) {            // where / ranges / binds applied by emit_terminator_lane
    //           let val = <projection ?? it>
    //           <fold val into acc>      // sum: acc += val; min: if (val < acc) acc = val; …
    //       }
    //       return <result>              // sum/min/max: acc; average: acc / cnt
    //   }, src…)
    var valueExpr : Expression?
    if (projection != null) {
        valueExpr = clone_expression(projection)
    } else {
        valueExpr = qmacro_expr() {
            $i(itName)
        }
    }
    var accType = mut_result_type(projection, elementType)
    let valBindName = qn("val", at)
    let firstName = qn("first", at)
    let cntName = qn("cnt", at)
    let workhorse = ((projection != null && projection._type != null && projection._type.isWorkhorseType)
        || (projection == null && elementType != null && elementType.isWorkhorseType))
    let isDoubleAccType = accType != null && accType.baseType == Type.tDouble
    var spec = build_accumulator_spec(opName, accName, cntName, firstName, accType, at)
    if (spec.returnExpr == null) return null
    var perElementStmts <- build_accumulator_perelement_stmts(opName, accName, valBindName, firstName, cntName, valueExpr, workhorse, isDoubleAccType)
    var tailStmts : array<Expression?>
    tailStmts |> push <| qmacro_expr() {
        return $e(spec.returnExpr)
    }
    return emit_terminator_lane(TerminatorSpec(
        prologue <- spec.prologue,
        preCondStmts <- preCondStmts,
        intermediateBinds <- intermediateBinds,
        whereCond = whereCond,
        postTakeWhereCond = postTakeWhereCond,
        perElementStmts <- perElementStmts,
        skipExpr = skipExpr,
        takeExpr = takeExpr,
        skipWhileCond = skipWhileCond,
        takeWhileCond = takeWhileCond,
        rangeNames = names,
        tailStmts <- tailStmts,
        dispatch = adapter->effective_dispatch(LoopDispatch(Each = null))
    ), adapter, at)
}

[macro_function]
def is_min_max_family(name : string) : bool {
    return (name == "min_by" || name == "max_by" || name == "min_max"
        || name == "min_max_by" || name == "min_max_average" || name == "min_max_average_by")
}

[macro_function]
def emit_min_max_family_lane(
                             opName : string;
                             var termCall : ExprCall?;
                             var adapter : SourceAdapter?;
                             var projection : Expression?;
                             var whereCond : Expression?;
                             var postTakeWhereCond : Expression?;
                             var intermediateBinds : array<Expression?>;
                             var preCondStmts : array<Expression?>;
                             var elementType : TypeDeclPtr;
                             itName : string;
                             names : RangeStateNames;
                             var skipExpr, takeExpr, skipWhileCond, takeWhileCond : Expression?;
                             at : LineInfo
                             ) : Expression? {
    // nolint:STYLE014 — multi-output reduce: min_by / max_by / min_max / min_max_by / min_max_average[_by].
    // Mirrors the daslib impls exactly (first-element latch, strict `less`, elif min/max arms, average
    // seeded from the first element and divided in TT) so splice == plain on every input. Emits:
    //   invoke((src…) {
    //       var mmfirst = true; var mmmin : T [; var mmmax : T ; var mmtot : T ; var mmcnt : uint64]
    //       for (it in src) {              // where / ranges / binds applied by emit_terminator_lane
    //           let mmval = <projection ?? it>
    //           if (mmfirst) { mmmin := mmval … mmfirst = false }
    //           else { if (<mmval beats mmmin>) mmmin := mmval; elif (<mmval beats mmmax>) mmmax := mmval [; mmtot += mmval] }
    //           [mmcnt ++]
    //       }
    //       [mmtot /= T(mmcnt)]
    //       return mmmin | (mmmin, mmmax) | (mmmin, mmmax, mmtot)
    //   }, src…)
    var accType = mut_result_type(projection, elementType)
    if (accType == null || !accType.canCopy) return null
    let isBy = ends_with(opName, "_by")
    var keyLam : Expression?
    if (isBy) {
        if (termCall == null || (termCall.arguments |> length) != 2) return null
        keyLam = termCall.arguments[1]
    }
    let hasAvg = opName == "min_max_average" || opName == "min_max_average_by"
    let isSingle = opName == "min_by" || opName == "max_by"
    let workhorse = accType.isWorkhorseType
    var valueExpr : Expression?
    if (projection != null) {
        valueExpr = clone_expression(projection)
    } else {
        valueExpr = qmacro_expr() {
            $i(itName)
        }
    }
    let valName = qn("mmval", at)
    let minName = qn("mmmin", at)
    let maxName = qn("mmmax", at)
    let totName = qn("mmtot", at)
    let cntName = qn("mmcnt", at)
    let firstName = qn("mmfirst", at)
    var minCmp : Expression?
    var maxCmp : Expression?
    if (isBy) {
        var keyVal = peel_lambda_replace_var(keyLam, qmacro($i(valName)))
        var keyMin = peel_lambda_replace_var(keyLam, qmacro($i(minName)))
        if (keyVal == null || keyMin == null) return null
        if (opName == "max_by") {
            minCmp = qmacro(_::less($e(keyMin), $e(keyVal)))
        } else {
            minCmp = qmacro(_::less($e(keyVal), $e(keyMin)))
        }
        if (!isSingle) {
            var keyMax = peel_lambda_replace_var(keyLam, qmacro($i(maxName)))
            if (keyMax == null) return null
            maxCmp = qmacro(_::less($e(keyMax), $e(keyVal)))
        }
    } else {
        minCmp = min_max_compare(workhorse, "min", valName, minName)
        if (!isSingle) {
            maxCmp = min_max_compare(workhorse, "max", valName, maxName)
        }
    }
    var prologue <- qmacro_block_to_array() {
        var $i(firstName) = true
        var $i(minName) : $t(accType)
    }
    if (!isSingle) {
        prologue |> push <| qmacro_expr() {
            var $i(maxName) : $t(accType)
        }
    }
    if (hasAvg) {
        prologue |> push_from <| qmacro_block_to_array() {
            var $i(totName) : $t(accType)
            var $i(cntName) : uint64
        }
    }
    var perElementStmts : array<Expression?>
    perElementStmts |> push <| qmacro_expr() {
        let $i(valName) = $e(valueExpr)
    }
    if (isSingle) {
        perElementStmts |> push <| qmacro_expr() {
            if ($i(firstName)) {
                $i(minName) := $i(valName)
                $i(firstName) = false
            } elif ($e(minCmp)) {
                $i(minName) := $i(valName)
            }
        }
    } elif (hasAvg) {
        perElementStmts |> push_from <| qmacro_block_to_array() {
            if ($i(firstName)) {
                $i(minName) := $i(valName)
                $i(maxName) := $i(valName)
                $i(totName) := $i(valName)
                $i(firstName) = false
            } else {
                if ($e(minCmp)) {
                    $i(minName) := $i(valName)
                } elif ($e(maxCmp)) {
                    $i(maxName) := $i(valName)
                }
                $i(totName) += $i(valName)
            }
            $i(cntName) ++
        }
    } else {
        perElementStmts |> push <| qmacro_expr() {
            if ($i(firstName)) {
                $i(minName) := $i(valName)
                $i(maxName) := $i(valName)
                $i(firstName) = false
            } else {
                if ($e(minCmp)) {
                    $i(minName) := $i(valName)
                } elif ($e(maxCmp)) {
                    $i(maxName) := $i(valName)
                }
            }
        }
    }
    var tailStmts : array<Expression?>
    if (isSingle) {
        tailStmts |> push <| qmacro_expr() {
            return $i(minName)
        }
    } elif (hasAvg) {
        if (workhorse) {
            let castName = describe(accType)
            tailStmts |> push <| qmacro_expr() {
                if ($i(cntName) != 0ul) {
                    $i(totName) /= $c(castName)($i(cntName))
                }
            }
        } else {
            tailStmts |> push <| qmacro_expr() {
                if ($i(cntName) != 0ul) {
                    $i(totName) /= $i(cntName)
                }
            }
        }
        tailStmts |> push <| qmacro_expr() {
            return ($i(minName), $i(maxName), $i(totName))
        }
    } else {
        tailStmts |> push <| qmacro_expr() {
            return ($i(minName), $i(maxName))
        }
    }
    return emit_terminator_lane(TerminatorSpec(
        prologue <- prologue,
        preCondStmts <- preCondStmts,
        intermediateBinds <- intermediateBinds,
        whereCond = whereCond,
        postTakeWhereCond = postTakeWhereCond,
        perElementStmts <- perElementStmts,
        skipExpr = skipExpr,
        takeExpr = takeExpr,
        skipWhileCond = skipWhileCond,
        takeWhileCond = takeWhileCond,
        rangeNames = names,
        tailStmts <- tailStmts,
        dispatch = adapter->effective_dispatch(LoopDispatch(Each = null))
    ), adapter, at)
}

[macro_function]
def emit_early_exit_lane(
                         opName : string;
                         var adapter : SourceAdapter?;
                         var projection : Expression?;
                         var whereCond : Expression?;
                         var postTakeWhereCond : Expression?;
                         var intermediateBinds : array<Expression?>;
                         var preCondStmts : array<Expression?>;
                         var elementType : TypeDeclPtr;
                         terminatorCall : ExprCall?;
                         itName : string;
                         names : RangeStateNames;
                         var skipExpr, takeExpr, skipWhileCond, takeWhileCond : Expression?;
                         at : LineInfo
                         ) : Expression? {
    // nolint:STYLE014 — early-exit / single-result lane: first[_or_default] / any / all / contains /
    // last[_or_default] / single[_or_default] / element_at[_or_default] / aggregate. Two emission modes:
    //   • directReturn (array / zip / xml — supports_direct_return): mid-loop `return X` exits the invoke.
    //   • decs (nested for_each_archetype lambda CAN'T return out): retain in a state var + `return true`
    //     (which stops the find walk), then the tail returns. Picks dispatch = Find / FindReturn / FindReturnNegated.
    // Canonical shape (first, directReturn):
    //   invoke((src…) {
    //       for (it in src) {                  // where / ranges / binds applied by emit_terminator_lane
    //           let vproj = <projection(it)>   // only when projection != null; else `it` is used directly
    //           return vproj
    //       }
    //       panic("sequence contains no elements"); return default<Ret>
    //   }, src…)
    // Same op on decs (state path): `var found=false; var fst:Ret; for_each{ fst:=v; found=true; return true }
    //   …; if(!found) panic(...); return fst`.
    // Per-op (directReturn shape; decs mirrors via found-flag + `return true`):
    //   first_or_default  → bind `d` once; empty tail returns `d` instead of panic
    //   any               → no pred: `return true` on first survivor; with pred: `if (p(v)) return true`; tail `return false` (decs no-range: FindReturn)
    //   all               → `if (!p(v)) return false`; tail `return true` (decs no-range: FindReturnNegated — inner returns true on first counterexample, negate)
    //   contains          → bind `cval` once; `if (v == cval) return true`; tail `return false` (decs no-range: FindReturn)
    //   last[_or_default] → full walk, `lst := v` each match; tail returns lst (panic/default if none). XML defers: store node handle, materialize once at the tail
    //   single            → full walk; 2nd match panics; empty panics
    //   element_at        → negative index panics/defaults pre-loop; `if (cnt == idx) return v; cnt++`; tail panic/default
    //   aggregate         → seed acc; `acc = aggFn(acc, v)` each element (workhorse `=`/`return`, else `<-`/`return <-`); tail returns acc
    if (opName != "any" && opName != "all" && opName != "contains" && opName != "aggregate") {
        var resType = mut_result_type(projection, elementType)
        if (resType == null || !resType.canCopy) return null
    }
    var perMatchStmts : array<Expression?>
    var valueName = itName
    let projBindName = qn("vproj", at)
    if (projection != null) {
        perMatchStmts |> push <| qmacro_expr() {
            let $i(projBindName) = $e(projection)
        }
        valueName = projBindName
    }
    var preludeStmts : array<Expression?>
    var tailStmts : array<Expression?>
    let directReturn = (adapter == null || adapter->supports_direct_return())
    var dispatch = directReturn ? LoopDispatch(Each = null) : adapter->effective_dispatch(LoopDispatch(Each = null))
    if (opName == "first") {
        var retType = mut_result_type(projection, elementType)
        if (directReturn) {
            perMatchStmts |> push <| qmacro_expr() {
                return $i(valueName)
            }
            tailStmts |> push_from <| qmacro_block_to_array() {
                panic("sequence contains no elements")
                return default<$t(retType)>
            }
        } else {
            let foundName = qn("found", at)
            let resultName = qn("fst", at)
            preludeStmts |> push_from <| qmacro_block_to_array() {
                var $i(foundName) = false
                var $i(resultName) : $t(retType)
            }
            perMatchStmts |> push_from <| qmacro_block_to_array() {
                $i(resultName) := $i(valueName)
                $i(foundName) = true
                return true
            }
            tailStmts |> push_from <| qmacro_block_to_array() {
                if (!$i(foundName)) {
                    panic("sequence contains no elements")
                }
                return $i(resultName)
            }
            dispatch = LoopDispatch(Find = null)
        }
    } elif (opName == "first_or_default") {
        let defaultName = qn("dval", at)
        preludeStmts |> push <| qmacro_expr() {
            let $i(defaultName) = $e(terminatorCall.arguments[1])
        }
        if (directReturn) {
            perMatchStmts |> push <| qmacro_expr() {
                return $i(valueName)
            }
            tailStmts |> push <| qmacro_expr() {
                return $i(defaultName)
            }
        } else {
            var retType = mut_result_type(projection, elementType)
            let foundName = qn("found", at)
            let resultName = qn("fst", at)
            preludeStmts |> push_from <| qmacro_block_to_array() {
                var $i(foundName) = false
                var $i(resultName) : $t(retType)
            }
            perMatchStmts |> push_from <| qmacro_block_to_array() {
                $i(resultName) := $i(valueName)
                $i(foundName) = true
                return true
            }
            tailStmts |> push_from <| qmacro_block_to_array() {
                if (!$i(foundName)) {
                    return $i(defaultName)
                }
                return $i(resultName)
            }
            dispatch = LoopDispatch(Find = null)
        }
    } elif (opName == "any") {
        let argCount = length(terminatorCall.arguments)
        var predExpr : Expression?
        if (argCount > 1) {
            predExpr = peel_lambda_rename_var(terminatorCall.arguments[1], valueName)
        }
        if (directReturn) {
            if (argCount > 1) {
                perMatchStmts |> push <| qmacro_expr() {
                    if ($e(predExpr)) {
                        return true
                    }
                }
            } else {
                perMatchStmts |> push <| qmacro_expr() {
                    return true
                }
            }
            tailStmts |> push <| qmacro_expr() {
                return false
            }
        } elif (dispatch is Find) {
            if (argCount > 1 && predExpr == null) return null
            let foundName = qn("found", at)
            preludeStmts |> push <| qmacro_expr() {
                var $i(foundName) = false
            }
            if (argCount > 1) {
                perMatchStmts |> push <| qmacro_expr() {
                    if ($e(predExpr)) {
                        $i(foundName) = true
                        return true
                    }
                }
            } else {
                perMatchStmts |> push_from <| qmacro_block_to_array() {
                    $i(foundName) = true
                    return true
                }
            }
            tailStmts |> push <| qmacro_expr() {
                return $i(foundName)
            }
        } else {
            if (argCount > 1 && predExpr == null) return null
            if (argCount > 1) {
                perMatchStmts |> push <| qmacro_expr() {
                    if ($e(predExpr)) return true
                }
            } else {
                perMatchStmts |> push <| qmacro_expr() {
                    return true
                }
            }
            dispatch = LoopDispatch(FindReturn = null)
        }
    } elif (opName == "all") {
        var predExpr = peel_lambda_rename_var(terminatorCall.arguments[1], valueName)
        if (directReturn) {
            perMatchStmts |> push <| qmacro_expr() {
                if (!$e(predExpr)) {
                    return false
                }
            }
            tailStmts |> push <| qmacro_expr() {
                return true
            }
        } elif (dispatch is Find) {
            if (predExpr == null) return null
            let foundName = qn("found", at)
            preludeStmts |> push <| qmacro_expr() {
                var $i(foundName) = false
            }
            perMatchStmts |> push <| qmacro_expr() {
                if (!$e(predExpr)) {
                    $i(foundName) = true
                    return true
                }
            }
            tailStmts |> push <| qmacro_expr() {
                return !$i(foundName)
            }
        } else {
            if (predExpr == null) return null
            perMatchStmts |> push <| qmacro_expr() {
                if (!$e(predExpr)) return true
            }
            dispatch = LoopDispatch(FindReturnNegated = null)
        }
    } elif (opName == "contains") {
        let containsValName = qn("cval", at)
        preludeStmts |> push <| qmacro_expr() {
            let $i(containsValName) = $e(terminatorCall.arguments[1])
        }
        if (directReturn) {
            perMatchStmts |> push <| qmacro_expr() {
                if ($i(valueName) == $i(containsValName)) {
                    return true
                }
            }
            tailStmts |> push <| qmacro_expr() {
                return false
            }
        } elif (dispatch is Find) {
            let foundName = qn("found", at)
            preludeStmts |> push <| qmacro_expr() {
                var $i(foundName) = false
            }
            perMatchStmts |> push <| qmacro_expr() {
                if ($i(valueName) == $i(containsValName)) {
                    $i(foundName) = true
                    return true
                }
            }
            tailStmts |> push <| qmacro_expr() {
                return $i(foundName)
            }
        } else {
            perMatchStmts |> push <| qmacro_expr() {
                if ($i(valueName) == $i(containsValName)) return true
            }
            dispatch = LoopDispatch(FindReturn = null)
        }
    } elif (opName == "last" || opName == "last_or_default") {
        var retType = mut_result_type(projection, elementType)
        let foundName = qn("found", at)
        let lastBindName = qn("lst", at)
        if (adapter != null && whereCond == null && postTakeWhereCond == null && skipExpr == null
                && takeExpr == null && skipWhileCond == null && takeWhileCond == null
                && empty(intermediateBinds) && empty(preCondStmts) && projection == null) {
            var lastDefault : Expression?
            if (opName == "last_or_default") {
                lastDefault = clone_expression(terminatorCall.arguments[1])
            }
            var back = adapter->emit_reverse_last_backward(itName, null, null, lastDefault, elementType, retType, at)
            if (back != null) return back
        }
        let deferLast = (adapter != null && adapter->defers_materialization()
            && projection == null && adapter->handle_type() != null)
        if (deferLast) {
            var handleType = adapter->handle_type()
            let lastNodeName = qn("lst_node", at)
            let matItName = qn("lst_mat", at)
            var matStmt = adapter->materialize_handle(qmacro($i(lastNodeName)), matItName, at)
            preludeStmts |> push_from <| qmacro_block_to_array() {
                var $i(foundName) = false
                var $i(lastNodeName) = default<$t(handleType)>
            }
            perMatchStmts |> push_from <| qmacro_block_to_array() {
                $i(foundName) = true
                $i(lastNodeName) = $e(adapter->current_handle_expr(at))
            }
            if (opName == "last") {
                tailStmts |> push_from <| qmacro_block_to_array() {
                    if (!$i(foundName)) {
                        panic("sequence contains no elements")
                    }
                    var $i(matItName) = default<$t(retType)>
                    $e(matStmt)
                    return <- $i(matItName)
                }
            } else {
                let defaultName = qn("dval", at)
                let dvCanCopy = (terminatorCall.arguments[1]._type != null && terminatorCall.arguments[1]._type.canCopy)
                preludeStmts |> push(emit_default_bind(defaultName, terminatorCall.arguments[1], dvCanCopy))
                tailStmts |> push_from <| qmacro_block_to_array() {
                    if (!$i(foundName)) {
                        $e(emit_default_return(defaultName, dvCanCopy))
                    }
                    var $i(matItName) = default<$t(retType)>
                    $e(matStmt)
                    return <- $i(matItName)
                }
            }
        } else {
            preludeStmts |> push_from <| qmacro_block_to_array() {
                var $i(foundName) = false
                var $i(lastBindName) : $t(retType)
            }
            perMatchStmts |> push_from <| qmacro_block_to_array() {
                $i(foundName) = true
                $i(lastBindName) := $i(valueName)
            }
            if (opName == "last") {
                tailStmts |> push_from <| qmacro_block_to_array() {
                    if (!$i(foundName)) {
                        panic("sequence contains no elements")
                    }
                    return <- $i(lastBindName)
                }
            } else {
                let defaultName = qn("dval", at)
                let dvCanCopy = (terminatorCall.arguments[1]._type != null && terminatorCall.arguments[1]._type.canCopy)
                preludeStmts |> push(emit_default_bind(defaultName, terminatorCall.arguments[1], dvCanCopy))
                tailStmts |> push_from <| qmacro_block_to_array() {
                    if (!$i(foundName)) {
                        $e(emit_default_return(defaultName, dvCanCopy))
                    }
                    return <- $i(lastBindName)
                }
            }
        }
    } elif (opName == "single" || opName == "single_or_default") {
        var retType = mut_result_type(projection, elementType)
        let foundName = qn("found", at)
        let bindName = qn("sng", at)
        preludeStmts |> push_from <| qmacro_block_to_array() {
            var $i(foundName) = false
            var $i(bindName) : $t(retType)
        }
        if (opName == "single") {
            perMatchStmts |> push_from <| qmacro_block_to_array() {
                if ($i(foundName)) {
                    panic("sequence contains more than one element")
                }
                $i(foundName) = true
                $i(bindName) := $i(valueName)
            }
            tailStmts |> push_from <| qmacro_block_to_array() {
                if (!$i(foundName)) {
                    panic("sequence contains no elements")
                }
                return <- $i(bindName)
            }
        } else {
            let defaultName = qn("dval", at)
            let dvCanCopy = (terminatorCall.arguments[1]._type != null && terminatorCall.arguments[1]._type.canCopy)
            preludeStmts |> push(emit_default_bind(defaultName, terminatorCall.arguments[1], dvCanCopy))
            if (directReturn) {
                perMatchStmts |> push_from <| qmacro_block_to_array() {
                    if ($i(foundName)) {
                        $e(emit_default_return(defaultName, dvCanCopy))
                    }
                    $i(foundName) = true
                    $i(bindName) := $i(valueName)
                }
                tailStmts |> push_from <| qmacro_block_to_array() {
                    if (!$i(foundName)) {
                        $e(emit_default_return(defaultName, dvCanCopy))
                    }
                    return <- $i(bindName)
                }
            } else {
                let stopName = qn("stop", at)
                preludeStmts |> push <| qmacro_expr() {
                    var $i(stopName) = false
                }
                perMatchStmts |> push_from <| qmacro_block_to_array() {
                    if ($i(foundName)) {
                        $i(stopName) = true
                        return true
                    }
                    $i(foundName) = true
                    $i(bindName) := $i(valueName)
                }
                tailStmts |> push_from <| qmacro_block_to_array() {
                    if ($i(stopName) || !$i(foundName)) {
                        $e(emit_default_return(defaultName, dvCanCopy))
                    }
                    return <- $i(bindName)
                }
                dispatch = LoopDispatch(Find = null)
            }
        }
    } elif (opName == "element_at" || opName == "element_at_or_default") {
        var retType = mut_result_type(projection, elementType)
        let idxName = qn("idx", at)
        let cntName = qn("ec", at)
        preludeStmts |> push <| qmacro_expr() {
            let $i(idxName) = $e(terminatorCall.arguments[1])
        }
        if (opName == "element_at") {
            preludeStmts |> push <| qmacro_expr() {
                if ($i(idxName) < 0) {
                    panic("element index out of range")
                }
            }
        } else {
            let tName = qn("emptyT", at)
            preludeStmts |> push_from <| qmacro_block_to_array() {
                if ($i(idxName) < 0) {
                    var $i(tName) : $t(retType)
                    return <- $i(tName)
                }
            }
        }
        preludeStmts |> push <| qmacro_expr() {
            var $i(cntName) = 0
        }
        if (directReturn) {
            perMatchStmts |> push_from <| qmacro_block_to_array() {
                if ($i(cntName) == $i(idxName)) {
                    return $i(valueName)
                }
                $i(cntName) ++
            }
            if (opName == "element_at") {
                let tName = qn("emptyT", at)
                tailStmts |> push_from <| qmacro_block_to_array() {
                    panic("element index out of range")
                    var $i(tName) : $t(retType)
                    return <- $i(tName)
                }
            } else {
                let tName = qn("emptyT", at)
                tailStmts |> push_from <| qmacro_block_to_array() {
                    var $i(tName) : $t(retType)
                    return <- $i(tName)
                }
            }
        } else {
            let foundName = qn("found", at)
            let resultName = qn("eares", at)
            preludeStmts |> push_from <| qmacro_block_to_array() {
                var $i(foundName) = false
                var $i(resultName) : $t(retType)
            }
            perMatchStmts |> push_from <| qmacro_block_to_array() {
                if ($i(cntName) == $i(idxName)) {
                    $i(resultName) := $i(valueName)
                    $i(foundName) = true
                    return true
                }
                $i(cntName) ++
            }
            if (opName == "element_at") {
                tailStmts |> push_from <| qmacro_block_to_array() {
                    if (!$i(foundName)) {
                        panic("element index out of range")
                    }
                    return <- $i(resultName)
                }
            } else {
                let tName = qn("emptyT", at)
                tailStmts |> push_from <| qmacro_block_to_array() {
                    if (!$i(foundName)) {
                        var $i(tName) : $t(retType)
                        return <- $i(tName)
                    }
                    return <- $i(resultName)
                }
            }
            dispatch = LoopDispatch(Find = null)
        }
    } elif (opName == "aggregate") {
        var seedExpr = clone_expression(terminatorCall.arguments[1])
        var aggFn = clone_expression(terminatorCall.arguments[2])
        var accType = clone_type(seedExpr._type)
        if (accType != null) {
            accType.flags.constant = false
            accType.flags.ref = false
        }
        let aggAccName = qn("agg", at)
        let aggIsWorkhorse = (seedExpr._type != null && seedExpr._type.isWorkhorseType)
        if (aggIsWorkhorse) {
            preludeStmts |> push <| qmacro_expr() {
                var $i(aggAccName) : $t(accType) = $e(seedExpr)
            }
        } else {
            preludeStmts |> push <| qmacro_expr() {
                var $i(aggAccName) : $t(accType) <- $e(seedExpr)
            }
        }
        var peeled = peel_lambda_rename_2vars(aggFn, aggAccName, valueName)
        if (peeled != null) {
            if (aggIsWorkhorse) {
                perMatchStmts |> push <| qmacro_expr() {
                    $i(aggAccName) = $e(peeled)
                }
            } else {
                perMatchStmts |> push <| qmacro_expr() {
                    $i(aggAccName) <- $e(peeled)
                }
            }
        } else {
            if (aggIsWorkhorse) {
                perMatchStmts |> push <| qmacro_expr() {
                    $i(aggAccName) = invoke($e(aggFn), $i(aggAccName), $i(valueName))
                }
            } else {
                perMatchStmts |> push <| qmacro_expr() {
                    $i(aggAccName) <- invoke($e(aggFn), $i(aggAccName), $i(valueName))
                }
            }
        }
        if (aggIsWorkhorse) {
            tailStmts |> push <| qmacro_expr() {
                return $i(aggAccName)
            }
        } else {
            tailStmts |> push <| qmacro_expr() {
                return <- $i(aggAccName)
            }
        }
    } else {
        return null
    }
    return emit_terminator_lane(TerminatorSpec(
        prologue <- preludeStmts,
        preCondStmts <- preCondStmts,
        intermediateBinds <- intermediateBinds,
        whereCond = whereCond,
        postTakeWhereCond = postTakeWhereCond,
        perElementStmts <- perMatchStmts,
        skipExpr = skipExpr,
        takeExpr = takeExpr,
        skipWhileCond = skipWhileCond,
        takeWhileCond = takeWhileCond,
        rangeNames = names,
        tailStmts <- tailStmts,
        dispatch = dispatch
    ), adapter, at)
}

[macro_function]
def emit_loop_or_count_lane(var c : Captures; var ctx : EmitCtx; at : LineInfo) : Expression? {    // nolint:STYLE037 — at the cap already; the +2 is the intermediate-bind flush guards, no natural seam
    // nolint:STYLE014 — the loop_or_count planner. Walks the head chain (where_ / select runs) into a whereCond,
    // a running projection, intermediate select-binds + preCond binds; then reads skip / take / skip_while /
    // take_while + an optional post-take where + terminator. Routes to one lane (the peeled where/select/range
    // slots thread into whichever is chosen):
    //   source has its own lane (decs)            → ctx.src->emit_loop_or_count
    //   counting, no where/limits, pure proj      → ctx.src->count_shortcut          (O(1), e.g. length(src))
    //   sum / min / max / average                 → emit_accumulator_lane
    //   first / any / all / contains / last / …   → emit_early_exit_lane             (any-no-pred → emit_any_empty_shortcut)
    //   count / long_count                        → emit_counter_lane,    loopBody = `acc ++` (gated)
    //   no terminator (to_array)                  → emit_fold_array_lane, perElementPush = `buf |> push_clone(projection ?? it)`
    // PR C Decs-arm: route to existing emit_decs_* lane fns. State hoists above for_each_archetype (different invoke shape from array side); unifying lane fns is out of scope per D1. Sources with their own lane delegate entirely — a null hook result bails to tier-2 (must NOT fall through to the array lane, which assumes an array `top`).
    if (ctx.src->has_own_loop_or_count_lane()) return ctx.src->emit_loop_or_count(c, ctx, at)
    var top = ctx.top
    let srcName = ctx.src->loop_source_name()
    let itName = qn("it", at)
    let accName = qn("acc", at)
    let names <- make_range_names(at)
    let hasTerminator = c.single |> key_exists("term")
    var lastName : string
    if (hasTerminator) {
        lastName = call_norm_name(c.single["term"])
    }
    let lane = hasTerminator ? classify_terminator(lastName) : LinqLane.ARRAY
    if (lane == LinqLane.UNKNOWN) return null
    let counting = lane == LinqLane.COUNTER || (lane == LinqLane.ACCUMULATOR && lastName == "long_count")
    if (top._type == null || top._type.firstType == null) return null
    var whereCond : Expression?
    var postTakeWhereCond : Expression?
    var projection : Expression?
    var intermediateBinds : array<Expression?>
    var preCondStmts : array<Expression?>
    var seenSelect = false
    var allProjectionsPure = true
    var elementType = clone_type(top._type.firstType)
    var lastBindName = itName
    for (call in c.many["head"]) {
        let opName = call_norm_name(call)
        if (opName == "where_") {
            var predicate : Expression?
            if (seenSelect) {
                if (has_sideeffects(projection)) return null
                if (!empty(intermediateBinds)) {
                    preCondStmts |> push_from(intermediateBinds)
                    intermediateBinds |> clear()
                }
                if (!counting) {
                    let wbName = "`vw`{at.line}`{at.column}`{length(preCondStmts)}"
                    var projType = clone_type(elementType)
                    preCondStmts |> push <| qmacro_expr() {
                        var $i(wbName) : $t(projType) := $e(projection)
                    }
                    var pvar = new ExprVar(at = at, name := wbName)
                    pvar._type = clone_type(elementType)
                    pvar._type.flags.ref = true
                    projection = pvar
                }
                predicate = peel_lambda_replace_var(call.arguments[1], projection)
            } else {
                predicate = peel_lambda_rename_var(call.arguments[1], itName)
            }
            if (whereCond == null) {
                whereCond = predicate
            } else {
                whereCond = qmacro($e(whereCond) && $e(predicate))
            }
        } elif (opName == "select") {
            if (projection != null) {
                if (has_sideeffects(projection)) {
                    allProjectionsPure = false
                }
                let bindName = "`v`{at.line}`{at.column}`{length(intermediateBinds)}"
                intermediateBinds |> push <| qmacro_expr() {
                    var $i(bindName) := $e(projection)
                }
                lastBindName = bindName
            }
            projection = peel_lambda_rename_var(call.arguments[1], lastBindName)
            elementType = clone_type(call._type.firstType)
            seenSelect = true
        } else {
            return null
        }
    }
    if (projection != null && has_sideeffects(projection)) {
        allProjectionsPure = false
    }
    var skipExpr : Expression?
    var skipWhileCond : Expression?
    var takeWhileCond : Expression?
    var takeExpr : Expression?
    if (c.single |> key_exists("skip")) {
        var skipArg = c.single["skip"].arguments[1]
        if (skipArg == null || skipArg._type == null || skipArg._type.baseType != Type.tInt) return null
        skipExpr = clone_expression(skipArg)
    }
    if (c.single |> key_exists("skip_while")) {
        if (seenSelect) return null
        var swArg = c.single["skip_while"].arguments[1]
        if (swArg == null) return null
        skipWhileCond = peel_lambda_rename_var(swArg, itName)
        if (skipWhileCond == null) return null
    }
    if (c.single |> key_exists("take_while")) {
        if (seenSelect) return null
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
    if (c.single |> key_exists("post_take_where")) {
        var ptwArg = c.single["post_take_where"].arguments[1]
        if (ptwArg == null) return null
        if (seenSelect) {
            if (has_sideeffects(projection)) return null
            if (!empty(intermediateBinds)) {
                preCondStmts |> push_from(intermediateBinds)
                intermediateBinds |> clear()
            }
            if (!counting) {
                let wbName = "`vw`{at.line}`{at.column}`{length(preCondStmts)}"
                var projType = clone_type(elementType)
                preCondStmts |> push <| qmacro_expr() {
                    var $i(wbName) : $t(projType) := $e(projection)
                }
                var pvar = new ExprVar(at = at, name := wbName)
                pvar._type = clone_type(elementType)
                pvar._type.flags.ref = true
                projection = pvar
            }
            postTakeWhereCond = peel_lambda_replace_var(ptwArg, projection)
        } else {
            postTakeWhereCond = peel_lambda_rename_var(ptwArg, itName)
        }
    }
    let noLimits = skipExpr == null && takeExpr == null && skipWhileCond == null && takeWhileCond == null
    if (counting && whereCond == null && allProjectionsPure && noLimits) {
        var sc = ctx.src->count_shortcut(lastName, at)
        if (sc != null) return sc
    }
    if (lane == LinqLane.ACCUMULATOR && !counting) {
        if (is_min_max_family(lastName)) {
            return emit_min_max_family_lane(lastName, c.single["term"], ctx.src, projection, whereCond, postTakeWhereCond,
                intermediateBinds, preCondStmts, elementType, itName, names,
                skipExpr, takeExpr, skipWhileCond, takeWhileCond, at)
        }
        return emit_accumulator_lane(lastName, ctx.src, projection, whereCond, postTakeWhereCond,
            intermediateBinds, preCondStmts, elementType, accName, itName, names,
            skipExpr, takeExpr, skipWhileCond, takeWhileCond, at)
    }
    if (lane == LinqLane.EARLY_EXIT) {
        let terminatorCall = c.single["term"]
        let isAnyNoPred = lastName == "any" && length(terminatorCall.arguments) == 1
        if (isAnyNoPred && whereCond == null && allProjectionsPure && noLimits
                && type_has_length(top._type))
            return emit_any_empty_shortcut(top, srcName, at)
        return emit_early_exit_lane(lastName, ctx.src, projection, whereCond, postTakeWhereCond,
            intermediateBinds, preCondStmts, elementType, terminatorCall, itName, names,
            skipExpr, takeExpr, skipWhileCond, takeWhileCond, at)
    }
    var loopBody : Expression?
    if (counting) {
        var stmts : array<Expression?>
        if (projection != null && has_sideeffects(projection)) {
            let finalBindName = qn("vfinal", at)
            stmts |> push <| qmacro_expr() {
                var $i(finalBindName) = $e(projection)
            }
        }
        var incExpr = qmacro_expr() {
            $i(accName) ++
        }
        stmts |> push(wrap_with_condition(incExpr, postTakeWhereCond))
        prepend_binds(stmts, intermediateBinds)
        wrap_with_ranges(stmts, skipExpr, takeExpr, skipWhileCond, takeWhileCond, names)
        loopBody = prepend_precond(wrap_with_condition(stmts_to_expr(stmts), whereCond), preCondStmts)
    } elif (lastName == "to_table") {
        var termCall = c.single["term"]
        if (termCall._type == null || length(termCall.arguments) != 1) return null
        var tabType = strip_const_ref(clone_type(termCall._type))
        let isSet = tabType.secondType == null || tabType.secondType.baseType == Type.tVoid
        var stmts : array<Expression?>
        var pushExpr : Expression?
        if (isSet) {
            var keyExpr = projection != null ? projection : qmacro($i(itName))
            pushExpr = qmacro_expr() {
                $i(accName) |> insert($e(keyExpr))
            }
        } elif (projection == null) {
            var f0 = "_0"
            var f1 = "_1"
            let elemTupT = top._type.firstType
            if (elemTupT != null && elemTupT.argNames |> length == 2) {
                f0 = string(elemTupT.argNames[0])
                f1 = string(elemTupT.argNames[1])
            }
            pushExpr = qmacro_expr() {
                $i(accName)[$i(itName).$f(f0)] := $i(itName).$f(f1)
            }
        } else {
            var proj = projection
            if (proj is ExprRef2Value) {
                proj = (proj as ExprRef2Value).subexpr
            }
            if (proj is ExprMakeTuple) {
                var mt = proj as ExprMakeTuple
                if (mt.values |> length != 2) return null
                var keyExpr = mt.values[0]
                var valExpr = mt.values[1]
                pushExpr = qmacro_expr() {
                    $i(accName)[$e(keyExpr)] := $e(valExpr)
                }
            } else {
                let kvbName = qn("kvb", at)
                var bindInit = qmacro_expr() {
                    let $i(kvbName) := $e(projection)
                }
                var bindInsert = qmacro_expr() {
                    $i(accName)[$i(kvbName)._0] := $i(kvbName)._1
                }
                var bindStmts : array<Expression?> <- [bindInit, bindInsert]
                pushExpr = stmts_to_expr(bindStmts)
            }
        }
        stmts |> push(wrap_with_condition(pushExpr, postTakeWhereCond))
        prepend_binds(stmts, intermediateBinds)
        wrap_with_ranges(stmts, skipExpr, takeExpr, skipWhileCond, takeWhileCond, names)
        var perElementPush = stmts_to_expr(stmts)
        var prologueStmts : array<Expression?>
        append_ranges_prelude(prologueStmts, skipExpr, takeExpr, skipWhileCond, names)
        var reserveStmts : array<Expression?>
        if (ctx.src->can_reserve_by_length() && whereCond == null && postTakeWhereCond == null) {
            let rtop = ctx.src->loop_source_expr()
            if (rtop != null && rtop._type != null && type_has_length(rtop._type)) {
                if (takeExpr != null) {
                    reserveStmts |> push <| qmacro_expr() {
                        $i(accName) |> reserve($e(takeExpr) < length($i(srcName)) ? $e(takeExpr) : length($i(srcName)))
                    }
                } else {
                    reserveStmts |> push <| qmacro_expr() {
                        $i(accName) |> reserve(length($i(srcName)))
                    }
                }
            }
        }
        var bufDecl = qmacro_expr() {
            var $i(accName) : $t(tabType)
        }
        var tailStmts : array<Expression?>
        tailStmts |> push(buffer_return(accName, false))
        return emit_fold_array_lane(FoldArraySpec(
            bufDeclStmt = bufDecl,
            bufElemType = elementType,
            bufName = accName,
            prologueStmts <- prologueStmts,
            reserveStmts <- reserveStmts,
            preCondStmts <- preCondStmts,
            whereCond = whereCond,
            perElementPush = perElementPush,
            tailStmts <- tailStmts,
            wrapIter = false
        ), ctx.src, at)
    } else {
        var stmts : array<Expression?>
        var pushExpr : Expression?
        if (projection != null) {
            pushExpr = qmacro_expr() {
                $i(accName) |> push_clone($e(projection))
            }
        } elif (whereCond != null || postTakeWhereCond != null || skipExpr != null || takeExpr != null
                || skipWhileCond != null || takeWhileCond != null) {
            pushExpr = qmacro_expr() {
                $i(accName) |> push_clone($i(itName))
            }
        } else {
            return null
        }
        stmts |> push(wrap_with_condition(pushExpr, postTakeWhereCond))
        prepend_binds(stmts, intermediateBinds)
        wrap_with_ranges(stmts, skipExpr, takeExpr, skipWhileCond, takeWhileCond, names)
        var perElementPush = stmts_to_expr(stmts)
        var prologueStmts : array<Expression?>
        append_ranges_prelude(prologueStmts, skipExpr, takeExpr, skipWhileCond, names)
        var reserveStmts : array<Expression?>
        let sourceHasLength = type_has_length(top._type)
        if (sourceHasLength && !ctx.expr_is_iterator) {
            if (takeExpr != null) {
                reserveStmts |> push <| qmacro_expr() {
                    $i(accName) |> reserve($e(takeExpr) < length($i(srcName)) ? $e(takeExpr) : length($i(srcName)))
                }
            } else {
                reserveStmts |> push <| qmacro_expr() {
                    $i(accName) |> reserve(length($i(srcName)))
                }
            }
        }
        var tailStmts : array<Expression?>
        tailStmts |> push(buffer_return(accName, ctx.expr_is_iterator))
        return emit_fold_array_lane(FoldArraySpec(
            prologueStmts <- prologueStmts,
            bufElemType = elementType,
            bufName = accName,
            reserveStmts <- reserveStmts,
            preCondStmts <- preCondStmts,
            whereCond = whereCond,
            perElementPush = perElementPush,
            tailStmts <- tailStmts,
            wrapIter = false
        ), ctx.src, at)
    }
    return emit_counter_lane(ctx.src, accName, itName, names,
        skipExpr, takeExpr, skipWhileCond, loopBody, at, lastName == "long_count")
}

[macro_function]
def build_loop_or_count_rows {
    splice_patterns |> emplace <| SplicePattern(
        name = "loop_or_count_general",
        chain <- [
            slot_chain_of(["where_", "select"], "head"),
            Slot(matcher = m_literal("skip"),       cardinality = c_opt(), capture_name = "skip"),
            Slot(matcher = m_literal("skip_while"), cardinality = c_opt(), capture_name = "skip_while"),
            Slot(matcher = m_literal("take_while"), cardinality = c_opt(), capture_name = "take_while"),
            Slot(matcher = m_literal("take"),       cardinality = c_opt(), capture_name = "take"),
            Slot(matcher = m_literal("where_"),     cardinality = c_opt(), capture_name = "post_take_where"),
            Slot(matcher = m_alias("loop_terminator_family"), cardinality = c_opt(), capture_name = "term")
        ],
        requires <- array<RequiresPredicate>(),
        emit = @@ < EmitFn > emit_loop_or_count_lane
    )
}

struct JoinAdapterPieces {
    collectInner : Expression?
    probeOuter   : Expression?
}

// nolint:STYLE014 — peels keya/keyb/result and builds the hashed-join collect + probe inners shared by the DecsJoin
// / ArrayJoin adapter wrap_source_loop (the two callsites differ only in how they wrap these with source iteration):
//   collectInner → hash[keyb(tupB)] |> push_clone(tupB)                                              // build side
//   probeOuter   → get(hash, keya(tupA), $(var arr) { for (tupB in arr) { let res = result(tupA, tupB); <body> } })
[macro_function, unused_argument(at)]
def build_join_adapter_pieces(
                              keyaLam        : Expression?;
                              keybLam        : Expression?;
                              resultLam      : Expression?;
                              tupAName       : string;
                              tupBName       : string;
                              jhashName      : string;
                              arrName        : string;
                              resName        : string;
                              tupBType       : TypeDeclPtr;
                              resultType     : TypeDeclPtr;
                              var body       : Expression?;
                              at             : LineInfo
                              ) : JoinAdapterPieces? {
    var keyaBody = peel_lambda_rename_var(keyaLam, tupAName)
    var keybBody = peel_lambda_rename_var(keybLam, tupBName)
    var resultBody = peel_lambda_rename_2vars(resultLam, tupAName, tupBName)
    if (keyaBody == null || keybBody == null || resultBody == null) return null
    var collectInner <- qmacro_expr() {
        // nolint:PERF006,LINT019 per-key bucket size unknown ahead of time
        $i(jhashName)[$e(keybBody)] |> push_clone($i(tupBName))
    }
    var probeOuter <- qmacro_expr() {
        get($i(jhashName), $e(keyaBody), $(var $i(arrName) : array<$t(tupBType)>) {
            for ($i(tupBName) in $i(arrName)) {
                let $i(resName) : $t(resultType) = $e(resultBody)
                $e(body)
            }
        })
    }
    return <- new JoinAdapterPieces(
        collectInner <- collectInner,
        probeOuter <- probeOuter
    )
}


[macro_function]
def buffer_return(bufName : string; isIterator : bool) : Expression? {
    if (isIterator) return <- qmacro_expr() {
        return <- $i(bufName).to_sequence_move()
    }
    return <- qmacro_expr() {
        return <- $i(bufName)
    }
}

[macro_function]
def order_top_n_call_name(orderName : string) : string {
    if (orderName == "order") return "top_n"
    if (orderName == "order_descending") return "top_n_descending"
    if (orderName == "order_by") return "top_n_by"
    if (orderName == "order_by_descending") return "top_n_by_descending"
    return ""
}

def order_min_call_name(orderName : string; hasKey : bool) : string {
    let isDesc = orderName == "order_descending" || orderName == "order_by_descending"
    if (hasKey) return isDesc ? "max_by" : "min_by"
    return isDesc ? "max" : "min"
}

[macro_function, unused_argument(elemType, at)]
def try_make_inline_cmp(orderKey : Expression?; orderName : string;
                        elemType : TypeDeclPtr; at : LineInfo) : Expression? {
    // nolint:STYLE014 — if orderKey is a peelable single-return block `$(x) => KEY`, build an inline 2-arg
    // comparator (untyped args; typer infers from the call site). Emits, asc / desc:
    //   $(v1, v2) { return _::less(KEY[x:=v1], KEY[x:=v2]) }    // descending swaps v1 / v2
    // Returns null when the key isn't inline-peelable (side-effects / multi-stmt) — caller passes the block instead.
    if (orderKey == null || !(orderKey is ExprMakeBlock)) return null
    var mblk = orderKey as ExprMakeBlock
    var blk = mblk._block as ExprBlock
    if (blk.arguments |> length != 1 || blk.list |> length != 1
            || !(blk.list[0] is ExprReturn)) return null
    var ret = blk.list[0] as ExprReturn
    if (ret.subexpr == null || has_sideeffects(ret.subexpr)) return null
    let argName = string(blk.arguments[0].name)
    var b1 = clone_expression(ret.subexpr)
    var b2 = clone_expression(ret.subexpr)
    var r1 : Template
    r1 |> renameVariable(argName, "v1")
    var r2 : Template
    r2 |> renameVariable(argName, "v2")
    apply_template(r1, b1.at, b1)
    apply_template(r2, b2.at, b2)
    var cmpExpr : Expression?
    if (orderName == "order_by_descending") {
        cmpExpr = qmacro(_::less($e(b2), $e(b1)))
    } else {
        cmpExpr = qmacro(_::less($e(b1), $e(b2)))
    }
    return qmacro($(v1, v2) {
        return $e(cmpExpr)
    })
}

[unused_argument(at)]
def make_inline_less_call(orderKey : Expression?; orderName : string;
                          var lhsExpr, rhsExpr : Expression?; at : LineInfo) : Expression? {
    if (orderKey == null || !(orderKey is ExprMakeBlock)) return null
    var mblk = orderKey as ExprMakeBlock
    var blk = mblk._block as ExprBlock
    if (blk.arguments |> length != 1 || blk.list |> length != 1
            || !(blk.list[0] is ExprReturn)) return null
    var ret = blk.list[0] as ExprReturn
    if (ret.subexpr == null || has_sideeffects(ret.subexpr)) return null
    let argName = string(blk.arguments[0].name)
    var b1 = clone_expression(ret.subexpr)
    var b2 = clone_expression(ret.subexpr)
    var r1 : Template
    r1 |> replaceVariable(argName, lhsExpr)
    var r2 : Template
    r2 |> replaceVariable(argName, rhsExpr)
    b1 = apply_template(r1, b1.at, b1)
    b2 = apply_template(r2, b2.at, b2)
    return qmacro(_::less($e(b2), $e(b1))) if (orderName == "order_by_descending")
    return qmacro(_::less($e(b1), $e(b2)))
}


[macro_function]
def private renamed_key_clone(body : Expression?; argName, newName : string) : Expression? {
    return clone_and_rename_var(body, argName, newName)
}

[macro_function, unused_argument(elemType, at)]
def try_make_inline_cmp_keys(orderKey : Expression?; descMask : uint;
                             elemType : TypeDeclPtr; at : LineInfo) : Expression? {
    // nolint:STYLE014 — composite N-key comparator from a `$(x) => (k1, k2, …)` tuple-key block. Per key i
    // (ascending unless bit i of descMask; LSB = first key): emit the tie-break pair
    //   if (_::less(ki[v1], ki[v2])) { return true }; if (_::less(ki[v2], ki[v1])) { return false }
    // with the final key a bare `return _::less(…)`; descending flips the operand order. Bit-order matches
    // the SQL peel + the reader's mask. Returns null when a key body has side-effects or the body isn't a tuple.
    if (orderKey == null || !(orderKey is ExprMakeBlock)) return null
    var mblk = orderKey as ExprMakeBlock
    var blk = mblk._block as ExprBlock
    if (blk.arguments |> length != 1 || blk.list |> length != 1 || !(blk.list[0] is ExprReturn)) return null
    var ret = blk.list[0] as ExprReturn
    var sub = ret.subexpr
    if (sub != null && sub is ExprRef2Value) {
        sub = (sub as ExprRef2Value).subexpr
    }
    if (sub == null || !(sub is ExprMakeTuple)) return null
    var mt = sub as ExprMakeTuple
    let n = mt.values |> length
    if (n < 1 || n > 4) return null
    for (kv in mt.values) {
        if (kv == null || has_sideeffects(kv)) return null
    }
    let argName = string(blk.arguments[0].name)
    var stmts : array<Expression?>
    for (i in range(n)) {
        let desc = (descMask & (1u << uint(i))) != 0u
        if (i == n - 1) {
            var v1e = renamed_key_clone(mt.values[i], argName, "v1")
            var v2e = renamed_key_clone(mt.values[i], argName, "v2")
            let cmp = desc ? qmacro(_::less($e(v2e), $e(v1e))) : qmacro(_::less($e(v1e), $e(v2e)))
            stmts |> push <| qmacro_expr() {
                return $e(cmp)
            }
        } else {
            var a1 = renamed_key_clone(mt.values[i], argName, "v1")
            var a2 = renamed_key_clone(mt.values[i], argName, "v2")
            let cmpLt = desc ? qmacro(_::less($e(a2), $e(a1))) : qmacro(_::less($e(a1), $e(a2)))
            stmts |> push <| qmacro_expr() {
                if ($e(cmpLt)) {
                    return true
                }
            }
            var b1 = renamed_key_clone(mt.values[i], argName, "v1")
            var b2 = renamed_key_clone(mt.values[i], argName, "v2")
            let cmpGt = desc ? qmacro(_::less($e(b1), $e(b2))) : qmacro(_::less($e(b2), $e(b1)))
            stmts |> push <| qmacro_expr() {
                if ($e(cmpGt)) {
                    return false
                }
            }
        }
    }
    var cmp = qmacro($(v1, v2) {})
    var cblk = (cmp as ExprMakeBlock)._block as ExprBlock
    for (s in stmts) {
        cblk.list |> push(s)
    }
    return cmp
}

[macro_function]
def build_order_family_rows {
    splice_patterns |> emplace <| SplicePattern(
        name = "order_streaming_min",
        chain <- [
            Slot(matcher = m_literal("select"),        cardinality = c_opt(), capture_name = "srcsel"),
            Slot(matcher = m_literal("where_"),       cardinality = c_opt(), capture_name = "where"),
            Slot(matcher = m_alias("distinct_family"), cardinality = c_opt(), capture_name = "distinct"),
            Slot(matcher = m_alias("order_family"),    cardinality = c_one(), capture_name = "order"),
            Slot(matcher = m_alias("first_family"),    cardinality = c_one(), capture_name = "term"),
            Slot(matcher = m_literal("select"),        cardinality = c_opt(), capture_name = "termsel")
        ],
        requires <- [@@ < RequiresPredicate > inline_cmp_available],
        emit = @@ < EmitFn > emit_streaming_min
    )
    splice_patterns |> emplace <| SplicePattern(
        name = "order_bounded_heap",
        chain <- [
            Slot(matcher = m_literal("select"),        cardinality = c_opt(), capture_name = "srcsel"),
            Slot(matcher = m_literal("where_"),       cardinality = c_opt(), capture_name = "where"),
            Slot(matcher = m_alias("distinct_family"), cardinality = c_opt(), capture_name = "distinct"),
            Slot(matcher = m_alias("order_family"),    cardinality = c_one(), capture_name = "order"),
            Slot(matcher = m_literal("take"),          cardinality = c_one(), capture_name = "take"),
            Slot(matcher = m_literal("select"),        cardinality = c_opt(), capture_name = "termsel")
        ],
        requires <- [@@ < RequiresPredicate > inline_cmp_available, @@ < RequiresPredicate > take_arg_is_int],
        emit = @@ < EmitFn > emit_bounded_heap
    )
    splice_patterns |> emplace <| SplicePattern(
        name = "order_then_plain_distinct",
        chain <- [
            Slot(matcher = m_literal("select"),     cardinality = c_opt(), capture_name = "srcsel"),
            Slot(matcher = m_alias("order_family"), cardinality = c_one(), capture_name = "order"),
            Slot(matcher = m_literal("distinct"),   cardinality = c_one(), capture_name = "distinct_after"),
            Slot(matcher = m_literal("take"),       cardinality = c_opt(), capture_name = "take"),
            Slot(matcher = m_alias("first_family"), cardinality = c_opt(), capture_name = "term"),
            Slot(matcher = m_literal("select"),     cardinality = c_opt(), capture_name = "termsel")
        ],
        requires <- [@@ < RequiresPredicate > take_arg_is_int],
        emit = @@ < EmitFn > emit_fused_prefilter
    )
    splice_patterns |> emplace <| SplicePattern(
        name = "order_fused_prefilter",
        chain <- [
            Slot(matcher = m_literal("select"),        cardinality = c_opt(), capture_name = "srcsel"),
            Slot(matcher = m_literal("where_"),       cardinality = c_opt(), capture_name = "where"),
            Slot(matcher = m_alias("distinct_family"), cardinality = c_opt(), capture_name = "distinct"),
            Slot(matcher = m_alias("order_family"),    cardinality = c_one(), capture_name = "order"),
            Slot(matcher = m_literal("take"),          cardinality = c_opt(), capture_name = "take"),
            Slot(matcher = m_alias("first_family"),    cardinality = c_opt(), capture_name = "term"),
            Slot(matcher = m_literal("select"),        cardinality = c_opt(), capture_name = "termsel")
        ],
        requires <- [@@ < RequiresPredicate > has_where_or_distinct, @@ < RequiresPredicate > take_arg_is_int],
        emit = @@ < EmitFn > emit_fused_prefilter
    )
    splice_patterns |> emplace <| SplicePattern(
        name = "order_buffer_helper_dispatch",
        chain <- [
            Slot(matcher = m_alias("order_family"), cardinality = c_one(), capture_name = "order"),
            Slot(matcher = m_literal("take"),       cardinality = c_opt(), capture_name = "take"),
            Slot(matcher = m_alias("first_family"), cardinality = c_opt(), capture_name = "term")
        ],
        requires <- [@@ < RequiresPredicate > take_arg_is_int, @@ < RequiresPredicate > array_source],
        emit = @@ < EmitFn > emit_buffer_helper_dispatch
    )
}

struct OrderCaptures {
    whereCond           : Expression?
    distinctName        : string
    distinctKey         : Expression?
    orderName           : string
    orderKey            : Expression?
    orderElemType       : TypeDeclPtr
    takeExpr            : Expression?
    firstName           : string
    firstDefaultExpr    : Expression?
    selectLam           : Expression?
    selectElemType      : TypeDeclPtr
    hasKey              : bool
    isMultiKey          : bool
    orderDescMask       : uint
    ok                  : bool
}

[macro_function, unused_argument(at)]
def extract_order_captures(var c : Captures; at : LineInfo; itName : string) : OrderCaptures {
    var oc : OrderCaptures
    oc.ok = false
    if (!(c.single |> key_exists("order"))) return oc
    let orderCall = c.single["order"]
    oc.orderName = c.single_name["order"]
    let argCount = orderCall.arguments |> length
    if ((oc.orderName == "order" || oc.orderName == "order_descending") && argCount >= 2) return oc
    oc.isMultiKey = oc.orderName == "order_by_keys"
    oc.hasKey = oc.orderName == "order_by" || oc.orderName == "order_by_descending" || oc.isMultiKey
    if (oc.hasKey && argCount >= 2) {
        oc.orderKey = clone_expression(orderCall.arguments[1])
    }
    if (oc.isMultiKey) {
        if (argCount < 3) return oc
        var maskArg = orderCall.arguments[2]
        if (maskArg != null && maskArg is ExprRef2Value) {
            maskArg = (maskArg as ExprRef2Value).subexpr
        }
        if (maskArg == null || !(maskArg is ExprConstUInt)) return oc
        oc.orderDescMask = (maskArg as ExprConstUInt).value
    }
    if (orderCall._type == null || orderCall._type.firstType == null) return oc
    oc.orderElemType = clone_type(orderCall._type.firstType)
    if (c.single |> key_exists("where")) {
        oc.whereCond = peel_lambda_rename_var(c.single["where"].arguments[1], itName)
    }
    if (c.single |> key_exists("distinct")) {
        let distinctCall = c.single["distinct"]
        oc.distinctName = c.single_name["distinct"]
        if (oc.distinctName == "distinct_by") {
            if ((distinctCall.arguments |> length) < 2) return oc
            oc.distinctKey = clone_expression(distinctCall.arguments[1])
        }
    } elif (c.single |> key_exists("distinct_after")) {
        oc.distinctName = "distinct"
    }
    if (c.single |> key_exists("take")) {
        let takeArg = c.single["take"].arguments[1]
        if (takeArg == null || takeArg._type == null || takeArg._type.baseType != Type.tInt) return oc
        oc.takeExpr = clone_expression(takeArg)
    }
    if (c.single |> key_exists("term")) {
        let termCall = c.single["term"]
        oc.firstName = c.single_name["term"]
        if (oc.firstName == "first_or_default") {
            if ((termCall.arguments |> length) < 2) return oc
            oc.firstDefaultExpr = clone_expression(termCall.arguments[1])
        }
    }
    if (c.single |> key_exists("termsel")) {
        var termselCall = c.single["termsel"]
        oc.selectLam = termselCall.arguments[1]
        if (oc.selectLam == null || termselCall._type == null || termselCall._type.firstType == null) return oc
        oc.selectElemType = clone_type(termselCall._type.firstType)
    }
    oc.ok = true
    return oc
}

[macro_function]
def emit_streaming_min(var c : Captures; var ctx : EmitCtx; at : LineInfo) : Expression? {
    // nolint:STYLE014 — `order_by(K) |> first[_or_default]()` collapses to a single O(n) min/max scan — no sort,
    // no buffer. Tracks the best element so far; the tail returns it. Generated (via emit_terminator_lane):
    //   invoke((src…) {
    //       var best = default<Elem>; var seen = false       // first_or_default also binds `d` once
    //       for (it in src) {                                // optional upstream where applied by the lane
    //           if (!seen) { best := it; seen = true } elif (<it beats best by key>) best := it
    //       }
    //       if (!seen) panic("…")                            // first; first_or_default → `return d`
    //       return <select(best) ?? best>
    //   }, src…)
    // XML defers: track (bestKey, bestNode) and build the survivor row once at the tail (skips materializing all N).
    let bindName = ctx.src->bind_name(at)
    var oc <- extract_order_captures(c, at, bindName)
    if (!oc.ok || oc.firstName == "" || oc.distinctName != "") return null
    var inlineCmp = oc.hasKey ? try_make_inline_cmp(oc.orderKey, oc.orderName, oc.orderElemType, at) : null
    if (inlineCmp == null) return null
    let bestName = qn("order_best", at)
    let seenName = qn("order_seen", at)
    var elemType = clone_type(oc.orderElemType)

    if (ctx.src->defers_materialization() && oc.hasKey) {
        var keyType = (oc.orderKey != null && oc.orderKey._type != null) ? strip_const_ref(clone_type(oc.orderKey._type.firstType)) : null
        var handleType = ctx.src->handle_type()
        var keyBody = peel_lambda_rename_var(oc.orderKey, bindName)
        if (keyType != null && handleType != null && keyBody != null) {
            let bestKeyName = qn("order_best_key", at)
            let bestNodeName = qn("order_best_node", at)
            let xkeyName = qn("order_xkey", at)
            let matItName = qn("order_mat_it", at)
            var dLess = (oc.orderName == "order_by_descending"
                ? qmacro(_::less($i(bestKeyName), $i(xkeyName)))
                : qmacro(_::less($i(xkeyName), $i(bestKeyName))))
            var dPerElement <- qmacro_block_to_array() {
                let $i(xkeyName) = $e(keyBody)
                if (!$i(seenName)) {
                    $i(bestKeyName) = $i(xkeyName)
                    $i(bestNodeName) = $e(ctx.src->current_handle_expr(at))
                    $i(seenName) = true
                } elif ($e(dLess)) {
                    $i(bestKeyName) = $i(xkeyName)
                    $i(bestNodeName) = $e(ctx.src->current_handle_expr(at))
                }
            }
            var dOutElemType = (oc.selectLam != null) ? oc.selectElemType : clone_type(oc.orderElemType)
            var matStmt = ctx.src->materialize_handle(qmacro($i(bestNodeName)), matItName, at)
            var dRet = (oc.selectLam != null
                ? peel_lambda_replace_var(oc.selectLam, qmacro($i(matItName)))
                : qmacro($i(matItName)))
            var dPrologue : array<Expression?>
            var dTail : array<Expression?>
            if (oc.firstName == "first") {
                dPrologue |> push_from <| qmacro_block_to_array() {
                    var $i(bestKeyName) = default<$t(keyType)>
                    var $i(bestNodeName) = default<$t(handleType)>
                    var $i(seenName) = false
                }
                dTail |> push <| qmacro_expr() {
                    panic("sequence contains no elements") if (!$i(seenName))
                }
                dTail |> push <| qmacro_expr() {
                    var $i(matItName) = default<$t(oc.orderElemType)>
                }
                dTail |> push(matStmt)
                dTail |> push <| qmacro_expr() {
                    return $e(dRet)
                }
            } else {
                let dBindName = qn("order_d", at)
                var dDefRet = (oc.selectLam != null
                    ? peel_lambda_replace_var(oc.selectLam, qmacro($i(dBindName)))
                    : qmacro($i(dBindName)))
                dPrologue |> push_from <| qmacro_block_to_array() {
                    let $i(dBindName) = $e(oc.firstDefaultExpr)
                    var $i(bestKeyName) = default<$t(keyType)>
                    var $i(bestNodeName) = default<$t(handleType)>
                    var $i(seenName) = false
                }
                dTail |> push <| qmacro_expr() {
                    return $e(dDefRet) if (!$i(seenName))
                }
                dTail |> push <| qmacro_expr() {
                    var $i(matItName) = default<$t(oc.orderElemType)>
                }
                dTail |> push(matStmt)
                dTail |> push <| qmacro_expr() {
                    return $e(dRet)
                }
            }
            return emit_terminator_lane(TerminatorSpec(
                prologue <- dPrologue,
                whereCond = oc.whereCond,
                perElementStmts <- dPerElement,
                tailStmts <- dTail,
                outElemType = dOutElemType
            ), ctx.src, at)
        }
    }
    var lessTest = make_inline_less_call(oc.orderKey, oc.orderName,
        qmacro($i(bindName)), qmacro($i(bestName)), at)
    var perElementStmts : array<Expression?>
    perElementStmts |> push <| qmacro_expr() {
        if (!$i(seenName)) {
            $i(bestName) := $i(bindName)
            $i(seenName) = true
        } elif ($e(lessTest)) {
            $i(bestName) := $i(bindName)
        }
    }
    var outElemType = (oc.selectLam != null) ? oc.selectElemType : elemType
    var prologue : array<Expression?>
    var tailStmts : array<Expression?>
    if (oc.firstName == "first") {
        var firstRetExpr : Expression?
        if (oc.selectLam != null) {
            firstRetExpr = peel_lambda_replace_var(oc.selectLam, qmacro($i(bestName)))
        } else {
            firstRetExpr = qmacro($i(bestName))
        }
        prologue |> push_from <| qmacro_block_to_array() {
            var $i(bestName) = default<$t(elemType)>
            var $i(seenName) = false
        }
        tailStmts |> push_from <| qmacro_block_to_array() {
            panic("sequence contains no elements") if (!$i(seenName))
            return $e(firstRetExpr)
        }
    } else {
        let dBindName = qn("order_d", at)
        var bestRetExpr : Expression?
        var dRetExpr : Expression?
        if (oc.selectLam != null) {
            bestRetExpr = peel_lambda_replace_var(oc.selectLam, qmacro($i(bestName)))
            dRetExpr = peel_lambda_replace_var(oc.selectLam, qmacro($i(dBindName)))
        } else {
            bestRetExpr = qmacro($i(bestName))
            dRetExpr = qmacro($i(dBindName))
        }
        prologue |> push_from <| qmacro_block_to_array() {
            let $i(dBindName) = $e(oc.firstDefaultExpr)
            var $i(bestName) = default<$t(elemType)>
            var $i(seenName) = false
        }
        tailStmts |> push_from <| qmacro_block_to_array() {
            return $e(bestRetExpr) if ($i(seenName))
            return $e(dRetExpr)
        }
    }
    return emit_terminator_lane(TerminatorSpec(
        prologue <- prologue,
        whereCond = oc.whereCond,
        perElementStmts <- perElementStmts,
        tailStmts <- tailStmts,
        outElemType = outElemType
    ), ctx.src, at)
}

[macro_function]
def private build_surrogate_type(keyType : TypeDeclPtr; handleType : TypeDeclPtr; at : LineInfo) : TypeDeclPtr {
    var t = new TypeDecl(baseType = Type.tTuple, at = at)
    t.argTypes |> push(clone_type(keyType))
    t.argTypes |> push(clone_type(handleType))
    return t
}

[macro_function, unused_argument(at)]
def private build_surrogate_cmp(descending : bool; at : LineInfo) : Expression? {
    if (descending)
        return qmacro($(v1, v2) { return _::less(v2._0, v1._0) })
    return qmacro($(v1, v2) { return _::less(v1._0, v2._0) })
}

[macro_function]
def private build_surrogate_materialize_loop(var ctx : EmitCtx; bufName, outBufName : string;
                                             rowType : TypeDeclPtr; var selectLam : Expression?;
                                             handleIsTuple : bool; at : LineInfo) : array<Expression?> {
    let surrName = qn("order_surr", at)
    let matItName = qn("order_mat_it", at)
    var pushItem = (selectLam != null
        ? peel_lambda_replace_var(selectLam, qmacro($i(matItName)))
        : qmacro($i(matItName)))
    var matHandle = (handleIsTuple ? qmacro($i(surrName)._1) : qmacro($i(surrName)))
    var matStmt = ctx.src->materialize_handle(matHandle, matItName, at)
    var stmts <- qmacro_block_to_array() {
        $i(outBufName) |> reserve(length($i(bufName)))
        for ($i(surrName) in $i(bufName)) {
            var $i(matItName) = default<$t(rowType)>
            $e(matStmt)
            $i(outBufName) |> push_clone($e(pushItem))
        }
        return <- $i(outBufName)
    }
    return <- stmts
}

// nolint:STYLE014 — `order_by(K) |> take(N)` via a bounded size-N heap kept during the walk (no full sort).
// Rides emit_fold_array_lane with the heap buffer; distinct gate + terminal _select optional. Per-element:
//   if (length(buf) < N) { buf |> push_clone(it); spliced_push_heap(buf, cmp) }
//   elif (<it beats buf[0]>) { spliced_pop_heap(buf, cmp); buf[last] := it; spliced_push_heap(buf, cmp) }
// Tail: order_inplace(buf, cmp); return <- buf  (or re-project for a terminal _select).
// XML defers: heap holds (orderKey, handle) surrogates; build the K survivor rows once at the tail.
[macro_function]
def emit_bounded_heap(var c : Captures; var ctx : EmitCtx; at : LineInfo) : Expression? {
    let bindName = ctx.src->bind_name(at)
    var oc <- extract_order_captures(c, at, bindName)
    if (!oc.ok || oc.takeExpr == null || oc.firstName != "") return null
    var inlineCmp = oc.hasKey ? try_make_inline_cmp(oc.orderKey, oc.orderName, oc.orderElemType, at) : null
    if (inlineCmp == null) return null
    let takeNName = qn("order_take_n", at)
    let bhBufName = qn("order_buf", at)
    let dsetName = qn("order_dset", at)
    let dkeyName = qn("order_dkey", at)
    var rowType = clone_type(oc.orderElemType)

    var deferred = ctx.src->defers_materialization() && oc.hasKey
    var keyType : TypeDeclPtr
    var handleType : TypeDeclPtr
    var keyBody : Expression?
    if (deferred) {
        keyType = (oc.orderKey != null && oc.orderKey._type != null) ? strip_const_ref(clone_type(oc.orderKey._type.firstType)) : null
        handleType = ctx.src->handle_type()
        keyBody = peel_lambda_rename_var(oc.orderKey, bindName)
        if (keyType == null || handleType == null || keyBody == null) {
            deferred = false
        }
    }

    var laneBufType : TypeDeclPtr
    var perElementPush : Expression?
    var postBufDeclStmts : array<Expression?>
    var tailStmts : array<Expression?>

    if (deferred) {
        laneBufType = build_surrogate_type(keyType, handleType, at)
        let xkeyName = qn("order_xkey", at)
        let outBufName = qn("order_out_buf", at)
        var surrCmp = build_surrogate_cmp(oc.orderName == "order_by_descending", at)
        var admitTest = (oc.orderName == "order_by_descending"
            ? qmacro(_::less($i(bhBufName)[0]._0, $i(xkeyName)))
            : qmacro(_::less($i(xkeyName), $i(bhBufName)[0]._0)))
        perElementPush = qmacro_block() {
            let $i(xkeyName) = $e(keyBody)
            if (length($i(bhBufName)) < $i(takeNName)) {
                $i(bhBufName) |> push(($i(xkeyName) => $e(ctx.src->current_handle_expr(at))))
                _::spliced_push_heap($i(bhBufName), $e(surrCmp))
            } elif ($e(admitTest)) {
                _::spliced_pop_heap($i(bhBufName), $e(surrCmp))
                $i(bhBufName)[length($i(bhBufName)) - 1] = ($i(xkeyName) => $e(ctx.src->current_handle_expr(at)))
                _::spliced_push_heap($i(bhBufName), $e(surrCmp))
            }
        }
        let outElemT = oc.selectLam != null ? clone_type(oc.selectElemType) : clone_type(rowType)
        postBufDeclStmts |> push_from <| qmacro_block_to_array() {
            var $i(outBufName) : array<$t(outElemT)>
            return <- $i(outBufName) if ($i(takeNName) <= 0)
        }
        tailStmts |> push <| qmacro_expr() {
            _::order_inplace($i(bhBufName), $e(surrCmp))
        }
        tailStmts |> push_from(build_surrogate_materialize_loop(ctx, bhBufName, outBufName, rowType, oc.selectLam, true, at))
    } else {
        laneBufType = clone_type(rowType)
        var lessTest = make_inline_less_call(oc.orderKey, oc.orderName,
            qmacro($i(bindName)), qmacro($i(bhBufName)[0]), at)
        perElementPush = qmacro_expr() {
            if (length($i(bhBufName)) < $i(takeNName)) {
                $i(bhBufName) |> push_clone($i(bindName))
                _::spliced_push_heap($i(bhBufName), $e(inlineCmp))
            } elif ($e(lessTest)) {
                _::spliced_pop_heap($i(bhBufName), $e(inlineCmp))
                $i(bhBufName)[length($i(bhBufName)) - 1] := $i(bindName)
                _::spliced_push_heap($i(bhBufName), $e(inlineCmp))
            }
        }
        tailStmts |> push <| qmacro_expr() {
            _::order_inplace($i(bhBufName), $e(inlineCmp))
        }
        if (oc.selectLam != null) {
            let outBufName = qn("order_proj_buf", at)
            let elemName = qn("order_proj_e", at)
            var projBody = peel_lambda_replace_var(oc.selectLam, qmacro($i(elemName)))
            postBufDeclStmts |> push_from <| qmacro_block_to_array() {
                var $i(outBufName) : array<$t(oc.selectElemType)>
                return <- $i(outBufName) if ($i(takeNName) <= 0)
            }
            tailStmts |> push_from <| qmacro_block_to_array() {
                $i(outBufName) |> reserve(length($i(bhBufName)))
                for ($i(elemName) in $i(bhBufName)) {
                    $i(outBufName) |> push_clone($e(projBody))
                }
                return <- $i(outBufName)
            }
        } else {
            postBufDeclStmts |> push <| qmacro_expr() {
                return <- $i(bhBufName) if ($i(takeNName) <= 0)
            }
            tailStmts |> push <| qmacro_expr() {
                return <- $i(bhBufName)
            }
        }
    }

    var distinctGate : DistinctGateSpec?
    if (oc.distinctName != "") {
        var dgKind : DistinctGateKind = (oc.distinctName == "distinct_by"
            ? DistinctGateKind(KeyLam = oc.distinctKey)
            : DistinctGateKind(NonKey = null))
        distinctGate = new DistinctGateSpec(
            kind = dgKind, bindName = bindName, bufElemType = clone_type(rowType),
            dsetName = dsetName, dkeyName = dkeyName
        )
    }
    var prologueStmts : array<Expression?>
    prologueStmts |> push <| qmacro_expr() {
        let $i(takeNName) = $e(oc.takeExpr)
    }
    return emit_fold_array_lane(FoldArraySpec(
        prologueStmts <- prologueStmts,
        bufElemType = laneBufType,
        bufName = bhBufName,
        postBufDeclStmts <- postBufDeclStmts,
        whereCond = oc.whereCond,
        distinctGate = distinctGate,
        perElementPush = perElementPush,
        tailStmts <- tailStmts,
        wrapIter = ctx.expr_is_iterator
    ), ctx.src, at)
}

// nolint:STYLE014 — no where / distinct / terminal _select: emit a direct call to the daslib order helper.
//   bare order               → order(src) / order_by(src, key)
//   order + take(N)          → top_n[_by](src, N[, key])
//   order + first            → array: panic-if-empty + min/max(src[, key]); iterator: top_n(_,1) |> first()
//   order + first_or_default → top_n(_,1) |> first_or_default(d)
// Iterator results wrap in .to_sequence_move() when the emission is array-shaped.
[macro_function]
def emit_buffer_helper_dispatch(var c : Captures; var ctx : EmitCtx; at : LineInfo) : Expression? {
    let itName = qn("it", at)
    var oc <- extract_order_captures(c, at, itName)
    if (!oc.ok || oc.whereCond != null || oc.distinctName != "" || oc.selectLam != null
            || (oc.isMultiKey && (oc.takeExpr != null || oc.firstName != ""))) return null
    var top = ctx.top
    var topExpr = clone_expression(top)
    topExpr.genFlags.alwaysSafe = true
    var inlineCmp = (oc.hasKey && !oc.isMultiKey) ? try_make_inline_cmp(oc.orderKey, oc.orderName, oc.orderElemType, at) : null
    let topNName = order_top_n_call_name(oc.orderName)
    let minMaxName = order_min_call_name(oc.orderName, oc.hasKey)
    let needIterWrap = ctx.expr_is_iterator
    var emission : Expression?
    var firstCount : Expression?
    if (oc.firstName != "") {
        firstCount = oc.takeExpr != null ? qmacro(($e(oc.takeExpr) > 0 ? 1 : 0)) : qmacro(1)
    }
    if (oc.firstName == "first") {
        if (top._type.isGoodArrayType) {
            var srcParamType = invoke_src_param_type(top)
            let firstSrcName = qn("first_src", at)
            var minMaxCall : Expression?
            if (oc.hasKey) {
                minMaxCall = qmacro($c(minMaxName)($i(firstSrcName), $e(oc.orderKey)))
            } else {
                minMaxCall = qmacro($c(minMaxName)($i(firstSrcName)))
            }
            if (oc.takeExpr != null) {
                emission = qmacro(invoke($($i(firstSrcName) : $t(srcParamType)) {
                    panic("sequence contains no elements") if ($e(oc.takeExpr) <= 0 || empty($i(firstSrcName)))
                    return $e(minMaxCall)
                }, $e(topExpr)))
            } else {
                emission = qmacro(invoke($($i(firstSrcName) : $t(srcParamType)) {
                    panic("sequence contains no elements") if (empty($i(firstSrcName)))
                    return $e(minMaxCall)
                }, $e(topExpr)))
            }
            emission = finalize_invoke(emission, at)
        } else {
            var topNCall : Expression?
            if (inlineCmp != null) {
                topNCall = qmacro(_::top_n_by_with_cmp($e(topExpr), $e(firstCount), $e(inlineCmp)))
            } elif (oc.hasKey) {
                topNCall = qmacro($c(topNName)($e(topExpr), $e(firstCount), $e(oc.orderKey)))
            } else {
                topNCall = qmacro($c(topNName)($e(topExpr), $e(firstCount)))
            }
            emission = qmacro(_::first($e(topNCall)))
        }
    } elif (oc.firstName == "first_or_default") {
        var topNCall : Expression?
        if (inlineCmp != null) {
            topNCall = qmacro(_::top_n_by_with_cmp($e(topExpr), $e(firstCount), $e(inlineCmp)))
        } elif (oc.hasKey) {
            topNCall = qmacro($c(topNName)($e(topExpr), $e(firstCount), $e(oc.orderKey)))
        } else {
            topNCall = qmacro($c(topNName)($e(topExpr), $e(firstCount)))
        }
        emission = qmacro(_::first_or_default($e(topNCall), $e(oc.firstDefaultExpr)))
    } elif (oc.takeExpr == null) {
        if (oc.isMultiKey) {
            emission = qmacro($c(oc.orderName)($e(topExpr), $e(oc.orderKey), $v(oc.orderDescMask)))
        } elif (inlineCmp != null) {
            emission = qmacro(_::order($e(topExpr), $e(inlineCmp)))
        } elif (oc.hasKey) {
            emission = qmacro($c(oc.orderName)($e(topExpr), $e(oc.orderKey)))
        } else {
            emission = qmacro($c(oc.orderName)($e(topExpr)))
        }
    } else {
        if (inlineCmp != null) {
            emission = qmacro(_::top_n_by_with_cmp($e(topExpr), $e(oc.takeExpr), $e(inlineCmp)))
        } elif (oc.hasKey) {
            emission = qmacro($c(topNName)($e(topExpr), $e(oc.takeExpr), $e(oc.orderKey)))
        } else {
            emission = qmacro($c(topNName)($e(topExpr), $e(oc.takeExpr)))
        }
    }
    let emissionIsArray = oc.takeExpr != null || (oc.firstName == "" && top._type.isGoodArrayType)
    if (needIterWrap && emissionIsArray) {
        emission = qmacro($e(emission).to_sequence_move())
    }
    emission.force_generated(true)
    return emission
}

// nolint:STYLE014 — where+order or distinct+order: one fused loop filters (and distinct-gates) survivors into a
// fresh buffer, then the tail sorts / min / top_n's that buffer. Rides emit_fold_array_lane:
//   for (it in src) { if (where) { <distinct gate> buf |> push_clone(it) } }     // perElementPush
//   tail: bare order → order_inplace(buf); +take → top_n[_by](buf, N);
//         +first → min/max(buf) (panic if empty); +first_or_default → top_n(buf,1) |> first_or_default(d)
// XML defers (distinct, no-take, no-first): buffer (orderKey, handle) surrogates, build survivor rows at the tail.
[macro_function]
def emit_fused_prefilter(var c : Captures; var ctx : EmitCtx; at : LineInfo) : Expression? {
    let bindName = ctx.src->bind_name(at)
    var oc <- extract_order_captures(c, at, bindName)
    if (!oc.ok
            || (oc.whereCond == null && oc.distinctName == "")
            || (oc.distinctName != "" && oc.takeExpr == null && oc.firstName != "")
            || (oc.isMultiKey && (oc.takeExpr != null || oc.firstName != "" || oc.distinctName != ""))) return null
    let bufName = qn("buf", at)
    let dsetName = qn("order_dset", at)
    let dkeyName = qn("order_dkey", at)
    var bufElemType = clone_type(oc.orderElemType)
    var inlineCmp = (oc.isMultiKey
        ? try_make_inline_cmp_keys(oc.orderKey, oc.orderDescMask, oc.orderElemType, at)
        : (oc.hasKey ? try_make_inline_cmp(oc.orderKey, oc.orderName, oc.orderElemType, at) : null))
    let topNName = order_top_n_call_name(oc.orderName)
    let inplaceName = "{oc.orderName}_inplace"
    let minMaxName = order_min_call_name(oc.orderName, oc.hasKey)
    var perElementPush : Expression? = qmacro_expr() {
        $i(bufName) |> push_clone($i(bindName))
    }
    var distinctGate : DistinctGateSpec?
    if (oc.distinctName != "") {
        var dgKind : DistinctGateKind = (oc.distinctName == "distinct_by"
            ? DistinctGateKind(KeyLam = oc.distinctKey)
            : DistinctGateKind(NonKey = null))
        distinctGate = new DistinctGateSpec(
            kind = dgKind, bindName = bindName, bufElemType = bufElemType,
            dsetName = dsetName, dkeyName = dkeyName
        )
    }
    var reserveStmts : array<Expression?>
    if (ctx.src->can_reserve_by_length() && oc.distinctName == "") {
        let top = ctx.src->loop_source_expr()
        let srcName = ctx.src->loop_source_name()
        if (top != null && top._type != null && type_has_length(top._type)) {
            reserveStmts |> push <| qmacro_expr() {
                $i(bufName) |> reserve(length($i(srcName)))
            }
        }
    }
    if (ctx.src->defers_materialization() && oc.hasKey && oc.takeExpr == null && oc.firstName == "" && oc.distinctName != "") {
        var keyType = (oc.orderKey != null && oc.orderKey._type != null) ? strip_const_ref(clone_type(oc.orderKey._type.firstType)) : null
        var handleType = ctx.src->handle_type()
        var keyBody = peel_lambda_rename_var(oc.orderKey, bindName)
        if (keyType != null && handleType != null && keyBody != null) {
            let dOutBufName = qn("order_out_buf", at)
            let dOutElemT = oc.selectLam != null ? clone_type(oc.selectElemType) : clone_type(oc.orderElemType)
            var dSurrCmp = build_surrogate_cmp(oc.orderName == "order_by_descending", at)
            var dPush = qmacro_expr() {
                $i(bufName) |> push(($e(keyBody) => $e(ctx.src->current_handle_expr(at))))
            }
            var dTail <- qmacro_block_to_array() {
                _::order_inplace($i(bufName), $e(dSurrCmp))
                var $i(dOutBufName) : array<$t(dOutElemT)>
            }
            dTail |> push_from(build_surrogate_materialize_loop(ctx, bufName, dOutBufName, oc.orderElemType, oc.selectLam, true, at))
            return emit_fold_array_lane(FoldArraySpec(
                bufElemType = build_surrogate_type(keyType, handleType, at),
                bufName = bufName,
                whereCond = oc.whereCond,
                distinctGate = distinctGate,
                perElementPush = dPush,
                tailStmts <- dTail,
                wrapIter = ctx.expr_is_iterator
            ), ctx.src, at)
        }
    }
    let elemName = qn("order_proj_e", at)
    let outBufName = qn("order_proj_buf", at)
    var projBody : Expression?
    if (oc.selectLam != null) {
        projBody = peel_lambda_replace_var(oc.selectLam, qmacro($i(elemName)))
    }
    var tailStmts : array<Expression?>
    var firstCount : Expression?
    if (oc.firstName != "") {
        firstCount = oc.takeExpr != null ? qmacro(($e(oc.takeExpr) > 0 ? 1 : 0)) : qmacro(1)
    }
    if (oc.firstName == "first") {
        if (oc.takeExpr != null) {
            tailStmts |> push <| qmacro_expr() {
                panic("sequence contains no elements") if ($e(oc.takeExpr) <= 0 || empty($i(bufName)))
            }
        } else {
            tailStmts |> push <| qmacro_expr() {
                panic("sequence contains no elements") if (empty($i(bufName)))
            }
        }
        var minMaxCall : Expression?
        if (oc.hasKey) {
            minMaxCall = qmacro($c(minMaxName)($i(bufName), $e(oc.orderKey)))
        } else {
            minMaxCall = qmacro($c(minMaxName)($i(bufName)))
        }
        if (oc.selectLam != null) {
            tailStmts |> push_from <| qmacro_block_to_array() {
                let $i(elemName) = $e(minMaxCall)
                return $e(projBody)
            }
        } else {
            tailStmts |> push <| qmacro_expr() {
                return $e(minMaxCall)
            }
        }
    } elif (oc.firstName == "first_or_default") {
        var topNCall : Expression?
        if (inlineCmp != null) {
            topNCall = qmacro(_::top_n_by_with_cmp($i(bufName), $e(firstCount), $e(inlineCmp)))
        } elif (oc.hasKey) {
            topNCall = qmacro($c(topNName)($i(bufName), $e(firstCount), $e(oc.orderKey)))
        } else {
            topNCall = qmacro($c(topNName)($i(bufName), $e(firstCount)))
        }
        if (oc.selectLam != null) {
            let dBindName = qn("order_d", at)
            tailStmts |> push_from <| qmacro_block_to_array() {
                let $i(dBindName) = $e(oc.firstDefaultExpr)
                let $i(elemName) = _::first_or_default($e(topNCall), $i(dBindName))
                return $e(projBody)
            }
        } else {
            tailStmts |> push <| qmacro_expr() {
                return _::first_or_default($e(topNCall), $e(oc.firstDefaultExpr))
            }
        }
    } elif (oc.takeExpr == null) {
        var sortCall : Expression?
        if (oc.isMultiKey) {
            if (inlineCmp != null) {
                sortCall = qmacro(stable_sort($i(bufName), $e(inlineCmp)))
            } else {
                sortCall = qmacro($c(inplaceName)($i(bufName), $e(oc.orderKey), $v(oc.orderDescMask)))
            }
        } elif (inlineCmp != null) {
            sortCall = qmacro(_::order_inplace($i(bufName), $e(inlineCmp)))
        } elif (oc.hasKey) {
            sortCall = qmacro($c(inplaceName)($i(bufName), $e(oc.orderKey)))
        } else {
            sortCall = qmacro($c(inplaceName)($i(bufName)))
        }
        tailStmts |> push(sortCall)
        if (oc.selectLam != null) {
            tailStmts |> push_from <| qmacro_block_to_array() {
                var $i(outBufName) : array<$t(oc.selectElemType)>
                $i(outBufName) |> reserve(length($i(bufName)))
                for ($i(elemName) in $i(bufName)) {
                    $i(outBufName) |> push_clone($e(projBody))
                }
                return <- $i(outBufName)
            }
        } else {
            tailStmts |> push <| qmacro_expr() {
                return <- $i(bufName)
            }
        }
    } else {
        var topNCall : Expression?
        if (inlineCmp != null) {
            topNCall = qmacro(_::top_n_by_with_cmp($i(bufName), $e(oc.takeExpr), $e(inlineCmp)))
        } elif (oc.hasKey) {
            topNCall = qmacro($c(topNName)($i(bufName), $e(oc.takeExpr), $e(oc.orderKey)))
        } else {
            topNCall = qmacro($c(topNName)($i(bufName), $e(oc.takeExpr)))
        }
        if (oc.selectLam != null) {
            let topResName = qn("order_top_res", at)
            tailStmts |> push_from <| qmacro_block_to_array() {
                var $i(topResName) <- $e(topNCall)
                var $i(outBufName) : array<$t(oc.selectElemType)>
                $i(outBufName) |> reserve(length($i(topResName)))
                for ($i(elemName) in $i(topResName)) {
                    $i(outBufName) |> push_clone($e(projBody))
                }
                return <- $i(outBufName)
            }
        } else {
            tailStmts |> push <| qmacro_expr() {
                return <- $e(topNCall)
            }
        }
    }
    return emit_fold_array_lane(FoldArraySpec(
        bufElemType = bufElemType,
        bufName = bufName,
        reserveStmts <- reserveStmts,
        whereCond = oc.whereCond,
        distinctGate = distinctGate,
        perElementPush = perElementPush,
        tailStmts <- tailStmts,
        wrapIter = ctx.expr_is_iterator
    ), ctx.src, at)
}

[macro_function]
def emit_reverse_counter(var c : Captures; var ctx : EmitCtx; at : LineInfo) : Expression? {
    // nolint:STYLE014 — `reverse() |> [where] |> [select] |> count()`: reversing doesn't change the count, so just
    // count survivors forward — no buffer, no reverse. Generated via emit_terminator_lane:
    //   var cnt = 0
    //   for (it in src) { [let vfinal = projection;]  cnt ++ }   // vfinal bind only if the projection has side-effects
    //   return cnt
    let bindName = ctx.src->bind_name(at)
    let cntName = qn("cnt", at)
    var whereCond : Expression?
    if (c.single |> key_exists("where")) {
        whereCond = peel_lambda_rename_var(c.single["where"].arguments[1], bindName)
    }
    var projection : Expression?
    if (c.single |> key_exists("proj")) {
        projection = peel_lambda_rename_var(c.single["proj"].arguments[1], bindName)
    }
    var perElementStmts : array<Expression?>
    if (projection != null && has_sideeffects(projection)) {
        let vfinalName = qn("vfinal", at)
        perElementStmts |> push_from <| qmacro_block_to_array() {
            var $i(vfinalName) = $e(projection)
            $i(cntName) ++
        }
    } else {
        perElementStmts |> push <| qmacro_expr() {
            $i(cntName) ++
        }
    }
    var prologue <- qmacro_block_to_array() {
        var $i(cntName) = 0
    }
    var tailStmts <- qmacro_block_to_array() {
        return $i(cntName)
    }
    return emit_terminator_lane(TerminatorSpec(
        prologue <- prologue,
        whereCond = whereCond,
        perElementStmts <- perElementStmts,
        tailStmts <- tailStmts
    ), ctx.src, at)
}

// nolint:STYLE014 — Rb: `reverse() |> first[_or_default]()` = the LAST surviving element. Walk forward, overwrite
// `last` on every match; no buffer. Generated via emit_terminator_lane:
//   var found = false; var last : Elem = default<Elem>           // first_or_default also binds `d` once
//   for (it in src) { last := <projection ?? it>; found = true }
//   first            → if (!found) panic("…"); return <select(last) ?? last>
//   first_or_default → return found ? <select(last) ?? last> : d
[macro_function]
def emit_reverse_walk_overwrite_scalar(var c : Captures; var ctx : EmitCtx; at : LineInfo) : Expression? {
    let bindName = ctx.src->bind_name(at)
    let foundName = qn("found", at)
    let lastName = qn("last", at)
    let dBindName = qn("d", at)
    var whereCond : Expression?
    if (c.single |> key_exists("where")) {
        whereCond = peel_lambda_rename_var(c.single["where"].arguments[1], bindName)
    }
    var projection : Expression?
    if (c.single |> key_exists("proj")) {
        projection = peel_lambda_rename_var(c.single["proj"].arguments[1], bindName)
    }
    var terminalSelectLam : Expression?
    var terminalSelectElemType : TypeDeclPtr
    if (c.single |> key_exists("termsel")) {
        terminalSelectLam = c.single["termsel"].arguments[1]
        if (terminalSelectLam == null || c.single["termsel"]._type == null || c.single["termsel"]._type.firstType == null) return null
        terminalSelectElemType = clone_type(c.single["termsel"]._type.firstType)
    }
    if (!(c.single |> key_exists("term"))) return null
    let terminatorCall = c.single["term"]
    let terminatorName = call_norm_name(terminatorCall)
    var lastType : TypeDeclPtr
    if (projection != null) {
        if (projection._type == null) return null
        lastType = strip_const_ref(clone_type(projection._type))
    } else {
        var srcElemType = ctx.src->element_type()
        if (srcElemType == null) return null
        lastType = strip_const_ref(srcElemType)
    }
    var outElemType = (terminalSelectLam != null) ? terminalSelectElemType : lastType
    if (whereCond == null) {
        var rowT = ctx.src->element_type()
        if (rowT != null) {
            var firstDefault : Expression?
            if (terminatorName == "first_or_default") {
                firstDefault = clone_expression(terminatorCall.arguments[1])
            }
            var back = ctx.src->emit_reverse_last_backward(bindName, projection, terminalSelectLam, firstDefault, rowT, outElemType, at)
            if (back != null) return back
        }
    }
    var valueExpr : Expression?
    if (projection != null) {
        valueExpr = clone_expression(projection)
    } else {
        valueExpr = qmacro_expr() {
            $i(bindName)
        }
    }
    var perElementStmts <- qmacro_block_to_array() {
        $i(lastName) := $e(valueExpr)
        $i(foundName) = true
    }
    var lastRetExpr : Expression?
    if (terminalSelectLam != null) {
        lastRetExpr = peel_lambda_replace_var(terminalSelectLam, qmacro($i(lastName)))
    } else {
        lastRetExpr = qmacro($i(lastName))
    }
    var dRetExpr : Expression? = qmacro($i(dBindName))
    var prologue : array<Expression?>
    if (terminatorName == "first_or_default") {
        prologue |> push_from <| qmacro_block_to_array() {
            let $i(dBindName) = $e(terminatorCall.arguments[1])
        }
    }
    prologue |> push_from <| qmacro_block_to_array() {
        var $i(foundName) = false
        var $i(lastName) : $t(lastType) = default<$t(lastType)>
    }
    var tailStmts : array<Expression?>
    if (terminatorName == "first") {
        tailStmts |> push_from <| qmacro_block_to_array() {
            if (!$i(foundName)) {
                panic("sequence contains no elements")
            }
            return $e(lastRetExpr)
        }
    } else {
        tailStmts |> push_from <| qmacro_block_to_array() {
            return $i(foundName) ? $e(lastRetExpr) : $e(dRetExpr)
        }
    }
    return emit_terminator_lane(TerminatorSpec(
        prologue <- prologue,
        whereCond = whereCond,
        perElementStmts <- perElementStmts,
        tailStmts <- tailStmts,
        outElemType = outElemType
    ), ctx.src, at)
}

// nolint:STYLE014 — R6: `reverse() |> take(N) [|> _select(F)] |> to_array` on an ARRAY source. Visits only the
// last N indices backward — no full-source push + O(length) reverse_inplace. Generated:
//   let len = length(src); let takeN = clamp(<N>, 0, len)
//   var buf : array<Elem>; buf |> reserve(takeN)
//   for (k in 0 .. takeN) { buf |> push_clone(src[len - 1 - k]) }
//   <terminal _select tail>; return <- buf
// With `termsel` the loop pushes RAW source elements; the projection runs post-loop via build_terminal_select_tail
// (matches the decs PR #2915 / R1-R4 discipline: all source reads finish before any projection side-effect runs).
[macro_function]
def emit_reverse_backward_index_walk(var c : Captures; var ctx : EmitCtx; at : LineInfo) : Expression? {
    var top = ctx.top
    let srcName = ctx.src->loop_source_name()
    if (!(c.single |> key_exists("take")) || top._type == null || top._type.firstType == null) return null
    var bufElemType = strip_const_ref(clone_type(top._type.firstType))
    var termselLam : Expression?
    var termselElemType : TypeDeclPtr
    if (c.single |> key_exists("termsel")) {
        termselLam = c.single["termsel"].arguments[1]
        if (termselLam == null || c.single["termsel"]._type == null || c.single["termsel"]._type.firstType == null) return null
        termselElemType = strip_const_ref(clone_type(c.single["termsel"]._type.firstType))
    }
    let bufName = qn("buf", at)
    let lenName = qn("rlen", at)
    let takeLimName = qn("rtakeLim", at)
    let takeNName = qn("rtn", at)
    let kName = qn("rk", at)
    var takeArg = c.single["take"].arguments[1]
    var prologueStmts <- qmacro_block_to_array() {
        let $i(lenName) = length($i(srcName))
        let $i(takeLimName) = $e(takeArg)
        let $i(takeNName) = $i(takeLimName) <= 0 ? 0 : ($i(takeLimName) < $i(lenName) ? $i(takeLimName) : $i(lenName))
    }
    var reserveStmts : array<Expression?>
    reserveStmts |> push <| qmacro_expr() {
        $i(bufName) |> reserve($i(takeNName))
    }
    var perElementPush : Expression? = qmacro_expr() {
        $i(bufName) |> push_clone($i(srcName)[$i(lenName) - 1 - $i(kName)])
    }
    var tailStmts : array<Expression?>
    var wrapIter = false
    if (termselLam != null) {
        tailStmts |> push_from <| build_terminal_select_tail(bufName, bufElemType, termselLam, termselElemType, at)
        wrapIter = ctx.expr_is_iterator
    } else {
        tailStmts |> push <| buffer_return(bufName, ctx.expr_is_iterator)
    }
    var spec = FoldArraySpec(
        prologueStmts <- prologueStmts,
        bufElemType = bufElemType,
        bufName = bufName,
        reserveStmts <- reserveStmts,
        perElementPush = perElementPush,
        tailStmts <- tailStmts,
        wrapIter = wrapIter
    )
    var composedPE = compose_fold_per_element(spec)
    if (composedPE == null) return null
    spec.customSourceLoop = qmacro_expr() {
        for ($i(kName) in 0 .. $i(takeNName)) {
            $e(composedPE)
        }
    }
    return emit_fold_array_lane(spec, ctx.src, at)
}

// nolint:STYLE014 — R-2a: `reverse() |> distinct[_by] |> to_array` on an ARRAY source. One backward source walk
// with a set-gated push — saves the cascade's reverse_to_array + distinct_by_inplace second walk. Rides
// emit_fold_array_lane with a customSourceLoop:
//   let len = length(src)
//   for (k in 0 .. len) { let it = src[len - 1 - k]; <distinct gate> buf |> push_clone(it) }
//   return <- buf
[macro_function]
def emit_reverse_backward_walk_dset_gate(var c : Captures; var ctx : EmitCtx; at : LineInfo) : Expression? {
    var top = ctx.top
    let srcName = ctx.src->loop_source_name()
    if (!(c.single |> key_exists("dist"))) return null
    let distinctCall = c.single["dist"]
    let distinctName = call_norm_name(distinctCall)
    var distinctKey : Expression?
    if (distinctName == "distinct_by") {
        if ((distinctCall.arguments |> length) < 2) return null
        distinctKey = clone_expression(distinctCall.arguments[1])
    }
    if (top._type == null || top._type.firstType == null) return null
    var bufElemType = strip_const_ref(clone_type(top._type.firstType))
    let itName = qn("it", at)
    let bufName = qn("buf", at)
    let dkeyName = qn("rev_dkey", at)
    let dsetName = qn("rev_dset", at)
    let idxName = qn("rev_k", at)
    let rlenName = qn("rev_len", at)
    var dgKind : DistinctGateKind = (distinctName == "distinct_by"
        ? DistinctGateKind(KeyLam = distinctKey)
        : DistinctGateKind(NonKey = null))
    var distinctGate = new DistinctGateSpec(
        kind = dgKind, bindName = itName, bufElemType = bufElemType,
        dsetName = dsetName, dkeyName = dkeyName
    )
    var prologueStmts : array<Expression?>
    prologueStmts |> push <| qmacro_expr() {
        let $i(rlenName) = length($i(srcName))
    }
    var preCondStmts : array<Expression?>
    preCondStmts |> push <| qmacro_expr() {
        let $i(itName) = $i(srcName)[$i(rlenName) - 1 - $i(idxName)]
    }
    var perElementPush : Expression? = qmacro_expr() {
        $i(bufName) |> push_clone($i(itName))
    }
    var tailStmts : array<Expression?>
    tailStmts |> push <| buffer_return(bufName, ctx.expr_is_iterator)
    var spec = FoldArraySpec(
        prologueStmts <- prologueStmts,
        bufElemType = bufElemType,
        bufName = bufName,
        preCondStmts <- preCondStmts,
        distinctGate = distinctGate,
        perElementPush = perElementPush,
        tailStmts <- tailStmts
    )
    var composedPE = compose_fold_per_element(spec)
    if (composedPE == null) return null
    spec.customSourceLoop = qmacro_expr() {
        for ($i(idxName) in 0 .. $i(rlenName)) {
            $e(composedPE)
        }
    }
    return emit_fold_array_lane(spec, ctx.src, at)
}

// nolint:STYLE014 — R-2b: `reverse() |> distinct[_by] |> to_array` over a FORWARD source (XML / decs / iterator).
// Single-pass keep-last: OVERWRITE `tab[key] = (seq, val)` per element (slot ends at the LAST forward occurrence),
// sort survivors by descending seq, emit — output-identical to the array backward walk. Generated:
//   var tab : table<unique_key(<key>); (int, Val)>; var seq = 0
//   for (it in src) { tab[unique_key(<key>)] = (seq, <it ?? handle>); seq ++ }
//   pairs <- values(tab); order_inplace(pairs, by descending seq); push each pairs._1 into outBuf; return <- outBuf
// XML defers (distinct_by only): Val = node handle; build the survivor rows once at the tail.
[macro_function]
def emit_reverse_distinct_forward_keeplast(var c : Captures; var ctx : EmitCtx; at : LineInfo) : Expression? {
    if (!(c.single |> key_exists("dist"))) return null
    let distinctCall = c.single["dist"]
    let distinctName = call_norm_name(distinctCall)
    let bindName = ctx.src->bind_name(at)
    var top = ctx.top
    var elemType = ((top != null && top._type != null && top._type.firstType != null)
        ? strip_const_ref(clone_type(top._type.firstType))
        : ctx.src->element_type())
    if (elemType == null) return null
    let isKeyed = distinctName == "distinct_by"
    if (isKeyed && ((distinctCall.arguments |> length) < 2 || distinctCall.arguments[1] == null)) return null
    var keyBody = isKeyed ? peel_lambda_rename_var(distinctCall.arguments[1], bindName) : qmacro($i(bindName))
    if (keyBody == null) return null
    var deferred = ctx.src->defers_materialization() && distinctName == "distinct_by"
    var valType : TypeDeclPtr
    var storeVal : Expression?
    if (deferred) {
        valType = ctx.src->handle_type()
        storeVal = ctx.src->current_handle_expr(at)
        if (valType == null || storeVal == null) {
            deferred = false
        }
    }
    if (!deferred) {
        valType = clone_type(elemType)
        storeVal = qmacro($i(bindName))
    }
    let tabName = qn("rdb_tab", at)
    let seqName = qn("rdb_seq", at)
    let pairsName = qn("rdb_pairs", at)
    let svName = qn("rdb_sv", at)
    let outBufName = qn("rdb_out", at)
    var intType = new TypeDecl(baseType = Type.tInt, at = at)
    var surrType = build_surrogate_type(intType, valType, at)
    var prologue : array<Expression?>
    if (isKeyed) {
        var keyLamClone = clone_expression(distinctCall.arguments[1])  // nolint:PERF023 load-bearing const-strip: $e wants Expression?, arg reads as const
        prologue |> push <| qmacro_expr() {
            var $i(tabName) : table<typedecl(_::unique_key(invoke($e(keyLamClone), default<$t(elemType)>))); $t(surrType)>
        }
    } else {
        prologue |> push <| qmacro_expr() {
            var $i(tabName) : table<typedecl(_::unique_key(default<$t(elemType)>)); $t(surrType)>
        }
    }
    prologue |> push <| qmacro_expr() {
        var $i(seqName) = 0
    }
    var perElement <- qmacro_block_to_array() {
        $i(tabName)[_::unique_key($e(keyBody))] = ($i(seqName), $e(storeVal))
        $i(seqName) ++
    }
    var seqDescCmp = build_surrogate_cmp(true, at)
    var tail <- qmacro_block_to_array() {
        var $i(pairsName) : array<$t(surrType)>
        $i(pairsName) |> reserve(length($i(tabName)))
        for ($i(svName) in values($i(tabName))) {
            $i(pairsName) |> push_clone($i(svName))
        }
        _::order_inplace($i(pairsName), $e(seqDescCmp))
        var $i(outBufName) : array<$t(elemType)>
    }
    if (deferred) {
        tail |> push_from(build_surrogate_materialize_loop(ctx, pairsName, outBufName, elemType, null, true, at))
    } else {
        let oName = qn("rdb_o", at)
        tail |> push_from <| qmacro_block_to_array() {
            $i(outBufName) |> reserve(length($i(pairsName)))
            for ($i(oName) in $i(pairsName)) {
                $i(outBufName) |> push_clone($i(oName)._1)
            }
            return <- $i(outBufName)
        }
    }
    return emit_terminator_lane(TerminatorSpec(
        prologue <- prologue,
        perElementStmts <- perElement,
        tailStmts <- tail,
        wrapIter = ctx.expr_is_iterator
    ), ctx.src, at)
}

// nolint:STYLE014 — R1-R4 catch-all reverse: buffer survivors forward, reverse_inplace, optional resize (take),
// optional terminal _select. Tries faster paths first (the source's skip-into-tail for `reverse |> take`; the XML
// handle-defer below), else rides emit_fold_array_lane:
//   var buf : array<Elem>; for (it in src) { if (where) buf |> push_clone(<projection ?? it>) }
//   reverse_inplace(buf); [buf |> resize(clamp(N, 0, len));] [re-project termsel into outBuf;] return <- buf
[macro_function]
def emit_reverse_buffer_inplace(var c : Captures; var ctx : EmitCtx; at : LineInfo) : Expression? {
    let bindName = ctx.src->bind_name(at)
    let bufName = qn("buf", at)
    var whereCond : Expression?
    if (c.single |> key_exists("where")) {
        whereCond = peel_lambda_rename_var(c.single["where"].arguments[1], bindName)
    }
    var projection : Expression?
    if (c.single |> key_exists("preproj")) {
        projection = peel_lambda_rename_var(c.single["preproj"].arguments[1], bindName)
    }
    var terminalSelectLam_check : Expression?
    var terminalSelectElemType_check : TypeDeclPtr
    var termsel_fast_path_ok = true
    if (c.single |> key_exists("termsel")) {
        terminalSelectLam_check = c.single["termsel"].arguments[1]
        if (terminalSelectLam_check == null || c.single["termsel"]._type == null || c.single["termsel"]._type.firstType == null) {
            termsel_fast_path_ok = false
        } else {
            terminalSelectElemType_check = c.single["termsel"]._type.firstType
        }
    }
    if ((c.single |> key_exists("take")) && whereCond == null && projection == null && termsel_fast_path_ok) {
        var rev = ctx.src->emit_reverse_skip_into_tail(c.single["take"].arguments[1], ctx.expr_is_iterator, terminalSelectLam_check, terminalSelectElemType_check, at)
        if (rev != null) return rev
    }
    if (ctx.src->defers_materialization() && (c.single |> key_exists("take")) && projection == null && termsel_fast_path_ok) {
        var handleType = ctx.src->handle_type()
        var dRowType = ctx.src->element_type()
        if (handleType != null && dRowType != null) {
            var dTermselLam : Expression?
            var dTermselElemType : TypeDeclPtr
            if (c.single |> key_exists("termsel")) {
                dTermselLam = c.single["termsel"].arguments[1]
                dTermselElemType = clone_type(terminalSelectElemType_check)
            }
            var dTakeExpr = c.single["take"].arguments[1]
            let dOutBufName = qn("rev_out_buf", at)
            let dTakeLimName = qn("rev_takeLim", at)
            let dOutElemT = dTermselLam != null ? clone_type(dTermselElemType) : clone_type(dRowType)
            var dHandlePush = qmacro_expr() {
                $i(bufName) |> push($e(ctx.src->current_handle_expr(at)))
            }
            var dTail <- qmacro_block_to_array() {
                _::reverse_inplace($i(bufName))
                let $i(dTakeLimName) = $e(dTakeExpr)
                $i(bufName) |> resize($i(dTakeLimName) <= 0 ? 0 : ($i(dTakeLimName) < length($i(bufName)) ? $i(dTakeLimName) : length($i(bufName))))
                var $i(dOutBufName) : array<$t(dOutElemT)>
            }
            dTail |> push_from(build_surrogate_materialize_loop(ctx, bufName, dOutBufName, dRowType, dTermselLam, false, at))
            return emit_fold_array_lane(FoldArraySpec(
                bufElemType = clone_type(handleType),
                bufName = bufName,
                whereCond = whereCond,
                perElementPush = dHandlePush,
                tailStmts <- dTail,
                wrapIter = ctx.expr_is_iterator
            ), ctx.src, at)
        }
    }
    var takeExpr : Expression?
    if (c.single |> key_exists("take")) {
        takeExpr = clone_expression(c.single["take"].arguments[1])
    }
    var terminalSelectLam : Expression?
    var terminalSelectElemType : TypeDeclPtr
    if (c.single |> key_exists("termsel")) {
        terminalSelectLam = c.single["termsel"].arguments[1]
        if (terminalSelectLam == null || c.single["termsel"]._type == null || c.single["termsel"]._type.firstType == null) return null
        terminalSelectElemType = clone_type(c.single["termsel"]._type.firstType)
    }
    var bufElemType : TypeDeclPtr
    if (projection != null) {
        if (projection._type == null) return null
        bufElemType = strip_const_ref(clone_type(projection._type))
    } else {
        var srcElemType = ctx.src->element_type()
        if (srcElemType == null) return null
        bufElemType = strip_const_ref(srcElemType)
    }
    var perElementPush : Expression?
    if (projection != null) {
        perElementPush = qmacro_expr() {
            $i(bufName) |> push_clone($e(projection))
        }
    } else {
        perElementPush = qmacro_expr() {
            $i(bufName) |> push_clone($i(bindName))
        }
    }
    var reserveStmts : array<Expression?>
    if (ctx.src->can_reserve_by_length() && whereCond == null) {
        let top = ctx.src->loop_source_expr()
        let srcName = ctx.src->loop_source_name()
        if (top != null && top._type != null && type_has_length(top._type)) {
            reserveStmts |> push <| qmacro_expr() {
                $i(bufName) |> reserve(length($i(srcName)))
            }
        }
    }
    var tailStmts : array<Expression?>
    tailStmts |> push <| qmacro_expr() {
        _::reverse_inplace($i(bufName))
    }
    if (takeExpr != null) {
        let takeLimName = qn("rev_takeLim", at)
        tailStmts |> push_from <| qmacro_block_to_array() {
            let $i(takeLimName) = $e(takeExpr)
            $i(bufName) |> resize($i(takeLimName) <= 0 ? 0 : ($i(takeLimName) < length($i(bufName)) ? $i(takeLimName) : length($i(bufName))))
        }
    }
    tailStmts |> push_from <| build_terminal_select_tail(bufName, bufElemType, terminalSelectLam, terminalSelectElemType, at)
    return emit_fold_array_lane(FoldArraySpec(
        bufElemType = bufElemType,
        bufName = bufName,
        reserveStmts <- reserveStmts,
        whereCond = whereCond,
        perElementPush = perElementPush,
        tailStmts <- tailStmts,
        wrapIter = ctx.expr_is_iterator
    ), ctx.src, at)
}

[macro_function]
def build_reverse_rows {
    splice_patterns |> emplace <| SplicePattern(
        name = "reverse_distinct_backward_walk",
        chain <- [
            Slot(matcher = m_literal("reverse"), cardinality = c_one()),
            Slot(matcher = m_alias("distinct_family"), cardinality = c_one(), capture_name = "dist")
        ],
        requires <- [@@ < RequiresPredicate > array_source, @@ < RequiresPredicate > no_terminator],
        emit = @@ < EmitFn > emit_reverse_backward_walk_dset_gate
    )
    splice_patterns |> emplace <| SplicePattern(
        name = "reverse_distinct_forward_keeplast",
        chain <- [
            Slot(matcher = m_literal("reverse"), cardinality = c_one()),
            Slot(matcher = m_alias("distinct_family"), cardinality = c_one(), capture_name = "dist")
        ],
        requires <- [@@ < RequiresPredicate > non_array_source, @@ < RequiresPredicate > no_terminator],
        emit = @@ < EmitFn > emit_reverse_distinct_forward_keeplast
    )
    splice_patterns |> emplace <| SplicePattern(
        name = "reverse_take_backward_index",
        chain <- [
            Slot(matcher = m_literal("reverse"), cardinality = c_one()),
            Slot(matcher = m_literal("take"), cardinality = c_one(), capture_name = "take"),
            Slot(matcher = m_literal("select"), cardinality = c_opt(), capture_name = "termsel")
        ],
        requires <- [@@ < RequiresPredicate > array_source, @@ < RequiresPredicate > take_arg_is_int, @@ < RequiresPredicate > no_terminator],
        emit = @@ < EmitFn > emit_reverse_backward_index_walk
    )
    splice_patterns |> emplace <| SplicePattern(
        name = "reverse_counter",
        chain <- [
            Slot(matcher = m_literal("where_"), cardinality = c_opt(), capture_name = "where"),
            Slot(matcher = m_literal("select"), cardinality = c_opt(), capture_name = "proj"),
            Slot(matcher = m_literal("reverse"), cardinality = c_one()),
            Slot(matcher = m_literal("count"), cardinality = c_one(), capture_name = "term", arity = 1)
        ],
        requires <- array<RequiresPredicate>(),
        emit = @@ < EmitFn > emit_reverse_counter
    )
    splice_patterns |> emplace <| SplicePattern(
        name = "reverse_walk_overwrite",
        chain <- [
            Slot(matcher = m_literal("where_"), cardinality = c_opt(), capture_name = "where"),
            Slot(matcher = m_literal("select"), cardinality = c_opt(), capture_name = "proj"),
            Slot(matcher = m_literal("reverse"), cardinality = c_one()),
            Slot(matcher = m_literal("select"), cardinality = c_opt(), capture_name = "termsel"),
            Slot(matcher = m_alias("first_family"), cardinality = c_one(), capture_name = "term")
        ],
        requires <- array<RequiresPredicate>(),
        emit = @@ < EmitFn > emit_reverse_walk_overwrite_scalar
    )
    splice_patterns |> emplace <| SplicePattern(
        name = "reverse_buffer_inplace",
        chain <- [
            Slot(matcher = m_literal("where_"), cardinality = c_opt(), capture_name = "where"),
            Slot(matcher = m_literal("select"), cardinality = c_opt(), capture_name = "preproj"),
            Slot(matcher = m_literal("reverse"), cardinality = c_one()),
            Slot(matcher = m_literal("take"), cardinality = c_opt(), capture_name = "take"),
            Slot(matcher = m_literal("select"), cardinality = c_opt(), capture_name = "termsel")
        ],
        requires <- [@@ < RequiresPredicate > take_arg_is_int],
        emit = @@ < EmitFn > emit_reverse_buffer_inplace
    )
}

[macro_function]
def emit_hashtable_dedup(var c : Captures; var ctx : EmitCtx; at : LineInfo) : Expression? {
    // nolint:STYLE014 — distinct / distinct_by lane (optional where / select / take / count). One forward pass with
    // a `seen` set; the first occurrence per key passes. Generated (canonical `distinct |> to_array` shape):
    //   var inscope seen : table<unique_key(<key>)>; var buf : array<Elem>
    //   for (it in src) {
    //       if (where) {
    //           let k = unique_key(<distinct_by key ?? it>)
    //           if (!seen |> key_exists(k)) { seen |> insert(k); buf |> push_clone(<projection ?? it>) }
    //       }
    //   }
    //   return <- buf
    // Variants: + take(N) → stop after N survivors; + count[/long_count][(pred)] → count survivors (acc ++, gated by
    //   pred on each first occurrence) instead of buffering.
    let bindName = ctx.src->bind_name(at)
    let bufName = qn("buf", at)
    let seenName = qn("seen", at)
    let keyName = qn("k", at)
    let takenName = qn("taken", at)
    let accName = qn("acc", at)
    let takeLimName = qn("takeLim", at)
    let pvName = qn("pv", at)
    var whereCond : Expression?
    if (c.single |> key_exists("where")) {
        whereCond = peel_lambda_rename_var(c.single["where"].arguments[1], bindName)
    }
    var projection : Expression?
    if (c.single |> key_exists("proj")) {
        projection = peel_lambda_rename_var(c.single["proj"].arguments[1], bindName)
    }
    if (!(c.single |> key_exists("dist"))) return null
    let distinctCall = c.single["dist"]
    let isDistinctBy = call_norm_name(distinctCall) == "distinct_by"
    var distinctKeyBlock : Expression?
    if (isDistinctBy) {
        if ((distinctCall.arguments |> length) < 2) return null
        distinctKeyBlock = clone_expression(distinctCall.arguments[1])
    }
    var takeExpr : Expression?
    if (c.single |> key_exists("take")) {
        takeExpr = clone_expression(c.single["take"].arguments[1])
    }
    var terminatorName : string  = ""
    var countPred : Expression?
    if (c.single |> key_exists("term")) {
        let terminatorCall = c.single["term"]
        let termName = call_norm_name(terminatorCall)
        let termArgs = terminatorCall.arguments |> length
        if ((termName == "count" || termName == "sum" || termName == "long_count") && termArgs == 1) {
            terminatorName = termName
        } elif ((termName == "count" || termName == "long_count") && termArgs == 2) {
            if (terminatorCall.arguments[1] == null) return null
            countPred = clone_expression(terminatorCall.arguments[1])
            terminatorName = termName
        } else {
            return null
        }
    }
    if (takeExpr != null && terminatorName != "") return null
    var elemType : TypeDeclPtr
    if (projection != null) {
        if (projection._type == null) return null
        elemType = strip_const_ref(clone_type(projection._type))
    } else {
        var srcElemType = ctx.src->element_type()
        if (srcElemType == null) return null
        elemType = strip_const_ref(srcElemType)
    }
    var stmts : array<Expression?>
    if (isDistinctBy) {
        stmts |> push <| qmacro_expr() {
            var inscope $i(seenName) : table<typedecl(_::unique_key(invoke($e(distinctKeyBlock), default<$t(elemType)>)))>
        }
    } else {
        stmts |> push <| qmacro_expr() {
            var inscope $i(seenName) : table<typedecl(_::unique_key(default<$t(elemType)>))>
        }
    }
    let isCountTerminator = terminatorName == "count" || terminatorName == "long_count"
    let isSumTerminator = terminatorName == "sum"
    let needBuffer = !isCountTerminator && !isSumTerminator
    if (needBuffer) {
        stmts |> push <| qmacro_expr() {
            var $i(bufName) : array<$t(elemType)>
        }
    } elif (isSumTerminator) {
        stmts |> push <| qmacro_expr() {
            var $i(accName) : $t(elemType) = default<$t(elemType)>
        }
    } elif (countPred != null && terminatorName == "count") {
        stmts |> push <| qmacro_expr() {
            var $i(accName) = 0
        }
    } elif (countPred != null && terminatorName == "long_count") {
        stmts |> push <| qmacro_expr() {
            var $i(accName) = 0l
        }
    }
    if (takeExpr != null) {
        stmts |> push_from <| qmacro_block_to_array() {
            let $i(takeLimName) = $e(takeExpr)
            var $i(takenName) = 0
        }
    }
    let bindProjection = projection != null && has_sideeffects(projection)
    var pushExpr : Expression?
    if (projection != null) {
        if (bindProjection) {
            pushExpr = qmacro_expr() {
                $i(pvName)
            }
        } else {
            pushExpr = clone_expression(projection)
        }
    } else {
        pushExpr = qmacro_expr() {
            $i(bindName)
        }
    }
    var keyExpr : Expression?
    if (isDistinctBy) {
        keyExpr = peel_lambda_replace_var(distinctKeyBlock, pushExpr)
    } else {
        keyExpr = clone_expression(pushExpr)
    }
    if (countPred != null) {
        countPred = peel_lambda_replace_var(countPred, pushExpr)
        if (countPred == null) return null
    }
    var consumeStmts : array<Expression?>
    if (takeExpr != null) {
        consumeStmts |> push <| qmacro_expr() {
            $i(takenName) ++
        }
    }
    consumeStmts |> push <| qmacro_expr() {
        $i(seenName) |> insert($i(keyName))
    }
    if (needBuffer) {
        consumeStmts |> push <| qmacro_expr() {
            $i(bufName) |> push_clone($e(pushExpr))
        }
    } elif (isSumTerminator) {
        consumeStmts |> push <| qmacro_expr() {
            $i(accName) += $e(pushExpr)
        }
    }
    if (countPred != null && isCountTerminator) {
        consumeStmts |> push <| qmacro_expr() {
            if ($e(countPred)) {
                $i(accName) ++
            }
        }
    }
    var consumeBody = stmts_to_expr(consumeStmts)
    var ifNew : Expression? = qmacro_expr() {
        if (!$i(seenName) |> key_exists($i(keyName))) {
            $e(consumeBody)
        }
    }
    var perMatchStmts : array<Expression?>
    if (bindProjection) {
        perMatchStmts |> push <| qmacro_expr() {
            let $i(pvName) = $e(projection)
        }
    }
    perMatchStmts |> push <| qmacro_expr() {
        let $i(keyName) = _::unique_key($e(keyExpr))
    }
    perMatchStmts |> push(ifNew)
    var perElement = stmts_to_expr(perMatchStmts)
    perElement = wrap_with_condition(perElement, whereCond)
    if (takeExpr != null) {
        var dtl = ctx.src->emit_distinct_take_loop(bindName, takenName, takeLimName, perElement, at)
        if (dtl == null) return null
        stmts |> push(dtl)
    } else {
        var srcLoop = ctx.src->wrap_source_loop(LoopDispatch(Each = null), perElement, at)
        if (srcLoop == null) return null
        stmts |> push(srcLoop)
    }
    if (terminatorName == "count") {
        if (countPred != null) {
            stmts |> push <| qmacro_expr() {
                return $i(accName)
            }
        } else {
            stmts |> push <| qmacro_expr() {
                return length($i(seenName))
            }
        }
    } elif (terminatorName == "long_count") {
        if (countPred != null) {
            stmts |> push <| qmacro_expr() {
                return $i(accName)
            }
        } else {
            stmts |> push <| qmacro_expr() {
                return long_length($i(seenName))
            }
        }
    } elif (isSumTerminator) {
        stmts |> push <| qmacro_expr() {
            return $i(accName)
        }
    } else {
        stmts |> push_from <| qmacro_block_to_array() {
            return <- $i(bufName)
        }
    }
    let needIterWrap = ctx.expr_is_iterator && needBuffer
    return ctx.src->wrap_invoke(stmts, null, needIterWrap, at)
}

[macro_function]
def build_distinct_rows {
    splice_patterns |> emplace <| SplicePattern(
        name = "distinct_take",
        chain <- [
            Slot(matcher = m_literal("where_"), cardinality = c_opt(), capture_name = "where"),
            Slot(matcher = m_literal("select"), cardinality = c_opt(), capture_name = "proj"),
            Slot(matcher = m_alias("distinct_family"), cardinality = c_one(), capture_name = "dist"),
            Slot(matcher = m_literal("take"), cardinality = c_one(), capture_name = "take"),
            Slot(matcher = m_alias("distinct_terminator_family"), cardinality = c_opt(), capture_name = "term")
        ],
        requires <- [@@ < RequiresPredicate > take_arg_is_int],
        emit = @@ < EmitFn > emit_hashtable_dedup
    )
    splice_patterns |> emplace <| SplicePattern(
        name = "distinct_main",
        chain <- [
            Slot(matcher = m_literal("where_"), cardinality = c_opt(), capture_name = "where"),
            Slot(matcher = m_literal("select"), cardinality = c_opt(), capture_name = "proj"),
            Slot(matcher = m_alias("distinct_family"), cardinality = c_one(), capture_name = "dist"),
            Slot(matcher = m_alias("distinct_terminator_family"), cardinality = c_opt(), capture_name = "term")
        ],
        requires <- array<RequiresPredicate>(),
        emit = @@ < EmitFn > emit_hashtable_dedup
    )
}

[macro_function]
def is_bucket_reducer_call(expr : Expression?; bindName : string) : tuple<bool; string; Expression?> {
    if (expr == null || !(expr is ExprCall)) return (false, "", null)
    let c = expr as ExprCall
    if (c.func == null) return (false, "", null)
    var fnName = string(c.func.name)
    if (c.func.fromGeneric != null) {
        fnName = string(c.func.fromGeneric.name)
    }
    let nArgs = c.arguments |> length
    if (nArgs == 1 && (fnName == "length" || fnName == "count" || fnName == "long_count"
            || fnName == "sum" || fnName == "min" || fnName == "max" || fnName == "first" || fnName == "average")) {
        if (peel_tuple_field_read(c.arguments[0], bindName, 1)) return (true, fnName, null)
    }
    if (nArgs == 1 && (fnName == "sum" || fnName == "min" || fnName == "max" || fnName == "first" || fnName == "average")) {
        let inner = c.arguments[0]
        if (inner != null && inner is ExprCall) {
            let innerCall = inner as ExprCall
            if (innerCall.func != null) {
                var innerName = string(innerCall.func.name)
                if (innerCall.func.fromGeneric != null) {
                    innerName = string(innerCall.func.fromGeneric.name)
                }
                if (innerName == "select"
                        && (innerCall.arguments |> length) == 2
                        && peel_tuple_field_read(innerCall.arguments[0], bindName, 1)) return (true, "{fnName}_inner_select", innerCall.arguments[1])
            }
        }
    }
    if (nArgs == 2 && (fnName == "sum" || fnName == "min" || fnName == "max" || fnName == "average")) {
        if (peel_tuple_field_read(c.arguments[0], bindName, 1)) {
            if (is_identity_lambda(c.arguments[1])) return (true, fnName, null)
            return (true, "{fnName}_inner_select", c.arguments[1])
        }
    }
    return (false, "", null)
}

[macro_function]
def is_identity_lambda(lam : Expression const?) : bool {
    if (lam == null || !(lam is ExprMakeBlock)) return false
    let blk = (lam as ExprMakeBlock)._block
    if (blk == null || !(blk is ExprBlock)) return false
    let eb = blk as ExprBlock
    if (length(eb.arguments) != 1 || length(eb.list) != 1) return false
    let st = eb.list[0]
    if (st == null || !(st is ExprReturn)) return false
    let body = match_peel_r2v((st as ExprReturn).subexpr)
    return body != null && body is ExprVar && (body as ExprVar).name == eb.arguments[0].name
}

struct ReducerSpec {
    slot : int
    name : string
    innerBody : Expression?
    accType : TypeDeclPtr
}

[macro_function]
def is_average_spec(spec : ReducerSpec) : bool {
    return spec.name == "average" || spec.name == "average_inner_select"
}

[macro_function]
def mk_avg_acc_type(at : LineInfo) : TypeDeclPtr {
    var t = new TypeDecl(baseType = Type.tTuple, at = at)
    t.argTypes |> emplace_new <| new TypeDecl(baseType = Type.tDouble, at = at)
    t.argTypes |> emplace_new <| new TypeDecl(baseType = Type.tUInt64, at = at)
    return t
}

[macro_function]
def slot_acc_type(bodyType : TypeDeclPtr; slot : int; isBareForm, isAverage : bool; at : LineInfo) : TypeDeclPtr {
    if (isAverage) return mk_avg_acc_type(at)
    if (isBareForm) return strip_const_ref(clone_type(bodyType))
    if (bodyType == null || (bodyType.argTypes |> length) <= slot) return null
    return strip_const_ref(clone_type(bodyType.argTypes[slot]))
}

[macro_function]
def recognize_reducer_specs(var groupProjBody : Expression?; bindName, itName : string) {
    var specs : array<ReducerSpec>
    if (groupProjBody == null) return <- (<- specs, false)
    let (okBare, rnBare, innerBare) = is_bucket_reducer_call(groupProjBody, bindName)
    if (okBare) {
        var innerBody : Expression?
        if (innerBare != null) {
            innerBody = peel_lambda_rename_var(innerBare, itName)
            if (innerBody == null) return <- (<- specs, false)
        }
        let isAvgBare = rnBare == "average" || rnBare == "average_inner_select"
        var accType : TypeDeclPtr = slot_acc_type(groupProjBody._type, 1, true, isAvgBare, groupProjBody.at)
        if (accType == null) return <- (<- specs, false)
        var spec = ReducerSpec(slot = 1, name = rnBare, innerBody = innerBody, accType = accType)
        specs |> push(spec)
        return <- (<- specs, false)
    }
    if (!(groupProjBody is ExprMakeTuple)) return <- (<- specs, false)
    let mt = groupProjBody as ExprMakeTuple
    let n = mt.values |> length
    if (n < 2 || !peel_tuple_field_read(mt.values[0], bindName, 0)) return <- (<- specs, false)
    specs |> reserve(n - 1)
    for (i in range(1, n)) {
        let (ok, rn, innerLam) = is_bucket_reducer_call(mt.values[i], bindName)
        if (!ok) {
            specs |> clear
            return <- (<- specs, false)
        }
        var innerBody : Expression?
        if (innerLam != null) {
            innerBody = peel_lambda_rename_var(innerLam, itName)
            if (innerBody == null) {
                specs |> clear
                return <- (<- specs, false)
            }
        }
        let isAvg = rn == "average" || rn == "average_inner_select"
        var accType : TypeDeclPtr = slot_acc_type(groupProjBody._type, i, false, isAvg, groupProjBody.at)
        if (accType == null) {
            specs |> clear
            return <- (<- specs, false)
        }
        var spec = ReducerSpec(slot = i, name = rn, innerBody = innerBody, accType = accType)
        specs |> push(spec)
    }
    return <- (<- specs, true)
}

[macro_function]
def mk_slot_ref(at : LineInfo; entryName : string; slot : int) : Expression? {
    var ev = new ExprVar(at = at)
    ev.name := entryName
    var ef = new ExprField(at = at)
    ef.name := "_{slot}"
    ef.value = ev
    return ef
}

typedef ReducerEmitterFn = function<(spec : ReducerSpec; at : LineInfo; entryName, itName : string) : tuple<Expression?; Expression?>>

[macro_function]
def mk_strictly_preferred(workhorse, isMin : bool; var valExpr : Expression?; var cmpExpr : Expression?) : Expression? {
    if (workhorse) {
        return qmacro($e(valExpr) < $e(cmpExpr)) if (isMin)
        return qmacro($e(valExpr) > $e(cmpExpr))
    }
    return qmacro(_::less($e(valExpr), $e(cmpExpr))) if (isMin)
    return qmacro(_::less($e(cmpExpr), $e(valExpr)))
}

[macro_function, unused_argument(itName)]
def mk_reducer_count(spec : ReducerSpec; at : LineInfo; entryName, itName : string) : tuple<Expression?; Expression?> {
    var l1 = mk_slot_ref(at, entryName, spec.slot)
    var l2 = mk_slot_ref(at, entryName, spec.slot)
    let missInit = qmacro_expr() {
        $e(l1) = 1
    }
    let hitUpdate = qmacro_expr() {
        $e(l2) ++
    }
    return (missInit, hitUpdate)
}

[macro_function, unused_argument(itName)]
def mk_reducer_long_count(spec : ReducerSpec; at : LineInfo; entryName, itName : string) : tuple<Expression?; Expression?> {
    var l1 = mk_slot_ref(at, entryName, spec.slot)
    var l2 = mk_slot_ref(at, entryName, spec.slot)
    let missInit = qmacro_expr() {
        $e(l1) = 1l
    }
    let hitUpdate = qmacro_expr() {
        $e(l2) ++
    }
    return (missInit, hitUpdate)
}

[macro_function]
def mk_reducer_sum(spec : ReducerSpec; at : LineInfo; entryName, itName : string) : tuple<Expression?; Expression?> {
    var l1 = mk_slot_ref(at, entryName, spec.slot)
    var l2 = mk_slot_ref(at, entryName, spec.slot)
    let missInit = qmacro_expr() {
        $e(l1) = $i(itName)
    }
    let hitUpdate = qmacro_expr() {
        $e(l2) += $i(itName)
    }
    return (missInit, hitUpdate)
}

[macro_function, unused_argument(itName)]
def mk_reducer_sum_inner(spec : ReducerSpec; at : LineInfo; entryName, itName : string) : tuple<Expression?; Expression?> {
    var l1 = mk_slot_ref(at, entryName, spec.slot)
    var l2 = mk_slot_ref(at, entryName, spec.slot)
    let missInit = qmacro_expr() {
        $e(l1) = $e(spec.innerBody)
    }
    let hitUpdate = qmacro_expr() {
        $e(l2) += $e(spec.innerBody)
    }
    return (missInit, hitUpdate)
}

[macro_function]
def mk_reducer_min_max(spec : ReducerSpec; at : LineInfo; entryName, itName : string) : tuple<Expression?; Expression?> {
    let workhorse = spec.accType != null && spec.accType.isWorkhorseType
    let isMin = spec.name == "min"
    var lInit = mk_slot_ref(at, entryName, spec.slot)
    var lCmp  = mk_slot_ref(at, entryName, spec.slot)
    var lAsn  = mk_slot_ref(at, entryName, spec.slot)
    var predExpr = mk_strictly_preferred(workhorse, isMin, qmacro($i(itName)), lCmp)
    let missInit = qmacro_expr() {
        $e(lInit) := $i(itName)
    }
    let hitUpdate = qmacro_expr() {
        if ($e(predExpr)) {
            $e(lAsn) := $i(itName)
        }
    }
    return (missInit, hitUpdate)
}

[macro_function, unused_argument(itName)]
def mk_reducer_min_max_inner(spec : ReducerSpec; at : LineInfo; entryName, itName : string) : tuple<Expression?; Expression?> {
    let workhorse = spec.accType != null && spec.accType.isWorkhorseType
    let isMin = spec.name == "min_inner_select"
    let tmp = "`vis`{at.line}`{at.column}`{spec.slot}"
    var lInit = mk_slot_ref(at, entryName, spec.slot)
    var lCmp  = mk_slot_ref(at, entryName, spec.slot)
    var lAsn  = mk_slot_ref(at, entryName, spec.slot)
    var predExpr = mk_strictly_preferred(workhorse, isMin, qmacro($i(tmp)), lCmp)
    let missInit = qmacro_expr() {
        $e(lInit) := $e(spec.innerBody)
    }
    let hitUpdate = qmacro_block() {
        let $i(tmp) = $e(spec.innerBody)
        if ($e(predExpr)) {
            $e(lAsn) := $i(tmp)
        }
    }
    return (missInit, hitUpdate)
}

[macro_function]
def mk_reducer_first(spec : ReducerSpec; at : LineInfo; entryName, itName : string) : tuple<Expression?; Expression?> {
    var l1 = mk_slot_ref(at, entryName, spec.slot)
    let missInit = qmacro_expr() {
        $e(l1) := $i(itName)
    }
    return (missInit, null)
}

[macro_function, unused_argument(itName)]
def mk_reducer_first_inner(spec : ReducerSpec; at : LineInfo; entryName, itName : string) : tuple<Expression?; Expression?> {
    var l1 = mk_slot_ref(at, entryName, spec.slot)
    let missInit = qmacro_expr() {
        $e(l1) := $e(spec.innerBody)
    }
    return (missInit, null)
}

// nolint:STYLE014 — deferred first-per-group (XML): build the full row from the source handle INSIDE the
// miss-branch, so the per-element walk reads only the group key (field-pruned) instead of `slot := it`
// pinning every row. Emits missInit (no hit update):
//   var mat = default<Row>
//   <materialize current handle → mat>
//   entry._slot := mat
[macro_function, unused_argument(itName)]
def mk_reducer_first_deferred(spec : ReducerSpec; at : LineInfo; entryName, itName : string;
                              var adapter : SourceAdapter?) : tuple<Expression?; Expression?> {
    var l1 = mk_slot_ref(at, entryName, spec.slot)
    var rowType = strip_const_ref(clone_type(spec.accType))
    let matName = qn("gbf_mat_{spec.slot}", at)
    var matStmt = adapter->materialize_handle(adapter->current_handle_expr(at), matName, at)
    let missInit = qmacro_block() {
        var $i(matName) = default<$t(rowType)>
        $e(matStmt)
        $e(l1) := $i(matName)
    }
    return (missInit, null)
}

[macro_function]
def mk_reducer_average(spec : ReducerSpec; at : LineInfo; entryName, itName : string) : tuple<Expression?; Expression?> {
    var lInit = mk_slot_ref(at, entryName, spec.slot)
    var lSum  = mk_slot_ref(at, entryName, spec.slot)
    var lCnt  = mk_slot_ref(at, entryName, spec.slot)
    let missInit = qmacro_expr() {
        $e(lInit) = (double($i(itName)), 1ul)
    }
    let hitUpdate = qmacro_block() {
        $e(lSum)._0 += double($i(itName))
        $e(lCnt)._1 ++
    }
    return (missInit, hitUpdate)
}

[macro_function, unused_argument(itName)]
def mk_reducer_average_inner(spec : ReducerSpec; at : LineInfo; entryName, itName : string) : tuple<Expression?; Expression?> {
    let tmp = "`vavg`{at.line}`{at.column}`{spec.slot}"
    var lInit = mk_slot_ref(at, entryName, spec.slot)
    var lSum  = mk_slot_ref(at, entryName, spec.slot)
    var lCnt  = mk_slot_ref(at, entryName, spec.slot)
    let missInit = qmacro_block() {
        let $i(tmp) = $e(spec.innerBody)
        $e(lInit) = (double($i(tmp)), 1ul)
    }
    let hitUpdate = qmacro_block() {
        let $i(tmp) = $e(spec.innerBody)
        $e(lSum)._0 += double($i(tmp))
        $e(lCnt)._1 ++
    }
    return (missInit, hitUpdate)
}

var reducer_emitters : table<string; ReducerEmitterFn> <- {
    "length"               => @@ < ReducerEmitterFn > mk_reducer_count,
    "count"                => @@ < ReducerEmitterFn > mk_reducer_count,
    "long_count"           => @@ < ReducerEmitterFn > mk_reducer_long_count,
    "sum"                  => @@ < ReducerEmitterFn > mk_reducer_sum,
    "sum_inner_select"     => @@ < ReducerEmitterFn > mk_reducer_sum_inner,
    "min"                  => @@ < ReducerEmitterFn > mk_reducer_min_max,
    "max"                  => @@ < ReducerEmitterFn > mk_reducer_min_max,
    "min_inner_select"     => @@ < ReducerEmitterFn > mk_reducer_min_max_inner,
    "max_inner_select"     => @@ < ReducerEmitterFn > mk_reducer_min_max_inner,
    "first"                => @@ < ReducerEmitterFn > mk_reducer_first,
    "first_inner_select"   => @@ < ReducerEmitterFn > mk_reducer_first_inner,
    "average"              => @@ < ReducerEmitterFn > mk_reducer_average,
    "average_inner_select" => @@ < ReducerEmitterFn > mk_reducer_average_inner
}

[macro_function]
def emit_reducer_branches(spec : ReducerSpec; at : LineInfo; entryName, itName : string;
                          var adapter : SourceAdapter? = null) : tuple<Expression?; Expression?> {
    if (spec.name == "first" && adapter != null && adapter->handle_type() != null)
        return mk_reducer_first_deferred(spec, at, entryName, itName, adapter)
    let emitter = reducer_emitters?[spec.name] ?? default<ReducerEmitterFn>
    return (null, null) if (emitter == null)
    return invoke(emitter, spec, at, entryName, itName)
}

[macro_function]
def mk_avg_subfield(at : LineInfo; entryName : string; slot, sub : int) : Expression? {
    var ev = new ExprVar(at = at)
    ev.name := entryName
    var ef1 = new ExprField(at = at)
    ef1.name := "_{slot}"
    ef1.value = ev
    var ef2 = new ExprField(at = at)
    ef2.name := "_{sub}"
    ef2.value = ef1
    return ef2
}

[macro_function]
def mk_avg_divide_expr(at : LineInfo; entryName : string; slot : int) : Expression? {
    var sumRef = mk_avg_subfield(at, entryName, slot, 0)
    var cntRef = mk_avg_subfield(at, entryName, slot, 1)
    return qmacro_expr() {
        $e(sumRef) / double($e(cntRef))
    }
}

[macro_function]
def mk_slot_output_expr(at : LineInfo; entryName : string; spec : ReducerSpec) : Expression? {
    if (is_average_spec(spec)) return mk_avg_divide_expr(at, entryName, spec.slot)
    return mk_slot_ref(at, entryName, spec.slot)
}

[macro_function]
def mk_hidden_acc_type(reducerName : string; isInnerSelect : bool;
                       bucketElemType, innerBodyType : TypeDeclPtr; at : LineInfo) : TypeDeclPtr {
    if (reducerName == "length" || reducerName == "count") return new TypeDecl(baseType = Type.tInt, at = at)
    if (reducerName == "long_count") return new TypeDecl(baseType = Type.tInt64, at = at)
    if (reducerName == "average" || reducerName == "average_inner_select") return mk_avg_acc_type(at)
    let src = isInnerSelect ? innerBodyType : bucketElemType
    if (src == null) return null
    return strip_const_ref(clone_type(src))
}

[macro_function]
def having_inner_bodies_render_same(a, b : Expression?) : bool {
    if (a == null || b == null) return (a == null) == (b == null)
    return describe(a) == describe(b)
}

[macro_function]
def having_reducer_append_if_missing(var expr : Expression?; hbName, itName : string;
                                     var specs : array<ReducerSpec>;
                                     bucketElemType : TypeDeclPtr; at : LineInfo) : bool {
    if (expr == null || !(expr is ExprCall)) return true
    let c = expr as ExprCall
    if (c.func == null || (c.arguments |> length) != 1) return true
    var fnName = string(c.func.name)
    if (c.func.fromGeneric != null) {
        fnName = string(c.func.fromGeneric.name)
    }
    let isReducer = (fnName == "length" || fnName == "count" || fnName == "long_count"
            || fnName == "sum" || fnName == "min" || fnName == "max"
            || fnName == "first" || fnName == "average")
    if (!isReducer) return true
    if (peel_tuple_field_read(c.arguments[0], hbName, 1)) {
        for (s in specs) {
            if (s.innerBody == null && s.name == fnName) return true
        }
        var accType = mk_hidden_acc_type(fnName, false, bucketElemType, null, at)
        if (accType == null) return true
        let newSlot = (specs |> length) + 1
        specs |> push <| ReducerSpec(slot = newSlot, name = fnName, innerBody = null, accType = accType)
        return true
    }
    let canInnerSelect = (fnName == "sum" || fnName == "min" || fnName == "max"
            || fnName == "first" || fnName == "average")
    if (!canInnerSelect) return true
    let inner = c.arguments[0]
    if (inner == null || !(inner is ExprCall)) return true
    let ic = inner as ExprCall
    if (ic.func == null || (ic.arguments |> length) != 2 || !peel_tuple_field_read(ic.arguments[0], hbName, 1)) return true
    var innerName = string(ic.func.name)
    if (ic.func.fromGeneric != null) {
        innerName = string(ic.func.fromGeneric.name)
    }
    if (innerName != "select") return true
    let comboName = "{fnName}_inner_select"
    var innerBody = peel_lambda_rename_var(ic.arguments[1], itName)
    if (innerBody == null || innerBody._type == null) return false
    if (!has_sideeffects(innerBody)) {
        for (s in specs) {
            if (s.name == comboName && having_inner_bodies_render_same(s.innerBody, innerBody)) return true
        }
    }
    var accType = mk_hidden_acc_type(comboName, true, bucketElemType, innerBody._type, at)
    if (accType == null) return false
    let newSlot = (specs |> length) + 1
    specs |> push <| ReducerSpec(slot = newSlot, name = comboName, innerBody = innerBody, accType = accType)
    return true
}

[macro_function]
def extend_specs_for_missing_having_reducers(var expr : Expression?; hbName, itName : string;
                                             var specs : array<ReducerSpec>;
                                             bucketElemType : TypeDeclPtr; at : LineInfo) : bool {
    if (expr == null) return true
    qm_peel_ref2value(expr)
    if (expr is ExprCall) {
        var c = expr as ExprCall
        if (!having_reducer_append_if_missing(expr, hbName, itName, specs, bucketElemType, at)) return false
        for (i in 0 .. (c.arguments |> length)) {
            if (!extend_specs_for_missing_having_reducers(c.arguments[i], hbName, itName, specs, bucketElemType, at)) return false
        }
        return true
    }
    if (expr is ExprField) {
        var f = expr as ExprField
        return extend_specs_for_missing_having_reducers(f.value, hbName, itName, specs, bucketElemType, at)
    }
    if (expr is ExprOp1) {
        var op = expr as ExprOp1
        return extend_specs_for_missing_having_reducers(op.subexpr, hbName, itName, specs, bucketElemType, at)
    }
    if (expr is ExprOp2) {
        var op = expr as ExprOp2
        if (!extend_specs_for_missing_having_reducers(op.left, hbName, itName, specs, bucketElemType, at)) return false
        return extend_specs_for_missing_having_reducers(op.right, hbName, itName, specs, bucketElemType, at)
    }
    if (expr is ExprOp3) {
        var op = expr as ExprOp3
        return (extend_specs_for_missing_having_reducers(op.subexpr, hbName, itName, specs, bucketElemType, at)
                && extend_specs_for_missing_having_reducers(op.left, hbName, itName, specs, bucketElemType, at)
                && extend_specs_for_missing_having_reducers(op.right, hbName, itName, specs, bucketElemType, at))
    }
    return true
}

[macro_function]
def rewrite_having_pred(var expr : Expression?; hbName, kvName, itName : string; specs : array<ReducerSpec>) : Expression? {
    if (expr == null) return expr
    qm_peel_ref2value(expr)
    if (expr is ExprCall) {
        var c = expr as ExprCall
        if (c.func != null && (c.arguments |> length) == 1) {
            var fnName = string(c.func.name)
            if (c.func.fromGeneric != null) {
                fnName = string(c.func.fromGeneric.name)
            }
            if (peel_tuple_field_read(c.arguments[0], hbName, 1)) {
                for (s in specs) {
                    if (s.innerBody == null && s.name == fnName) return mk_slot_output_expr(expr.at, kvName, s)
                }
                return null
            }
            let inner = c.arguments[0]
            if (inner != null && inner is ExprCall) {
                let ic = inner as ExprCall
                if (ic.func != null && (ic.arguments |> length) == 2 && peel_tuple_field_read(ic.arguments[0], hbName, 1)) {
                    var innerName = string(ic.func.name)
                    if (ic.func.fromGeneric != null) {
                        innerName = string(ic.func.fromGeneric.name)
                    }
                    if (innerName == "select") {
                        let comboName = "{fnName}_inner_select"
                        var rwBody = peel_lambda_rename_var(ic.arguments[1], itName)
                        for (s in specs) {
                            if (s.name == comboName && having_inner_bodies_render_same(s.innerBody, rwBody)) return mk_slot_output_expr(expr.at, kvName, s)
                        }
                        return null
                    }
                }
            }
        }
        for (i in 0 .. (c.arguments |> length)) {
            var sub = rewrite_having_pred(c.arguments[i], hbName, kvName, itName, specs)
            if (sub == null) return null
            c.arguments[i] = sub
        }
        return expr
    }
    if (expr is ExprField) {
        var f = expr as ExprField
        if (f.value != null && f.value is ExprVar) {
            let v = f.value as ExprVar
            if (v.name == hbName) {
                if (f.name == "_0") return mk_slot_ref(expr.at, kvName, 0)
                return null
            }
        }
        var sub = rewrite_having_pred(f.value, hbName, kvName, itName, specs)
        if (sub == null) return null
        f.value = sub
        return expr
    }
    if (expr is ExprVar) {
        let v = expr as ExprVar
        if (v.name == hbName) return null
        return expr
    }
    if (expr is ExprOp1) {
        var op = expr as ExprOp1
        var sub = rewrite_having_pred(op.subexpr, hbName, kvName, itName, specs)
        if (sub == null) return null
        op.subexpr = sub
        return expr
    }
    if (expr is ExprOp2) {
        var op = expr as ExprOp2
        var l = rewrite_having_pred(op.left, hbName, kvName, itName, specs)
        if (l == null) return null
        op.left = l
        var r = rewrite_having_pred(op.right, hbName, kvName, itName, specs)
        if (r == null) return null
        op.right = r
        return expr
    }
    if (expr is ExprOp3) {
        var op = expr as ExprOp3
        var s = rewrite_having_pred(op.subexpr, hbName, kvName, itName, specs)
        if (s == null) return null
        op.subexpr = s
        var l = rewrite_having_pred(op.left, hbName, kvName, itName, specs)
        if (l == null) return null
        op.left = l
        var r = rewrite_having_pred(op.right, hbName, kvName, itName, specs)
        if (r == null) return null
        op.right = r
        return expr
    }
    return expr
}

class HbScanner : AstVisitor {
    vname : string
    hit : bool = false
    def HbScanner(n : string) {
        vname = n
    }
    def override visitExprVar(var expr : ExprVar?) : Expression? {
        if (expr.name == vname) {
            hit = true
        }
        return expr
    }
}

def expr_uses_var(var e : Expression?; name : string) : bool {
    if (e == null) return false
    var astVisitor = new HbScanner(name)
    make_visitor(*astVisitor) $(astVisitorAdapter) {
        visit_expression(e, astVisitorAdapter)
    }
    let hit = astVisitor.hit
    unsafe {
        delete astVisitor
    }
    return hit
}

[macro_function]
def plan_group_by_core(var calls : array<tuple<ExprCall?; LinqCall?>>;    // nolint:STYLE037 — at the cap already; +1 is the wrap_source_loop null guard, no natural seam
                       var keyBlock : Expression?;
                       var groupProjCall : ExprCall?;
                       var havingCall : ExprCall?;
                       var trailingWhereCall : ExprCall?;
                       terminatorName : string;
                       orderName : string;
                       var orderKey : Expression?;
                       exprIsIterator : bool;
                       at : LineInfo;
                       var adapter : SourceAdapter?) : Expression? {
    // nolint:STYLE014 — group_by planner. Phase 1: walk the source into `tab`, running each reducer's missInit
    // (new key) / hitUpdate (repeat key). Phase 2: emit one output row per group, then optional having / order /
    // terminator. Generated:
    //   var tab : table<unique_key(<key>); (KeyT, slot1, …, slotN)>; var dummy : (KeyT, slot1, …)
    //   for (it in src) {                                        // upstream where/select segments applied here
    //       let k = <keyBlock>; let uk = unique_key(k)
    //       var entry & = tab?[uk] ?? dummy                       // one hash-op on hits, two on misses
    //       if (addr(entry) == addr(dummy)) { <slot missInits>; dummy._0 = k; tab[uk] = dummy; dummy = default }
    //       else { <slot hitUpdates> }
    //   }
    //   var buf : array<OutRow>
    //   for (kv in values(tab)) { let row = (kv._0, <per-slot output: avg divides, else field ref>); [if (having)] buf |> push_clone(row) }
    //   [order buf]; [terminator: count/…]; return <- buf
    // Prefix keeps hoisted names per-source distinct ("" array / "decs_" decs / "djoin_" decs-join / "ajoin_" array-join / "xml_" xml). The adapter supplies its own.
    let prefix = adapter->name_prefix()
    let tabName    = qn("{prefix}tab", at)
    let kName      = qn("{prefix}k", at)
    let ukName     = qn("{prefix}uk", at)
    let entryName  = qn("{prefix}entry", at)
    let kvName     = qn("{prefix}kv", at)
    let bufName    = qn("{prefix}buf", at)
    let bindName   = qn("{prefix}gpb", at)
    let cntName    = qn("{prefix}cnt", at)
    let dummyName  = qn("{prefix}dummy", at)
    let outName    = qn("{prefix}out", at)
    var segBinds : array<array<Expression?>>
    var segWheres : array<Expression?>
    var currentBinds : array<Expression?>
    var currentWhere : Expression?
    var projection : Expression?
    var lastBindName = adapter->bind_name(at)
    var elemType = strip_const_ref(adapter->element_type())
    var totalBindCount = 0
    for (i in 0 .. (calls |> length)) {
        let opName = calls[i]._1.name
        if (opName == "where_") {
            if (projection != null) {
                let bn = "`{prefix}v`{at.line}`{at.column}`{totalBindCount}"
                totalBindCount ++
                currentBinds |> push <| qmacro_expr() {
                    var $i(bn) := $e(projection)
                }
                lastBindName = bn
                projection = null
            }
            var predicate = peel_lambda_rename_var(calls[i]._0.arguments[1], lastBindName)
            if (predicate == null) return null
            currentWhere = merge_where_cond(currentWhere, predicate)
        } elif (opName == "select") {
            if (currentWhere != null) {
                segBinds |> emplace(currentBinds)
                segWheres |> push(currentWhere)
                currentWhere = null
            }
            if (projection != null) {
                let bn = "`{prefix}v`{at.line}`{at.column}`{totalBindCount}"
                totalBindCount ++
                currentBinds |> push <| qmacro_expr() {
                    var $i(bn) := $e(projection)
                }
                lastBindName = bn
            }
            projection = peel_lambda_rename_var(calls[i]._0.arguments[1], lastBindName)
            if (projection == null) return null
            elemType = strip_const_ref(clone_type(calls[i]._0._type.firstType))
        } else {
            return null
        }
    }
    if (projection != null) {
        let bn = "`{prefix}v`{at.line}`{at.column}`{totalBindCount}"
        totalBindCount ++
        currentBinds |> push <| qmacro_expr() {
            var $i(bn) := $e(projection)
        }
        lastBindName = bn
    }
    segBinds |> emplace(currentBinds)
    segWheres |> push(currentWhere)
    let itName = lastBindName
    var groupProjBody = peel_lambda_rename_var(groupProjCall.arguments[1], bindName)
    if (groupProjBody == null || groupProjBody._type == null) return null
    var (specs, usesNamedTuple) = recognize_reducer_specs(groupProjBody, bindName, itName)
    if (empty(specs)) return null
    let userVisibleSlotCount = specs |> length
    var havingPred : Expression?
    if (havingCall != null) {
        let hbName = qn("{prefix}hb", at)
        var rawPred = peel_lambda_rename_var(havingCall.arguments[1], hbName)
        if (rawPred == null || !extend_specs_for_missing_having_reducers(rawPred, hbName, itName, specs, elemType, at)) return null
        havingPred = rewrite_having_pred(rawPred, hbName, kvName, itName, specs)
        if (havingPred == null || expr_uses_var(havingPred, hbName)) return null
    }
    var trailingWherePred : Expression?
    if (trailingWhereCall != null) {
        trailingWherePred = peel_lambda_rename_var(trailingWhereCall.arguments[1], outName)
        if (trailingWherePred == null) return null
    }
    let hasHidden = (specs |> length) > userVisibleSlotCount
    if (hasHidden && !usesNamedTuple) return null
    var hasAvg = false
    for (s in specs) {
        if (is_average_spec(s)) {
            hasAvg = true
        }
    }
    var stmts : array<Expression?>
    if (usesNamedTuple) {
        var tabValueType = strip_const_ref(clone_type(groupProjBody._type))
        if (hasAvg) {
            for (s in specs) {
                if (is_average_spec(s) && s.slot <= userVisibleSlotCount && s.slot < (tabValueType.argTypes |> length)) {
                    tabValueType.argTypes[s.slot] = mk_avg_acc_type(at)
                }
            }
        }
        if (hasHidden) {
            let preserveNames = (tabValueType.argNames |> length) == (tabValueType.argTypes |> length)
            for (s in specs) {
                if (s.slot > userVisibleSlotCount) {
                    tabValueType.argTypes |> push <| clone_type(s.accType)
                    if (preserveNames) {
                        let nameIdx = tabValueType.argNames |> length
                        tabValueType.argNames |> resize(nameIdx + 1)
                        tabValueType.argNames[nameIdx] := "_hidden_{s.slot}"
                    }
                }
            }
        }
        var tabValueType2 = clone_type(tabValueType)
        stmts |> push_from <| qmacro_block_to_array() {
            var inscope $i(tabName) : table<typedecl(_::unique_key(invoke($e(keyBlock), default<$t(elemType)>))); $t(tabValueType)>
            var $i(dummyName) : $t(tabValueType2)
        }
    } else {
        var accType  = clone_type(specs[0].accType)
        var accType2 = clone_type(specs[0].accType)
        stmts |> push_from <| qmacro_block_to_array() {
            var inscope $i(tabName) : table<typedecl(_::unique_key(invoke($e(keyBlock), default<$t(elemType)>))); tuple<typedecl(invoke($e(keyBlock), default<$t(elemType)>)) -const -&; $t(accType)>>
            var $i(dummyName) : tuple<typedecl(invoke($e(keyBlock), default<$t(elemType)>)) -const -&; $t(accType2)>
        }
    }
    var missInitStmts : array<Expression?>
    var hitUpdateStmts : array<Expression?>
    var reducerAdapter : SourceAdapter? = (adapter->defers_materialization() && itName == adapter->bind_name(at)) ? adapter : null
    for (s in specs) {
        let (mi, hu) = emit_reducer_branches(s, at, entryName, itName, reducerAdapter)
        if (mi != null) {
            missInitStmts |> push(clone_expression(mi))
        }
        if (hu != null) {
            hitUpdateStmts |> push(clone_expression(hu))
        }
    }
    if (empty(missInitStmts)) return null
    var missInit = stmts_to_expr(missInitStmts)
    let hasHitUpdate = !empty(hitUpdateStmts)
    var tableUpdate : Expression?
    if (hasHitUpdate) {
        var hitUpdate = stmts_to_expr(hitUpdateStmts)
        tableUpdate = qmacro_expr() {
            unsafe {
                var $i(entryName) & = $i(tabName)?[$i(ukName)] ?? $i(dummyName)
                if (addr($i(entryName)) == addr($i(dummyName))) {
                    $e(missInit)
                    $i(dummyName)._0 = $i(kName)
                    $i(tabName)[$i(ukName)] = $i(dummyName)
                    $i(dummyName) = default<typedecl($i(dummyName))>
                } else {
                    $e(hitUpdate)
                }
            }
        }
    } else {
        tableUpdate = qmacro_expr() {
            unsafe {
                var $i(entryName) & = $i(tabName)?[$i(ukName)] ?? $i(dummyName)
                if (addr($i(entryName)) == addr($i(dummyName))) {
                    $e(missInit)
                    $i(dummyName)._0 = $i(kName)
                    $i(tabName)[$i(ukName)] = $i(dummyName)
                    $i(dummyName) = default<typedecl($i(dummyName))>
                }
            }
        }
    }
    var keyExpr = peel_lambda_rename_var(keyBlock, itName)
    var coreBody = qmacro_block() {
        let $i(kName) = $e(keyExpr)
        let $i(ukName) = _::unique_key($i(kName))
        $e(tableUpdate)
    }
    var body : Expression? = coreBody
    var idx = (segBinds |> length) - 1
    while (idx >= 0) {
        if (segWheres[idx] != null) {
            body = wrap_with_condition(body, clone_expression(segWheres[idx]))
        }
        if (!empty(segBinds[idx])) {
            var thisSeg <- segBinds[idx]
            thisSeg |> push(body)
            body = stmts_to_expr(thisSeg)
        }
        idx --
    }
    var srcLoop = adapter->wrap_source_loop(LoopDispatch(Each = null), body, at)
    if (srcLoop == null) return null
    stmts |> push(srcLoop)
    let needOutput = terminatorName != "count" || trailingWherePred != null
    var outputExpr : Expression?
    var bufElemType : TypeDeclPtr
    if (needOutput) {
        if (!usesNamedTuple) {
            if (hasAvg) {
                outputExpr = mk_avg_divide_expr(at, kvName, 1)
            } else {
                outputExpr = qmacro_expr() {
                    $i(kvName)._1
                }
            }
        } elif (hasAvg || hasHidden) {
            var mt = new ExprMakeTuple(at = at)
            let slotCount = (groupProjBody._type.argTypes |> length)
            let hasNames = (groupProjBody._type.argNames |> length) == slotCount
            if (hasNames) {
                mt.recordNames |> resize(slotCount)
            }
            mt.values |> push(mk_slot_ref(at, kvName, 0))
            if (hasNames) {
                mt.recordNames[0] := string(groupProjBody._type.argNames[0])
            }
            for (i in 1 .. slotCount) {
                var v : Expression?
                for (sp in specs) {
                    if (sp.slot == i) {
                        v = mk_slot_output_expr(at, kvName, sp)
                    }
                }
                mt.values |> push(v)
                if (hasNames) {
                    mt.recordNames[i] := string(groupProjBody._type.argNames[i])
                }
            }
            outputExpr = mt
        } else {
            outputExpr = qmacro_expr() {
                $i(kvName)
            }
        }
        bufElemType = clone_type(groupProjBody._type)
        if (bufElemType != null) {
            bufElemType.flags.constant = false
            bufElemType.flags.ref = false
        }
    }
    var retType : TypeDeclPtr
    if (terminatorName == "count") {
        retType = new TypeDecl(baseType = Type.tInt)
        if (havingPred != null && trailingWherePred != null) {
            stmts |> push <| qmacro_block() {
                var $i(cntName) = 0
                for ($i(kvName) in values($i(tabName))) {
                    if ($e(havingPred)) {
                        let $i(outName) : $t(bufElemType) = $e(outputExpr)
                        if ($e(trailingWherePred)) {
                            $i(cntName) ++
                        }
                    }
                }
                return $i(cntName)
            }
        } elif (trailingWherePred != null) {
            stmts |> push <| qmacro_block() {
                var $i(cntName) = 0
                for ($i(kvName) in values($i(tabName))) {
                    let $i(outName) : $t(bufElemType) = $e(outputExpr)
                    if ($e(trailingWherePred)) {
                        $i(cntName) ++
                    }
                }
                return $i(cntName)
            }
        } elif (havingPred != null) {
            stmts |> push <| qmacro_block() {
                var $i(cntName) = 0
                for ($i(kvName) in values($i(tabName))) {
                    if ($e(havingPred)) {
                        $i(cntName) ++
                    }
                }
                return $i(cntName)
            }
        } else {
            stmts |> push <| qmacro_expr() {
                return length($i(tabName))
            }
        }
    } else {
        if (exprIsIterator) {
            retType = new TypeDecl(baseType = Type.tIterator)
            retType.firstType = clone_type(bufElemType)
        } else {
            retType = new TypeDecl(baseType = Type.tArray)
            retType.firstType = clone_type(bufElemType)
        }
        stmts |> push_from <| qmacro_block_to_array() {
            var $i(bufName) : array<$t(bufElemType)>
            $i(bufName) |> reserve(length($i(tabName)))
        }
        if (havingPred != null && trailingWherePred != null) {
            stmts |> push <| qmacro_expr() {
                for ($i(kvName) in values($i(tabName))) {
                    if ($e(havingPred)) {
                        let $i(outName) : $t(bufElemType) = $e(outputExpr)
                        if ($e(trailingWherePred)) {
                            $i(bufName) |> push_clone($i(outName))
                        }
                    }
                }
            }
        } elif (trailingWherePred != null) {
            stmts |> push <| qmacro_expr() {
                for ($i(kvName) in values($i(tabName))) {
                    let $i(outName) : $t(bufElemType) = $e(outputExpr)
                    if ($e(trailingWherePred)) {
                        $i(bufName) |> push_clone($i(outName))
                    }
                }
            }
        } elif (havingPred != null) {
            stmts |> push <| qmacro_expr() {
                for ($i(kvName) in values($i(tabName))) {
                    if ($e(havingPred)) {
                        $i(bufName) |> push_clone($e(outputExpr))
                    }
                }
            }
        } else {
            stmts |> push <| qmacro_expr() {
                for ($i(kvName) in values($i(tabName))) {
                    $i(bufName) |> push_clone($e(outputExpr))
                }
            }
        }
        if (orderName != "") {
            var inlineCmp = try_make_inline_cmp(orderKey, orderName, bufElemType, at)
            if (inlineCmp == null) return null
            stmts |> push <| qmacro_expr() {
                sort($i(bufName), $e(inlineCmp))
            }
        }
        stmts |> push(buffer_return(bufName, exprIsIterator))
    }
    return adapter->wrap_invoke(stmts, retType, false, at)
}

[macro_function]
def emit_group_by(var c : Captures; var ctx : EmitCtx; at : LineInfo) : Expression? {
    var gbCall = c.single["gb"]
    var keyBlock = clone_expression(gbCall.arguments[1])
    var groupProjCall = c.single["groupproj"]
    var havingCall : ExprCall?
    if (c.single |> key_exists("having")) {
        havingCall = c.single["having"]
    }
    var trailingWhereCall : ExprCall?
    if (c.single |> key_exists("trailing_where")) {
        trailingWhereCall = c.single["trailing_where"]
    }
    var terminatorName = ""
    if (c.single |> key_exists("term")) {
        terminatorName = c.single_name["term"]
    }
    var orderName = ""
    var orderKey : Expression?
    if (c.single |> key_exists("trailing_order")) {
        var orderCall = c.single["trailing_order"]
        orderName = c.single_name["trailing_order"]
        orderKey = clone_expression(orderCall.arguments[1])
    }
    var headCalls : array<tuple<ExprCall?; LinqCall?>>
    headCalls |> reserve(length(c.many["head"]))
    for (call in c.many["head"]) {
        let nm = call_norm_name(call)
        if (!(linqCalls |> key_exists(nm))) return null
        let lc = unsafe(linqCalls?[nm])
        headCalls |> push((call, lc))
    }
    var adapter = ctx.src->build_group_by_adapter(c, ctx, at)
    if (adapter == null) return null
    return plan_group_by_core(headCalls, keyBlock, groupProjCall, havingCall, trailingWhereCall,
        terminatorName, orderName, orderKey, ctx.expr_is_iterator, at, adapter)
}

[macro_function]
def build_group_by_rows {
    splice_patterns |> emplace <| SplicePattern(
        name = "group_by",
        chain <- [
            slot_chain_of(["where_", "select"], "head"),
            Slot(matcher = m_literal("join"),          cardinality = c_opt(), capture_name = "upstream_join", arity = 5),
            Slot(matcher = m_literal("group_by_lazy"), cardinality = c_one(), capture_name = "gb", arity = 2),
            Slot(matcher = m_literal("having_"),       cardinality = c_opt(), capture_name = "having"),
            Slot(matcher = m_literal("select"),        cardinality = c_one(), capture_name = "groupproj"),
            Slot(matcher = m_literal("where_"),        cardinality = c_opt(), capture_name = "trailing_where", arity = 2),
            Slot(matcher = m_alias("order_by_family"), cardinality = c_opt(), capture_name = "trailing_order", arity = 2),
            Slot(matcher = m_literal("count"),         cardinality = c_opt(), capture_name = "term", arity = 1)
        ],
        requires <- [@@ < RequiresPredicate > can_group_by_source, @@ < RequiresPredicate > decs_join_invariants, @@ < RequiresPredicate > no_order_with_count],
        emit = @@ < EmitFn > emit_group_by
    )
}

struct DecsBridgeShape {
    reqHashExpr : ExpressionPtr
    erqExpr     : ExpressionPtr
    archName    : string
    forExpr     : ExpressionPtr
    iterNames   : array<string>
    userNames   : array<string>
    elementType : TypeDeclPtr
}

[macro_function]
def get_call_short_name(c : ExprCall?) : string {
    let s = string(c.name)
    let bt = s |> rfind("`")
    if (bt >= 0) {
        let prev = s |> slice(0, bt) |> rfind("`")
        if (prev >= 0) return s |> slice(prev + 1, bt)
    }
    let cc = s |> rfind("::")
    if (cc >= 0) return s |> slice(cc + 2)
    return s
}

[macro_function]
def extract_decs_bridge(var top : Expression?) : DecsBridgeShape? {
    if (!(top is ExprInvoke)) return null
    var topInv = top as ExprInvoke
    if (topInv.arguments |> length != 1
            || !(topInv.arguments[0] is ExprMakeBlock)) return null
    var mblk = topInv.arguments[0] as ExprMakeBlock
    if (!(mblk._block is ExprBlock)) return null
    var blk = mblk._block as ExprBlock
    if (blk.list |> length != 3
            || !empty(blk.arguments)
            || !(blk.list[0] is ExprLet)
            || !(blk.list[1] is ExprCall)
            || !(blk.list[2] is ExprReturn)) return null
    var letStmt = blk.list[0] as ExprLet
    if (letStmt.variables |> length != 1) return null
    var resVar = letStmt.variables[0]
    if (!resVar._type.isGoodArrayType
            || resVar._type.firstType.baseType != Type.tTuple) return null
    let resVarName = string(resVar.name)
    var feaCall = blk.list[1] as ExprCall
    if (feaCall.name != "for_each_archetype"
            || feaCall.arguments |> length != 3
            || !(feaCall.arguments[0] is ExprConstUInt64)
            || !(feaCall.arguments[1] is ExprAddr)
            || !(feaCall.arguments[2] is ExprMakeBlock)) return null
    var feaBlkExpr = feaCall.arguments[2] as ExprMakeBlock
    if (!(feaBlkExpr._block is ExprBlock)) return null
    var feaBlk = feaBlkExpr._block as ExprBlock
    if (feaBlk.arguments |> length != 1
            || feaBlk.list |> length != 1
            || !(feaBlk.list[0] is ExprFor)) return null
    let archName = string(feaBlk.arguments[0].name)
    var forExpr = feaBlk.list[0] as ExprFor
    if (empty(forExpr.sources)
            || !(forExpr.body is ExprBlock)) return null
    var forBody = forExpr.body as ExprBlock
    if (forBody.list |> length != 1
            || !(forBody.list[0] is ExprCall)) return null
    var pushCall = forBody.list[0] as ExprCall
    if (get_call_short_name(pushCall) != "push"
            || pushCall.arguments |> length != 2
            || !(pushCall.arguments[0] is ExprVar)
            || (pushCall.arguments[0] as ExprVar).name != resVarName
            || !(pushCall.arguments[1] is ExprMakeTuple)) return null
    var mkTup = pushCall.arguments[1] as ExprMakeTuple
    if (mkTup.recordNames |> length != forExpr.sources |> length
            || mkTup.values |> length != forExpr.sources |> length) return null
    var iterNames : array<string>
    iterNames |> reserve(mkTup.values |> length)
    if ((forExpr.iteratorVariables |> length) == (forExpr.sources |> length)) {
        for (v in forExpr.iteratorVariables) {
            iterNames |> push(string(v.name))
        }
    } else {
        for (v in mkTup.values) {
            var inner = v
            if (inner is ExprRef2Value) {
                inner = (inner as ExprRef2Value).subexpr
            }
            if (!(inner is ExprVar)) return null
            iterNames |> push(string((inner as ExprVar).name))
        }
    }
    var userNames <- [for (n in mkTup.recordNames); string(n)]
    var retStmt = blk.list[2] as ExprReturn
    if (retStmt.subexpr == null || !(retStmt.subexpr is ExprCall)) return null
    let retCall = retStmt.subexpr as ExprCall
    if (get_call_short_name(retCall) != "to_sequence"
            || empty(retCall.arguments)
            || !(retCall.arguments[0] is ExprVar)
            || (retCall.arguments[0] as ExprVar).name != resVarName) return null
    var info = new DecsBridgeShape(
        reqHashExpr = clone_expression(feaCall.arguments[0]),
        erqExpr = clone_expression(feaCall.arguments[1]),
        archName := archName,
        forExpr = clone_expression(forExpr),
        iterNames <- iterNames,
        userNames <- userNames,
        elementType = clone_type(resVar._type.firstType)
    )
    return info
}

[macro_function]
def emit_default_bind(name : string; argExpr : Expression?; canCopy : bool) : Expression? {
    var out : Expression?
    if (canCopy) {
        out = qmacro_expr() {
            let $i(name) = $e(argExpr)
        }
    } else {
        out = qmacro_expr() {
            var $i(name) <- clone_to_move($e(argExpr))
        }
    }
    return out
}

[macro_function]
def emit_default_return(name : string; canCopy : bool) : Expression? {
    var out : Expression?
    if (canCopy) {
        out = qmacro_expr() {
            return $i(name)
        }
    } else {
        out = qmacro_expr() {
            return <- clone_to_move($i(name))
        }
    }
    return out
}

[macro_function]
def is_primitive_join_key_type(keyType : TypeDeclPtr) : bool {
    return (keyType != null
        && (keyType.baseType == Type.tInt || keyType.baseType == Type.tInt64
            || keyType.baseType == Type.tUInt || keyType.baseType == Type.tUInt64
            || keyType.baseType == Type.tInt8 || keyType.baseType == Type.tUInt8
            || keyType.baseType == Type.tInt16 || keyType.baseType == Type.tUInt16
            || keyType.baseType == Type.tString
            || keyType.baseType == Type.tFloat || keyType.baseType == Type.tDouble
            || keyType.baseType == Type.tBool))
}

struct UpstreamJoinCore {
    keyaLam    : Expression?
    keybLam    : Expression?
    resultLam  : Expression?
    keyType    : TypeDeclPtr
    resultType : TypeDeclPtr
}

[macro_function]
def extract_upstream_join_core(var joinCall : ExprCall?) : UpstreamJoinCore? {
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
    return new UpstreamJoinCore(keyaLam = keyaLam, keybLam = keybLam, resultLam = resultLam,
                                keyType = keyType,
                                resultType = strip_const_ref(clone_type(resultLam._type.firstType)))
}

struct UpstreamJoinArraySrcB {
    srcbClone : Expression?
    srcbType  : TypeDeclPtr
    tupBType  : TypeDeclPtr
}

[macro_function]
def extract_upstream_join_array_srcb(var joinCall : ExprCall?) : UpstreamJoinArraySrcB? {
    var srcbSrc = joinCall.arguments[1]
    if (srcbSrc == null || srcbSrc._type == null || srcbSrc._type.firstType == null
            || !(srcbSrc._type.isGoodArrayType || srcbSrc._type.isArray || srcbSrc._type.isIterator)) {
        return null
    }
    var srcbClone = clone_expression(srcbSrc)
    srcbClone.genFlags.alwaysSafe = true
    return new UpstreamJoinArraySrcB(srcbClone = srcbClone,
                                     srcbType = invoke_src_param_type(srcbSrc),
                                     tupBType = strip_const_ref(clone_type(srcbSrc._type.firstType)))
}

struct JoinStandalonePieces {
    preludeStmts  : array<Expression?>
    probeOuter    : Expression?
    returnStmt    : Expression?
    invokeRetType : TypeDeclPtr
}

// nolint:STYLE014 — builds the prelude / probe-bucket / return for a hashed join in all 6 shape variants (caller
// owns source extraction, tup-bind, source-loop wrapping + the invoke wrap; see emit_array_join / emit_decs_join_impl
// / emit_xml_join). probeOuter is always `get(hash, keya(a), $(var arr) { <body> })`. By shape:
//   count, no where  → prelude `var cnt = 0`;         body `cnt += length(arr)`;                                  return `cnt`
//   count + where    → prelude `var cnt = 0`;         body `for (b in arr) { let res = result(a,b); if (where) cnt ++ }`
//   to_array, no bind → prelude `var buf : array<R>`; body `for (b in arr) { buf |> push_clone(result(a,b)) }`;    return `<- buf`
//   + select          → body `for (b in arr) { let res = result(a,b); buf |> push_clone(select(res)) }`
//   + where           → body `for (b in arr) { let res = result(a,b); if (where) buf |> push_clone(res) }`
//   + select + where  → body `for (b in arr) { let res = result(a,b); if (where) buf |> push_clone(select(res)) }`
// The optional leading `_where` of join_general — the predicate lambda filtering srcA before the join.
// Returns null when absent OR malformed; callers that need to bail on a malformed slot re-check key_exists.
[macro_function]
def extract_join_lead_where(var c : Captures) : Expression? {
    if (!(c.single |> key_exists("lead_where"))) return null
    var call = c.single["lead_where"]
    return call != null ? call.arguments[1] : null
}

struct JoinPairCore {
    preludeStmts  : array<Expression?>
    pairStmts     : array<Expression?>
    returnStmt    : Expression?
    invokeRetType : TypeDeclPtr
    cntName       : string
}

[macro_function]
def build_join_pair_core(
                         var joinCall  : ExprCall?;
                         var whereLam  : Expression?;
                         var selectLam : Expression?;
                         countOnly     : bool;
                         tupAName      : string;
                         bElemName     : string;
                         namePrefix    : string;
                         at            : LineInfo
                         ) : JoinPairCore? {
    let cntName     = qn(namePrefix + "_cnt", at)
    let bufName     = qn(namePrefix + "_buf", at)
    let resBindName = qn(namePrefix + "_res", at)
    var preludeStmts : array<Expression?>
    var pairStmts : array<Expression?>
    var returnStmt : Expression?
    var invokeRetType : TypeDeclPtr
    if (countOnly) {
        invokeRetType = new TypeDecl(baseType = Type.tInt, at = at)
        preludeStmts |> push <| qmacro_expr() {
            var $i(cntName) : int = 0
        }
        if (whereLam == null) {
            pairStmts |> push <| qmacro_expr() {
                $i(cntName) ++
            }
        } else {
            var resultLam = joinCall.arguments[4]
            if (resultLam == null || resultLam._type == null || resultLam._type.firstType == null) return null
            var resultBody = peel_lambda_rename_2vars(resultLam, tupAName, bElemName)
            if (resultBody == null) return null
            let joinResultType = strip_const_ref(clone_type(resultLam._type.firstType))
            var wherePred = peel_lambda_replace_var(whereLam, qmacro($i(resBindName)))
            pairStmts |> push_from <| qmacro_block_to_array() {
                let $i(resBindName) : $t(joinResultType) = $e(resultBody)
                if ($e(wherePred)) {
                    $i(cntName) ++
                }
            }
        }
        returnStmt = qmacro_expr() {
            return $i(cntName)
        }
        return <- new JoinPairCore(preludeStmts <- preludeStmts, pairStmts <- pairStmts,
                                   returnStmt = returnStmt, invokeRetType = invokeRetType, cntName = cntName)
    }
    var resultLam = joinCall.arguments[4]
    if (resultLam == null || resultLam._type == null || resultLam._type.firstType == null) return null
    var resultBody = peel_lambda_rename_2vars(resultLam, tupAName, bElemName)
    if (resultBody == null) return null
    var resultType : TypeDeclPtr
    if (selectLam != null) {
        var selBody = peel_lambda_single_return(selectLam)
        if (selBody == null || selBody._type == null) return null
        resultType = strip_const_ref(clone_type(selBody._type))
    } else {
        resultType = strip_const_ref(clone_type(resultLam._type.firstType))
    }
    if (resultType == null) return null
    invokeRetType = new TypeDecl(baseType = Type.tArray, firstType = clone_type(resultType), at = at)
    preludeStmts |> push <| qmacro_expr() {
        var $i(bufName) : array<$t(resultType)>
    }
    let needBind = selectLam != null || whereLam != null
    if (needBind) {
        let joinResultType = strip_const_ref(clone_type(resultLam._type.firstType))
        var pushExpr : Expression?
        if (selectLam != null) {
            var projBody = peel_lambda_replace_var(selectLam, qmacro($i(resBindName)))
            pushExpr = qmacro($i(bufName) |> push_clone($e(projBody)))
        } else {
            pushExpr = qmacro($i(bufName) |> push_clone($i(resBindName)))
        }
        if (whereLam != null) {
            var wherePred = peel_lambda_replace_var(whereLam, qmacro($i(resBindName)))
            pairStmts |> push_from <| qmacro_block_to_array() {
                let $i(resBindName) : $t(joinResultType) = $e(resultBody)
                if ($e(wherePred)) {
                    $e(pushExpr)
                }
            }
        } else {
            pairStmts |> push_from <| qmacro_block_to_array() {
                let $i(resBindName) : $t(joinResultType) = $e(resultBody)
                $e(pushExpr)
            }
        }
    } else {
        pairStmts |> push <| qmacro($i(bufName) |> push_clone($e(resultBody)))
    }
    returnStmt = qmacro_expr() {
        return <- $i(bufName)
    }
    return <- new JoinPairCore(preludeStmts <- preludeStmts, pairStmts <- pairStmts,
                               returnStmt = returnStmt, invokeRetType = invokeRetType, cntName = "")
}

[macro_function]
def build_join_standalone_pieces(
                                 var joinCall   : ExprCall?;
                                 var whereLam   : Expression?;
                                 var selectLam  : Expression?;
                                 var leadWhereLam : Expression?;
                                 countOnly      : bool;
                                 var keyaBody   : Expression?;
                                 tupAName       : string;
                                 hashName       : string;
                                 tupBType       : TypeDeclPtr;
                                 isGroupJoin    : bool;
                                 namePrefix     : string;
                                 at             : LineInfo
                                 ) : JoinStandalonePieces? {
    let bElemName   = qn(namePrefix + "_b", at)
    let arrName     = qn(namePrefix + "_arr", at)
    let bufName     = qn(namePrefix + "_buf", at)
    let emptyArrName = qn(namePrefix + "_empty", at)
    var preludeStmts : array<Expression?>
    var probeInnerStmts : array<Expression?>
    var returnStmt : Expression?
    var invokeRetType : TypeDeclPtr
    var groupEmptyExpr : Expression?
    if (!isGroupJoin) {
        var core = build_join_pair_core(joinCall, whereLam, selectLam, countOnly, tupAName, bElemName, namePrefix, at)
        if (core == null) return null
        preludeStmts |> push_from(core.preludeStmts)
        returnStmt = core.returnStmt
        invokeRetType = core.invokeRetType
        if (countOnly && whereLam == null) {
            let cntName = core.cntName
            probeInnerStmts |> push <| qmacro_expr() {
                $i(cntName) += length($i(arrName))
            }
        } else {
            var pairStmts <- core.pairStmts
            probeInnerStmts |> push <| qmacro_expr() {
                for ($i(bElemName) in $i(arrName)) {
                    $b(pairStmts)
                }
            }
        }
    } else {
        var resultLam = joinCall.arguments[4]
        if (resultLam == null || resultLam._type == null || resultLam._type.firstType == null) return null
        var resultBody = peel_lambda_rename_2vars(resultLam, tupAName, arrName)
        if (resultBody == null) return null
        let resultType = strip_const_ref(clone_type(resultLam._type.firstType))
        if (resultType == null) return null
        invokeRetType = new TypeDecl(baseType = Type.tArray, firstType = clone_type(resultType), at = at)
        preludeStmts |> push <| qmacro_expr() {
            var $i(bufName) : array<$t(resultType)>
        }
        probeInnerStmts |> push <| qmacro_expr() {
            $i(bufName) |> push_clone($e(resultBody))
        }
        var resultBodyEmpty = peel_lambda_rename_2vars(resultLam, tupAName, emptyArrName)
        if (resultBodyEmpty == null) return null
        groupEmptyExpr = qmacro($i(bufName) |> push_clone($e(resultBodyEmpty)))
        returnStmt = qmacro_expr() {
            return <- $i(bufName)
        }
    }
    var probeOuter <- qmacro_expr() {
        get($i(hashName), $e(keyaBody), $(var $i(arrName) : array<$t(tupBType)>) {
            $b(probeInnerStmts)
        })
    }
    if (isGroupJoin) {
        var getProbe = probeOuter
        probeOuter = qmacro_expr() {
            if (!$e(getProbe)) {
                var $i(emptyArrName) : array<$t(tupBType)>
                $e(groupEmptyExpr)
            }
        }
    }
    if (leadWhereLam != null) {
        var leadPred = peel_lambda_rename_var(leadWhereLam, tupAName)
        if (leadPred == null) return null
        var probeInner = probeOuter
        probeOuter = qmacro_expr() {
            if ($e(leadPred)) {
                $e(probeInner)
            }
        }
    }
    return <- new JoinStandalonePieces(
        preludeStmts <- preludeStmts,
        probeOuter <- probeOuter,
        returnStmt = returnStmt,
        invokeRetType = invokeRetType
    )
}

[macro_function]
def join_hash_decl(hashName : string; keyType, tupBType : TypeDeclPtr) : Expression? {
    //! The join's build-side buckets, rebuilt per call: `inscope` frees them at scope exit, unless a bucket holds pointers its finalizer would free
    if (tupBType.isSafeToDelete) {
        return qmacro_expr() {
            var inscope $i(hashName) : table<$t(keyType); array<$t(tupBType)>>
        }
    }
    return qmacro_expr() {
        var $i(hashName) : table<$t(keyType); array<$t(tupBType)>>
    }
}

[macro_function]
def join_srcb_table_call(var joinCall : ExprCall?) : ExprCall? {
    if (joinCall == null || (joinCall.arguments |> length) < 2) return null
    var srcb = joinCall.arguments[1]
    if (srcb == null || !(srcb is ExprCall)) return null
    var call = srcb as ExprCall
    let name = get_call_short_name(call)
    if ((name != "each_kv" && name != "keys")
            || call._type == null || !call._type.isIterator || call._type.firstType == null
            || (call.arguments |> length) != 1) {
        return null
    }
    let srcT = call.arguments[0]._type
    if (srcT == null || !srcT.isGoodTableType) return null
    return call
}

[macro_function]
def is_bare_key_ref(keyBody : Expression?; bindName : string; kvLane : bool) : bool {
    match (keyBody) {
        if (ExprField(name = "key", value = ExprVar(name = match_expr(bindName)))) {
            return kvLane
        }
        if (ExprVar(name = match_expr(bindName))) {
            return !kvLane
        }
        if (_) {
            return false
        }
    }
    return false
}

[macro_function]
def build_join_probe_pieces(
                            var joinCall   : ExprCall?;
                            var whereLam   : Expression?;
                            var selectLam  : Expression?;
                            var leadWhereLam : Expression?;
                            countOnly      : bool;
                            var keyaBody   : Expression?;
                            tupAName       : string;
                            tabSrcName     : string;
                            srcbKv         : bool;
                            namePrefix     : string;
                            at             : LineInfo
                            ) : JoinStandalonePieces? {
    let bElemName = qn(namePrefix + "_b", at)
    let kName     = qn(namePrefix + "_k", at)
    let pName     = qn(namePrefix + "_p", at)
    var core = build_join_pair_core(joinCall, whereLam, selectLam, countOnly, tupAName, bElemName, namePrefix, at)
    if (core == null) return null
    var pairStmts <- core.pairStmts
    var probeOuter : Expression?
    if (countOnly && whereLam == null) {
        probeOuter = qmacro_expr() {
            if (key_exists($i(tabSrcName), $e(keyaBody))) {
                $b(pairStmts)
            }
        }
    } elif (srcbKv) {
        let vName = qn(namePrefix + "_v", at)
        var pairBlock = qmacro_block() {
            $b(pairStmts)
        }
        var (allUsed, usedFields) = collect_row_usage(pairBlock, bElemName)
        allUsed = needs_full_row_bind(usedFields, allUsed)
        if (allUsed) {
            probeOuter = qmacro_block() {
                let $i(kName) = $e(keyaBody)
                let $i(pName) = unsafe($i(tabSrcName)?[$i(kName)])
                if ($i(pName) != null) {
                    let $i(bElemName) = (key = $i(kName), value = *$i(pName))
                    $e(pairBlock)
                }
            }
        } else {
            var fieldToLocal <- { "key" => kName, "value" => vName }
            var flatBlock = flatten_row_to_locals(pairBlock, bElemName, fieldToLocal)
            if (usedFields |> has_value("value")) {
                probeOuter = qmacro_block() {
                    let $i(kName) = $e(keyaBody)
                    let $i(pName) = unsafe($i(tabSrcName)?[$i(kName)])
                    if ($i(pName) != null) {
                        let $i(vName) & = unsafe(*$i(pName))
                        $e(flatBlock)
                    }
                }
            } else {
                probeOuter = qmacro_block() {
                    let $i(kName) = $e(keyaBody)
                    if (key_exists($i(tabSrcName), $i(kName))) {
                        $e(flatBlock)
                    }
                }
            }
        }
    } else {
        probeOuter = qmacro_block() {
            let $i(kName) = $e(keyaBody)
            if (key_exists($i(tabSrcName), $i(kName))) {
                let $i(bElemName) = $i(kName)
                $b(pairStmts)
            }
        }
    }
    if (leadWhereLam != null) {
        var leadPred = peel_lambda_rename_var(leadWhereLam, tupAName)
        if (leadPred == null) return null
        var probeInner = probeOuter
        probeOuter = qmacro_expr() {
            if ($e(leadPred)) {
                $e(probeInner)
            }
        }
    }
    return <- new JoinStandalonePieces(
        preludeStmts <- core.preludeStmts,
        probeOuter <- probeOuter,
        returnStmt = core.returnStmt,
        invokeRetType = core.invokeRetType
    )
}

[macro_function]
def build_join_rows {
    splice_patterns |> emplace <| SplicePattern(
        name = "join_general",
        chain <- [
            Slot(matcher = m_literal("where_"),  cardinality = c_opt(), capture_name = "lead_where", arity = 2),
            Slot(matcher = SlotMatcher(one_of <- ["join", "group_join"]), cardinality = c_one(), capture_name = "join", arity = 5),
            Slot(matcher = m_literal("where_"),  cardinality = c_opt(), capture_name = "trailing_where", arity = 2),
            Slot(matcher = m_literal("select"),  cardinality = c_opt(), capture_name = "trailing_select"),
            Slot(matcher = m_literal("count"),   cardinality = c_opt(), capture_name = "term", arity = 1)
        ],
        requires <- [@@ < RequiresPredicate > can_join_source, @@ < RequiresPredicate > join_no_select_with_count],
        emit = @@ < EmitFn > emit_join
    )
}

[macro_function]
def emit_join(var c : Captures; var ctx : EmitCtx; at : LineInfo) : Expression? {
    return ctx.src->emit_join_hook(c, ctx, at)
}

[macro_function]
def emit_array_join(var c : Captures; var ctx : EmitCtx; at : LineInfo) : Expression? {
    // nolint:STYLE014 — emits a hashed equi-join as a 2-source invoke (mirrors Zip's 2-source wrap):
    // build a hash from srcB, probe it with srcA. The lead loop and param spelling come from the adapter
    // (wrap_source_loop / invoke_param_type), so any direct-return loop source rides this — array `for`,
    // table slot walk. Generated code (canonical no-count/no-where/no-select shape, array lead):
    //   invoke(
    //       (srcA, srcB) {
    //           var buf  : array<Result>
    //           var hash : table<Key; array<B>>
    //           for (b in srcB) {                          // build: bucket every B row under its key
    //               hash[keyb(b)] |> push_clone(b)
    //           }
    //           for (a in srcA) {                          // probe: for each A, walk its matching bucket
    //               get(hash, keya(a), $(var bucket) {
    //                   for (b in bucket) { buf |> push_clone(result(a, b)) }
    //               })
    //           }
    //           return <- buf
    //       },
    //       topSrc, srcbSrc)
    // Variants (prelude / probe-body / return are supplied by build_join_standalone_pieces):
    //   count()    → `var cnt = 0`; probe body is `cnt += length(bucket)` (bucket-length fast path, no per-pair loop); `return cnt`
    //   where(p)   → per pair: `let res = result(a, b); if (p(res)) { ...push/incr... }`
    //   select(f)  → push `f(res)` instead of `res` (buffer element type = f's return type)
    // Table-srcB probe: when srcB is `each_kv(tab)` / `keys(tab)` joined on its bare key (inner join only),
    // there is no hash/build loop — srcB binds the table itself and the per-A probe is a key lookup
    // (build_join_probe_pieces). Unique table keys make probe ≡ hash semantics exactly.
    var joinCall = c.single["join"]
    if (!srcb_is_array_shaped(c, "join")) return null
    let isGroupJoin = c.single_name |> key_exists("join") && c.single_name["join"] == "group_join"
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
    if ((isGroupJoin && (countOnly || whereLam != null || selectLam != null)) || (!countOnly && ctx.expr_is_iterator)) return null
    var topSrc = ctx.src->loop_source_expr()
    if (topSrc == null || topSrc._type == null || topSrc._type.firstType == null) return null
    var srcbSrc = joinCall.arguments[1]
    if (srcbSrc == null || srcbSrc._type == null || srcbSrc._type.firstType == null) return null
    var keyaLam = joinCall.arguments[2]
    var keybLam = joinCall.arguments[3]
    if (keyaLam == null || keybLam == null || keyaLam._type == null) return null
    let keyType = strip_const_ref(clone_type(keyaLam._type.firstType))
    if (!is_primitive_join_key_type(keyType)) return null
    let srcAName  = ctx.src->loop_source_name()
    let srcBName  = qn("ajoin_srcB", at)
    let tupAName  = ctx.src->bind_name(at)
    let tupBName  = qn("ajoin_tup_b", at)
    let hashName  = qn("ajoin_hash", at)
    var keyaBody = peel_lambda_rename_var(keyaLam, tupAName)
    var keybBody = peel_lambda_rename_var(keybLam, tupBName)
    if (keyaBody == null || keybBody == null) return null
    let tupBType = strip_const_ref(clone_type(srcbSrc._type.firstType))
    var srcbTab = join_srcb_table_call(joinCall)
    let srcbKv = srcbTab != null && get_call_short_name(srcbTab) == "each_kv"
    let probeMode = srcbTab != null && !isGroupJoin && is_bare_key_ref(keybBody, tupBName, srcbKv)
    var pieces : JoinStandalonePieces?
    if (probeMode) {
        pieces = build_join_probe_pieces(joinCall, whereLam, selectLam, leadWhereLam, countOnly, keyaBody, tupAName, srcBName, srcbKv, "ajoin", at)
    } else {
        pieces = build_join_standalone_pieces(joinCall, whereLam, selectLam, leadWhereLam, countOnly, keyaBody, tupAName, hashName, tupBType, isGroupJoin, "ajoin", at)
    }
    if (pieces == null) return null
    var allStmts := pieces.preludeStmts
    if (!probeMode) {
        allStmts |> push(join_hash_decl(hashName, keyType, tupBType))
        allStmts |> push <| qmacro_expr() {
            for ($i(tupBName) in $i(srcBName)) {
                // nolint:PERF006,LINT019 per-key bucket size unknown ahead of time
                $i(hashName)[$e(keybBody)] |> push_clone($i(tupBName))
            }
        }
    }
    var probeLoop = ctx.src->wrap_source_loop(LoopDispatch(Each = null), pieces.probeOuter, at)
    if (probeLoop == null) return null
    allStmts |> push(probeLoop)
    allStmts |> push(pieces.returnStmt)
    var invokeRetType = pieces.invokeRetType
    var topClone = clone_expression(topSrc)
    topClone.genFlags.alwaysSafe = true
    var srcbArg = probeMode ? srcbTab.arguments[0] : srcbSrc
    var srcbClone = clone_expression(srcbArg)
    srcbClone.genFlags.alwaysSafe = true
    let srcAParamType = ctx.src->invoke_param_type()
    var srcBParamType : TypeDeclPtr
    if (probeMode) {
        srcBParamType = strip_const_ref(clone_type(srcbArg._type))
        srcBParamType.flags.constant = true
    } else {
        srcBParamType = invoke_src_param_type(srcbSrc)
    }
    var emission = qmacro(invoke(
        $($i(srcAName) : $t(srcAParamType), $i(srcBName) : $t(srcBParamType)) : $t(invokeRetType) {
            $b(allStmts)
        },
        $e(topClone), $e(srcbClone)))
    return finalize_invoke(emission, at)
}

