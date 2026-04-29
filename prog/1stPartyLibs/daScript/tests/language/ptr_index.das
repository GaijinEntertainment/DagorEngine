options gen2
options no_unused_function_arguments = false
require dastest/testing_boost public

def pi_fn_with_default_null_ptr(var a : int? implicit = default<int?>) {
    var b : int
    a ?? b = 2
}

[test]
def test_ptr_index(t : T?) {
    t |> run("pointer deref and index") @(t : T?) {
        let tf = fixed_array<int>(1, 2, 3, 4, 5)
        unsafe {
            let pt = addr(tf[0])
            t |> equal(deref(pt), 1)
            t |> equal(pt[0], 1)
            t |> equal(pt[1], 2)
            t |> equal(pt[2], 3)
            t |> equal(pt[3], 4)
            t |> equal(pt[4], 5)
        }
    }
    t |> run("default null pointer argument") @(t : T?) {
        pi_fn_with_default_null_ptr()
        t |> success(true)
    }
}
