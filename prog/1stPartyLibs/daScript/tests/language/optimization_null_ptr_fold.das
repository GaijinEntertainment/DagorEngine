options gen2
options no_coverage

require dastest/testing_boost public

// regression: the remove-unused pass propagates a read-only local's constexpression
// init into its reads (ast_unused.cpp visit(ExprVar)). The let coerced the init to
// the variable's type - a null literal (void?) into a typed pointer being the
// canonical case - and no infer runs after the substitution, so the clone must be
// retyped to the variable's type. Before the fix, `null?[0]` reached simulate with
// a void? subexpr and SimulateVisitor::visit(ExprSafeAt*) crashed on the null
// firstType (SIGSEGV, not a diagnostic).

def safe_or_default(p : float?; i : int64) : float {
    return unsafe(p?[i]) ?? -1.0
}

[test]
def test_null_ptr_let_fold(t : T?) {
    t |> run("safe-index of a propagated null local") @(t : T?) {
        let nullp : float? = null
        t |> equal(unsafe(nullp?[0]) ?? -1.0, -1.0)
        t |> equal(unsafe(nullp?[int64(1)]) ?? -2.0, -2.0)
    }
    t |> run("propagated null local through a call") @(t : T?) {
        let nullp : float? = null
        t |> equal(safe_or_default(nullp, int64(0)), -1.0)
    }
    t |> run("live pointer control") @(t : T?) {
        var buf : float[4]
        for (k in range(4)) {
            buf[k] = float(k) * 10.0
        }
        var p = unsafe(addr(buf[0]))
        t |> equal(unsafe(p?[2]) ?? -1.0, 20.0)
        t |> equal(safe_or_default(p, int64(3)), 30.0)
    }
}
