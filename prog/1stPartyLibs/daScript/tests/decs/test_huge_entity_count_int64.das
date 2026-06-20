options gen2
options persistent_heap = true   // c.data > INT_MAX bytes needs PersistentHeapAllocator
options gc

require daslib/decs_boost
require daslib/fio
require dastest/testing_boost public

// Phase 8c memory-gated probe: push component byte storage (c.data) past
// INT_MAX bytes to exercise the long_length / int64 resize cascade widened
// in part 3 of this branch. Before PR-C: `length(c.data) + c.stride * count`
// in create_entities (line 986) panicked at > INT_MAX c.data bytes per the
// long-array surface contract. With PR-C, the resize routes through int64.
//
// One [decs_template] field of type float4[4] = 64 bytes stride (the max
// per-component payload, bounded by ComponentValue.data : float4[4]).
// 35 M entities * 64 = 2.24 GB c.data > INT_MAX (2.147 GB).
//
// Entity count stays well under INT_MAX so we isolate the BYTE-storage
// overflow (the entity-count overflow probe would require 2 G+ entities
// for a memory-prohibitive 40+ GB).
//
// Memory: ~3 GB (2.24 GB c.data + 700 MB entityLookup at 35M entries).
// Gated on DASLANG_HUGE_HEAP_TESTS=1:
//   $env:DASLANG_HUGE_HEAP_TESTS = "1"
//   bin/Release/daslang.exe dastest/dastest.das -- --test tests/decs/test_huge_entity_count_int64.das

def huge_enabled() : bool {
    static_if (typeinfo sizeof(type<int?>) < 8) {
        return false
    }
    return has_env_variable("DASLANG_HUGE_HEAP_TESTS") && get_env_variable("DASLANG_HUGE_HEAP_TESTS") == "1"
}

// 64-byte component payload — the maximum per ComponentValue.data : float4[4].
[decs_template]
struct HugePayload {
    payload : float4[4]
}

let HUGE_N = 35_000_000        // 35 M entities (fits in int comfortably)
// At sizeof(HugePayload.payload) = 64 bytes, c.data = 2.24 GB > INT_MAX.

[test]
def test_long_length_resize_cascade(t : T?) {
    if (!huge_enabled()) return
    restart()

    create_entities(HUGE_N) $(eid : EntityId; i : int; var cmp : ComponentMap) {
        apply_decs_template(cmp, default<HugePayload>)
    }

    // long_entity_count fits in int here, but the field is int64.
    t |> equal(long_entity_count(), int64(HUGE_N))
    t |> equal(decsState.allArchetypes[0].size, int64(HUGE_N))

    // The point of the probe: at 2.24 GB c.data the WIDENED resize cascade
    // in create_entities ran without hitting length()'s INT_MAX panic.
    // If part 3's widening regressed, create_entities above would have
    // panicked before we got here.

    restart()
}
