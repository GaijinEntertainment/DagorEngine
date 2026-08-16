options gen2
options auto_inline_functions = false   // lint fixtures assert SOURCE shapes; splices rewrite them
// LINT021: a 64-bit local that is never consumed as 64-bit
//
// Problem: an int64/uint64 local whose every use sits inside an int(...)/uint(...)
// narrowing cast is a 64-bit detour - the wide value is computed and immediately
// thrown away at every sink, silently truncating above 2^31. Either narrow once
// at the declaration, or lift the sinks to 64-bit (range64, int64 resize).
// Math that leaves the top unbounded (max/abs, operators) between the cast and
// the variable does not count as 64-bit consumption; min/clamp/sign cap the
// value (int(clamp(x, lo, hi)) is deliberate saturation), and any other call,
// comparison, write, or capture disqualifies.
//
// Bad pattern:
//   let keep = long_length(buf) - consumed * HOP
//   arr |> do_resize(int(max(keep, 0l)))       // every use narrows
//
// Good pattern:
//   let keep = length(buf) - int(consumed) * HOP
//   arr |> do_resize(max(keep, 0))

expect 50503:3

require daslib/lint
require math

def take_int(x : int) {
    print("{x}\n")
}

def take_int64(x : int64) {
    print("{x}\n")
}

def passthru(x : int64) : int64 {
    return x
}

// bad: single narrowed use (1 warning)
def bad_single_use(consumed : int64) {
    let keep = consumed * 2l
    take_int(int(keep))
}

// bad: every use narrows through transparent math (1 warning)
def bad_transparent_math(pending : array<float>; consumed : int64) : int {
    let keep = long_length(pending) - consumed * 160l
    take_int(int(max(keep, 0l)))
    take_int(int(max(keep, 0l)))
    return int(abs(keep))
}

// bad: uint64 local narrowed through uint(...) (1 warning)
def bad_uint64(n : uint64) {
    let mask = n * 2ul
    take_int(int(uint(mask)))
}

// good: clamp caps the value - int(clamp(...)) is deliberate saturation
def good_clamped(consumed : int64) : int {
    let probe = consumed * 2l
    return int(clamp(probe, 0l, 65536l))
}

// good: one bare 64-bit use among the narrows
def good_mixed_use(consumed : int64) {
    let keep = consumed * 2l
    take_int(int(keep))
    take_int64(keep)
}

// good: a comparison is 64-bit consumption
def good_compared(consumed : int64) {
    let keep = consumed * 2l
    if (keep > 0l) {
        take_int(int(keep))
    }
}

// good: reassignment - we no longer reason about the value
def good_reassigned(consumed : int64) : int {
    var keep = consumed * 2l
    keep += 1l
    return int(keep)
}

// good: the value escapes into a non-transparent call under the cast
def good_escapes(consumed : int64) {
    let keep = consumed * 2l
    take_int(int(passthru(keep)))
}

// good: captured by a lambda - consumption unknown
def good_captured(consumed : int64) : int {
    let keep = consumed * 2l
    let f = @() : int {
        return int(keep)
    }
    return invoke(f)
}

// good: consumed 64-bit by a range64 constructor
def good_range64_use(consumed : int64) : int64 {
    var s = 0l
    for (i in range64(consumed * 2l)) {
        s += i
    }
    return s
}

// good: parameters are not tracked - only initialized locals
def good_param_narrow(k : int64) {
    take_int(int(k))
}
