options gen2

require dastest/testing_boost public

[test]
def test_one(t : T?) {
    t |> success(true, "ok")
}
