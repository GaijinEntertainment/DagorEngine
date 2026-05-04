options gen2
require daslib/functional
require dastest/testing_boost public


[test]
def test_flatten(t : T?) {
    t |> run("flatten 1") @(t : T?) {
        var qc = 0
        var q <- [iterator for(x in range(5)); x]
        for (i, j in flatten(q), count()) {
            t |> equal(i, j)
            qc ++
        }
        t |> equal(qc, 5)
    }
    t |> run("flatten 2") @(t : T?) {
        var qc = 0
        var qq <- [iterator for(x in range(3)); [iterator for(y in range(4)); x * 4 + y]]
        for (i, j in flatten(qq), count()) {
            t |> equal(i, j)
            qc ++
        }
        t |> equal(qc, 12)
    }
    t |> run("flatten 3") @(t : T?) {
        var qc = 0
        var qqq <- [iterator for(x in range(3)); [iterator for(y in range(2)); [iterator for(z in range(2)); x * 2 * 2 + y * 2 + z]]]
        for (i, j in flatten(qqq), count()) {
            t |> equal(i, j)
            qc ++
        }
        t |> equal(qc, 12)
    }
}
