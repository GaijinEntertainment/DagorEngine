options gen2
require dastest/testing_boost public
require daslib/das_source_formatter
require strings

[test]
def test_raw_assignment_tokens_survive_formatting(t : T?) {
    t |> run("the formatter keeps !== !<- and !:= as one token each") @(t : T?) {
        let source = "options gen2\ndef raw(var a : int; b : int) \{\n    a !== b\n    a !<- b\n    a !:= b\n\}\n"
        let formatted = format_source_string(source)
        t |> success(find(formatted, "a !== b") >= 0, "raw copy kept: {formatted}")
        t |> success(find(formatted, "a !<- b") >= 0, "raw move kept: {formatted}")
        t |> success(find(formatted, "a !:= b") >= 0, "raw clone kept: {formatted}")
        t |> success(find(formatted, "! ") < 0, "no raw token split after the bang: {formatted}")
    }
}
