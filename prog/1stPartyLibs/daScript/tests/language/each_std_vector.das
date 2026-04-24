options gen2
require UnitTest
require dastest/testing_boost public

[test]
def test_each_std_vector(t : T?) {
    testFooArray() <| $(arr) {
        t |> equal(length(arr), 10)
        t |> equal(capacity(arr), 16)
        let index = find_index_if(each(arr)) <| $(foo) {
            return foo.fooData == 5
        }
        t |> equal(index, 5)
    }
}
