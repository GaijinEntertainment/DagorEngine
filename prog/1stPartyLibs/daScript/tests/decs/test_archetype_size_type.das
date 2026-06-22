options gen2
options persistent_heap = true
options gc

require daslib/decs_boost
require dastest/testing_boost public

// Phase 8c type-contract: Archetype.size is int64, long_entity_count() returns int64.
// Before PR-C: Archetype.size : int silently wraps past INT_MAX entities (per
// the original audit). Asserting the field types at test time means a future
// revert flips this test red.
//
// Uses runtime `equal` rather than `static_assert` because dasfmt inserts a
// space between `default<X>` and `.field` that the typer rejects as
// non-constexpr in the static_assert path. typeinfo strings are still
// compile-time-evaluated, so the runtime check is effectively a const compare.

[test]
def test_archetype_size_is_int64(t : T?) {
    var _a : Archetype                                  // referenced only by typeinfo below
    t |> equal(typeinfo stripped_typename(_a.size), "int64")
}

[test]
def test_long_entity_count_returns_int64(t : T?) {
    // PR-C surface contract: `entity_count() : int` (panics if > INT_MAX) and
    // the int64-safe accessor is `long_entity_count() : int64`. Mirrors the
    // `length()` / `long_length()` pattern.
    let n = long_entity_count()
    t |> equal(typeinfo stripped_typename(n), "int64")
    t |> equal(n, 0_l)
}

[test]
def test_entity_count_keeps_int_return(t : T?) {
    let n = entity_count()
    t |> equal(typeinfo stripped_typename(n), "int")
    t |> equal(n, 0)
}
