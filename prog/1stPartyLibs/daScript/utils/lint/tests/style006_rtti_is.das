options gen2
// STYLE006: string(__rtti) comparison should use `is` operator
//
// Problem: string(x.__rtti) == "ExprFoo" is verbose and fragile.
//
// Bad pattern:
//   if (string(expr.__rtti) == "ExprConst") { ... }
//
// Good pattern:
//   if (expr is ExprConst) { ... }

require daslib/ast_boost

// bad: string(__rtti) comparison (1 warning)
def bad_rtti_comparison(expr : Expression?) {
    if (string(expr.__rtti) == "ExprReturn") {
        pass
    }
}

// good: using `is` operator
def good_is_check(expr : Expression?) {
    if (expr is ExprReturn) {
        pass
    }
}
