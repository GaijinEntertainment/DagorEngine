options gen2
require dastest/testing_boost

def get_back(a : array<int?>) : int? const& {
    let n = length(a)
    unsafe {
        return a[n - 1]
    }
}

[test]
def test_ptr_ref_return(t : T?) {
    t |> run("array ptr ref") @(t : T?) {
        var x = 42
        var ptrs : array<int?>
        var p = unsafe(addr(x))
        ptrs |> push(p)
        let q = get_back(ptrs)
        t |> equal(q, p)
        t |> success(!jit_enabled() || is_jit_function(@@get_back))
    }
}
