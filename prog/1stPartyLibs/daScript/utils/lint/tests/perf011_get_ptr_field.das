options gen2
// PERF011: get_ptr(x).field should be x.field
//
// Problem: get_ptr() is unnecessary for field access on smart_ptr.
//
// Bad pattern:
//   let name = get_ptr(prog).flags
//
// Good pattern:
//   let name = prog.flags

expect 31208:1

require daslib/perf_lint
require daslib/ast_boost

// bad: get_ptr for field access (1 warning)
def bad_get_ptr_field(prog : ProgramPtr) {
    let f = get_ptr(prog).flags
}

// good: direct field access
def good_field_access(prog : ProgramPtr) {
    let f = prog.flags
}
