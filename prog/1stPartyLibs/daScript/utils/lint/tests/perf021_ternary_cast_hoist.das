options gen2
// PERF021: cond ? T(a) : T(b) on workhorse cast T — hoist as T(cond ? a : b).
//
// Problem:
//   `cond ? string(a) : string(b)` emits two ExprCall nodes that both convert to
//   the same target type. The work is identical regardless of branch, and the
//   parser/typer doesn't fold them. Two cast dispatches, twice the source noise.
//
// Bad pattern:
//   def to_str(c : bool; a, b : int) : string {
//       return c ? string(a) : string(b)             // PERF021
//   }
//
// Good pattern:
//   def to_str(c : bool; a, b : int) : string {
//       return string(c ? a : b)
//   }
//
// Side-effect note: the rewrite is unconditionally safe. The original ternary
// evaluates exactly one of `a` / `b` (whichever branch is taken); so does the
// hoisted form. Argument evaluation count is unchanged, only the number of
// cast dispatches drops from 1-of-2 to 1.
//
// dastest readers: same approach as the perf020 fixture — parameters (not
// const literals) keep the runtime forms intact under inference-time folding.

expect 31208:12

require daslib/perf_lint

struct SomeStruct {
    v : int64
}

// --- Bad patterns (one PERF021 each — runtime, survive folding) ---

def bad_string_from_int(c : bool; a, b : int) : string {
    return c ? string(a) : string(b)                         // PERF021
}

def bad_string_from_int64(c : bool; a, b : int64) : string {
    return c ? string(a) : string(b)                         // PERF021
}

def bad_int64_widen(c : bool; a, b : int) : int64 {
    return c ? int64(a) : int64(b)                           // PERF021
}

def bad_int_narrow(c : bool; a, b : int64) : int {
    return c ? int(a) : int(b)                               // PERF021
}

def bad_uint_signedness(c : bool; a, b : int) : uint {
    return c ? uint(a) : uint(b)                             // PERF021
}

def bad_float_from_int(c : bool; a, b : int) : float {
    return c ? float(a) : float(b)                           // PERF021
}

def bad_double_from_int(c : bool; a, b : int) : double {
    return c ? double(a) : double(b)                         // PERF021
}

def bad_int8(c : bool; a, b : int) : int8 {
    return c ? int8(a) : int8(b)                             // PERF021
}

def bad_uint16(c : bool; a, b : int) : uint16 {
    return c ? uint16(a) : uint16(b)                         // PERF021
}

def bad_field_access(c : bool; x, y : SomeStruct) : string {
    return c ? string(x.v) : string(y.v)                     // PERF021 — args are field accesses, same baseType
}

def bad_subexpr(c : bool; a, b, d : int) : int64 {
    return c ? int64(a + b) : int64(d)                       // PERF021 — args are int / int sub-expressions
}

def bad_same_hex_string(c : bool; a, b : int) : string {
    return c ? string(a, true) : string(b, true)             // PERF021 — same hex flag, hoist is equivalent
}

// --- Good patterns (no warnings) ---

def good_one_branch_cast(c : bool; a : int; b : int64) : int64 {
    return c ? int64(a) : b                                  // only one branch is a cast
}

def good_different_cast(c : bool; a, b : int) : int64 {
    return c ? int64(a) : int64(b) + 1l                      // RHS isn't a bare cast (sub-expr around the cast)
}

def good_arg_basetype_differs(c : bool; a : int; b : int64) : string {
    return c ? string(a) : string(b)                         // same cast, but arg baseTypes differ — rewrite needs widen
}

def good_user_named_ctor(c : bool; a, b : int) : SomeStruct {
    return c ? SomeStruct(v = int64(a)) : SomeStruct(v = int64(b))   // SomeStruct ctor, not a workhorse cast
}

def good_vector_ctor(c : bool; x, y : float) : float2 {
    return c ? float2(x, y) : float2(y, x)                   // 2-arg ctor — single-arg gate excludes
}

def good_literal_branches(c : bool) : string {
    return c ? "yes" : "no"                                  // no casts at all
}

def good_call_one_side_only(c : bool; a : int) : int {
    return c ? a + 1 : a - 1                                 // ternary, no casts on either side
}

def good_different_hex_string(c : bool; a, b : int) : string {
    return c ? string(a, true) : string(b, false)            // hex flag differs — hoisting would lose semantic info
}
