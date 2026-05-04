options gen2
require UnitTest
require dastest/testing_boost public

[test]
def test_stdvec_r2v(t : T?) {
    testFooArray() <| $(arr) {
        for (i in range(arr |> length)) {
            t |> equal(arr[i].fooData, i)
        }
    }
}
