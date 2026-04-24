options gen2
require dastest/testing_boost

def try_recover(a : array<int>) {
    var res : int
    try {
        res = a[1]
    } recover {
        res = -1
    }
    return res
}

[test]
def test_try_recover(t : T?) {
    t |> run("try recover") @(t : T?) {
        var a : array<int>
        t |> equal(-1, try_recover(a))
        a |> resize(2)
        a[1] = 13
        t |> equal(13, try_recover(a))
    }
}
