options gen2
// LINT005: Redundant reinterpret cast
//
// Problem: reinterpret<T>(x) where T is the same type as x is a no-op.
//
// Bad pattern:
//   let y = unsafe(reinterpret<int?>(x))   // x is already int?
//
// Good pattern:
//   let y = x

// bad: reinterpret to same pointer type (1 warning)
def bad_redundant_reinterpret(var x : int?) : int? {
    return unsafe(reinterpret<int?>(x))
}

// good: reinterpret to same const pointer type (not caught — outer const from param read
// makes types differ; acceptable false negative to avoid flagging const-stripping casts)
def good_same_const(x : int const?) : int const? {
    return unsafe(reinterpret<int const?>(x))
}

// good: reinterpret to different type
def good_different_type(x : int?) : float? {
    return unsafe(reinterpret<float?>(x))
}

// good: reinterpret strips const
def good_strip_const(x : int const?) : int? {
    return unsafe(reinterpret<int?>(x))
}
