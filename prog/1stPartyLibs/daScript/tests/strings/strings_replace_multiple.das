options gen2
require dastest/testing_boost
require daslib/strings_boost
require daslib/faker
require daslib/fuzzer

[test]
def test_replace_multiple(t : T?) {
    t |> run("replace multiple") @@(t : T?) {
        // Test with multiple occurrences of different substrings
        t |> equal("_1_2_1_2", replace_multiple("_foo_bar_foo_bar", [(text = "foo", replacement = "1"), (text = "bar", replacement = "2")]))

        // Test with multiple occurrences of the same substring
        t |> equal("1_1_1", replace_multiple("foo_foo_foo", [(text = "foo", replacement = "1")]))

        // Test with substring as a prefix and a suffix
        t |> equal("1_text_2", replace_multiple("foo_text_bar", [(text = "foo", replacement = "1"), (text = "bar", replacement = "2")]))

        // Test with no matching substring (no replacement)
        t |> equal("hello_world", replace_multiple("hello_world", [(text = "foo", replacement = "1"), (text = "bar", replacement = "2")]))

        // Test with special characters in the substrings
        // we do not support overlap, otherwise 1_2_3 occurs
        t |> equal("1$23", replace_multiple("@#$_*&^_%$", [(text = "@#", replacement = "1"), (text = "_*&^", replacement = "2"), (text = "_%$", replacement = "3")]))

        // Test with an empty source string
        t |> equal("", replace_multiple("", [(text = "foo", replacement = "1"), (text = "bar", replacement = "2")]))

        // Test with an empty array of replacements
        let empty : array<tuple<text : string; replacement : string>>
        t |> equal("hello_world", replace_multiple("hello_world", empty))

        // Test with overlapping replacements
        // we do not support overlap, otherwise 1_23 occurs
        t |> equal("13baz", replace_multiple("foobarbaz", [(text = "foo", replacement = "1"), (text = "oob", replacement = "2"), (text = "bar", replacement = "3")]))

        // Test with multiple replacements in the same substring
        t |> equal("1_2_3", replace_multiple("foo_bar_baz", [(text = "foo", replacement = "1"), (text = "bar", replacement = "2"), (text = "baz", replacement = "3")]))
    }
}

