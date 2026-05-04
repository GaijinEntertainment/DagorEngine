options gen2
// PERF008: Unnecessary get_ptr() for is/as type checks
//
// Problem:
//   Calling get_ptr() before is/as is unnecessary because smart_ptr
//   supports type checks directly for Expression and TypeDecl types.
//
// Bad pattern:
//   if (get_ptr(expr) is ExprVar) { ... }
//   var v = get_ptr(expr) as ExprVar
//
// Good pattern:
//   if (expr is ExprVar) { ... }
//   var v = expr as ExprVar

expect 40217:2

require daslib/perf_lint
require daslib/ast_boost

// --- Bad patterns (warnings) ---

def bad_is_with_get_ptr(expr : ExpressionPtr) : bool {
    return get_ptr(expr) is ExprVar         // PERF008
}

def bad_as_with_get_ptr(expr : ExpressionPtr) {
    var v = get_ptr(expr) as ExprCall       // PERF008
    print("{v}\n")
}

// --- Good patterns (no warnings) ---

def good_is_direct(expr : ExpressionPtr) : bool {
    return expr is ExprVar
}

def good_as_direct(expr : ExpressionPtr) {
    var v = expr as ExprCall
    print("{v}\n")
}

def good_is_raw_ptr(expr : Expression?) : bool {
    return expr is ExprVar
}
