options gen2
options rtti
require dastest/testing_boost public
require daslib/rtti

// ============================================================
//  sscan_json_at — addr+TypeInfo entry point
//
//  Mutates the value at `addr` from the parsed JSON, given the TypeInfo
//  baked from the call site (typically via `typeinfo rtti_typeinfo(value)`).
// ============================================================

struct private Pair {
    a : int
    b : float
}

[test]
def test_sscan_json_at_int(t : T?) {
    var x : int = 0
    let ok = sscan_json_at("123", unsafe(addr(x)), typeinfo rtti_typeinfo(x))
    t |> equal(ok, true)
    t |> equal(x, 123)
}

[test]
def test_sscan_json_at_int64(t : T?) {
    var x : int64 = 0l
    let ok = sscan_json_at("9999999999", unsafe(addr(x)), typeinfo rtti_typeinfo(x))
    t |> equal(ok, true)
    t |> equal(x, 9999999999l)
}

[test]
def test_sscan_json_at_bool(t : T?) {
    var x : bool = false
    let ok = sscan_json_at("true", unsafe(addr(x)), typeinfo rtti_typeinfo(x))
    t |> equal(ok, true)
    t |> equal(x, true)
}

[test]
def test_sscan_json_at_string(t : T?) {
    var s : string
    let ok = sscan_json_at("\"world\"", unsafe(addr(s)), typeinfo rtti_typeinfo(s))
    t |> equal(ok, true)
    t |> equal(s, "world")
}

[test]
def test_sscan_json_at_struct(t : T?) {
    var p = Pair(a = 0, b = 0.0)
    let ok = sscan_json_at("\{\"a\":42,\"b\":1.5\}", unsafe(addr(p)), typeinfo rtti_typeinfo(p))
    t |> equal(ok, true)
    t |> equal(p.a, 42)
    t |> equal(p.b, 1.5)
}

[test]
def test_sscan_json_at_array(t : T?) {
    var arr : array<int>
    let ok = sscan_json_at("[10, 20, 30]", unsafe(addr(arr)), typeinfo rtti_typeinfo(arr))
    t |> equal(ok, true)
    t |> equal(length(arr), 3)
    t |> equal(arr[0], 10)
    t |> equal(arr[1], 20)
    t |> equal(arr[2], 30)
}

[test]
def test_sscan_json_at_table(t : T?) {
    var tab : table<string; int>
    let ok = sscan_json_at("\{\"x\":1,\"y\":2\}", unsafe(addr(tab)), typeinfo rtti_typeinfo(tab))
    t |> equal(ok, true)
    t |> equal(length(tab), 2)
    t |> equal(tab["x"], 1)
    t |> equal(tab["y"], 2)
}

[test]
def test_sscan_json_at_round_trip(t : T?) {
    var p = Pair(a = 7, b = 3.5)
    let json = sprint_json_at(unsafe(addr(p)), typeinfo rtti_typeinfo(p), false)
    var q = Pair(a = 0, b = 0.0)
    let ok = sscan_json_at(json, unsafe(addr(q)), typeinfo rtti_typeinfo(q))
    t |> equal(ok, true)
    t |> equal(q.a, p.a)
    t |> equal(q.b, p.b)
}

[test]
def test_sscan_json_at_empty_json(t : T?) {
    // Empty JSON string: builtin returns false (no parse), addr unchanged.
    var x : int = 99
    t |> equal(sscan_json_at("", unsafe(addr(x)), typeinfo rtti_typeinfo(x)), false)
    t |> equal(x, 99)
}
