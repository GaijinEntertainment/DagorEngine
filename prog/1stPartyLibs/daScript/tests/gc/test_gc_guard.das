options gen2
options indenting = 4
options rtti

require daslib/ast
require daslib/ast_match
require daslib/ast_boost
require daslib/templates_boost
require dastest/testing_boost public

[test]
def test_ast_gc_guard_basic(t : T?) {
    let before = gc_thread_root_count()
    ast_gc_guard() {
        var expr = qmacro(type<int>)
        let r = qmatch(expr, type<int>)
        t |> success(r.matched, "type<int> should match inside gc_guard")
    }
    let after = gc_thread_root_count()
    t |> success(after <= before, "gc_guard should sweep temporary TypeDecl nodes (before={before}, after={after})")
}

[test]
def test_ast_gc_guard_sweeps(t : T?) {
    let before = gc_thread_root_count()
    ast_gc_guard() {
        // Create several TypeDecl nodes via qmacro
        var e1 = qmacro(type<int>)
        var e2 = qmacro(type<float>)
        var e3 = qmacro(type<array<int>>)
        var captured : TypeDeclPtr
        qmatch(e1, type<$t(captured)>)
        qmatch(e2, type<$t(captured)>)
        qmatch(e3, type<$t(captured)>)
    }
    let after = gc_thread_root_count()
    t |> success(after <= before, "gc_guard should sweep all temp nodes (before={before}, after={after})")
}
