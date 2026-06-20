options gen2
options rtti
require dastest/testing_boost public
require daslib/rtti
require strings

// ============================================================
//  sprint_json_at — addr+TypeInfo entry point
//
//  Equivalent to sprint_json but takes (addr, ti) directly instead
//  of routing through an `any` arg + SimNode_CallBase::types[].
//  Lets call sites that have only a raw `void?` address (e.g. from
//  the dasImgui widget dispatcher) serialize without a per-call-site
//  daslang thunk.
// ============================================================

var private g_int = 42
var private g_uint = 0xFFu
var private g_int64 = 123456789l
var private g_bool_t = true
var private g_bool_f = false
var private g_string = "hello"
var private g_escape = "a\nb"

struct private Pair {
    a : int
    b : float
}
var private g_pair = Pair(a = 7, b = 3.5)

var private g_array : array<int>
var private g_table : table<string; int>

[init]
def fill_array_and_table() {
    g_array |> push(1)
    g_array |> push(2)
    g_array |> push(3)
    g_table |> insert("x", 10)
    g_table |> insert("y", 20)
}

[test]
def test_sprint_json_at_matches_sprint_json(tt : T?) {
    tt |> run("int") @(t : T?) {
        let a = sprint_json(g_int, false)
        let b = sprint_json_at(unsafe(addr(g_int)), typeinfo rtti_typeinfo(g_int), false)
        t |> equal(a, b)
    }
    tt |> run("uint") @(t : T?) {
        let a = sprint_json(g_uint, false)
        let b = sprint_json_at(unsafe(addr(g_uint)), typeinfo rtti_typeinfo(g_uint), false)
        t |> equal(a, b)
    }
    tt |> run("int64") @(t : T?) {
        let a = sprint_json(g_int64, false)
        let b = sprint_json_at(unsafe(addr(g_int64)), typeinfo rtti_typeinfo(g_int64), false)
        t |> equal(a, b)
    }
    tt |> run("bool true") @(t : T?) {
        let a = sprint_json(g_bool_t, false)
        let b = sprint_json_at(unsafe(addr(g_bool_t)), typeinfo rtti_typeinfo(g_bool_t), false)
        t |> equal(a, b)
    }
    tt |> run("bool false") @(t : T?) {
        let a = sprint_json(g_bool_f, false)
        let b = sprint_json_at(unsafe(addr(g_bool_f)), typeinfo rtti_typeinfo(g_bool_f), false)
        t |> equal(a, b)
    }
    tt |> run("string") @(t : T?) {
        let a = sprint_json(g_string, false)
        let b = sprint_json_at(unsafe(addr(g_string)), typeinfo rtti_typeinfo(g_string), false)
        t |> equal(a, b)
    }
    tt |> run("string with escapes") @(t : T?) {
        let a = sprint_json(g_escape, false)
        let b = sprint_json_at(unsafe(addr(g_escape)), typeinfo rtti_typeinfo(g_escape), false)
        t |> equal(a, b)
    }
    tt |> run("struct") @(t : T?) {
        let a = sprint_json(g_pair, false)
        let b = sprint_json_at(unsafe(addr(g_pair)), typeinfo rtti_typeinfo(g_pair), false)
        t |> equal(a, b)
    }
    tt |> run("array<int>") @(t : T?) {
        let a = sprint_json(g_array, false)
        let b = sprint_json_at(unsafe(addr(g_array)), typeinfo rtti_typeinfo(g_array), false)
        t |> equal(a, b)
    }
    tt |> run("table<string;int>") @(t : T?) {
        let a = sprint_json(g_table, false)
        let b = sprint_json_at(unsafe(addr(g_table)), typeinfo rtti_typeinfo(g_table), false)
        t |> equal(a, b)
    }
}

[test]
def test_sprint_json_at_human_readable(t : T?) {
    let compact = sprint_json_at(unsafe(addr(g_pair)), typeinfo rtti_typeinfo(g_pair), false)
    let human = sprint_json_at(unsafe(addr(g_pair)), typeinfo rtti_typeinfo(g_pair), true)
    t |> equal(empty(compact), false)
    t |> equal(empty(human), false)
    // Human-readable form contains newlines; compact form doesn't.
    t |> equal(find(human, "\n") >= 0, true)
    t |> equal(find(compact, "\n") < 0, true)
}
