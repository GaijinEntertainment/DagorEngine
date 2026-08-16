options gen2
options bound_check_elision
options rtti
options no_coverage
require dastest/testing_boost public
require daslib/ast
require daslib/ast_boost
require daslib/rtti

// Bound-check elision sets ExprAt.noBoundCheck (surfaced as atFlags.no_bound_check) on accesses
// proven in range; ast_simulate then emits the unchecked node. The result is identical either way,
// so correctness alone can't prove the pass fired — we inspect the flag on the optimized AST.

class AtFlagCounter : AstVisitor {
    total : int = 0
    elided : int = 0
    def override visitExprAt(var expr : ExprAt?) : ExpressionPtr {
        total++
        if (expr.atFlags.no_bound_check) {
            elided++
        }
        return expr
    }
}

// (elided, total) ExprAt count in the named (already optimized) function
def count_at(nm : string) : int2 {
    var res = int2(0, 0)
    compiling_module() |> for_each_function(nm) $(func) {
        var v = new AtFlagCounter()
        make_visitor(*v) $(adapter) {
            visit(func.body, adapter)
        }
        res += int2(v.elided, v.total)
        unsafe { delete v }
    }
    return res
}

// ===== targets: every access is provably in range -> all elided =====

[export] def target_const_fixed : int {
    var a : int[4]
    a[0] = 0; a[1] = 1; a[2] = 4; a[3] = 9
    return a[0] + a[2] + a[3]
}

[export] def target_induction_fixed : int {
    var a : int[8]
    var s = 0
    for (i in range(8)) { a[i] = i }
    for (i in range(0, 8)) { s += a[i] }
    return s
}

[export] def target_vector_const : float {
    let v = float4(1.0, 2.0, 3.0, 4.0)
    return v[0] + v[3]
}

[export] def target_dyn_length(x : array<int>) : int {
    var s = 0
    for (i in range(length(x))) { s += x[i] }   // nolint:PERF018 -- indexed access is the point (elision target)
    return s
}

[export] def target_guard_then(var a : array<int>; x : uint) {
    if (x < uint(length(a))) { a[x] = int(x) * 10 }
}

[export] def target_guard_early(var a : array<int>; x : uint) {
    if (x >= uint(length(a))) { return }
    a[x] = int(x) * 10
}

// manual override: not provable, but [hint(unsafe_range_check)] forces the mark (as the JIT does)
[export, hint(unsafe_range_check)] def target_hint(a : array<int>; i : int) : int {
    return a[i]
}

// ===== targets: NOT provable -> nothing elided =====

// const index but the array length is unknown (dynamic) -> can't prove 3 < length
[export] def target_const_dyn(x : array<int>) : int {
    return x[3]
}

// unconstrained index into a dynamic array
[export] def target_unprovable(x : array<int>; k : int) : int {
    return x[k]
}

// the loop's length bound is killed because the body mutates the array
[export] def target_mutated(var x : array<int>) {
    for (i in range(length(x))) {
        x[i] = i
        x |> erase(0)
    }
}

[test]
def test_elision_fired(t : T?) {
    ast_gc_guard() {
        // all-elided cases: elided == total, and there is at least one access
        for (nm in ["target_const_fixed", "target_induction_fixed", "target_vector_const",
                    "target_dyn_length", "target_guard_then", "target_guard_early", "target_hint"]) {
            t |> run("all accesses elided: {nm}") <| @(t : T?) {
                let r = count_at(nm)
                t |> success(r.y > 0, "{nm} has accesses ({r.y})")
                t |> equal(r.x, r.y)
            }
        }
        // not-provable cases: no access elided
        for (nm in ["target_const_dyn", "target_unprovable", "target_mutated"]) {
            t |> run("no access elided: {nm}") <| @(t : T?) {
                let r = count_at(nm)
                t |> success(r.y > 0, "{nm} has accesses ({r.y})")
                t |> equal(r.x, 0)
            }
        }
    }
}

// behavioral safety net: the elided path must compute the same values as the checked path
[test]
def test_elision_behavior(t : T?) {
    t |> equal(target_const_fixed(), 0 + 4 + 9)
    t |> equal(target_induction_fixed(), 28)
    t |> equal(target_vector_const(), 5.0)
    var d : array<int>
    d |> resize(5)
    for (i in range(5)) { d[i] = i * 2 }
    t |> equal(target_dyn_length(d), 0 + 2 + 4 + 6 + 8)
    var g : array<int>
    g |> resize(4)
    target_guard_then(g, 3u)
    target_guard_then(g, 100u)   // out of range: guarded, no write, no crash
    t |> equal(g[3], 30)
}
