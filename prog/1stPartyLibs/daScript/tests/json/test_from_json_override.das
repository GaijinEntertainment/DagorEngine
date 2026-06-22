options gen2

require daslib/json_boost
require daslib/linq_boost public
require dastest/testing_boost public

// Option-B contract: from_json is a flat-object query source (like from_xml_node) — it reads top-level
// fields BY NAME and never honors a custom whole-row from_JV(Row) override. The escape hatch for override
// semantics is to materialize the raw array with an explicit from_JV first. These tests pin both halves.

struct OverrideRow {
    name : string
    tag  : int
}

// A user override that always returns a sentinel, distinct from any by-name materialize.
[unused_argument(v, ent)]
def from_JV(v : JsonValue const explicit?; ent : OverrideRow; defV : OverrideRow = OverrideRow()) : OverrideRow {
    return OverrideRow(name = "OVERRIDDEN", tag = 999)
}

def mkjson() : JsonValue? {
    return JV([JV(OverrideRow(name = "real0", tag = 0)), JV(OverrideRow(name = "real1", tag = 1))])
}

[test]
def test_from_json_ignores_override(t : T?) {
    let jv = mkjson()
    // from_json reads top-level fields by name — the from_JV(OverrideRow) override is NOT invoked.
    let rows <- [for (r in from_json(jv, type<OverrideRow>)); r]
    t |> equal(length(rows), 2)
    t |> equal(rows[0].name, "real0")
    t |> equal(rows[0].tag, 0)
    t |> equal(rows[1].name, "real1")
    t |> equal(rows[1].tag, 1)
}

[test]
def test_fused_lane_ignores_override(t : T?) {
    let jv = mkjson()
    // The fused JsonAdapter materializes whole rows by name too — override still ignored.
    let rows <- _fold(unsafe(from_json(jv, type<OverrideRow>))._where(_.tag >= 0).to_array())
    t |> equal(length(rows), 2)
    t |> equal(rows[0].name, "real0")
    t |> equal(rows[1].name, "real1")
}

[test]
def test_array_escape_honors_override(t : T?) {
    let jv = mkjson()
    // The escape hatch: an explicit from_JV(e, type<Row>) over the raw array DOES honor the override.
    let esc <- [for (e in jv.value as _array); from_JV(e, type<OverrideRow>)]
    t |> equal(length(esc), 2)
    t |> equal(esc[0].name, "OVERRIDDEN")
    t |> equal(esc[0].tag, 999)
    t |> equal(esc[1].name, "OVERRIDDEN")
}
