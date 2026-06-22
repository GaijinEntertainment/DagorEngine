options gen2
// STYLE020: from_JV(v, type<T>, defV) on a scalar — use 'v ?? defV'
//
// Problem:
//   `daslib/json_boost` provides `operator ??` overloads for every supported
//   scalar JsonValue → primitive conversion (int, uint, int8/16/64,
//   uint8/16/64, float, double, bool, string). The three-arg `from_JV(v,
//   type<T>, defV)` form is redundant — `v ?? defV` is one fewer call,
//   reads better, and uses the operator that's already there.
//
// Bad pattern:
//   let n = from_JV(jv, type<int>, 13)
//
// Good pattern:
//   let n = jv ?? 13
//
// Out of scope: vector/table/struct/enum/bitfield overloads of from_JV
// have no matching `operator ??`, so they stay silent.

expect 31209:6

require daslib/json_boost
require daslib/style_lint

// --- Bad: scalar from_JV with matching ?? overload ---

def bad_int(jv : JsonValue?) : int {
    return from_JV(jv, type<int>, 13)
}

def bad_float(jv : JsonValue?) : float {
    return from_JV(jv, type<float>, 1.5f)
}

def bad_bool(jv : JsonValue?) : bool {
    return from_JV(jv, type<bool>, false)
}

def bad_string(jv : JsonValue?) : string {
    return from_JV(jv, type<string>, "default")
}

def bad_uint64(jv : JsonValue?) : uint64 {
    return from_JV(jv, type<uint64>, 0ul)
}

def bad_double(jv : JsonValue?) : double {
    return from_JV(jv, type<double>, 0.0lf)
}

// --- Good: already using ?? ---

def good_coalesce_int(jv : JsonValue?) : int {
    return jv ?? 13
}

def good_coalesce_string(jv : JsonValue?) : string {
    return jv ?? "default"
}

// --- Good: non-scalar from_JV stays silent (no ?? overload) ---

struct MyStruct {
    x : int
    y : float
}

def good_struct_from_jv(jv : JsonValue?) : MyStruct {
    var s : MyStruct
    from_JV(jv, s)
    return s
}
