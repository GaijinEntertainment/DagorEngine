options gen2
options no_unused_function_arguments = false
require dastest/testing_boost public

def fa_add(a, b : int) : int {
    return a + b
}

[test]
def test_func_addr(t : T?) {
    t |> run("function pointer invoke") @(t : T?) {
        let fn = @@fa_add
        let val = invoke(fn, 1, 2)
        t |> equal(val, 3)
    }
    t |> run("function pointer comparison") @(t : T?) {
        let fn = @@fa_add
        t |> equal(fn != null, true)
        t |> equal(fn == @@fa_add, true)
        t |> equal(fn == null, false)
        t |> equal(fn != @@fa_add, false)
    }
    t |> run("null function pointer") @(t : T?) {
        let qq : function
        t |> equal(qq == null, true)
    }
    t |> run("invoke null function panics") @(t : T?) {
        var failed = false
        try {
            let fn2 : function<(a, b : int) : int>
            invoke(fn2, 1, 2)
        } recover {
            failed = true
        }
        t |> equal(failed, true)
    }
}
