options gen2

require dastest/testing_boost public

[test]
def test_pass_one(t : T?) {
    t |> run("sub_a") <| @(t : T?) {
        t |> success(true, "ok")
    }
}

[test]
def test_pass_two(t : T?) {
    t |> success(true, "ok")
}
