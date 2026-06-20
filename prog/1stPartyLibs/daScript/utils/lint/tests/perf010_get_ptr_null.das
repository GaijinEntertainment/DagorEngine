options gen2
// PERF010: get_ptr(x) == null should be x == null
//
// Problem: get_ptr() is unnecessary for null comparison on smart_ptr.
//
// Bad pattern:
//   if (get_ptr(ptr) == null) { ... }
//
// Good pattern:
//   if (ptr == null) { ... }

expect 31208:1

require daslib/perf_lint
require daslib/ast_boost

// bad: get_ptr for null check (1 warning)
def bad_get_ptr_null(prog : ProgramPtr) {
    if (get_ptr(prog) == null) {
        print("null\n")
    }
}

// good: direct null check
def good_null_check(prog : ProgramPtr) {
    if (prog == null) {
        print("null\n")
    }
}
