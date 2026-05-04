options gen2
// PERF007: Unnecessary string(das_string) in comparison
//
// Problem:
//   Wrapping a das_string in string() before comparing with == or != allocates
//   a new string unnecessarily. das_string supports direct comparison with
//   string literals and other das_string values.
//
// Bad pattern:
//   if (string(name) == "foo") { ... }
//   if (string(a) == string(b)) { ... }
//
// Good pattern:
//   if (name == "foo") { ... }
//   if (a == b) { ... }

expect 40217:3

require daslib/perf_lint
require daslib/ast_boost

// --- Bad patterns (warnings) ---

def bad_string_eq(name : das_string) : bool {
    return string(name) == "foo"            // PERF007
}

def bad_string_ne(name : das_string) : bool {
    return "bar" != string(name)            // PERF007
}

def bad_both_string(a, b : das_string) : bool {
    return string(a) == string(b)           // PERF007
}

// --- Good patterns (no warnings) ---

def good_direct_eq(name : das_string) : bool {
    return name == "foo"
}

def good_direct_ne(name : das_string) : bool {
    return "bar" != name
}

def good_direct_both(a, b : das_string) : bool {
    return a == b
}

def good_int_to_string(x : int) : bool {
    return string(x) == "42"               // int-to-string, not das_string
}
