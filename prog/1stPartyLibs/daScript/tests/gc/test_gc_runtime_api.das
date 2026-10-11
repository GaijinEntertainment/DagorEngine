options gen2
options persistent_heap
options gc
options no_aot

require dastest/testing_boost public

struct ApiGarbage {
    values : array<int>
}

var g_api_garbage : ApiGarbage?

def private make_api_garbage() {
    for (i in range(128)) {
        g_api_garbage = new ApiGarbage(values <- [for (x in range(64)); x + i])
    }
    g_api_garbage = null
}

[test]
def gc_runtime_api_reports_and_rejects_invalid_budgets(t : T?) {
    let original_budget = gc_allocation_budget()

    set_gc_allocation_budget(1048576ul, 2097152ul)
    t |> equal(gc_allocation_budget(), urange64(1048576ul, 2097152ul),
        "budget getter preserves heap/string order")

    for (bad, i in fixed_array(urange64(0ul, 1ul), urange64(1ul, 0ul)), count()) {
        var rejected = false
        try {
            set_gc_allocation_budget(bad.x, bad.y)
        } recover {
            rejected = true
        }
        t |> success(rejected, i == 0
            ? "zero heap budget must be rejected"
            : "zero string budget must be rejected")
        t |> equal(gc_allocation_budget(), urange64(1048576ul, 2097152ul),
            "a rejected budget leaves both values unchanged")
    }

    let heap_ops_before = heap_operation_counts(false)
    let string_ops_before = heap_operation_counts(true)
    var scratch : array<int>
    scratch |> resize(64)
    scratch |> resize(512)
    delete scratch
    let heap_ops_after = heap_operation_counts(false)
    t |> success(heap_ops_after.x > heap_ops_before.x, "explicit array release increments free calls")
    t |> success(heap_ops_after.y > heap_ops_before.y, "array growth increments reallocations")
    t |> equal(heap_operation_counts(true), string_ops_before,
        "value-heap operations do not enter string-heap counters")

    let collections_before = gc_collection_stats()
    let reclaimed_before = gc_reclaimed_stats()
    let tick_before = gc_last_collection_tick()
    make_api_garbage()
    unsafe(heap_collect(true, false))

    let collections_after = gc_collection_stats()
    let pause_after = gc_pause_stats()
    let reclaimed_after = gc_reclaimed_stats()
    let tick_after = gc_last_collection_tick()
    t |> equal(collections_after.x, collections_before.x + 1ul,
        "collection count observes the completed sweep")
    t |> success(collections_after.y >= collections_before.y,
        "cumulative collection time is monotonic")
    t |> success(pause_after.y >= pause_after.x,
        "peak collection pause is at least the last pause")
    t |> success(reclaimed_after.x > reclaimed_before.x,
        "reclaimed heap bytes observe unreachable payloads")
    t |> success(reclaimed_after.y >= reclaimed_before.y,
        "reclaimed string bytes are monotonic")
    t |> success(tick_after > 0l && tick_after >= tick_before,
        "last collection tick observes the completed sweep")
    t |> success(!unsafe(heap_collect_if_needed()),
        "fresh baselines and large budgets decline an immediate repeat")
    set_gc_allocation_budget(original_budget.x, original_budget.y)
}
