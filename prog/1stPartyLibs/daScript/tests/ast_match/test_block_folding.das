options gen2
options indenting = 4
options rtti

require daslib/ast
require daslib/rtti
require daslib/ast_match
require daslib/ast_boost
require daslib/templates_boost
require dastest/testing_boost public

// ── Target functions — these go through the full compiler/optimizer ──

// BlockFolding: dead code after return is removed
[export, marker(no_coverage)]
def fold_dead_code(a : int) : int {
    return a + 1
    return a + 2
}

// CondFolding: if/else return → ternary
[export, marker(no_coverage)]
def fold_if_else_return(a, b, c : int) : int {
    if (a < 5) {
        return b
    } else {
        return c
    }
}

// CondFolding: chain of if-return → nested ternary
[export, marker(no_coverage)]
def fold_if_chain(a, b : int) : int {
    if (a == 1) {
        return 10
    }
    if (a == 2) {
        return 20
    }
    if (a == 3) {
        return 30
    }
    return b
}

// BlockFolding: trailing return on void function removed
[export, marker(no_coverage)]
def fold_void_trailing_return() : void {
    print("hello\n")
    return
}

// BlockFolding: nested block flattened into parent
[export, marker(no_coverage)]
def fold_nested_block(a : int) : int {
    {
        var x = a + 1
        return x
    }
}

// BlockFolding: dead code after terminating if/else
[export, marker(no_coverage)]
def fold_dead_after_if_else(a, b : int) : int {
    if (a > 0) {
        return a
    } else {
        return b
    }
    return 0
}

// CondFolding: if-break chain → if/else in for
[export, marker(no_coverage)]
def fold_if_break(a : int) : int {
    var s = 0
    for (i in range(10)) {
        if (i == a) {
            break
        }
        s += i
    }
    return s
}

// CondFolding: if-continue chain → if/else in for
[export, marker(no_coverage)]
def fold_if_continue(a : int) : int {
    var s = 0
    for (i in range(10)) {
        if (i == a) {
            continue
        }
        s += i
    }
    return s
}

// CondFolding: longer chain — 5 conditions
[export, marker(no_coverage)]
def fold_if_chain_5(a : int) : int {
    if (a == 1) { return 10; }
    if (a == 2) { return 20; }
    if (a == 3) { return 30; }
    if (a == 4) { return 40; }
    if (a == 5) { return 50; }
    return 0
}

// CondFolding: if-return with computation before return
[export, marker(no_coverage)]
def fold_if_with_work(a, b : int) : int {
    if (a > 0) {
        return a + b
    }
    return a - b
}

// ── Helpers ─────────────────────────────────────────────────────────

// No string-based lookup — use @@func_name directly in each test

def match_report(t : T?; r : QMatchResult; msg : string) {
    if (!r.matched && r.expr != null) {
        t |> success(false, "{msg}: error={int(r.error)} rtti={qm_rtti(r.expr)}")
    } elif (!r.matched) {
        t |> success(false, "{msg}: error={int(r.error)} expr=null")
    } else {
        t |> success(true, msg)
    }
}

// ── Tests ───────────────────────────────────────────────────────────

// Dead code after return should be removed — body is just `return a + 1`
[test]
def test_dead_code_after_return(t : T?) {
    var inscope func <- find_module_function_via_rtti(compiling_module(), @@fold_dead_code)
    if (get_ptr(func) == null) { t |> success(false, "function not found"); return ; }
    let r = qmatch_function(func) $(a : int) : int {
        return a + 1
    }
    t |> match_report(r, "dead code after return should be folded")
}

// If-else return folds to ternary: return (a < 5) ? b : c
[test]
def test_if_else_to_ternary(t : T?) {
    var inscope func <- find_module_function_via_rtti(compiling_module(), @@fold_if_else_return)
    if (get_ptr(func) == null) { t |> success(false, "function not found"); return ; }
    var inscope cond, left, right : ExpressionPtr
    let r = qmatch_function(func) $(a, b, c : int) : int {
        return $e(cond) ? $e(left) : $e(right)
    }
    t |> match_report(r, "if/else return should fold to ternary")
}

// If-chain folds to nested ternary: (a==1) ? 10 : ((a==2) ? 20 : ((a==3) ? 30 : b))
[test]
def test_if_chain_to_nested_ternary(t : T?) {
    var inscope func <- find_module_function_via_rtti(compiling_module(), @@fold_if_chain)
    if (get_ptr(func) == null) { t |> success(false, "function not found"); return ; }
    // Match the outer structure: return (cond ? val : rest)
    var inscope outer_cond, outer_val, outer_rest : ExpressionPtr
    let r = qmatch_function(func) $(a, b : int) : int {
        return $e(outer_cond) ? $e(outer_val) : $e(outer_rest)
    }
    t |> match_report(r, "if-chain should fold to nested ternary")
    if (r.matched) {
        // The inner rest should also be a ternary
        t |> success(outer_rest != null && (outer_rest is ExprOp3), "inner should be ternary too")
    }
}

// Void trailing return removed — body is print("hello\n"), no return
[test]
def test_void_trailing_return(t : T?) {
    var inscope func <- find_module_function_via_rtti(compiling_module(), @@fold_void_trailing_return)
    if (get_ptr(func) == null) { t |> success(false, "function not found"); return ; }
    // After optimization: just print("hello\n") — trailing return removed
    // Fake args (__context__, __lineinfo__) are filtered automatically
    let r = qmatch_function(func) $ {
        print("hello\n")
    }
    t |> match_report(r, "void trailing return should be removed, body is print")
}

// Nested block flattened: body is `var x = a+1; return x` (not wrapped in block)
[test]
def test_nested_block_flattened(t : T?) {
    var inscope func <- find_module_function_via_rtti(compiling_module(), @@fold_nested_block)
    if (get_ptr(func) == null) { t |> success(false, "function not found"); return ; }
    let r = qmatch_function(func) $(a : int) : int {
        var x = a + 1
        return x
    }
    t |> match_report(r, "nested block should be flattened")
}

// Dead code after terminating if/else: folds to ternary, return 0 removed
[test]
def test_dead_after_if_else(t : T?) {
    var inscope func <- find_module_function_via_rtti(compiling_module(), @@fold_dead_after_if_else)
    if (get_ptr(func) == null) { t |> success(false, "function not found"); return ; }
    var inscope cond, left, right : ExpressionPtr
    let r = qmatch_function(func) $(a, b : int) : int {
        return $e(cond) ? $e(left) : $e(right)
    }
    t |> match_report(r, "dead code after terminating if/else should be removed")
}

// If-break chain: inside for, `if (cond) break; stmt` → `if (cond) break else stmt`
[test]
def test_if_break_chain(t : T?) {
    var inscope func <- find_module_function_via_rtti(compiling_module(), @@fold_if_break)
    if (get_ptr(func) == null) { t |> success(false, "function not found"); return ; }
    // The for body should have the if with else (not two separate statements)
    var inscope body : ExpressionPtr
    let r = qmatch_function(func) $(a : int) : int {
        var s = 0
        for (i in $e(body)) {
            if (i == a) {
                break
            } else {
                s += i
            }
        }
        return s
    }
    t |> match_report(r, "if-break chain should fold to if/else")
}

// If-continue chain: same pattern with continue
[test]
def test_if_continue_chain(t : T?) {
    var inscope func <- find_module_function_via_rtti(compiling_module(), @@fold_if_continue)
    if (get_ptr(func) == null) { t |> success(false, "function not found"); return ; }
    var inscope body : ExpressionPtr
    let r = qmatch_function(func) $(a : int) : int {
        var s = 0
        for (i in $e(body)) {
            if (i == a) {
                continue
            } else {
                s += i
            }
        }
        return s
    }
    t |> match_report(r, "if-continue chain should fold to if/else")
}

// 5-condition chain: all fold to single nested ternary return
[test]
def test_if_chain_5(t : T?) {
    var inscope func <- find_module_function_via_rtti(compiling_module(), @@fold_if_chain_5)
    if (get_ptr(func) == null) { t |> success(false, "function not found"); return ; }
    // Should be a single return with deeply nested ternary
    var inscope expr : ExpressionPtr
    let r = qmatch_function(func) $(a : int) : int {
        return $e(expr)
    }
    t |> match_report(r, "5-condition chain should fold to single return")
    if (r.matched) {
        // The return value should be a ternary (ExprOp3)
        t |> success(expr != null && (expr is ExprOp3), "result should be ternary")
    }
}

// if-return with work: `if (a>0) { return a+b; } return a-b` → `return (a>0) ? (a+b) : (a-b)`
[test]
def test_if_return_with_work(t : T?) {
    var inscope func <- find_module_function_via_rtti(compiling_module(), @@fold_if_with_work)
    if (get_ptr(func) == null) { t |> success(false, "function not found"); return ; }
    var inscope cond, left, right : ExpressionPtr
    let r = qmatch_function(func) $(a, b : int) : int {
        return $e(cond) ? $e(left) : $e(right)
    }
    t |> match_report(r, "if-return with work should fold to ternary")
}
