options gen2
expect 30950
expect 30341

require dastest/testing_boost public

// pointer arithmetic on void? is prohibited — it used to slip past the guard (a void? carries a
// non-null tVoid firstType) and lower with stride sizeof(void) == 0, silently returning the
// unmoved pointer.

def foo : void ? {
    var x = 0
    unsafe {
        var p = reinterpret<void?>(addr(x))
        return p + 4
    }
}

[test]
def test_never_runs(t : T?) {
    t |> success(foo() != null)
}
