options gen2
require dastest/testing_boost public

[test]
def test_basic_lambda(t : T?) {
    t |> run("lambda invocation") @(t : T?) {
        var lam <- @(x : int) : int {
            return x * 2
        }
        t |> equal(invoke(lam, 5), 10)
        t |> equal(invoke(lam, -3), -6)
        delete lam
    }
}

[test]
def test_lambda_capture(t : T?) {
    t |> run("capture by copy") @(t : T?) {
        let factor = 3
        var lam <- @(x : int) : int {
            return x * factor
        }
        t |> equal(invoke(lam, 5), 15)
        delete lam
    }
}

def apply_transform(values : array<int>; transform : function<(x : int) : int>) : array<int> {
    var result : array<int>
    for (v in values) {
        result |> push(invoke(transform, v))
    }
    return <- result
}

def fn_double(x : int) : int {
    return x * 2
}

[test]
def test_higher_order(t : T?) {
    t |> run("apply function pointer to array") @(t : T?) {
        var arr <- [1, 2, 3, 4, 5]
        var result <- apply_transform(arr, @@fn_double)
        t |> equal(length(result), 5)
        t |> equal(result[0], 2)
        t |> equal(result[1], 4)
        t |> equal(result[2], 6)
        t |> equal(result[3], 8)
        t |> equal(result[4], 10)
    }
}

[test]
def test_block_argument(t : T?) {
    t |> run("block as last argument") @(t : T?) {
        var sum = 0
        for (i in range(5)) {
            sum += i
        }
        t |> equal(sum, 10)
    }
}
