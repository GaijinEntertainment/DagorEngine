options gen2
// PERF008: Unnecessary get_ptr() for is/as type checks
//
// After GC migration, Expression/TypeDecl are no longer smart_ptr,
// so get_ptr() can't be called on them. This rule still exists for
// any user-defined smart_ptr types with __rtti, but there are no
// built-in smart_ptr types left that support is/as.
//
// This test verifies no false positives on raw pointer is/as.

require daslib/perf_lint
require daslib/ast_boost

// good: direct is/as on raw pointer (no warnings)

def good_is_raw_ptr(expr : ExpressionPtr) : bool {
    return expr is ExprVar
}

def good_as_raw_ptr(expr : ExpressionPtr) {
    var v = expr as ExprCall
    unsafe { print("{v}\n"); }
}
