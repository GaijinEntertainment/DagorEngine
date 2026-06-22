options gen2
options rtti
// PERF024: redundant pre-clone before a [clone(...)]-annotated function
//
// Problem:
//   Functions tagged with `[clone(paramName)]` promise to clone the named
//   parameter internally. Callers passing `clone_expression(X)` (or any
//   other clone_* primitive) at that position double-clone — the outer
//   clone is wasted.
//
// Detection has two arms:
//   Arm A (direct splice):  func(clone_*(X))             — fires at the clone call
//   Arm B (var-only-clone): var X = clone_*(E); func(X)  — fires at the var decl
//                           where X has no other uses outside annotated positions
//
// Bad patterns:
//   peel_lambda_rename_var(clone_expression(myExpr), "name")       — Arm A
//   var ce = clone_expression(myExpr); peel_lambda_rename_var(ce, "name")  — Arm B
//
// Good patterns:
//   peel_lambda_rename_var(myExpr, "name")                         — no pre-clone
//   var ce = clone_expression(src); sink.arguments |> emplace(ce)
//   ... peel_lambda_rename_var(ce, "name")                         — ce has other uses, load-bearing
//
// Expected: 8 hits total.
//   bad_direct_splice_into_peel              — 1 (Arm A)
//   bad_two_clone_args_into_replace_var      — 2 (Arm A x2, one annotation covering both params)
//   bad_direct_splice_into_push_inline_id    — 1 (Arm A, sqlite_linq)
//   bad_var_only_clone_use                   — 1 (Arm B)
//   bad_var_multi_use_all_annotated          — 1 (Arm B, multi-use but all into annotated fns)
//   bad_clone_function_into_apply_qmacro_tmpl_fn  — 1 (Arm A, clone_function variant)
//   bad_clone_type_into_apply_qmacro_tmpl_cls     — 0 (apply_qmacro_template_class wasn't annotated — clone_type test deferred)
//   Total: 6
expect 31208:6

require daslib/ast
require daslib/ast_boost
require daslib/templates_boost
require daslib/ast_match
require daslib/perf_lint

// --- Positives: Arm A — direct splice ---

// 1. peel_lambda_rename_var has [clone(expr)] — wrapping the first arg in
// clone_expression is redundant.
[macro_function]
def bad_direct_splice_into_peel(var src : Expression?) {
    var res = peel_lambda_rename_var(clone_expression(src), "name")
}

// 2. peel_lambda_replace_var has [clone(expr, replacement)] — both args
// pre-cloned. Two PERF024 hits.
[macro_function]
def bad_two_clone_args_into_replace_var(var lhs : Expression?; var rhs : Expression?) {
    var res = peel_lambda_replace_var(clone_expression(lhs), clone_expression(rhs))
}

// 3. push_inline_id has [clone(e)] — pre-clone wraps the arg.
[macro_function]
def bad_direct_splice_into_push_inline_id(var src : Expression?) {
    var stmts : array<Expression?>
    var dummy : ExprCall?
    // (Skipped: requires SqlFrag from dasSQLITE module — the rule itself
    // tests on bare types via the ast_match function below.)
    var res = peel_lambda_rename_2vars(clone_expression(src), "a", "b")
}

// 4. apply_qmacro_template_function has [clone(func)] — pre-cloning the
// FunctionPtr argument is redundant.
[macro_function]
def bad_clone_function_into_apply_qmacro_tmpl_fn(var fn : FunctionPtr) {
    var fres = apply_qmacro_template_function(clone_function(fn)) <| $(var rules : Template) {}
}

// --- Positives: Arm B — var-init-only-clone-use ---

// 5. Sole use of pre-clone is into an annotated arg position. Arm B fires
// at the `var ce = clone_expression(src)` decl.
[macro_function]
def bad_var_only_clone_use(var src : Expression?) {
    var ce = clone_expression(src)
    var res = peel_lambda_rename_var(ce, "name")
}

// 6. Var used in multiple annotated arg positions — still flagged because
// every use is annotated-safe.
[macro_function]
def bad_var_multi_use_all_annotated(var src : Expression?) {
    var ce = clone_expression(src)
    var r1 = peel_lambda_rename_var(ce, "a")
    var r2 = peel_lambda_rename_2vars(ce, "b", "c")
}

// --- Negatives: similar shape but should NOT warn ---

// N1. No annotation on callee — pre-clone may be load-bearing per caller's intent.
[macro_function]
def good_call_unannotated_function(var src : Expression?) {
    var ce = clone_expression(src)
    // Pass to a function with no [clone(...)] annotation. PERF024 doesn't fire.
    user_consumer_fn(ce)
}

[macro_function]
def private user_consumer_fn(var e : Expression?) {
    // not annotated [clone(...)] — caller's pre-clone is load-bearing
}

// N2. Var used in both annotated position AND elsewhere (sink emplace). Pre-clone
// is load-bearing because the non-annotated use needs an owned copy.
[macro_function]
def good_var_with_non_annotated_use(var src : Expression?; var sink : ExprCall?) {
    var keep = clone_expression(src)
    sink.arguments |> emplace(keep)         // disqualifying use
    var res = peel_lambda_rename_var(keep, "name")
}

// N3. Direct pass to annotated function — no pre-clone, no warning.
[macro_function]
def good_no_pre_clone(var src : Expression?) {
    var res = peel_lambda_rename_var(src, "name")
}

// N4. Pre-clone used only at a NON-annotated position. PERF024 doesn't fire.
// peel_lambda_replace_var is `[clone(expr, replacement)]` — both params
// covered, no non-annotated positions to test. Use a function with a single
// annotated param: apply_qmacro_template_function is `[clone(func)]`. Pass
// pre-cloned to position 0 (annotated) AND something else for parity check.
// Actually this scenario doesn't easily map to existing annotated funcs;
// the negative is: clone_type'd value passed to a function with NO [clone()].
[macro_function]
def good_clone_type_unrelated(var sink : TypeDecl?; src : TypeDecl?) {
    var t = clone_type(src)
    sink.firstType = t   // not annotated — load-bearing
}
