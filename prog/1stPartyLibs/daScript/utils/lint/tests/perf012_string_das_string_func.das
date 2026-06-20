options gen2
// PERF012: string(das_string) passed to strings module function
//
// Problem: Wrapping a das_string in string() before passing to a strings
// module function allocates unnecessarily. Use peek(das_string) instead.
//
// Bad pattern:
//   let pos = find(string(name), "foo")
//
// Good pattern:
//   peek(name) $(s) { let pos = find(s, "foo") }

require daslib/ast_boost
require strings

// bad: string(das_string) passed to find (1 warning)
def bad_find_string(name : das_string) : int {
    return find(string(name), "foo")
}

// good: using peek
def good_peek(name : das_string) : int {
    var result = -1
    peek(name) $(s) {
        result = find(s, "foo")
    }
    return result
}

// good: string(int) is not das_string
def good_string_of_int(x : int) : int {
    return find(string(x), "4")
}
