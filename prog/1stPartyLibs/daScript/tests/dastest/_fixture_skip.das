options gen2
require dastest/testing_boost

[test]
def test_skipped(t : T?) {
    t->skip("not implemented yet")
}

[test]
def test_not_skipped(t : T?) {
    t |> success(true)
}
