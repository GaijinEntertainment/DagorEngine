options gen2
options auto_inline_functions = false   // lint fixtures assert SOURCE shapes; splices rewrite them
// PERF020: T(x) where x is already T (workhorse type) is a no-op cast — drop it.
//
// Problem:
//   `int64(a)` where `a : int64` produces a real ExprCall node — the parser /
//   typer does NOT elide same-type casts. So the call costs one dispatch and
//   the source costs reader-attention for zero work.
//
// Bad pattern:
//   def widen(a : int64) : int64 {
//       return int64(a)                  // PERF020
//   }
//
// Good pattern:
//   def widen(a : int64) : int64 {
//       return a
//   }
//
// Note for dastest readers: this test uses function PARAMETERS (not literals)
// so the runtime forms survive constant folding under normal compile policies.
// `int64(5l)` would be const-folded to a single ExprConstInt64 during inference
// (which dastest uses), leaving the rule nothing to attach to. The lint runner
// sets `no_optimizations=true / no_infer_time_folding=true` and would also
// detect the const form, but dastest exercises only the runtime forms here.
// (Same approach as the PERF019 fixture — see perf019_int_cast_collapse.das.)

expect 31208:11

require daslib/perf_lint

struct SomeStruct {
    value : int64
}

// --- Bad patterns (one PERF020 each — runtime, survive folding) ---

def bad_int64(a : int64) : int64 {
    return int64(a)                                         // PERF020
}

def bad_int(a : int) : int {
    return int(a)                                           // PERF020
}

def bad_uint(a : uint) : uint {
    return uint(a)                                          // PERF020
}

def bad_uint64(a : uint64) : uint64 {
    return uint64(a)                                        // PERF020
}

def bad_int8(a : int8) : int8 {
    return int8(a)                                          // PERF020
}

def bad_uint16(a : uint16) : uint16 {
    return uint16(a)                                        // PERF020
}

def bad_float(a : float) : float {
    return float(a)                                         // PERF020
}

def bad_double(a : double) : double {
    return double(a)                                        // PERF020
}

def bad_string(a : string) : string {
    return string(a)                                        // PERF020
}

def bad_field_access(s : SomeStruct) : int64 {
    return int64(s.value)                                   // PERF020 — s.value is already int64
}

def bad_subexpr(a, b : int) : int {
    return int(a + b)                                       // PERF020 — a+b is already int
}

// --- Good patterns (no warnings) ---

def good_widen(a : int) : int64 {
    return int64(a)                                         // narrowing/widening across int sizes
}

def good_signed_to_unsigned(a : int) : uint {
    return uint(a)                                          // signedness change
}

def good_unsigned_to_signed(a : uint) : int {
    return int(a)                                           // signedness change
}

def good_narrow(a : int64) : int {
    return int(a)                                           // narrowing
}

def good_float_to_int(a : float) : int {
    return int(a)                                           // type-class change
}

def good_int_to_float(a : int) : float {
    return float(a)                                         // type-class change
}

def good_float_to_double(a : float) : double {
    return double(a)                                        // float-precision change
}

def good_double_to_float(a : double) : float {
    return float(a)                                         // float-precision change
}

def good_vector_ctor(x, y : float) : float2 {
    return float2(x, y)                                     // 2-arg ctor — single-arg gate excludes
}

// A generic's one source line serves every instance: the cast below is a no-op where TT binds
// float and the required widen where it binds float16, so the verdict is not actionable and the
// rule stays silent inside instantiated bodies. (dasLLAMA's GPU kernels read a KV element this
// way — one score-fill generic over the f16 and f32 mirrors.)
def widen_elem(a : auto(TT)) : float {
    return float(a)                                         // no PERF020 — generic instance
}

def calls_both_instances(f : float; h : float16) : float {
    return widen_elem(f) + widen_elem(h)
}
