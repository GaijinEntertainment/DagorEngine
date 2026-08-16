options gen2
options auto_inline_functions = false   // lint fixtures assert SOURCE shapes; splices rewrite them
// STYLE021: table<string; JsonValue?> built by repeated const-key insert —
// use the named-tuple JV form `var v = JV((k1=..., k2=...))` (json_boost).
//
// Division of labor with STYLE031: const-string-key runs into a JV table are
// owned by STYLE021 (the JV named-tuple form is the stronger suggestion);
// computed-key runs fall through to STYLE031 (a table literal accepts
// computed keys, JV((...)) does not).

expect 31209:2

require daslib/style_lint
require daslib/json_boost

def make_key(i : int) : string => "k{i}"

// --- Bad patterns (must fire — 1 x STYLE021 + 1 x STYLE031) ---

def bad_jv_const_keys() : int {
    var args : table<string; JsonValue?>              // STYLE021 — JV((k1=..., k2=...))
    args |> insert("k1", JV(1))
    args |> insert("k2", JV(2))
    return length(args)
}

def bad_jv_computed_keys(i : int) : int {
    var args : table<string; JsonValue?>              // STYLE031 — computed keys, table literal
    args |> insert(make_key(i), JV(1))
    args |> insert(make_key(i + 1), JV(2))
    return length(args)
}

// --- Good patterns (must NOT fire) ---

def good_named_tuple_jv() : JsonValue? {
    // The idiomatic form STYLE021 suggests.
    var args = JV((k1 = 1, k2 = 2))
    return args
}

def good_single_insert() : int {
    var args : table<string; JsonValue?>
    args |> insert("k1", JV(1))
    return length(args)
}
