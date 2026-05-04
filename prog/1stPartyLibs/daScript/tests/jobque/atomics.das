options gen2
require dastest/testing_boost public
require jobque


[test]
def test_lpipe(t : T?) {
    t |> run("atomic32") @(t : T?) {
        with_atomic32() $(ch) {
            ch |> set(13)
            t |> equal(13, ch |> get)
            t |> equal(14, ch |> inc)
            t |> equal(13, ch |> dec)
        }
    }
    t |> run("atomic64") @(t : T?) {
        var ch = atomic64_create()
        ch |> set(13l)
        t |> equal(13l, ch |> get)
        t |> equal(14l, ch |> inc)
        t |> equal(13l, ch |> dec)
        unsafe(atomic64_remove(ch))
    }
}


