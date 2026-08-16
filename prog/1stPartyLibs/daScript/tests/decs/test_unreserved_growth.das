options gen2
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options max_unreserved_size = 16384

require dastest/testing_boost public
require daslib/decs_boost

// decs grows component storage and the entity lookup by appending; with the 16 KB
// max_unreserved_size above, crossing it proves those growth sites reserve deliberately
// (ensure_capacity) instead of tripping the guard.

[decs_template]
struct GrowthItem {
    value : int
    pos : float3
}

[test]
def test_per_entity_growth_crosses_the_limit(t : T?) {
    t |> run("one-at-a-time creation grows storage past the limit") @(t : T?) {
        restart()
        for (_i in range(3000)) {                      // 3000 * 16 B stride crosses 16 KB many times over
            create_entity <| @(eid, cmp) {
                apply_decs_template(cmp, GrowthItem(value = 1, pos = float3(1)))
            }
        }
        commit()
        var total = 0
        query <| $(git : GrowthItem) {
            total += git.value
        }
        t |> equal(total, 3000)
    }
}

[test]
def test_bulk_growth_crosses_the_limit(t : T?) {
    t |> run("bulk creation grows storage and the lookup past the limit") @(t : T?) {
        restart()
        create_entities(3000) <| $(eid : EntityId; i : int; var cmp : ComponentMap) {
            apply_decs_template(cmp, GrowthItem(value = 1, pos = float3(float(i))))
        }
        commit()
        var total = 0
        query <| $(git : GrowthItem) {
            total += git.value
        }
        t |> equal(total, 3000)
    }
}
