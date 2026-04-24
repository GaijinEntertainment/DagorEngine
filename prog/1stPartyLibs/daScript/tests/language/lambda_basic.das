options gen2
options no_unused_function_arguments = false
require dastest/testing_boost public

[sideeffects]
def onlyNull(bar : lambda<(a : int) : int>) {
    if (bar != null) {
        panic("has to be null")
    }
}

def addX(a : int) {
    return @(b : int) {
        return a + b
    }
}

[test]
def test_lambda(t : T?) {
    t |> run("capture and invoke") @(t : T?) {
        var CNT = 0
        let counter <- @(extra : int) : int {
            return CNT++ + extra
        }
        var x = invoke(counter, 13)
        CNT = 100500
        let y = invoke(counter, 13)
        t |> equal(x, 13)
        t |> equal(y, 14)
    }
    t |> run("null lambda check") @(t : T?) {
        let l : lambda<(a : int) : int>
        onlyNull(l)
        t |> success(true)
    }
    t |> run("addX returns lambda") @(t : T?) {
        let tt <- addX(1)
        let q = invoke(tt, 2)
        t |> equal(q, 3)
    }
}
