options gen2
options persistent_heap = true
options gc

require dastest/testing_boost public

struct Foo {
    a : array<int>
}

[test]
def test_lambda_capture_const(var t : T?) {
    let f : Foo?    // we are capturing const value here, which needs a finalizer
    var l <- @ {// value will not be finalized, because its captured as const
        let lenF =  f != null ? length(f.a) : 0
        t |> equal(lenF, 0)
    }
    invoke(l)
    delete l
}


