options gen2
require dastest/testing_boost

[test]
def test_will_fail(t : T?) {
    t |> equal(1, 2)
}

[test]
def test_will_pass(t : T?) {
    t |> success(true)
}
