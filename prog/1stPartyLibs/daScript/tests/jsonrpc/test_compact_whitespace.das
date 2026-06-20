options gen2
options no_unused_block_arguments = false
options no_unused_function_arguments = false

require dastest/testing_boost public
require daslib/jsonrpc

// ===== compact_json_whitespace =====

[test]
def test_strips_newlines_tabs_between_tokens(t : T?) {
    let r = compact_json_whitespace("\{\n\t\"fps\" : 60,\n\t\"paused\" : false\n}")
    t |> equal(r, "\{\"fps\":60,\"paused\":false}")
}

[test]
def test_preserves_whitespace_inside_strings(t : T?) {
    let r = compact_json_whitespace("\{\n\t\"msg\" : \"hello\\nworld with spaces\"\n}")
    t |> equal(r, "\{\"msg\":\"hello\\nworld with spaces\"}")
}

[test]
def test_preserves_escaped_quotes_inside_strings(t : T?) {
    let r = compact_json_whitespace("\{\n\t\"q\" : \"a\\\"b\\\"c\"\n}")
    t |> equal(r, "\{\"q\":\"a\\\"b\\\"c\"}")
}

[test]
def test_empty_string_returns_empty(t : T?) {
    t |> equal(compact_json_whitespace(""), "")
}

[test]
def test_already_compact_unchanged(t : T?) {
    let r = compact_json_whitespace("\{\"a\":1,\"b\":[1,2,3]}")
    t |> equal(r, "\{\"a\":1,\"b\":[1,2,3]}")
}

[test]
def test_carriage_returns_stripped(t : T?) {
    let r = compact_json_whitespace("\{\r\n  \"x\":1\r\n}")
    t |> equal(r, "\{\"x\":1}")
}
