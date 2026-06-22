options gen2
// STYLE001 false positives — patterns that use <| legitimately
//
// These should produce ZERO style warnings.

require daslib/algorithm
require strings

// Lambda shorthand: <| $(params) => expr requires <| (parser doesn't accept $(params) => without pipe)
def good_lambda_shorthand() {
    var arr <- [3, 1, 2]
    sort(arr) <| $(a, b) => a < b
}

// Lambda shorthand with complex expression
def good_lambda_shorthand_complex() {
    var arr <- [["hello", "world", "abc"]]
    sort(arr) <| $(a, b) => length(a) < length(b)
}

// Value pipe: <| expr is valid style, only block pipes should be flagged
def good_value_pipe() {
    var arr <- [1, 2, 3]
    arr |> push <| 42
}
