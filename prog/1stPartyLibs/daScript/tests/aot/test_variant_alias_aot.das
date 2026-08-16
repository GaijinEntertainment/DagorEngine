options gen2
require UnitTest
require dastest/testing_boost public

// Regression for #3269 (fuzzer AOT codegen). The AOT C++ emitter wrapped a call argument in
// das_alias<Alias>::to(...) whenever the callee parameter was an alias type. That is only correct
// for extern/bound (builtIn) callees whose C++ signature spells the alias name; emitter-generated
// callees always spell parameters structurally, so the wrap mismatched the parameter and the
// generated C++ failed to compile. bug C: delete of a handled variant alias (SampleVariant) ->
// generated finalize(). bug G: builtin push/to_array_move of a variant element -> generic
// placeholder name leaked. Both go through the now builtIn-gated alias wrap in daslib/aot_cpp.das.

variant U_F {
    i_value : uint
    f_value : float
}

[sideeffects]
def test_delete_handled_variant_alias(t : T?) {
    var s = makeSampleS()
    t |> success(s is s_value)
    delete s
    t |> success(s is i_value)
}

def test_variant_element_array(t : T?) {
    var v : U_F
    unsafe {
        set_variant_index(v, 0)
        v.i_value = 7u
    }
    var arr : array<U_F>
    arr |> push(v)
    t |> equal(length(arr), 1)
    t |> success(arr[0] is i_value)
    t |> equal(arr[0] as i_value, 7u)
}

[test]
def test_variant_alias_aot(t : T?) {
    test_delete_handled_variant_alias(t)
    test_variant_element_array(t)
}
