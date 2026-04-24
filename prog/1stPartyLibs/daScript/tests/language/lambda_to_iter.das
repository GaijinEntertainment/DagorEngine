options gen2
options no_unused_function_arguments = false
require dastest/testing_boost public

struct LamIterFoo {
    value : int
}

[test]
def test_lambda_to_iter(t : T?) {
    t |> run("int ref lambda as iterator") @(t : T?) {
        var count = 0
        let lam <- @(var a : int&) : bool {
            if (count < 10) {
                a = count++
                return true
            } else {
                return false
            }
        }
        for (x, tx in each(lam), range(10)) {
            t |> equal(x, tx)
        }
    }
    t |> run("struct lambda as iterator") @(t : T?) {
        var count = 0
        let lamF <- @(var a : LamIterFoo) : bool {
            if (count < 10) {
                a.value = count++
                return true
            } else {
                return false
            }
        }
        for (y, ty in each(lamF), range(10)) {
            t |> equal(y.value, ty)
        }
    }
}
