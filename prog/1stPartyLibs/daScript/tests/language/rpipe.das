options gen2
require dastest/testing_boost public

def rp_addX(a, b) {
    return a + b
}

[test]
def test_rpipe(t : T?) {
    t |> run("pipe operators") @(t : T?) {
        let v = 12 |> rp_addX(2) |> rp_addX(3)
        t |> equal(v, 17)
        var c = 1 |> rp_addX <| 2
        c ++
        t |> equal(c, 4)
    }
}
