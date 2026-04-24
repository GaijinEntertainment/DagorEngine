options gen2
options no_unused_function_arguments = false
options no_unused_block_arguments = false
require dastest/testing_boost public
require daslib/regex
// NOTE: no explicit "require strings" — should be visible via regex's "require strings public"

[test]
def test_strings_visible_via_regex(t : T?) {
    t |> run("slice available") @(t : T?) {
        let s = "hello world"
        t |> equal(slice(s, 0, 5), "hello")
    }
    t |> run("starts_with available") @(t : T?) {
        t |> equal(starts_with("hello", "hel"), true)
    }
    t |> run("ends_with available") @(t : T?) {
        t |> equal(ends_with("hello", "llo"), true)
    }
    t |> run("to_lower available") @(t : T?) {
        t |> equal(to_lower("ABC"), "abc")
    }
    t |> run("operator [] with slice from strings") @(t : T?) {
        var re : Regex
        regex_compile(re, "(\\w+)@(\\w+)")
        let str = "user@host"
        let m = regex_match(re, str)
        t |> equal(m, 9)
        let r1 = re[1]
        t |> equal(slice(str, r1.x, r1.y), "user")
        let r2 = re[2]
        t |> equal(slice(str, r2.x, r2.y), "host")
    }
}
