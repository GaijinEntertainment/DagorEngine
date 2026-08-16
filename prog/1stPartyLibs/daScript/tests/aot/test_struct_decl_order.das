options gen2
require dastest/testing_boost public
require _aot_so_mod

// Regression for issue #3212: an AOT-required module whose struct holds a by-value field
// of a later-declared struct. The generated C++ for THIS file emits _aot_so_mod's structs
// (Outer holds Inner by value, declared Outer-first), which only compiles once the emitter
// emits the required module's struct definitions dependency-first.

[test]
def test_struct_decl_order(t : T?) {
    t |> run("aot required-module by-value struct in reverse decl order") @(t : T?) {
        var o : Outer
        o.inner.x = 1
        o.inner.y = 5
        o.tag = 2
        t |> equal(o.inner.x, 1)
        t |> equal(o.inner.y, 5)
        t |> equal(o.tag, 2)
    }
}
