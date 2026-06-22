options gen2
options rtti
// PERF023: var X = clone_expression(E); ... $e(X) ... — qmacro splice already clones
//
// Problem:
//   The qmacro / qmacro_block / qmacro_expr / qmacro_block_to_array family
//   internally goes through apply_template (templates_boost.das), which calls
//   clone_expression on every substitution input. Pre-cloning into a local
//   variable and then splicing the local via $e(...) wastes one clone per
//   splice site for zero correctness benefit.
//
// Detection (post-expansion form the lint sees):
//   After [call_macro] expansion, `qmacro_expr() { ... $e(X) ... }` becomes
//   `apply_qblock_expr(ExprQuote(...), ExprMakeBlock { _block = ExprBlock {
//   list = [ ... replaceVariable(rules, "temp`var`N", add_ptr_ref(X)), ... ] }})`.
//   The `add_ptr_ref(...)` wrapper is the lint's reliable signal — apply_qrules
//   installs it at every $e splice site and at no other place.
//
// Bad pattern:
//   var defaultExpr = clone_expression(terminatorCall.arguments[1])
//   stmts |> push <| qmacro_expr() {
//       let $i(dvalName) = $e(defaultExpr)
//   }
//
// Good pattern:
//   stmts |> push <| qmacro_expr() {
//       let $i(dvalName) = $e(terminatorCall.arguments[1])
//   }
//
// Multi-use is still safe to flag — apply_template clones on EVERY substitution,
// so 3 splice sites get 3 fresh clones whether you pre-clone once or not.
// For multi-use cases where the source expression has side effects, prefer
// `let baseE = E; ... $e(baseE) ...` (1 eval, N clones) over inlining E N
// times. For pure reads (variables, fields, array indexing of existing AST),
// inlining is equivalent.

// Expected: 6 hits total.
//   bad_single_use_clone     — 1 (single clone, single splice)
//   bad_multi_use_clone      — 3 (three clones of the same source, three splices)
//   bad_nested_splice        — 1 (clone, then splice into a nested expression)
//   bad_bound_name_pattern   — 1 (clone, splice via $i-bound name)
expect 31208:6

require daslib/ast
require daslib/ast_boost
require daslib/templates_boost
require daslib/perf_lint

// --- Positives: 4 sites should fire ---

// 1. Single use, single splice — the canonical case.
[macro_function]
def bad_single_use_clone(var stmts : array<Expression?>; var src : Expression?) {
    var defaultExpr = clone_expression(src)
    stmts |> push <| qmacro_expr() {
        return $e(defaultExpr)
    }
}

// 2. Multi-use, all-splice — three pre-clones for three splice sites.
[macro_function]
def bad_multi_use_clone(var stmts : array<Expression?>; var src : Expression?) {
    var a = clone_expression(src)
    var b = clone_expression(src)
    var c = clone_expression(src)
    stmts |> push <| qmacro_expr() {
        let _x = $e(a) <= 0 ? 0 : ($e(b) < 10 ? $e(c) : 10)
    }
}

// 3. Splice inside a nested expression (still inside add_ptr_ref subtree).
[macro_function]
def bad_nested_splice(var stmts : array<Expression?>; var src : Expression?) {
    var inner = clone_expression(src)
    stmts |> push <| qmacro_expr() {
        let _y = some_helper_fn($e(inner), 42)
    }
}

// 4. Bound-name pattern via $i — the exact shape from linq_fold's emit_*_lane
// helpers (let $i(name) = $e(bound)).
[macro_function]
def bad_bound_name_pattern(var stmts : array<Expression?>; var src : Expression?) {
    let name = "dval"
    var bound = clone_expression(src)
    stmts |> push <| qmacro_expr() {
        let $i(name) = $e(bound)
    }
}

// --- Negatives: similar shape but should NOT warn ---

// N1. Init is NOT clone_expression — produces an Expression via direct
// allocation. The var is spliced into a qmacro, but PERF023 only fires when
// the initializer is `clone_expression(...)`.
[macro_function]
def good_no_clone(var stmts : array<Expression?>; at : LineInfo) {
    var direct : Expression? = new ExprConstInt(at = at, value = 42)
    stmts |> push <| qmacro_expr() {
        return $e(direct)
    }
}

// N2. Var consumed by non-splice consumer too. apply_template clones for the
// splice; the assignment still needs its own owned copy, so the pre-clone is
// load-bearing.
[macro_function]
def good_partial_use(var stmts : array<Expression?>; src : Expression?;
                     var sink : ExprCall?) {
    var keep = clone_expression(src)
    sink.arguments |> emplace(keep)
    stmts |> push <| qmacro_expr() {
        return $e(keep)
    }
}

// N3. Pre-clone but never referenced inside a splice — no warning to fire
// (safe_uses < 1). The pre-clone may still be wasted, but PERF023 isn't the
// rule to flag it (it'd need a different "unused clone" rule).
[macro_function]
def good_no_splice_use(var sink : ExprCall?; src : Expression?) {
    var keep = clone_expression(src)
    sink.arguments |> emplace(keep)
}

// N4. Init is clone_type, not clone_expression — out of PERF023 scope.
// clone_type doesn't pair with the add_ptr_ref wrapper (types take a
// different path through apply_qrules: $<TT> tags emit add_type_ptr_ref,
// but clone_type call sites typically feed direct AST construction, not
// qmacro splices).
[macro_function]
def good_clone_type_unrelated(var sink : TypeDecl?; src : TypeDecl?) {
    var t = clone_type(src)
    sink.firstType = t
}
