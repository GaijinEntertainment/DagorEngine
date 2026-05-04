options gen2
require dastest/testing_boost public

def sb_test_arg(a : string) : string {
    return "hello,{a}!"
}

[test]
def test_string_builder(t : T?) {
    t |> run("string interpolation") @(t : T?) {
        let empty : string = ""
        let q : string = "blah" + "blah"
        let s : string = "hello,world!"
        let s2 : string = "S = {s}"
        let s3 : string = "S2 = \{{s2}}"
        let s4 : string = "\"say\" 2 + 2 = {2+2}"
        let s5 : string = "\"say\" 2 + 2 = {2+2}{empty}"
        t |> equal(q, "blahblah")
        t |> equal(s2, "S = hello,world!")
        t |> equal(s3, "S2 = \{S = hello,world!}")
        t |> equal(s4, "\"say\" 2 + 2 = 4")
        t |> equal(s4, s5)
    }
}
