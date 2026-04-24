options gen2

require dastest/testing_boost public

[test]
def test_fixture_passes(t : T?) {
    t |> run("always passes") <| @(t : T?) {
        t |> success(true, "ok")
    }
}
