options gen2
// options log=true, logCpp=true

require UnitTest
require dastest/testing_boost public

[test]
def test_das_string(t : T?) {
    test_das_string() <| $(var s, s2 : das_string) {
        s2 := "test_das_string"
        t |> equal("{s}", "test_das_string")
        t |> equal(string(s), "test_das_string")
        peek(s) <| $(st) {
            t |> equal(st, "test_das_string")
        }
        t |> equal(s, "test_das_string")
        s := "out_of_it"
        t |> equal("{s}", "out_of_it")
        t |> equal(string(s), "out_of_it")
    }
}

