options gen2
options gc

require daslib/decs_boost
require dastest/testing_boost public

// Phase 8c runtime probe: bulk-create at a realistic size exercises the int64
// arithmetic neighbors (eidx*stride, arch.size += int64(count), long_entity_count
// accumulator) end-to-end. The real >INT_MAX scenario is gated by EntityId.id
// + length()/resize cascade widening (followup); this probe is the small-N
// regression test that catches widening drift via the natural API.

[decs_template]
struct BulkItem {
    payload : uint8
}

[test]
def test_bulk_create_long_entity_count(t : T?) {
    restart()
    let N = 100_000
    create_entities(N) $(eid : EntityId; i : int; var cmp : ComponentMap) {
        apply_decs_template(cmp, BulkItem(payload = uint8(i & int(0xFF))))
    }
    commit()
    // long_entity_count() returns int64; runtime value matches at non-trivial N.
    t |> equal(long_entity_count(), int64(N))
    // Archetype.size is int64 and matches.
    t |> equal(decsState.allArchetypes[0].size, int64(N))
    // entity_count() (int return) round-trips at this size — no panic, no truncation.
    t |> equal(entity_count(), N)
    restart()
}

[test]
def test_archetype_size_arithmetic_neighbors(t : T?) {
    // Create N entities, delete the first half, verify arch.size -= and the
    // remove_entity (di * stride) path stay coherent after widening.
    restart()
    let N = 1000
    var eids : array<EntityId>
    eids |> reserve(N)
    for (i in range(N)) {
        let eid = create_entity() @(eid, cmp) {
            apply_decs_template(cmp, BulkItem(payload = uint8(i & int(0xFF))))
        }
        eids |> push(eid)
    }
    commit()
    t |> equal(decsState.allArchetypes[0].size, int64(N))
    for (i in range(N / 2)) {
        delete_entity(eids[i])
    }
    commit()
    t |> equal(decsState.allArchetypes[0].size, int64(N / 2))
    t |> equal(long_entity_count(), int64(N / 2))
    restart()
}
