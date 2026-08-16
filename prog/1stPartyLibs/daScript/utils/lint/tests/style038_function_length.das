options gen2
options auto_inline_functions = false   // lint fixtures assert SOURCE shapes; splices rewrite them
options _function_length = 10           // keep fixture functions small
// STYLE038: function exceeds the line-count limit
//
// Problem: an overgrown function is hard to navigate, review, and test even
// when its branching stays simple (straight-line emitters, giant literals).
// Length is physical lines, def line through closing brace, comments and
// blank lines included.
//
// Functions only: a closure's span always sits inside its host's, so the host
// trips first and a separate closure check would only double-report.
//
// Override the default limit per module with 'options _function_length = N'
// (this fixture uses 10); 0 disables the rule for the module (see
// style038_function_length_off.das).
// Suppress a deliberate keep with '// nolint:STYLE038' on the def line.

expect 31209:2

require daslib/style_lint

// 14 physical lines -> over the limit of 10
def bad_long_function(x : int) : int {                  // STYLE038
    var r = x
    r += 11
    r += 2
    r += 3
    r += 4
    r += 5
    r += 6
    r += 7
    r += 8
    r += 9
    r += 10
    return r
}

// a long block argument is billed to its host — one warning, at the host
def bad_long_closure(x : int) : int {                   // STYLE038
    var r = 0
    with_it(x) $(y) {
        r += y
        r += 11
        r += 2
        r += 3
        r += 4
        r += 5
        r += 6
        r += 7
        r += 8
        r += 9
    }
    return r
}

def with_it(x : int; blk : block<(y : int) : void>) {
    invoke(blk, x)
}

// exactly at the limit: 10 physical lines, silent
def good_at_threshold(x : int) : int {
    var r = x
    r += 11
    r += 2
    r += 3
    r += 4
    r += 5
    r += 6
    return r
}

// deliberate keep, suppressed on the def line
def suppressed_long(x : int) : int {                    // nolint:STYLE038
    var r = x
    r += 11
    r += 2
    r += 3
    r += 4
    r += 5
    r += 6
    r += 7
    r += 8
    r += 9
    r += 10
    return r
}
