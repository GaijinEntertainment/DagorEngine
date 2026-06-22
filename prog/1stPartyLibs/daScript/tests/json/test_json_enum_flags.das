options gen2
options rtti
require dastest/testing_boost public
require daslib/rtti
require daslib/json
require daslib/json_boost
require strings

// ============================================================
//  Enum-as-flags JSON serialization.
//
//  An OR-combined enum value (e.g. ImGui's `OpenOnArrow | DefaultOpen`)
//  matches no single enumerator. The reflective JSON writer used to emit
//  NOTHING for such a value, producing invalid JSON ({"flags": ,...}) that
//  dropped the entire surrounding object on parse. It now emits the number,
//  which round-trips (scanEnum already accepts a bare integer).
// ============================================================

enum private TreeFlag {
    none       = 0
    on_arrow   = 1
    default_on = 2
    framed     = 4
}

var private g_single   = TreeFlag.default_on
var private g_none     : TreeFlag
var private g_combined : TreeFlag

struct private WidgetState {
    flags  : TreeFlag
    opened : bool
}
var private g_widget : WidgetState

[init]
def setup() {
    // on_arrow | default_on == 3 — no single enumerator equals 3. Plain enums don't
    // define `|` (ImGui's flag enums get it from their C++ binding); build the combined
    // value the way it actually arrives in memory — an OR'd int reinterpreted as the enum.
    g_combined = unsafe(reinterpret<TreeFlag>(int(TreeFlag.on_arrow) | int(TreeFlag.default_on)))
    g_widget.flags = g_combined
    g_widget.opened = true
}

[test]
def test_enum_flags_serialize(tt : T?) {
    tt |> run("single value serializes to its name") @(t : T?) {
        let s = sprint_json_at(unsafe(addr(g_single)), typeinfo rtti_typeinfo(g_single), false)
        t |> equal(s, "\"default_on\"")
    }
    tt |> run("zero value serializes to its name") @(t : T?) {
        let s = sprint_json_at(unsafe(addr(g_none)), typeinfo rtti_typeinfo(g_none), false)
        t |> equal(s, "\"none\"")
    }
    tt |> run("combined value serializes to the number, not empty") @(t : T?) {
        let s = sprint_json_at(unsafe(addr(g_combined)), typeinfo rtti_typeinfo(g_combined), false)
        t |> equal(s, "3")
    }
    tt |> run("combined value round-trips through sscan") @(t : T?) {
        var back : TreeFlag
        let ok = sscan_json_at("3", unsafe(addr(back)), typeinfo rtti_typeinfo(back))
        t |> equal(ok, true)
        t |> equal(int(back), 3)
    }
    tt |> run("struct with a combined enum field stays parseable") @(t : T?) {
        // The regression: a combined flags field used to blank out, leaving
        // {"flags": ,"opened":true} — invalid, so the whole object dropped.
        let s = sprint_json_at(unsafe(addr(g_widget)), typeinfo rtti_typeinfo(g_widget), false)
        var err : string
        let parsed = read_json(s, err)
        t |> equal(parsed != null, true)
        t |> equal(parsed?["opened"] ?? false, true)
        t |> equal(parsed?["flags"] ?? 0, 3)
    }
}
