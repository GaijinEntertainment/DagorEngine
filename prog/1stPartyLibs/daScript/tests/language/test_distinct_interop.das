options gen2
require dastest/testing_boost public
require UnitTest

[test]
def test_cpp_registered_distinct(t : T?) {
    t |> run("C++ extern takes and returns the distinct type") @(t : T?) {
        let id = NativeId(6)
        let nxt = native_id_next(id)
        t |> success(nxt == NativeId(7))
        t |> equal(*nxt, 7)
    }
    t |> run("nominal: NativeId is not int") @(t : T?) {
        var v = NativeId(1)
        *v = 3
        t |> equal(*v, 3)
        t |> success(NativeId(3) == v)
    }
}
