options gen2
options persistent_heap = true
options gc

require dastest/testing_boost public

def test_aka_func(t : T?; var x aka X : int) {
    let tx = X
    t |> equal(tx, x)
}

def call_block(blk : block<(arg : int) : void>) {
    invoke(blk, 5)
}

[test]
def test_aka_everything(t : T?) {
    t |> run("basic aka") @(t : T?) {
        var x aka X = 5
        X = 13
        t |> equal(x, 13)
    }
    t |> run("aka function") @(t : T?) {
        test_aka_func(t, 5)
    }
    t |> run("aka block") @(t : T?) {
        call_block() $(x aka X) {
            let tx = x
            t |> equal(tx, X)
        }
    }
    t |> run("nested aka block") @(t : T?) {
        var tot = 0
        var same = 0
        for (x aka X in range(10)) {
            for (x_ in range(10)) {
                tot ++
                if (x_ == X) {
                    same ++
                }
            }
        }
        t |> equal(tot, 100)
        t |> equal(same, 10)
    }
}

