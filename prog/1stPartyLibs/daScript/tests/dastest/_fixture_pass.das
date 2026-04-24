options gen2
require dastest/testing_boost

require daslib/fio

[test]
def test_passing(t : T?) {
    sleep(1u)
    t |> success(true)
}

[test]
def test_also_passing(t : T?) {
    sleep(1u)
    t |> equal(1, 1)
}
